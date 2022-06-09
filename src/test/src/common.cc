/*--------
 * Module:			common.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */
#include "common.h"

SQLHENV env;
SQLHDBC conn;

std::string
format_diagnostic(const std::string& msg, SQLSMALLINT htype, SQLHANDLE handle) {
	return msg + get_diagnostic(handle, htype);
}

std::string
get_diagnostic(SQLHANDLE handle, SQLSMALLINT htype) {
	std::string buf;
	SQLCHAR     sqlstate[32];
	SQLCHAR     message[1000];
	SQLINTEGER	nativeerror;
	SQLSMALLINT textlen;
	SQLRETURN	ret;
	SQLSMALLINT	recno = 0;

	do {
		recno++;
		ret = SQLGetDiagRec(htype, handle, recno, sqlstate, &nativeerror,
							message, sizeof(message), &textlen);
		if (ret == SQL_INVALID_HANDLE) {
			return "Invalid handle";
		}
		else if (SQL_SUCCEEDED(ret)) {
			buf.append(reinterpret_cast<char*>(sqlstate));
			buf.append("=");
			buf.append(reinterpret_cast<char*>(message));
			buf.append("\n");
		}
	} while (ret == SQL_SUCCESS);

	if (ret == SQL_NO_DATA && recno == 1) {
		return "No error information";
	}

	return buf;
}

void print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle) {
	std::cout << format_diagnostic(std::string(msg), htype, handle);
}

void print_diag(const std::string& msg, SQLSMALLINT htype, SQLHANDLE handle) {
	std::cout << format_diagnostic(msg, htype, handle);
}

const char * const default_dsn = "Arrow";
const char * const test_dsn_env = "DSN";
const char * const test_dsn_ansi = "psqlodbc_test_dsn_ansi";

const char *get_test_dsn(void)
{
	char	*env = getenv(test_dsn_env);

	if (NULL != env && env[0])
		return env;
	return default_dsn;
}

int IsAnsi(void)
{
	return (NULL != strstr(get_test_dsn(), "_ansi"));
}

bool
test_connect_ext(char *extraparams, std::string *err_msg)
{
	SQLRETURN ret;
	SQLCHAR str[1024];
	SQLSMALLINT strl;
	char dsn[1024];
	const char * const test_dsn = get_test_dsn();
	char *envvar;

	/*
	 *	Use an environment variable to switch settings of connection
	 *	strings throughout the regression test. Note that extraparams
	 *	parameters have precedence over the environment variable.
	 *	ODBC spec says
	 *		If any keywords are repeated in the connection string,
	 *		the driver uses the value associated with the first
	 *		occurrence of the keyword.
	 *	But the current psqlodbc driver uses the value associated with
	 *	the last occurrence of the keyword. Here we place extraparams
	 *	both before and after the value of the environment variable
	 *	so as to protect the priority order whichever way we take.
	 */
	if ((envvar = getenv("COMMON_CONNECTION_STRING_FOR_REGRESSION_TEST")) != NULL && envvar[0] != '\0')
	{
		if (NULL == extraparams)
			snprintf(dsn, sizeof(dsn), "DSN=%s;%s", test_dsn, envvar);
		else
			snprintf(dsn, sizeof(dsn), "DSN=%s;%s;%s;%s",
			 test_dsn, extraparams, envvar, extraparams);
	}
	else
		snprintf(dsn, sizeof(dsn), "DSN=%s;%s",
			 test_dsn, extraparams ? extraparams : "");

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

	SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);
	ret = SQLDriverConnect(conn, NULL, (SQLCHAR*) dsn, SQL_NTS,
						   str, sizeof(str), &strl,
						   SQL_DRIVER_COMPLETE);
  if (SQL_SUCCEEDED(ret)){
    return true;
  }else{
    *err_msg = get_diagnostic(conn, SQL_HANDLE_DBC);
    return false;
  }
}

bool
test_connect(std::string *err_msg)
{
  return test_connect_ext(NULL, err_msg);
}

