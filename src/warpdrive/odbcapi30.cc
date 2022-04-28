/*-------
 * Module:			odbcapi30.c
 *
 * Description:		This module contains routines related to ODBC 3.0
 *			most of their implementations are temporary
 *			and must be rewritten properly.
 *			2001/07/23	inoue
 *
 * Classes:			n/a
 *
 * API functions:	SQLAllocHandle, SQLBindParam, SQLCloseCursor,
			SQLColAttribute, SQLCopyDesc, SQLEndTran,
			SQLFetchScroll, SQLFreeHandle, SQLGetDescField,
			SQLGetDescRec, SQLGetDiagField, SQLGetDiagRec,
			SQLGetEnvAttr, SQLGetConnectAttr, SQLGetStmtAttr,
			SQLSetConnectAttr, SQLSetDescField, SQLSetDescRec,
			SQLSetEnvAttr, SQLSetStmtAttr, SQLBulkOperations
 *-------
 */

#include "wdodbc.h"
#include "sqlext.h"
#include "misc.h"

#include <stdio.h>
#include <string.h>

#include "environ.h"
#include "connection.h"
#include "statement.h"
#include "wdapifunc.h"

#include <odbcabstraction/exceptions.h>
#include "ODBCConnection.h"
#include "ODBCDescriptor.h"
#include "ODBCEnvironment.h"
#include "ODBCStatement.h"

using namespace ODBC;
using namespace driver::odbcabstraction;

extern "C"
{

/*	SQLAllocConnect/SQLAllocEnv/SQLAllocStmt -> SQLAllocHandle */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLAllocHandle(SQLSMALLINT HandleType,
			   SQLHANDLE InputHandle, SQLHANDLE * OutputHandle)
{
  SQLRETURN rc = SQL_SUCCESS;
	MYLOG(0, "Entering\n");
	switch (HandleType)
	{
		case SQL_HANDLE_ENV:
                    // Note: nowhere to write errors to.
                    try {
                       return WD_AllocEnv(OutputHandle);
                    } catch (const std::exception& e) {
                      MYLOG(0, "Error occurred allocating environment handle: %s", e.what());
                      return SQL_ERROR;
                    } catch (...) {
                      MYLOG(0, "Unknown error occurred allocating environment handle.");
                      return SQL_ERROR;
                    }
		case SQL_HANDLE_DBC:
                {
                  return ODBCEnvironment::ExecuteWithDiagnostics(InputHandle, rc, [&]() -> SQLRETURN {
                    return WD_AllocConnect(InputHandle, OutputHandle);
                                  });
}
		case SQL_HANDLE_STMT:
                  return ODBCConnection::ExecuteWithDiagnostics(InputHandle, rc, [&]() -> SQLRETURN {
                    return WD_AllocStmt(InputHandle, OutputHandle,
                                        PODBC_EXTERNAL_STATEMENT |
                                            PODBC_INHERIT_CONNECT_OPTIONS);
                                      });
		case SQL_HANDLE_DESC:
                  return ODBCConnection::ExecuteWithDiagnostics(InputHandle, rc, [&]() -> SQLRETURN {
                    return WD_AllocDesc(InputHandle, OutputHandle);
                                      });
		default:
			return SQL_ERROR;
	}
}

/*	SQLBindParameter/SQLSetParam -> SQLBindParam */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLBindParam(HSTMT StatementHandle,
			 SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
			 SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
			 SQLSMALLINT ParameterScale, PTR ParameterValue,
			 SQLLEN *StrLen_or_Ind)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    throw DriverException("Unsupported function", "HYC00");
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    int BufferLength = 512; /* Is it OK ? */

    MYLOG(0, "Entering\n");
    ret = WD_BindParameter(StatementHandle, ParameterNumber, SQL_PARAM_INPUT,
                           ValueType, ParameterType, LengthPrecision,
                           ParameterScale, ParameterValue, BufferLength,
                           StrLen_or_Ind);
    return ret;
        });
}

