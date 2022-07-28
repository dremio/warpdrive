/*-------
 * Module:			odbcapi30.cc
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
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
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
#include <odbcabstraction/logger.h>
#include <odbcabstraction/odbc_impl/ODBCConnection.h>
#include <odbcabstraction/odbc_impl/ODBCDescriptor.h>
#include <odbcabstraction/odbc_impl/ODBCEnvironment.h>
#include <odbcabstraction/odbc_impl/ODBCStatement.h>

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
  LOG_TRACE("[{}] Entry with parameters: HandleType '{}', InputHandle '{}'", __FUNCTION__, HandleType, InputHandle);
  SQLRETURN rc = SQL_SUCCESS;
	switch (HandleType) {
		case SQL_HANDLE_ENV:
      // Note: nowhere to write errors to.
      try {
        rc = WD_AllocEnv(OutputHandle);
        LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
        return rc;
      } catch (const std::exception& e) {
        rc = SQL_ERROR;
        LOG_ERROR("[{}] Error occurred allocating environment handle:\n{}\nExiting with return code {}", __FUNCTION__, e.what(), rc);
        return rc;
      } catch (...) {
        rc = SQL_ERROR;
        LOG_ERROR("[{}] Unknown error occurred allocating environment handle. Exiting with return code {}", __FUNCTION__, rc);
        return rc;
      }
		case SQL_HANDLE_DBC:
      rc = ODBCEnvironment::ExecuteWithDiagnostics(InputHandle, rc, [&]() -> SQLRETURN {
        return WD_AllocConnect(InputHandle, OutputHandle);
      });
      LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
      return rc;
		case SQL_HANDLE_STMT:
      rc = ODBCConnection::ExecuteWithDiagnostics(InputHandle, rc, [&]() -> SQLRETURN {
        return WD_AllocStmt(InputHandle, OutputHandle,
                            PODBC_EXTERNAL_STATEMENT |
                            PODBC_INHERIT_CONNECT_OPTIONS);
      });
      LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
      return rc;
		case SQL_HANDLE_DESC:
      rc = ODBCConnection::ExecuteWithDiagnostics(InputHandle, rc, [&]() -> SQLRETURN {
        return WD_AllocDesc(InputHandle, OutputHandle);
      });
      LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
      return rc;
		default:
      rc = SQL_ERROR;
      LOG_ERROR("[{}] Invalid HandleType '{}'. Exiting with return code {}", __FUNCTION__, HandleType, rc);
      return rc;
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
  LOG_TRACE("[{}] Entry with parameters: StatementHandle '{}', ParameterNumber '{}', ValueType '{}', ParameterType '{}', LengthPrecision '{}', ParameterScale '{}', ParameterValue '{}'",
            __FUNCTION__, StatementHandle, ParameterNumber, ValueType, ParameterType, LengthPrecision, ParameterScale, ParameterValue);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    throw DriverException("Unsupported function", "HYC00");
    RETCODE ret;
    StatementClass *stmt = (StatementClass *)StatementHandle;
    int BufferLength = 512; /* Is it OK ? */

    ret = WD_BindParameter(StatementHandle, ParameterNumber, SQL_PARAM_INPUT,
                           ValueType, ParameterType, LengthPrecision,
                           ParameterScale, ParameterValue, BufferLength,
                           StrLen_or_Ind);
    return ret;
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
}

