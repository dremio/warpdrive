/*-------
 * Module:			environ.cc
 *
 * Description:		This module contains routines related to
 *					the environment, such as storing connection handles,
 *					and returning errors.
 *
 * Classes:			EnvironmentClass (Functions prefix: "EN_")
 *
 * API functions:	SQLAllocEnv, SQLFreeEnv, SQLError
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *-------
 */

#include "environ.h"
#include "misc.h"

#include "connection.h"
#include "dlg_specific.h"
#include "statement.h"
#include <stdlib.h>
#include <string.h>
#include "wdapifunc.h"
#include <memory>
#include <mutex>
#ifdef	WIN32
#include <winsock2.h>
#endif /* WIN32 */
#include "loadlib.h"
#include "new_driver.h"
#include <odbcabstraction/odbc_impl/ODBCEnvironment.h>
#include <odbcabstraction/spi/driver.h>
#include <odbcabstraction/logger.h>


/* The one instance of the handles */
static int conns_count = 0;
static ConnectionClass **conns = NULL;

#if defined(WIN_MULTITHREAD_SUPPORT)
CRITICAL_SECTION	conns_cs;
CRITICAL_SECTION	common_cs; /* commonly used for short term blocking */
CRITICAL_SECTION	common_lcs; /* commonly used for not necessarily short term blocking */
#elif defined(POSIX_MULTITHREAD_SUPPORT)
pthread_mutex_t     conns_cs;
pthread_mutex_t     common_cs;
pthread_mutex_t     common_lcs;
#endif /* WIN_MULTITHREAD_SUPPORT */

void	shortterm_common_lock(void)
{
	ENTER_COMMON_CS;
}
void	shortterm_common_unlock(void)
{
	LEAVE_COMMON_CS;
}

int	getConnCount(void)
{
	return conns_count;
}
ConnectionClass * const *getConnList(void)
{
	return conns;
}

namespace {
	// The singleton instance of the driver.
	static std::shared_ptr<driver::odbcabstraction::Driver> s_driver;
	static std::mutex s_driverLock;

void InitializeDriverIfNeeded() {
  std::lock_guard<std::mutex> lock(s_driverLock);
  if (s_driver == nullptr) {
    s_driver = CreateDriver();
    s_driver->RegisterLog();

    LOG_DEBUG("Driver initialized");
  }
}
}

using namespace ODBC;

RETCODE		SQL_API
WD_AllocEnv(HENV * phenv)
{
	CSTR func = "WD_AllocEnv";
	SQLRETURN	ret = SQL_SUCCESS;

	MYLOG(0, "entering\n");

	/*
	 * For systems on which none of the constructor-making
	 * techniques in psqlodbc.c work:
	 * It's ok to call initialize_global_cs() twice.
	 */
	{
		initialize_global_cs();
		InitializeDriverIfNeeded();
	}

	try {
		*phenv = reinterpret_cast<HENV>(new ODBCEnvironment(s_driver));
	} catch (std::bad_alloc&) {
		*phenv = SQL_NULL_HENV;
		EN_log_error(func, "Error allocating environment", NULL);
		ret = SQL_ERROR;
	}

	MYLOG(0, "leaving phenv=%p\n", *phenv);
	return ret;
}


RETCODE		SQL_API
WD_FreeEnv(HENV henv)
{
	CSTR func = "WD_FreeEnv";
	SQLRETURN	ret = SQL_SUCCESS;
	ODBCEnvironment *env = reinterpret_cast<ODBCEnvironment*>(henv);

	MYLOG(0, "entering env=%p\n", env);

	try {
		if (env)
		{
			env->GetDiagnostics().Clear();
			delete env;
			MYLOG(0, "   ok\n");
			return SQL_SUCCESS;
		}

		EN_log_error(func, "Error freeing environment", NULL);
		return SQL_ERROR;
	}
	catch (const driver::odbcabstraction::DriverException& ex) {
		env->GetDiagnostics().AddError(ex);
		return SQL_ERROR;
	}
	catch (const std::bad_alloc& ex) {
		env->GetDiagnostics().AddError(
			driver::odbcabstraction::DriverException("A memory allocation error occurred.", "HY001"));
		return SQL_ERROR;
	}
	catch (const std::exception& ex) {
		env->GetDiagnostics().AddError(
			driver::odbcabstraction::DriverException(ex.what()));
		return SQL_ERROR;
	}
	catch (...) {
		env->GetDiagnostics().AddError(
			driver::odbcabstraction::DriverException("An unknown error occurred."));
		return SQL_ERROR;
	}
}