/*	New function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLCloseCursor(HSTMT StatementHandle)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    ODBCStatement *stmt = ODBCStatement::of(StatementHandle);

    MYLOG(0, "Entering\n");

    stmt->closeCursor(false);
    return SQL_SUCCESS;
        });
}

#ifndef	UNICODE_SUPPORTXX
/*	SQLColAttributes -> SQLColAttribute */
WD_EXPORT_SYMBOL
SQLRETURN	SQL_API
SQLColAttribute(SQLHSTMT StatementHandle,
				SQLUSMALLINT ColumnNumber,
				SQLUSMALLINT FieldIdentifier,
				SQLPOINTER CharacterAttribute,
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength,
#if defined(WITH_UNIXODBC) || defined(_WIN64) || defined(SQLCOLATTRIBUTE_SQLLEN)
				SQLLEN *NumericAttribute
#else
				SQLPOINTER NumericAttribute
#endif
			)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_ColAttributes(StatementHandle, ColumnNumber, FieldIdentifier,
                           CharacterAttribute, BufferLength, StringLength,
                           static_cast<SQLLEN *>(NumericAttribute));
        });
}
#endif /* UNICODE_SUPPORTXX */

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLCopyDesc(SQLHDESC SourceDescHandle,
			SQLHDESC TargetDescHandle)
{
  if (!SourceDescHandle || !TargetDescHandle) {
    return SQL_INVALID_HANDLE;
  }

  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(TargetDescHandle, rc, [&]() -> SQLRETURN {
    throw DriverException("Unsupported function.", "HYC00");
    RETCODE ret;

    MYLOG(0, "Entering\n");
    ret = WD_CopyDesc(SourceDescHandle, TargetDescHandle);
    return ret;
        });
}

/*	SQLTransact -> SQLEndTran */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle,
		   SQLSMALLINT CompletionType)
{
  SQLRETURN rc = SQL_SUCCESS;
	RETCODE	ret;

	MYLOG(0, "Entering\n");
	switch (HandleType)
	{
		case SQL_HANDLE_ENV:
                  return ODBCEnvironment::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
                    throw DriverException("Unsupported function", "HYC00");
                    return WD_Transact(Handle, SQL_NULL_HDBC, CompletionType);
                                      });
		case SQL_HANDLE_DBC:
                  return ODBCConnection::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
                    throw DriverException("Unsupported function", "HYC00");
                    return WD_Transact(SQL_NULL_HENV, Handle, CompletionType);
                                      });
		default:
			return SQL_ERROR;
			break;
	}
	return ret;
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLFetchScroll(HSTMT StatementHandle,
			   SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    if (FetchOrientation != SQL_FETCH_NEXT) {
      throw DriverException("Fetch type unsupported", "HY106");
    }

    struct ARDArraySizeTracker {
      ARDArraySizeTracker(ODBCDescriptor *ard, SQLLEN newSize)
          : m_ard(ard), m_newSize(newSize), m_oldSize(ard->GetArraySize()) {
        if (m_newSize != m_oldSize) {
          m_ard->SetHeaderField(SQL_DESC_ARRAY_SIZE,
                                reinterpret_cast<SQLPOINTER>(m_newSize), 0);
        }
      }

      ~ARDArraySizeTracker() {
        if (m_newSize != m_oldSize) {
          m_ard->SetHeaderField(SQL_DESC_ARRAY_SIZE,
                                reinterpret_cast<SQLPOINTER>(m_oldSize), 0);
        }
      }

      ODBCDescriptor *m_ard;
      SQLLEN m_newSize;
      SQLLEN m_oldSize;
    };

    ODBCStatement *stmt = ODBCStatement::of(StatementHandle);
    ODBCDescriptor *ard = stmt->GetARD();

    ARDArraySizeTracker tracker(ard, FetchOffset);

    RETCODE result = WD_Fetch(StatementHandle);
    return result;
  });
}

/*	SQLFree(Connect/Env/Stmt) -> SQLFreeHandle */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  SQLRETURN rc = SQL_SUCCESS;
	RETCODE		ret;
	MYLOG(0, "Entering\n");

	switch (HandleType)
	{
		case SQL_HANDLE_ENV:
                  return ODBCEnvironment::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
                    return WD_FreeEnv(Handle);
                                      });
			break;
		case SQL_HANDLE_DBC:
                  return ODBCConnection::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
                    return WD_FreeConnect(Handle);
                                      });
			break;
		case SQL_HANDLE_STMT:
                  return ODBCStatement::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
                    return WD_FreeStmt(Handle, SQL_DROP);
                                      });
			break;
		case SQL_HANDLE_DESC:
                  return ODBCDescriptor::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
                    return WD_FreeDesc(Handle);
                                      });
			break;
		default:
			ret = SQL_ERROR;
			break;
	}
	return ret;
}
		