bool
test_disconnect(std::string *err_msg)
{
  SQLRETURN rc;
  rc = SQLDisconnect(conn);
  if (!SQL_SUCCEEDED(rc))
  {
    *err_msg = "SQLDisconnect failed";
    return false;
  }

  rc = SQLFreeHandle(SQL_HANDLE_DBC, conn);
  if (!SQL_SUCCEEDED(rc))
  {
    *err_msg = "SQLFreeHandle failed";
    return false;
  }
  conn = nullptr;

  rc = SQLFreeHandle(SQL_HANDLE_ENV, env);
  if (!SQL_SUCCEEDED(rc))
  {
    *err_msg = format_diagnostic("SQLFreeHandle failed", SQL_HANDLE_ENV, env);
    return false;
  }
  env = nullptr;
  return true;
}

const char *
datatype_str(SQLSMALLINT datatype)
{
	static char buf[100];

	switch (datatype)
	{
		case SQL_CHAR:
			return "CHAR";
		case SQL_VARCHAR:
			return "VARCHAR";
		case SQL_LONGVARCHAR:
			return "LONGVARCHAR";
		case SQL_WCHAR:
			return "WCHAR";
		case SQL_WVARCHAR:
			return "WVARCHAR";
		case SQL_WLONGVARCHAR:
			return "WLONGVARCHAR";
		case SQL_DECIMAL:
			return "DECIMAL";
		case SQL_NUMERIC:
			return "NUMERIC";
		case SQL_SMALLINT:
			return "SMALLINT";
		case SQL_INTEGER:
			return "INTEGER";
		case SQL_REAL:
			return "REAL";
		case SQL_FLOAT:
			return "FLOAT";
		case SQL_DOUBLE:
			return "DOUBLE";
		case SQL_BIT:
			return "BIT";
		case SQL_TINYINT:
			return "TINYINT";
		case SQL_BIGINT:
			return "BIGINT";
		case SQL_BINARY:
			return "BINARY";
		case SQL_VARBINARY:
			return "VARBINARY";
		case SQL_LONGVARBINARY:
			return "LONGVARBINARY";
		case SQL_TYPE_DATE:
			return "TYPE_DATE";
		case SQL_TYPE_TIME:
			return "TYPE_TIME";
		case SQL_TYPE_TIMESTAMP:
			return "TYPE_TIMESTAMP";
		case SQL_GUID:
			return "GUID";
		default:
			snprintf(buf, sizeof(buf), "unknown sql type %d", datatype);
			return buf;
	}
}

const char *nullable_str(SQLSMALLINT nullable)
{
	static char buf[100];

	switch(nullable)
	{
		case SQL_NO_NULLS:
			return "not nullable";
		case SQL_NULLABLE:
			return "nullable";
		case SQL_NULLABLE_UNKNOWN:
			return "nullable_unknown";
		default:
			snprintf(buf, sizeof(buf), "unknown nullable value %d", nullable);
			return buf;
	}
}

boost::optional<std::string>
print_result_meta_series(HSTMT hstmt,
						 SQLSMALLINT *colids,
						 SQLSMALLINT numcols,
             std::string *err_msg)
{
  std::string result;
	for (int i = 0; i < numcols; i++)
	{
		SQLRETURN rc;
		SQLCHAR colname[50];
		SQLSMALLINT colnamelen;
		SQLSMALLINT datatype;
		SQLULEN colsize;
		SQLSMALLINT decdigits= 0;
		SQLSMALLINT nullable;

		rc = SQLDescribeCol(hstmt, colids[i],
							colname, sizeof(colname),
							&colnamelen,
							&datatype,
							&colsize,
							&decdigits,
							&nullable);
		if (!SQL_SUCCEEDED(rc))
		{
      *err_msg= format_diagnostic("SQLDescribeCol failed", SQL_HANDLE_STMT, hstmt);
			return boost::none;
		}
    char buffer [200];
		snprintf(buffer, 200, "%s: %s(%u) digits: %d, %s\n",
			   colname, datatype_str(datatype), (unsigned int) colsize,
			   decdigits, nullable_str(nullable));
    result.append(buffer);
	}
  return result;
}