#define	SIZEOF_SQLSTATE	6

static void
WD_sqlstate_set(const EnvironmentClass *env, UCHAR *szSqlState, const char *ver3str, const char *ver2str)
{
	strncpy_null((char *) szSqlState, EN_is_odbc3(env) ? ver3str : ver2str, SIZEOF_SQLSTATE);
}

WD_ErrorInfo *
ER_Constructor(SDWORD errnumber, const char *msg)
{
	WD_ErrorInfo	*error;
	ssize_t		aladd, errsize;

	if (DESC_OK == errnumber)
		return NULL;
	if (msg)
	{
		errsize = strlen(msg);
		aladd = errsize - sizeof(error->__error_message) + 1;
		if (aladd < 0)
			aladd = 0;
	}
	else
	{
		errsize = -1;
		aladd = 0;
	}
	error = (WD_ErrorInfo *) malloc(sizeof(WD_ErrorInfo) + aladd);
	if (error)
	{
		memset(error, 0, sizeof(WD_ErrorInfo));
		error->status = errnumber;
		error->errorsize = (Int2) errsize;
		if (errsize > 0)
			memcpy(error->__error_message, msg, errsize);
		error->__error_message[errsize] = '\0';
		error->recsize = -1;
	}
	return error;
}

void
ER_Destructor(WD_ErrorInfo *self)
{
	free(self);
}

WD_ErrorInfo *
ER_Dup(const WD_ErrorInfo *self)
{
	WD_ErrorInfo	*newErr;
	Int4		alsize;

	if (!self)
		return NULL;
	alsize = sizeof(WD_ErrorInfo);
	if (self->errorsize  > 0)
		alsize += self->errorsize;
	newErr = (WD_ErrorInfo *) malloc(alsize);
	if (newErr)
		memcpy(newErr, self, alsize);

	return newErr;
}

#define	DRVMNGRDIV	511
/*		Returns the next SQL error information. */
RETCODE		SQL_API
ER_ReturnError(WD_ErrorInfo *pgerror,
			   SQLSMALLINT	RecNumber,
			   SQLCHAR * szSqlState,
			   SQLINTEGER * pfNativeError,
			   SQLCHAR * szErrorMsg,
			   SQLSMALLINT cbErrorMsgMax,
			   SQLSMALLINT * pcbErrorMsg,
			   UWORD flag)
{
	/* CC: return an error of a hstmt  */
	WD_ErrorInfo	*error;
	BOOL		partial_ok = ((flag & PODBC_ALLOW_PARTIAL_EXTRACT) != 0);
	const char	*msg;
	SWORD		msglen, stapos, wrtlen, pcblen;

	if (!pgerror)
		return SQL_NO_DATA_FOUND;
	error = pgerror;
	msg = error->__error_message;
	MYLOG(0, "entering status = %d, msg = #%s#\n", error->status, msg);
	msglen = (SQLSMALLINT) strlen(msg);
	/*
	 *	Even though an application specifies a larger error message
	 *	buffer, the driver manager changes it silently.
	 *	Therefore we divide the error message into ...
	 */
	if (error->recsize < 0)
	{
		if (cbErrorMsgMax > 0)
			error->recsize = cbErrorMsgMax - 1; /* apply the first request */
		else
			error->recsize = DRVMNGRDIV;
	}
	else if (1 == RecNumber && cbErrorMsgMax > 0)
		error->recsize = cbErrorMsgMax - 1;
	if (RecNumber < 0)
	{
		if (0 == error->errorpos)
			RecNumber = 1;
		else
			RecNumber = 2 + (error->errorpos - 1) / error->recsize;
	}
	stapos = (RecNumber - 1) * error->recsize;
	if (stapos > msglen)
		return SQL_NO_DATA_FOUND;
	pcblen = wrtlen = msglen - stapos;
	if (pcblen > error->recsize)
		pcblen = error->recsize;
	if (0 == cbErrorMsgMax)
		wrtlen = 0;
	else if (wrtlen >= cbErrorMsgMax)
	{
		if (partial_ok)
			wrtlen = cbErrorMsgMax - 1;
		else if (cbErrorMsgMax <= error->recsize)
			wrtlen = cbErrorMsgMax - 1;
		else
			wrtlen = error->recsize;
	}
	if (wrtlen > pcblen)
		wrtlen = pcblen;
	if (NULL != pcbErrorMsg)
		*pcbErrorMsg = pcblen;

	if ((NULL != szErrorMsg) && (cbErrorMsgMax > 0))
	{
		memcpy(szErrorMsg, msg + stapos, wrtlen);
		szErrorMsg[wrtlen] = '\0';
	}

	if (NULL != pfNativeError)
		*pfNativeError = error->status;

	if (NULL != szSqlState)
		strncpy_null((char *) szSqlState, error->sqlstate, 6);

	MYLOG(0, "	     szSqlState = '%s',len=%d, szError='%s'\n", szSqlState, pcblen, szErrorMsg);
	if (wrtlen < pcblen)
		return SQL_SUCCESS_WITH_INFO;
	else
		return SQL_SUCCESS;
}