#ifndef	UNICODE_SUPPORTXX
/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetDescField(SQLHDESC DescriptorHandle,
				SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
				PTR Value, SQLINTEGER BufferLength,
				SQLINTEGER *StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_GetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value,
                          BufferLength, StringLength);
        });
}

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetDescRec(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLCHAR *Name,
			  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
			  SQLSMALLINT *Type, SQLSMALLINT *SubType,
			  SQLLEN *Length, SQLSMALLINT *Precision,
			  SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_GetDescRec(DescriptorHandle, RecNumber, Name, BufferLength,
                        StringLength, Type, SubType, Length, Precision, Scale,
                        Nullable);
        });
}

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
				SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
				PTR DiagInfo, SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{
  // Note: Do not use the diag manager here.
	RETCODE	ret;
        try {

          MYLOG(0, "Entering Handle=(%u,%p) Rec=%d Id=%d info=(%p,%d)\n",
                HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo,
                BufferLength);
          return WD_GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier,
                                DiagInfo, BufferLength, StringLength);
        } catch (std::exception& ex) {
          MYLOG(0, "Error getting diagnostics: %s", ex.what());
          return SQL_ERROR;
        } catch (...) {
          MYLOG(0, "Unknown error getting diagnostics.");
          return SQL_ERROR;
        }
}

/*	SQLError -> SQLDiagRec */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle,
			  SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
			  SQLINTEGER *NativeError, SQLCHAR *MessageText,
			  SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
  // Note: Do not use the diag manager here.
	RETCODE	ret;
        try {

          MYLOG(0, "Entering\n");
          ret =
              WD_GetDiagRec(HandleType, Handle, RecNumber, Sqlstate,
                            NativeError, MessageText, BufferLength, TextLength);
          return ret;
        } catch (std::exception& ex) {
          MYLOG(0, "Error getting diagnostics: %s", ex.what());
          return SQL_ERROR;
        } catch (...) {
          MYLOG(0, "Unknown error getting diagnostics.");
          return SQL_ERROR;
        }
}
#endif /* UNICODE_SUPPORTXX */

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetEnvAttr(HENV EnvironmentHandle,
			  SQLINTEGER Attribute, PTR Value,
			  SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCEnvironment::ExecuteWithDiagnostics(EnvironmentHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    ODBC::ODBCEnvironment *env =
        reinterpret_cast<ODBC::ODBCEnvironment *>(EnvironmentHandle);

    MYLOG(0, "Entering " FORMAT_INTEGER "\n", Attribute);
    ret = SQL_SUCCESS;
    switch (Attribute) {
    case SQL_ATTR_CONNECTION_POOLING:
      *((SQLINTEGER *)Value) = env->getConnectionPooling();
      break;
    case SQL_ATTR_CP_MATCH:
      *((SQLINTEGER *)Value) = SQL_CP_RELAXED_MATCH;
      break;
    case SQL_ATTR_ODBC_VERSION:
      *((SQLINTEGER *)Value) = env->getODBCVersion();
      break;
    case SQL_ATTR_OUTPUT_NTS:
      *((SQLINTEGER *)Value) = SQL_TRUE;
      break;
    default:
      throw DriverException("Invalid environment attribute", "HY024");
    }
    return ret;
        });
}