boost::optional<std::string>
print_result_meta(HSTMT hstmt, std::string *err_msg)
{
	SQLRETURN rc;
	SQLSMALLINT numcols, i;
	SQLSMALLINT *colids;

	rc = SQLNumResultCols(hstmt, &numcols);
	if (!SQL_SUCCEEDED(rc))
	{
		*err_msg = format_diagnostic("SQLNumResultCols failed", SQL_HANDLE_STMT, hstmt);
		return boost::none;
	}

	colids = (SQLSMALLINT *) malloc(numcols * sizeof(SQLSMALLINT));
	for (i = 0; i < numcols; i++){
    colids[i] = i + 1;
  }

  boost::optional<std::string> result = print_result_meta_series(hstmt, colids, numcols, err_msg);
	return result.has_value() ? result : boost::none;
}

/*
 * Initialize a buffer with "XxXxXx..." to indicate an uninitialized value.
 */
static void
invalidate_buf(char *buf, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		if (i % 2 == 0)
			buf[i] = 'X';
		else
			buf[i] = 'x';
	}
	buf[len - 1] = '\0';
}

/*
 * Print result only for the selected columns.
 */
boost::optional<std::string>
get_result_series(HSTMT hstmt, SQLSMALLINT *colids, SQLSMALLINT numcols, SQLINTEGER rowcount, std::string *err_msg)
{
  SQLRETURN rc;
  SQLINTEGER	rowc = 0;
  std::string result_buf;
  while (rowcount <0 || rowc < rowcount)
  {
    rc = SQLFetch(hstmt);
    if (rc == SQL_NO_DATA)
      break;
    if (rc == SQL_SUCCESS)
    {
      char buf[40];
      SQLLEN ind;

      rowc++;
      for (int i = 0; i < numcols; i++)
      {
        /*
         * Initialize the buffer with garbage, so that we see readily
         * if SQLGetData fails to set the value properly or forgets
         * to null-terminate it.
         */
        invalidate_buf(buf, sizeof(buf));
        rc = SQLGetData(hstmt, colids[i], SQL_C_CHAR, buf, sizeof(buf), &ind);
        if (!SQL_SUCCEEDED(rc))
        {
          *err_msg = format_diagnostic("SQLGetData failed", SQL_HANDLE_STMT, hstmt);
          return boost::none;
        }

        // Check null data
        if (ind == SQL_NULL_DATA){
          result_buf.append("NULL");
        }else{
          result_buf.append(buf, strlen(buf));
        }

        // Check tab addition
        if (i != numcols -1){
          result_buf.append("\t");
        }
      }
      result_buf.append("\n");
    }
    else
    {
      *err_msg = format_diagnostic("SQLFetch failed", SQL_HANDLE_STMT, hstmt);
      return boost::none;
    }
  }
  return result_buf;
}

/*
 * Print result on all the columns
 */
boost::optional<std::string>
get_result(HSTMT hstmt, std::string *err_msg)
{
  SQLRETURN rc;
  SQLSMALLINT numcols, i;
  SQLSMALLINT *colids;
  boost::optional<std::string> result;

  rc = SQLNumResultCols(hstmt, &numcols);
  if (!SQL_SUCCEEDED(rc))
  {
    *err_msg = format_diagnostic("SQLNumResultCols failed", SQL_HANDLE_STMT, hstmt);
    return boost::none;
  }

  colids = (SQLSMALLINT *) malloc(numcols * sizeof(SQLSMALLINT));
  for (i = 0; i < numcols; i++)
    colids[i] = i + 1;
  result = get_result_series(hstmt, colids, numcols, -1, err_msg);
  free(colids);

  return result.has_value() ? result : boost::none;
}