RETCODE		SQL_API
WD_ConnectError(HDBC hdbc,
				   SQLSMALLINT	RecNumber,
				   SQLCHAR * szSqlState,
				   SQLINTEGER * pfNativeError,
				   SQLCHAR * szErrorMsg,
				   SQLSMALLINT cbErrorMsgMax,
				   SQLSMALLINT * pcbErrorMsg,
				   UWORD flag)
{
	ConnectionClass *conn = (ConnectionClass *) hdbc;
	EnvironmentClass *env = (EnvironmentClass *) conn->henv;
	char		*msg;
	int		status;
	BOOL	once_again = FALSE;
	ssize_t		msglen;

	MYLOG(0, "entering hdbc=%p <%d>\n", hdbc, cbErrorMsgMax);
	if (RecNumber != 1 && RecNumber != -1)
		return SQL_NO_DATA_FOUND;
	if (cbErrorMsgMax < 0)
		return SQL_ERROR;
	if (CONN_EXECUTING == conn->status || !CC_get_error(conn, &status, &msg) || NULL == msg)
	{
		MYLOG(0, "CC_Get_error returned nothing.\n");
		if (NULL != szSqlState)
			strncpy_null((char *) szSqlState, "00000", SIZEOF_SQLSTATE);
		if (NULL != pcbErrorMsg)
			*pcbErrorMsg = 0;
		if ((NULL != szErrorMsg) && (cbErrorMsgMax > 0))
			szErrorMsg[0] = '\0';

		return SQL_NO_DATA_FOUND;
	}
	MYLOG(0, "CC_get_error: status = %d, msg = #%s#\n", status, msg);

	msglen = strlen(msg);
	if (NULL != pcbErrorMsg)
	{
		*pcbErrorMsg = (SQLSMALLINT) msglen;
		if (cbErrorMsgMax == 0)
			once_again = TRUE;
		else if (msglen >= cbErrorMsgMax)
			*pcbErrorMsg = cbErrorMsgMax - 1;
	}
	if ((NULL != szErrorMsg) && (cbErrorMsgMax > 0))
		strncpy_null((char *) szErrorMsg, msg, cbErrorMsgMax);
	if (NULL != pfNativeError)
		*pfNativeError = status;

	if (NULL != szSqlState)
	{
		if (conn->sqlstate[0])
			strncpy_null((char *) szSqlState, conn->sqlstate, SIZEOF_SQLSTATE);
		else
		switch (status)
		{
			case CONN_OPTION_VALUE_CHANGED:
				WD_sqlstate_set(env, szSqlState, "01S02", "01S02");
				break;
			case CONN_TRUNCATED:
				WD_sqlstate_set(env, szSqlState, "01004", "01004");
				/* data truncated */
				break;
			case CONN_INIREAD_ERROR:
				WD_sqlstate_set(env, szSqlState, "IM002", "IM002");
				/* data source not found */
				break;
			case CONNECTION_SERVER_NOT_REACHED:
			case CONN_OPENDB_ERROR:
				WD_sqlstate_set(env, szSqlState, "08001", "08001");
				/* unable to connect to data source */
				break;
			case CONN_INVALID_AUTHENTICATION:
			case CONN_AUTH_TYPE_UNSUPPORTED:
				WD_sqlstate_set(env, szSqlState, "28000", "28000");
				break;
			case CONN_STMT_ALLOC_ERROR:
				WD_sqlstate_set(env, szSqlState, "HY001", "S1001");
				/* memory allocation failure */
				break;
			case CONN_IN_USE:
				WD_sqlstate_set(env, szSqlState, "HY000", "S1000");
				/* general error */
				break;
			case CONN_UNSUPPORTED_OPTION:
				WD_sqlstate_set(env, szSqlState, "HYC00", "IM001");
				/* driver does not support this function */
				break;
			case CONN_INVALID_ARGUMENT_NO:
				WD_sqlstate_set(env, szSqlState, "HY009", "S1009");
				/* invalid argument value */
				break;
			case CONN_TRANSACT_IN_PROGRES:
				WD_sqlstate_set(env, szSqlState, "HY011", "S1011");
				break;
			case CONN_NO_MEMORY_ERROR:
				WD_sqlstate_set(env, szSqlState, "HY001", "S1001");
				break;
			case CONN_NOT_IMPLEMENTED_ERROR:
				WD_sqlstate_set(env, szSqlState, "HYC00", "S1C00");
				break;
			case CONN_ILLEGAL_TRANSACT_STATE:
				WD_sqlstate_set(env, szSqlState, "25000", "S1010");
				break;
			case CONN_VALUE_OUT_OF_RANGE:
				WD_sqlstate_set(env, szSqlState, "HY019", "22003");
				break;
			case CONNECTION_COULD_NOT_SEND:
			case CONNECTION_COULD_NOT_RECEIVE:
			case CONNECTION_COMMUNICATION_ERROR:
			case CONNECTION_NO_RESPONSE:
				WD_sqlstate_set(env, szSqlState, "08S01", "08S01");
				break;
			default:
				WD_sqlstate_set(env, szSqlState, "HY000", "S1000");
				/* general error */
				break;
		}
	}

	MYLOG(0, "	     szSqlState = '%s',len=" FORMAT_SSIZE_T ", szError='%s'\n", szSqlState ? (char *) szSqlState : PRINT_NULL, msglen, szErrorMsg ? (char *) szErrorMsg : PRINT_NULL);
	if (once_again)
	{
		CC_set_errornumber(conn, status);
		return SQL_SUCCESS_WITH_INFO;
	}
	else
		return SQL_SUCCESS;
}