#ifndef	UNICODE_SUPPORTXX
/*	SQLGetConnectOption -> SQLGetconnectAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetConnectAttr(HDBC ConnectionHandle,
				  SQLINTEGER Attribute, PTR Value,
				  SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret = SQL_SUCCESS;

    MYLOG(0, "Entering " FORMAT_UINTEGER "\n", Attribute);
    return WD_GetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength,
                            StringLength, false);
    return ret;
        });
}

/*	SQLGetStmtOption -> SQLGetStmtAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetStmtAttr(HSTMT StatementHandle,
			   SQLINTEGER Attribute, PTR Value,
			   SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering Handle=%p " FORMAT_INTEGER "\n", StatementHandle,
          Attribute);
    return WD_GetStmtAttr(StatementHandle, Attribute, Value, BufferLength,
                         StringLength, false);
        });
}

/*	SQLSetConnectOption -> SQLSetConnectAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetConnectAttr(HDBC ConnectionHandle,
				  SQLINTEGER Attribute, PTR Value,
				  SQLINTEGER StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering " FORMAT_INTEGER "\n", Attribute);
    return WD_SetConnectAttr(ConnectionHandle, Attribute, Value, StringLength,
                            false);
        });
}

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetDescField(SQLHDESC DescriptorHandle,
				SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
				PTR Value, SQLINTEGER BufferLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering h=%p rec=%d field=%d val=%p\n", DescriptorHandle,
          RecNumber, FieldIdentifier, Value);
    return WD_SetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value,
                          BufferLength);
        });
}

/*	new fucntion */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetDescRec(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLSMALLINT Type,
			  SQLSMALLINT SubType, SQLLEN Length,
			  SQLSMALLINT Precision, SQLSMALLINT Scale,
			  PTR Data, SQLLEN *StringLength,
			  SQLLEN *Indicator)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length,
                        Precision, Scale, Data, StringLength, Indicator);
        });
}
#endif /* UNICODE_SUPPORTXX */

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetEnvAttr(HENV EnvironmentHandle,
			  SQLINTEGER Attribute, PTR Value,
			  SQLINTEGER StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCEnvironment::ExecuteWithDiagnostics(EnvironmentHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    ODBC::ODBCEnvironment *env =
        reinterpret_cast<ODBC::ODBCEnvironment *>(EnvironmentHandle);

    MYLOG(0, "Entering att=" FORMAT_INTEGER "," FORMAT_ULEN "\n", Attribute,
          (SQLULEN)Value);
    switch (Attribute) {
    case SQL_ATTR_CONNECTION_POOLING:
      switch ((ULONG_PTR)Value) {
      case SQL_CP_OFF:
      case SQL_CP_ONE_PER_DRIVER:
        env->setConnectionPooling(CAST_UPTR(SQLUINTEGER, Value));
        ret = SQL_SUCCESS;
        break;
      default:
        ret = SQL_SUCCESS_WITH_INFO;
      }
      break;
    case SQL_ATTR_CP_MATCH:
      /* *((unsigned int *) Value) = SQL_CP_RELAXED_MATCH; */
      ret = SQL_SUCCESS;
      break;
    case SQL_ATTR_ODBC_VERSION:
      env->setODBCVersion(CAST_UPTR(SQLUINTEGER, Value));
      ret = SQL_SUCCESS;
      break;
    case SQL_ATTR_OUTPUT_NTS:
      if (SQL_TRUE == CAST_UPTR(SQLUINTEGER, Value))
        ret = SQL_SUCCESS;
      else
        ret = SQL_SUCCESS_WITH_INFO;
      break;
    default:
      throw DriverException("Invalid environment attribute", "HY024");
    }
    if (SQL_SUCCESS_WITH_INFO == ret) {
      ODBCEnvironment::of(EnvironmentHandle)->GetDiagnostics().AddWarning(
          "Option value changed",
          "01S02", ODBCErrorCodes_GENERAL_WARNING);
    }
    return ret;
        });
}

#ifndef	UNICODE_SUPPORTXX
/*	SQLSet(Param/Scroll/Stmt)Option -> SQLSetStmtAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetStmtAttr(HSTMT StatementHandle,
			   SQLINTEGER Attribute, PTR Value,
			   SQLINTEGER StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret = SQL_SUCCESS;

    MYLOG(0, "Entering Handle=%p " FORMAT_INTEGER "," FORMAT_ULEN "\n",
          StatementHandle, Attribute, (SQLULEN)Value);
    ret =
        WD_SetStmtAttr(StatementHandle, Attribute, Value, StringLength, false);
    return ret;
        });
}
#endif /* UNICODE_SUPPORTXX */

#define SQL_FUNC_ESET(pfExists, uwAPI) \
		(*(((UWORD*) (pfExists)) + ((uwAPI) >> 4)) \
			|= (1 << ((uwAPI) & 0x000F)) \
				)
RETCODE		SQL_API
WD_GetFunctions30(HDBC hdbc, SQLUSMALLINT fFunction, SQLUSMALLINT FAR * pfExists)
{
	if (fFunction != SQL_API_ODBC3_ALL_FUNCTIONS)
		return SQL_ERROR;
	memset(pfExists, 0, sizeof(UWORD) * SQL_API_ODBC3_ALL_FUNCTIONS_SIZE);

	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLALLOCCONNECT); 1 deprecated */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLALLOCENV); 2 deprecated */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLALLOCSTMT); 3 deprecated */

	/*
	 * for (i = SQL_API_SQLBINDCOL; i <= 23; i++) SQL_FUNC_ESET(pfExists,
	 * i);
	 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLBINDCOL);		/* 4 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLCANCEL);		/* 5 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLCOLATTRIBUTE);	/* 6 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLCONNECT);		/* 7 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLDESCRIBECOL);	/* 8 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLDISCONNECT);		/* 9 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLERROR);  10 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLEXECDIRECT);		/* 11 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLEXECUTE);		/* 12 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLFETCH);	/* 13 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLFREECONNECT); 14 deprecated */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLFREEENV); 15 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLFREESTMT);		/* 16 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETCURSORNAME);	/* 17 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLNUMRESULTCOLS);	/* 18 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLPREPARE);		/* 19 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLROWCOUNT);		/* 20 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETCURSORNAME);	/* 21 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLSETPARAM); 22 deprecated */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLTRANSACT); 23 deprecated */

	/*
	 * for (i = 40; i < SQL_API_SQLEXTENDEDFETCH; i++)
	 * SQL_FUNC_ESET(pfExists, i);
	 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLCOLUMNS);		/* 40 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLDRIVERCONNECT);	/* 41 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLGETCONNECTOPTION); 42 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETDATA);		/* 43 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETFUNCTIONS);	/* 44 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETINFO);		/* 45 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLGETSTMTOPTION); 46 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETTYPEINFO);	/* 47 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLPARAMDATA);		/* 48 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLPUTDATA);		/* 49 */

	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLSETCONNECTIONOPTION); 50 deprecated */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLSETSTMTOPTION); 51 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSPECIALCOLUMNS);	/* 52 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSTATISTICS);		/* 53 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLTABLES);		/* 54 */