/*	New function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLCloseCursor(HSTMT StatementHandle)
{
  LOG_TRACE("[{}] Entry with parameters: StatementHandle '{}'", __FUNCTION__, StatementHandle);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    ODBCStatement *stmt = ODBCStatement::of(StatementHandle);

    stmt->closeCursor(false);
    return SQL_SUCCESS;
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
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
  LOG_TRACE("[{}] Entry with parameters: StatementHandle '{}', ColumnNumber '{}', FieldIdentifier '{}', CharacterAttribute '{}', BufferLength '{}'", __FUNCTION__, StatementHandle, ColumnNumber, FieldIdentifier, CharacterAttribute, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    return WD_ColAttributes(StatementHandle, ColumnNumber, FieldIdentifier,
                           CharacterAttribute, BufferLength, StringLength,
                           static_cast<SQLLEN *>(NumericAttribute));
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
}
#endif /* UNICODE_SUPPORTXX */

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLCopyDesc(SQLHDESC SourceDescHandle,
			SQLHDESC TargetDescHandle)
{
  LOG_TRACE("[{}] Entry with parameters: SourceDescHandle '{}', TargetDescHandle '{}'", __FUNCTION__, SourceDescHandle, TargetDescHandle);
  SQLRETURN rc = SQL_SUCCESS;
  if (!SourceDescHandle || !TargetDescHandle) {
    rc = SQL_INVALID_HANDLE;
    LOG_TRACE("[{}] No valid source/target SQLHDESC was provided. Exiting with return code {}", __FUNCTION__, rc);
    return rc;
  }

  rc = ODBCDescriptor::ExecuteWithDiagnostics(TargetDescHandle, rc, [&]() -> SQLRETURN {
    throw DriverException("Unsupported function.", "HYC00");
    RETCODE ret;

    ret = WD_CopyDesc(SourceDescHandle, TargetDescHandle);
    return ret;
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

/*	SQLTransact -> SQLEndTran */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLEndTran(SQLSMALLINT HandleType, SQLHANDLE Handle,
		   SQLSMALLINT CompletionType)
{
  LOG_TRACE("[{}] Entry with parameters: HandleType '{}', Handle '{}', CompletionType '{}'", __FUNCTION__, HandleType, Handle, CompletionType);
  SQLRETURN rc = SQL_SUCCESS;

  switch (HandleType) {
    case SQL_HANDLE_ENV:
      rc = ODBCEnvironment::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
        throw DriverException("Unsupported function", "HYC00");
      });
      break;
    case SQL_HANDLE_DBC:
      rc = ODBCConnection::ExecuteWithDiagnostics(Handle, rc, [&]() -> SQLRETURN {
        auto *conn = reinterpret_cast<ODBC::ODBCConnection *>(Handle);
        SQLUINTEGER value;
        conn->GetConnectAttr(SQL_ATTR_AUTOCOMMIT, &value, sizeof(SQLUINTEGER), nullptr, false);
        if (value != SQL_AUTOCOMMIT_ON) {
          throw DriverException("Unsupported function", "HYC00");
        } else {
          return SQL_SUCCESS;
        }
      });
      break;
    default:
      rc = SQL_ERROR;
      LOG_ERROR("[{}] Invalid HandleType '{}'. Exiting with return code {}", __FUNCTION__, HandleType, rc);
      break;
  }
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLFetchScroll(HSTMT StatementHandle,
			   SQLSMALLINT FetchOrientation, SQLLEN FetchOffset)
{
  LOG_TRACE("[{}] Entry with parameters: StatementHandle '{}', FetchOrientation '{}', FetchOffset '{}'", __FUNCTION__, StatementHandle, FetchOrientation, FetchOffset);
  SQLRETURN rc = SQL_SUCCESS;

  rc = ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    if (FetchOrientation != SQL_FETCH_NEXT) {
      throw DriverException("Fetch type unsupported", "HY106");
    }

    return WD_Fetch(StatementHandle);
  });

  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

/*	SQLFree(Connect/Env/Stmt) -> SQLFreeHandle */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLFreeHandle(SQLSMALLINT HandleType, SQLHANDLE Handle)
{
  LOG_TRACE("[{}] Entry with parameters: HandleType '{}', Handle '{}'", __FUNCTION__, HandleType, Handle);
  SQLRETURN rc = SQL_SUCCESS;

  // Do not use ExecuteWithDiagnostics, since freeing handles destroys the diagnostics
  // and ExecuteWithDiagnostics tries to check them after the lambda body.
  switch (HandleType) {
    case SQL_HANDLE_ENV:
      rc = WD_FreeEnv(Handle);
      break;
    case SQL_HANDLE_DBC:
      rc = WD_FreeConnect(Handle);
      break;
    case SQL_HANDLE_STMT:
      rc = WD_FreeStmt(Handle, SQL_DROP);
      break;
    case SQL_HANDLE_DESC:
      rc = WD_FreeDesc(Handle);
      break;
    default:
      rc = SQL_INVALID_HANDLE;
      LOG_ERROR("[{}] Invalid HandleType '{}'. Exiting with return code {}", __FUNCTION__, HandleType, rc);
      break;
  }
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
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
  LOG_TRACE("[{}] Entry with parameters: DescriptorHandle '{}', RecNumber '{}', FieldIdentifier '{}', Value '{}', BufferLength '{}'", __FUNCTION__, DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    return WD_GetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value,
                          BufferLength, StringLength);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
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
  LOG_TRACE("[{}] Entry with parameters: DescriptorHandle '{}', RecNumber '{}', BufferLength '{}'", __FUNCTION__, DescriptorHandle, RecNumber, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    return WD_GetDescRec(DescriptorHandle, RecNumber, Name, BufferLength,
                        StringLength, Type, SubType, Length, Precision, Scale,
                        Nullable);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
				SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
				PTR DiagInfo, SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength)
{
  LOG_TRACE("[{}] Entry with parameters: HandleType '{}', Handle '{}', RecNumber '{}', DiagIdentifier '{}', DiagInfo '{}', BufferLength '{}'", __FUNCTION__, HandleType, Handle, RecNumber, DiagIdentifier, DiagInfo, BufferLength);
  // Note: Do not use the diag manager here.
  RETCODE ret;
  try {
    ret = WD_GetDiagField(HandleType, Handle, RecNumber, DiagIdentifier,
                           DiagInfo, BufferLength, StringLength);

    LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, ret);
    return ret;
  } catch (const std::exception &ex) {
    ret = SQL_ERROR;
    LOG_ERROR("[{}] Error getting diagnostics:\n{}\nExiting with return code {}", __FUNCTION__, ex.what(), ret);
    return ret;
  } catch (...) {
    ret = SQL_ERROR;
    LOG_ERROR("[{}] Unknown error getting diagnostics. Exiting with return code {}", __FUNCTION__, ret);
    return ret;
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
  LOG_TRACE("[{}] Entry with parameters: HandleType '{}', Handle '{}', RecNumber '{}'", __FUNCTION__, HandleType, Handle, RecNumber);
  // Note: Do not use the diag manager here.
  RETCODE ret;
  try {
    ret = WD_GetDiagRec(HandleType, Handle, RecNumber, Sqlstate, NativeError, MessageText, BufferLength, TextLength);
    LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, ret);
    return ret;
  } catch (const std::exception &ex) {
    ret = SQL_ERROR;
    LOG_ERROR("[{}] Error getting diagnostics:\n{}\nExiting with return code {}", __FUNCTION__, ex.what(), ret);
    return ret;
  } catch (...) {
    ret = SQL_ERROR;
    LOG_ERROR("[{}] Unknown error getting diagnostics. Exiting with return code {}", __FUNCTION__, ret);
    return ret;
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
  LOG_TRACE("[{}] Entry with parameters: EnvironmentHandle '{}', Attribute '{}', Value '{}', BufferLength '{}'", __FUNCTION__, EnvironmentHandle, Attribute, Value, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCEnvironment::ExecuteWithDiagnostics(EnvironmentHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    auto *env = reinterpret_cast<ODBC::ODBCEnvironment *>(EnvironmentHandle);

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
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

#ifndef	UNICODE_SUPPORTXX
/*	SQLGetConnectOption -> SQLGetconnectAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetConnectAttr(HDBC ConnectionHandle,
				  SQLINTEGER Attribute, PTR Value,
				  SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
  LOG_TRACE("[{}] Entry with parameters: ConnectionHandle '{}', Attribute '{}', Value '{}', BufferLength '{}'", __FUNCTION__, ConnectionHandle, Attribute, Value, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    return WD_GetConnectAttr(ConnectionHandle, Attribute, Value, BufferLength,
                            StringLength, false);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

/*	SQLGetStmtOption -> SQLGetStmtAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetStmtAttr(HSTMT StatementHandle,
			   SQLINTEGER Attribute, PTR Value,
			   SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
  LOG_TRACE("[{}] Entry with parameters: StatementHandle '{}', Attribute '{}', Value '{}', BufferLength '{}'", __FUNCTION__, StatementHandle, Attribute, Value, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    return WD_GetStmtAttr(StatementHandle, Attribute, Value, BufferLength,
                         StringLength, false);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

/*	SQLSetConnectOption -> SQLSetConnectAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetConnectAttr(HDBC ConnectionHandle,
				  SQLINTEGER Attribute, PTR Value,
				  SQLINTEGER StringLength)
{
  LOG_TRACE("[{}] Entry with parameters: ConnectionHandle '{}', Attribute '{}', Value '{}', StringLength '{}'", __FUNCTION__, ConnectionHandle, Attribute, Value, StringLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    return WD_SetConnectAttr(ConnectionHandle, Attribute, Value, StringLength,
                            false);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetDescField(SQLHDESC DescriptorHandle,
				SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
				PTR Value, SQLINTEGER BufferLength)
{
  LOG_TRACE("[{}] Entry with parameters: DescriptorHandle '{}', RecNumber '{}', FieldIdentifier '{}', Value '{}', BufferLength '{}'", __FUNCTION__, DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    return WD_SetDescField(DescriptorHandle, RecNumber, FieldIdentifier, Value,
                          BufferLength);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
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
  LOG_TRACE("[{}] Entry with parameters: DescriptorHandle '{}', RecNumber '{}', Type '{}', SubType '{}', Length '{}', Precision '{}', Scale '{}', Data '{}'", __FUNCTION__, DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    return WD_SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length,
                        Precision, Scale, Data, StringLength, Indicator);
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}
#endif /* UNICODE_SUPPORTXX */

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetEnvAttr(HENV EnvironmentHandle,
			  SQLINTEGER Attribute, PTR Value,
			  SQLINTEGER StringLength)
{
  LOG_TRACE("[{}] Entry with parameters: EnvironmentHandle '{}', Attribute '{}', Value '{}', StringLength '{}'", __FUNCTION__, EnvironmentHandle, Attribute, Value, StringLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCEnvironment::ExecuteWithDiagnostics(EnvironmentHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    auto *env = reinterpret_cast<ODBC::ODBCEnvironment *>(EnvironmentHandle);

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
    if (ret == SQL_SUCCESS_WITH_INFO) {
      ODBCEnvironment::of(EnvironmentHandle)->GetDiagnostics().AddWarning(
          "Option value changed",
          "01S02", ODBCErrorCodes_GENERAL_WARNING);
    }
    return ret;
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}

#ifndef	UNICODE_SUPPORTXX
/*	SQLSet(Param/Scroll/Stmt)Option -> SQLSetStmtAttr */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetStmtAttr(HSTMT StatementHandle,
			   SQLINTEGER Attribute, PTR Value,
			   SQLINTEGER StringLength)
{
  LOG_TRACE("[{}] Entry with parameters: StatementHandle '{}', Attribute '{}', Value '{}', StringLength '{}'", __FUNCTION__, StatementHandle, Attribute, Value, StringLength);
  SQLRETURN rc = SQL_SUCCESS;
  rc = ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret = SQL_SUCCESS;

    LOG_TRACE("Entering Handle=%p " FORMAT_INTEGER "," FORMAT_ULEN "\n",
          StatementHandle, Attribute, (SQLULEN)Value);
    ret =
        WD_SetStmtAttr(StatementHandle, Attribute, Value, StringLength, false);
    return ret;
        });
  LOG_TRACE("[{}] Exiting successfully with return code {}", __FUNCTION__, rc);
  return rc;
}
#endif /* UNICODE_SUPPORTXX */

#define SQL_FUNC_ESET(pfExists, uwAPI) \
		(*(((UWORD*) (pfExists)) + ((uwAPI) >> 4)) \
			|= (1 << ((uwAPI) & 0x000F)) \
				)

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
    SC_clear_error(stmt);
    StartRollbackState(stmt);
    ret = WD_BulkOperations(hstmt, operation);
    ret = DiscardStatementSvp(stmt, ret, FALSE);
    LEAVE_STMT_CS(stmt);
    return SQL_ERROR;
        });
}

}

RETCODE		SQL_API
WD_GetFunctions30(HDBC hdbc, SQLUSMALLINT fFunction, SQLUSMALLINT FAR* pfExists)
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