RETCODE		SQL_API
WD_EnvError(HENV henv,
			   SQLSMALLINT	RecNumber,
			   SQLCHAR * szSqlState,
			   SQLINTEGER * pfNativeError,
			   SQLCHAR * szErrorMsg,
			   SQLSMALLINT cbErrorMsgMax,
			   SQLSMALLINT * pcbErrorMsg,
			   UWORD flag)
{
	EnvironmentClass *env = (EnvironmentClass *) henv;
	char		*msg = NULL;
	int		status;

	MYLOG(0, "entering henv=%p <%d>\n", henv, cbErrorMsgMax);
	if (RecNumber != 1 && RecNumber != -1)
		return SQL_NO_DATA_FOUND;
	if (cbErrorMsgMax < 0)
		return SQL_ERROR;
	/*if (!EN_get_error(env, &status, &msg) || NULL == msg)
	{
		MYLOG(0, "EN_get_error: msg = #%s#\n", msg);

		if (NULL != szSqlState)
			WD_sqlstate_set(env, szSqlState, "00000", "00000");
		if (NULL != pcbErrorMsg)
			*pcbErrorMsg = 0;
		if ((NULL != szErrorMsg) && (cbErrorMsgMax > 0))
			szErrorMsg[0] = '\0';

		return SQL_NO_DATA_FOUND;
	}*/
	MYLOG(0, "EN_get_error: status = %d, msg = #%s#\n", status, msg);

	if (NULL != pcbErrorMsg)
		*pcbErrorMsg = (SQLSMALLINT) strlen(msg);
	if ((NULL != szErrorMsg) && (cbErrorMsgMax > 0))
		strncpy_null((char *) szErrorMsg, msg, cbErrorMsgMax);
	if (NULL != pfNativeError)
		*pfNativeError = status;

	if (szSqlState)
	{
		switch (status)
		{
			case ENV_ALLOC_ERROR:
				/* memory allocation failure */
				WD_sqlstate_set(env, szSqlState, "HY001", "S1001");
				break;
			default:
				WD_sqlstate_set(env, szSqlState, "HY000", "S1000");
				/* general error */
				break;
		}
	}

	return SQL_SUCCESS;
}


/*
 * EnvironmentClass implementation
 */
EnvironmentClass *
EN_Constructor(void)
{
	EnvironmentClass *rv = NULL;
#ifdef WIN32
	WORD		wVersionRequested;
	WSADATA		wsaData;
	const int	major = 2, minor = 2;

	/* Load the WinSock Library */
	wVersionRequested = MAKEWORD(major, minor);

	if (WSAStartup(wVersionRequested, &wsaData))
	{
		MYLOG(0, " WSAStartup error\n");
		return rv;
	}
	/* Verify that this is the minimum version of WinSock */
	if (LOBYTE(wsaData.wVersion) >= 1 &&
	    (LOBYTE(wsaData.wVersion) >= 2 ||
	     HIBYTE(wsaData.wVersion) >= 1))
		;
	else
	{
		MYLOG(0, " WSAStartup version=(%d,%d)\n",
			LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));
		goto cleanup;
	}