//	if (ci->drivers.lie)
//		SQL_FUNC_ESET(pfExists, SQL_API_SQLBROWSECONNECT);	/* 55 */
//	if (ci->drivers.lie)
//		SQL_FUNC_ESET(pfExists, SQL_API_SQLCOLUMNPRIVILEGES);	/* 56 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLDATASOURCES);	/* 57 */
//	if (SUPPORT_DESCRIBE_PARAM(ci) || ci->drivers.lie)
//		SQL_FUNC_ESET(pfExists, SQL_API_SQLDESCRIBEPARAM); /* 58 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLEXTENDEDFETCH); /* 59 deprecated ? */

	/*
	 * for (++i; i < SQL_API_SQLBINDPARAMETER; i++)
	 * SQL_FUNC_ESET(pfExists, i);
	 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLFOREIGNKEYS);	/* 60 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLMORERESULTS);	/* 61 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLNATIVESQL);		/* 62 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLNUMPARAMS);		/* 63 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLPARAMOPTIONS); 64 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLPRIMARYKEYS);	/* 65 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLPROCEDURECOLUMNS);	/* 66 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLPROCEDURES);		/* 67 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETPOS);		/* 68 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLSETSCROLLOPTIONS); 69 deprecated */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLTABLEPRIVILEGES);	/* 70 */
	/* SQL_FUNC_ESET(pfExists, SQL_API_SQLDRIVERS); */	/* 71 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLBINDPARAMETER);	/* 72 */

	SQL_FUNC_ESET(pfExists, SQL_API_SQLALLOCHANDLE);	/* 1001 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLBINDPARAM);		/* 1002 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLCLOSECURSOR);	/* 1003 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLCOPYDESC);		/* 1004 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLENDTRAN);		/* 1005 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLFREEHANDLE);		/* 1006 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETCONNECTATTR);	/* 1007 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETDESCFIELD);	/* 1008 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETDESCREC); /* 1009 not implemented yet */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETDIAGFIELD); /* 1010 minimal implementation */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETDIAGREC);		/* 1011 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETENVATTR);		/* 1012 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLGETSTMTATTR);	/* 1014 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETCONNECTATTR);	/* 1016 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETDESCFIELD);	/* 1017 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETDESCREC); /* 1018 not implemented yet */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETENVATTR);		/* 1019 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLSETSTMTATTR);	/* 1020 */
	SQL_FUNC_ESET(pfExists, SQL_API_SQLFETCHSCROLL);	/* 1021 */
	//SQL_FUNC_ESET(pfExists, SQL_API_SQLBULKOPERATIONS);	/* 24 */

	return SQL_SUCCESS;
}

RETCODE	SQL_API
SQLBulkOperations(HSTMT hstmt, SQLSMALLINT operation)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    throw DriverException("Unsupported function", "HYC00");
    StatementClass *stmt = (StatementClass *)hstmt;

    if (SC_connection_lost_check(stmt, __FUNCTION__))
      return SQL_ERROR;

    ENTER_STMT_CS(stmt);
    MYLOG(0, "Entering Handle=%p %d\n", hstmt, operation);
    SC_clear_error(stmt);
    StartRollbackState(stmt);
    ret = WD_BulkOperations(hstmt, operation);
    ret = DiscardStatementSvp(stmt, ret, FALSE);
    LEAVE_STMT_CS(stmt);
    return SQL_ERROR;
        });
}

}