#endif /* WIN32 */

	rv = (EnvironmentClass *) malloc(sizeof(EnvironmentClass));
	if (NULL == rv)
	{
		MYLOG(0, " malloc error\n");
		goto cleanup;
	}
	rv->errormsg = 0;
	rv->errornumber = 0;
	rv->flag = 0;
	INIT_ENV_CS(rv);
cleanup:
#ifdef WIN32
	if (NULL == rv)
	{
		WSACleanup();
	}
#endif /* WIN32 */

	return rv;
}


char
EN_Destructor(EnvironmentClass *self)
{
	int		lf, nullcnt;
	char		rv = 1;

	MYLOG(0, "entering self=%p\n", self);
	if (!self)
		return 0;

	/*
	 * the error messages are static strings distributed throughout the
	 * source--they should not be freed
	 */

	/* Free any connections belonging to this environment */
	ENTER_CONNS_CS;
	for (lf = 0, nullcnt = 0; lf < conns_count; lf++)
	{
		if (NULL == conns[lf])
			nullcnt++;
		else if (conns[lf]->henv == self)
		{
			if (CC_Destructor(conns[lf]))
				conns[lf] = NULL;
			else
				rv = 0;
			nullcnt++;
		}
	}
	if (conns && nullcnt >= conns_count)
	{
		MYLOG(0, "clearing conns count=%d\n", conns_count);
		free(conns);
		conns = NULL;
		conns_count = 0;
	}
	LEAVE_CONNS_CS;
	DELETE_ENV_CS(self);
	free(self);

#ifdef WIN32
	WSACleanup();
#endif
	MYLOG(0, "leaving rv=%d\n", rv);
#ifdef	_MEMORY_DEBUG_
	debug_memory_check();
#endif   /* _MEMORY_DEBUG_ */
	return rv;
}


char
EN_get_error(EnvironmentClass *self, int *number, const char **message)
{
	if (self && self->errormsg && self->errornumber)
	{
		*message = self->errormsg;
		*number = self->errornumber;
		self->errormsg = 0;
		self->errornumber = 0;
		return 1;
	}
	else
		return 0;
}

#define	INIT_CONN_COUNT	128

char
EN_add_connection(EnvironmentClass *self, ConnectionClass *conn)
{
	int	i, alloc;
	ConnectionClass	**newa;
	char	ret = FALSE;

	MYLOG(0, "entering self = %p, conn = %p\n", self, conn);

	ENTER_CONNS_CS;
	for (i = 0; i < conns_count; i++)
	{
		if (!conns[i])
		{
			conn->henv = self;
			conns[i] = conn;
			ret = TRUE;
			MYLOG(0, "       added at i=%d, conn->henv = %p, conns[i]->henv = %p\n", i, conn->henv, conns[i]->henv);
			goto cleanup;
		}
	}
	if (conns_count > 0)
		alloc = 2 * conns_count;
	else
		alloc = INIT_CONN_COUNT;
	if (newa = (ConnectionClass **) realloc(conns, alloc * sizeof(ConnectionClass *)), NULL == newa)
		goto cleanup;
	conn->henv = self;
	newa[conns_count] = conn;
	conns = newa;
	ret = TRUE;
	MYLOG(0, "       added at %d, conn->henv = %p, conns[%d]->henv = %p\n", conns_count, conn->henv, conns_count, conns[conns_count]->henv);
	for (i = conns_count + 1; i < alloc; i++)
		conns[i] = NULL;
	conns_count = alloc;
cleanup:
	LEAVE_CONNS_CS;
	return ret;
}


char
EN_remove_connection(EnvironmentClass *self, ConnectionClass *conn)
{
	int			i;

	for (i = 0; i < conns_count; i++)
		if (conns[i] == conn && conns[i]->status != CONN_EXECUTING)
		{
			ENTER_CONNS_CS;
			conns[i] = NULL;
			LEAVE_CONNS_CS;
			return TRUE;
		}

	return FALSE;
}


void
EN_log_error(const char *func, const char *desc, EnvironmentClass *self)
{
	if (self)
		MYLOG(0, "ENVIRON ERROR: func=%s, desc='%s', errnum=%d, errmsg='%s'\n", func, desc, self->errornumber, self->errormsg);
	else
		MYLOG(0, "INVALID ENVIRON HANDLE ERROR: func=%s, desc='%s'\n", func, desc);
}
