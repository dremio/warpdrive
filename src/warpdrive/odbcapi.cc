/*-------
 * Module:			odbcapi.cc
 *
 * Description:		This module contains routines related to
 *					preparing and executing an SQL statement.
 *
 * Classes:			n/a
 *
 * API functions:	SQLAllocConnect, SQLAllocEnv, SQLAllocStmt,
			SQLBindCol, SQLCancel, SQLColumns, SQLConnect,
			SQLDataSources, SQLDescribeCol, SQLDisconnect,
			SQLError, SQLExecDirect, SQLExecute, SQLFetch,
			SQLFreeConnect, SQLFreeEnv, SQLFreeStmt,
			SQLGetConnectOption, SQLGetCursorName, SQLGetData,
			SQLGetFunctions, SQLGetInfo, SQLGetStmtOption,
			SQLGetTypeInfo, SQLNumResultCols, SQLParamData,
			SQLPrepare, SQLPutData, SQLRowCount,
			SQLSetConnectOption, SQLSetCursorName, SQLSetParam,
			SQLSetStmtOption, SQLSpecialColumns, SQLStatistics,
			SQLTables, SQLTransact, SQLColAttributes,
			SQLColumnPrivileges, SQLDescribeParam, SQLExtendedFetch,
			SQLForeignKeys, SQLMoreResults, SQLNativeSql,
			SQLNumParams, SQLParamOptions, SQLPrimaryKeys,
			SQLProcedureColumns, SQLProcedures, SQLSetPos,
			SQLTablePrivileges, SQLBindParameter
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *-------
 */

#include "wdodbc.h"
#include "sqlext.h"
#include <stdio.h>
#include <string.h>

#include "misc.h"
#include "wdapifunc.h"
#include "environ.h"
#include "connection.h"
#include "statement.h"
#include "qresult.h"
#include "loadlib.h"

#include <odbcabstraction/exceptions.h>
#include <odbcabstraction/odbc_impl/ODBCDescriptor.h>
#include <odbcabstraction/odbc_impl/ODBCConnection.h>
#include <odbcabstraction/odbc_impl/ODBCStatement.h>

using namespace ODBC;
using namespace driver::odbcabstraction;

BOOL	SC_connection_lost_check(StatementClass *stmt, const char *funcname)
{
  return FALSE;
#if 0
	ConnectionClass	*conn = SC_get_conn(stmt);
	char	message[64];

	if (NULL != conn->pqconn)
		return	FALSE;
	SC_clear_error(stmt);
	SPRINTF_FIXED(message, "%s unable due to the connection lost", funcname);
	SC_set_error(stmt, STMT_COMMUNICATION_ERROR, message, funcname);
	return TRUE;
#endif
}

static BOOL theResultIsEmpty(const StatementClass *stmt)
{
	QResultClass	*res = SC_get_Result(stmt);
	if (NULL == res)
		return FALSE;
	return (0 == QR_get_num_total_tuples(res));
}

extern "C" {
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLBindCol(HSTMT StatementHandle,
		   SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
		   PTR TargetValue, SQLLEN BufferLength,
		   SQLLEN *StrLen_or_Ind)
{
  SQLRETURN rc = SQL_SUCCESS;
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_BindCol(StatementHandle, ColumnNumber, TargetType, TargetValue,
                           BufferLength, StrLen_or_Ind);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLCancel(HSTMT StatementHandle)
{
  SQLRETURN rc = SQL_SUCCESS;

  auto func = [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_Cancel(StatementHandle);
  };
  return ODBCStatement::ExecuteWithDiagnostics<decltype(func), false>(StatementHandle, rc, func);
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLColumns(HSTMT StatementHandle,
		   SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
		   SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
		   SQLCHAR *TableName, SQLSMALLINT NameLength3,
		   SQLCHAR *ColumnName, SQLSMALLINT NameLength4)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLColumns";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName,
                  *clName = ColumnName;
          return WD_Columns(StatementHandle, ctName, NameLength1, scName, NameLength2,
                           tbName, NameLength3, clName, NameLength4);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLConnect(HDBC ConnectionHandle,
		   SQLCHAR *ServerName, SQLSMALLINT NameLength1,
		   SQLCHAR *UserName, SQLSMALLINT NameLength2,
		   SQLCHAR *Authentication, SQLSMALLINT NameLength3)
{
  SQLRETURN rc = SQL_SUCCESS;
        return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_Connect(ConnectionHandle, ServerName, NameLength1, UserName,
                           NameLength2, Authentication, NameLength3);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLDriverConnect(HDBC hdbc,
				 HWND hwnd,
				 SQLCHAR * szConnStrIn,
				 SQLSMALLINT cbConnStrIn,
				 SQLCHAR * szConnStrOut,
				 SQLSMALLINT cbConnStrOutMax,
				 SQLSMALLINT * pcbConnStrOut,
				 SQLUSMALLINT fDriverCompletion)
{
  SQLRETURN rc = SQL_SUCCESS;
        return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() -> SQLRETURN {

          MYLOG(0, "Entering\n");
          return WD_DriverConnect(hdbc, hwnd, szConnStrIn, cbConnStrIn, szConnStrOut,
                                 cbConnStrOutMax, pcbConnStrOut, fDriverCompletion);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLBrowseConnect(HDBC hdbc,
				 SQLCHAR *szConnStrIn,
				 SQLSMALLINT cbConnStrIn,
				 SQLCHAR *szConnStrOut,
				 SQLSMALLINT cbConnStrOutMax,
				 SQLSMALLINT *pcbConnStrOut)
{
  SQLRETURN rc = SQL_SUCCESS;

	MYLOG(0, "Entering\n");
        return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() -> SQLRETURN {
          return WD_BrowseConnect(hdbc, szConnStrIn, cbConnStrIn, szConnStrOut,
                                 cbConnStrOutMax, pcbConnStrOut);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLDataSources(HENV EnvironmentHandle,
			   SQLUSMALLINT Direction, SQLCHAR *ServerName,
			   SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1,
			   SQLCHAR *Description, SQLSMALLINT BufferLength2,
			   SQLSMALLINT *NameLength2)
{
	MYLOG(0, "Entering\n");

	/*
	 * return WD_DataSources(EnvironmentHandle, Direction, ServerName,
	 * BufferLength1, NameLength1, Description, BufferLength2,
	 * NameLength2);
	 */
	return SQL_ERROR;
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLDescribeCol(HSTMT StatementHandle,
			   SQLUSMALLINT ColumnNumber, SQLCHAR *ColumnName,
			   SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
			   SQLSMALLINT *DataType, SQLULEN *ColumnSize,
			   SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
  SQLRETURN rc = SQL_SUCCESS;
	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          return WD_DescribeCol(StatementHandle, ColumnNumber, ColumnName,
                               BufferLength, NameLength, DataType, ColumnSize,
                               DecimalDigits, Nullable);
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLDisconnect(HDBC ConnectionHandle)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering for %p\n", ConnectionHandle);
    return WD_Disconnect(ConnectionHandle);
        });
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLExecDirect(HSTMT StatementHandle,
			  SQLCHAR *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLExecDirect";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          UWORD flag = 0;

          MYLOG(0, "Entering\n");
          return WD_ExecDirect(StatementHandle, StatementText, TextLength, flag);
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLExecute(HSTMT StatementHandle)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLExecute";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          UWORD flag = 0;

          MYLOG(0, "Entering\n");
          return WD_Execute(StatementHandle, flag);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLFetch(HSTMT StatementHandle)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    return WD_Fetch(StatementHandle);
  });
}


WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLFreeStmt(HSTMT StatementHandle,
			SQLUSMALLINT Option)
{
  SQLRETURN rc = SQL_SUCCESS;
	MYLOG(0, "Entering\n");

    // Special case for SQL_DROP. Can't inspect diagnostics if the handle is already deleted.
    if (Option == SQL_DROP) {
        return WD_FreeStmt(StatementHandle, Option);
    }

    return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
        return WD_FreeStmt(StatementHandle, Option);
            });
}


#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetCursorName(HSTMT StatementHandle,
				 SQLCHAR *CursorName, SQLSMALLINT BufferLength,
				 SQLSMALLINT *NameLength)
{
  SQLRETURN rc = SQL_SUCCESS;
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return
              WD_GetCursorName(StatementHandle, CursorName, BufferLength, NameLength);
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetData(HSTMT StatementHandle,
		   SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType,
		   PTR TargetValue, SQLLEN BufferLength,
		   SQLLEN *StrLen_or_Ind)
{
  SQLRETURN rc = SQL_SUCCESS;

        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_GetData(StatementHandle, ColumnNumber, TargetType, TargetValue,
                           BufferLength, StrLen_or_Ind);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetFunctions(HDBC ConnectionHandle,
				SQLUSMALLINT FunctionId, SQLUSMALLINT *Supported)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;

    MYLOG(0, "Entering\n");
    if (FunctionId == SQL_API_ODBC3_ALL_FUNCTIONS)
      return WD_GetFunctions30(ConnectionHandle, FunctionId, Supported);
    else
      return WD_GetFunctions(ConnectionHandle, FunctionId, Supported);
        });
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetInfo(HDBC ConnectionHandle,
		   SQLUSMALLINT InfoType, PTR InfoValue,
		   SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_GetInfo(ConnectionHandle, InfoType, InfoValue, BufferLength,
                     StringLength, FALSE);
        });
}


WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetTypeInfo(HSTMT StatementHandle,
			   SQLSMALLINT DataType)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLGetTypeInfo";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          return WD_GetTypeInfo(StatementHandle, DataType);
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLNumResultCols(HSTMT StatementHandle,
				 SQLSMALLINT *ColumnCount)
{
  SQLRETURN rc = SQL_SUCCESS;
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_NumResultCols(StatementHandle, ColumnCount);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLParamData(HSTMT StatementHandle,
			 PTR *Value)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_ParamData(StatementHandle, Value);
        });
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLPrepare(HSTMT StatementHandle,
		   SQLCHAR *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLPrepare";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_Prepare(StatementHandle, StatementText, TextLength);
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLPutData(HSTMT StatementHandle,
		   PTR Data, SQLLEN StrLen_or_Ind)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_PutData(StatementHandle, Data, StrLen_or_Ind);
        });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLRowCount(HSTMT StatementHandle,
			SQLLEN *RowCount)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_RowCount(StatementHandle, RowCount);
        });
}


#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetCursorName(HSTMT StatementHandle,
				 SQLCHAR *CursorName, SQLSMALLINT NameLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_SetCursorName(StatementHandle, CursorName, NameLength);
        });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetParam(HSTMT StatementHandle,
			SQLUSMALLINT ParameterNumber, SQLSMALLINT ValueType,
			SQLSMALLINT ParameterType, SQLULEN LengthPrecision,
			SQLSMALLINT ParameterScale, PTR ParameterValue,
			SQLLEN *StrLen_or_Ind)
{
	MYLOG(0, "Entering\n");

	/*
	 * return WD_SetParam(StatementHandle, ParameterNumber, ValueType,
	 * ParameterType, LengthPrecision, ParameterScale, ParameterValue,
	 * StrLen_or_Ind);
	 */
	return SQL_ERROR;
}


#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSpecialColumns(HSTMT StatementHandle,
				  SQLUSMALLINT IdentifierType, SQLCHAR *CatalogName,
				  SQLSMALLINT NameLength1, SQLCHAR *SchemaName,
				  SQLSMALLINT NameLength2, SQLCHAR *TableName,
				  SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
				  SQLUSMALLINT Nullable)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLSpecialColumns";
	RETCODE	ret;
       return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported function", "HYC00");
                });
}
/*
SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName;
	/*else
		ret = WD_SpecialColumns(StatementHandle, IdentifierType, ctName,
			NameLength1, scName, NameLength2, tbName, NameLength3,
							Scope, Nullable);
	if (SQL_SUCCESS == ret && theResultIsEmpty(stmt))
	{
		BOOL	ifallupper = TRUE, reexec = FALSE;
		SQLCHAR *newCt =NULL, *newSc = NULL, *newTb = NULL;
		ConnectionClass *conn = SC_get_conn(stmt);

		if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier
			ifallupper = FALSE;
		if (newCt = make_lstring_ifneeded(conn, CatalogName, NameLength1, ifallupper), NULL != newCt)
		{
			ctName = newCt;
			reexec = TRUE;
		}
		if (newSc = make_lstring_ifneeded(conn, SchemaName, NameLength2, ifallupper), NULL != newSc)
		{
			scName = newSc;
			reexec = TRUE;
		}
		if (newTb = make_lstring_ifneeded(conn, TableName, NameLength3, ifallupper), NULL != newTb)
		{
			tbName = newTb;
			reexec = TRUE;
		}
		if (reexec)
		{
	/*		ret = WD_SpecialColumns(StatementHandle, IdentifierType, ctName,
			NameLength1, scName, NameLength2, tbName, NameLength3,
							Scope, Nullable);
			if (newCt)
				free(newCt);
			if (newSc)
				free(newSc);
			if (newTb)
				free(newTb);
		}
	}
	ret = DiscardStatementSvp(stmt, ret, FALSE);
	LEAVE_STMT_CS(stmt);
	return ret;*/
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLStatistics(HSTMT StatementHandle,
			  SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
			  SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
			  SQLCHAR *TableName, SQLSMALLINT NameLength3,
			  SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLStatistics";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
	RETCODE	ret;
/*	StatementClass *stmt = (StatementClass *) StatementHandle;
	SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName;

	MYLOG(0, "Entering\n");
	if (SC_connection_lost_check(stmt, __FUNCTION__))
		return SQL_ERROR;

	ENTER_STMT_CS(stmt);
	SC_clear_error(stmt);
	StartRollbackState(stmt);
	if (SC_opencheck(stmt, func))
		ret = SQL_ERROR;
	else
		ret = WD_Statistics(StatementHandle, ctName, NameLength1,
				 scName, NameLength2, tbName, NameLength3,
				 Unique, Reserved);
	if (SQL_SUCCESS == ret && theResultIsEmpty(stmt))
	{
		BOOL	ifallupper = TRUE, reexec = FALSE;
		SQLCHAR *newCt =NULL, *newSc = NULL, *newTb = NULL;
		ConnectionClass *conn = SC_get_conn(stmt);

		if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier
			ifallupper = FALSE;
		if (newCt = make_lstring_ifneeded(conn, CatalogName, NameLength1, ifallupper), NULL != newCt)
		{
			ctName = newCt;
			reexec = TRUE;
		}
		if (newSc = make_lstring_ifneeded(conn, SchemaName, NameLength2, ifallupper), NULL != newSc)
		{
			scName = newSc;
			reexec = TRUE;
		}
		if (newTb = make_lstring_ifneeded(conn, TableName, NameLength3, ifallupper), NULL != newTb)
		{
			tbName = newTb;
			reexec = TRUE;
		}
		if (reexec)
		{
			ret = WD_Statistics(StatementHandle, ctName, NameLength1,
				 scName, NameLength2, tbName, NameLength3,
				 Unique, Reserved);
			if (newCt)
				free(newCt);
			if (newSc)
				free(newSc);
			if (newTb)
				free(newTb);
		}
	}
	ret = DiscardStatementSvp(stmt, ret, FALSE);
	LEAVE_STMT_CS(stmt);
	return ret;
 */});
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLTables(HSTMT StatementHandle,
		  SQLCHAR *CatalogName, SQLSMALLINT NameLength1,
		  SQLCHAR *SchemaName, SQLSMALLINT NameLength2,
		  SQLCHAR *TableName, SQLSMALLINT NameLength3,
		  SQLCHAR *TableType, SQLSMALLINT NameLength4)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLTables";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          SQLCHAR *ctName = CatalogName, *scName = SchemaName, *tbName = TableName;
          UWORD flag = 0;

          MYLOG(0, "Entering\n");
          return WD_Tables(StatementHandle, ctName, NameLength1, scName, NameLength2,
                          tbName, NameLength3, TableType, NameLength4);
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLColumnPrivileges(HSTMT hstmt,
					SQLCHAR *szCatalogName,
					SQLSMALLINT cbCatalogName,
					SQLCHAR *szSchemaName,
					SQLSMALLINT cbSchemaName,
					SQLCHAR *szTableName,
					SQLSMALLINT cbTableName,
					SQLCHAR *szColumnName,
					SQLSMALLINT cbColumnName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLColumnPrivileges";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw driver::odbcabstraction::DriverException("Unsupported function.", "HYC00");

          /*	RETCODE	ret;
                  StatementClass *stmt = (StatementClass *) hstmt;
                  SQLCHAR	*ctName = szCatalogName, *scName = szSchemaName,
                          *tbName = szTableName, *clName = szColumnName;
                  UWORD	flag = 0;

                  MYLOG(0, "Entering\n");
                  if (SC_connection_lost_check(stmt, __FUNCTION__))
                          return SQL_ERROR;

                  ENTER_STMT_CS(stmt);
                  SC_clear_error(stmt);
                  StartRollbackState(stmt);
                  if (stmt->options.metadata_id)
                          flag |= PODBC_NOT_SEARCH_PATTERN;
                  if (SC_opencheck(stmt, func))
                          ret = SQL_ERROR;
                  else
                          ret = WD_ColumnPrivileges(hstmt, ctName, cbCatalogName,
                                          scName, cbSchemaName, tbName, cbTableName,
                                                          clName, cbColumnName, flag);
                  if (SQL_SUCCESS == ret && theResultIsEmpty(stmt))
                  {
                          BOOL	ifallupper = TRUE, reexec = FALSE;
                          SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL, *newCl
             = NULL; ConnectionClass *conn = SC_get_conn(stmt);

                          if (SC_is_lower_case(stmt, conn)) /* case-insensitive
             identifier ifallupper = FALSE; if (newCt = make_lstring_ifneeded(conn,
             szCatalogName, cbCatalogName, ifallupper), NULL != newCt)
                          {
                                  ctName = newCt;
                                  reexec = TRUE;
                          }
                          if (newSc = make_lstring_ifneeded(conn, szSchemaName,
             cbSchemaName, ifallupper), NULL != newSc)
                          {
                                  scName = newSc;
                                  reexec = TRUE;
                          }
                          if (newTb = make_lstring_ifneeded(conn, szTableName,
             cbTableName, ifallupper), NULL != newTb)
                          {
                                  tbName = newTb;
                                  reexec = TRUE;
                          }
                          if (newCl = make_lstring_ifneeded(conn, szColumnName,
             cbColumnName, ifallupper), NULL != newCl)
                          {
                                  clName = newCl;
                                  reexec = TRUE;
                          }
                          if (reexec)
                          {
                                  ret = WD_ColumnPrivileges(hstmt, ctName,
             cbCatalogName, scName, cbSchemaName, tbName, cbTableName, clName,
             cbColumnName, flag); if (newCt) free(newCt); if (newSc) free(newSc); if
             (newTb) free(newTb); if (newCl) free(newCl);
                          }
                  }
                  ret = DiscardStatementSvp(stmt, ret, FALSE);
                  LEAVE_STMT_CS(stmt);
                  return ret;*/
          return SQL_ERROR;
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLDescribeParam(HSTMT hstmt,
				 SQLUSMALLINT ipar,
				 SQLSMALLINT *pfSqlType,
				 SQLULEN *pcbParamDef,
				 SQLSMALLINT *pibScale,
				 SQLSMALLINT *pfNullable)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
    return WD_DescribeParam(hstmt, ipar, pfSqlType, pcbParamDef, pibScale,
                           pfNullable);
        });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLExtendedFetch(HSTMT hstmt,
				 SQLUSMALLINT fFetchType,
				 SQLLEN irow,
				 SQLULEN *pcrow,
				 SQLUSMALLINT *rgfRowStatus)
{
  SQLRETURN rc = SQL_SUCCESS;
  MYLOG(0, "Entering\n");
  return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
    if (fFetchType != SQL_FETCH_NEXT) {
      throw DriverException("Fetch type unsupported", "HY016");
    }

    struct IRDFieldTracker {
      IRDFieldTracker(ODBCDescriptor* ird, SQLPOINTER newRowsFetched, SQLPOINTER newRowStatus)
          : m_ird(ird), m_newRowsFetched(newRowsFetched),
            m_oldRowsFetched(nullptr), m_newRowStatus(newRowStatus),
            m_oldRowStatus(nullptr) {
        ird->GetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, &m_oldRowsFetched, 0,
                            nullptr);
        ird->GetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, &m_oldRowStatus, 0,
                            nullptr);

        if (m_newRowsFetched != m_oldRowsFetched) {
          m_ird->SetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, m_newRowsFetched,
                                0);
        }

        if (m_newRowStatus != m_oldRowStatus) {
          m_ird->SetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, m_newRowStatus, 0);
        }
      }

      ~IRDFieldTracker() {
        if (m_newRowsFetched != m_oldRowsFetched) {
          m_ird->SetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, m_oldRowsFetched,
                                0);
        }

        if (m_newRowStatus != m_oldRowStatus) {
          m_ird->SetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, m_oldRowStatus, 0);
        }
      }

      ODBCDescriptor* m_ird;
      SQLPOINTER m_newRowsFetched;
      SQLPOINTER m_oldRowsFetched;
      SQLPOINTER m_newRowStatus;
      SQLPOINTER m_oldRowStatus;
    };

    ODBCStatement* stmt = ODBCStatement::of(hstmt);
    ODBCDescriptor* ird = stmt->GetIRD();
    IRDFieldTracker irdTracker(ird, pcrow, rgfRowStatus);
    RETCODE result = WD_Fetch(hstmt);
    return result;
  });
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLForeignKeys(HSTMT hstmt,
			   SQLCHAR *szPkCatalogName,
			   SQLSMALLINT cbPkCatalogName,
			   SQLCHAR *szPkSchemaName,
			   SQLSMALLINT cbPkSchemaName,
			   SQLCHAR *szPkTableName,
			   SQLSMALLINT cbPkTableName,
			   SQLCHAR *szFkCatalogName,
			   SQLSMALLINT cbFkCatalogName,
			   SQLCHAR *szFkSchemaName,
			   SQLSMALLINT cbFkSchemaName,
			   SQLCHAR *szFkTableName,
			   SQLSMALLINT cbFkTableName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLForeignKeys";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported function", "HYC00");
          RETCODE ret;
          StatementClass *stmt = (StatementClass *)hstmt;
          SQLCHAR *pkctName = szPkCatalogName, *pkscName = szPkSchemaName,
                  *pktbName = szPkTableName, *fkctName = szFkCatalogName,
                  *fkscName = szFkSchemaName, *fktbName = szFkTableName;

          MYLOG(0, "Entering\n");
          if (SC_connection_lost_check(stmt, __FUNCTION__))
            return SQL_ERROR;

          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          if (SC_opencheck(stmt, func))
            ret = SQL_ERROR;
          /*	else
                          ret = WD_ForeignKeys(hstmt, pkctName, cbPkCatalogName,
                                  pkscName, cbPkSchemaName, pktbName, cbPkTableName,
                                  fkctName, cbFkCatalogName, fkscName, cbFkSchemaName,
                                  fktbName, cbFkTableName);*/
          if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
            BOOL ifallupper = TRUE, reexec = FALSE;
            SQLCHAR *newPkct = NULL, *newPksc = NULL, *newPktb = NULL,
                    *newFkct = NULL, *newFksc = NULL, *newFktb = NULL;
            ConnectionClass *conn = SC_get_conn(stmt);

            if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier */
              ifallupper = FALSE;
            if (newPkct = make_lstring_ifneeded(conn, szPkCatalogName,
                                                cbPkCatalogName, ifallupper),
                NULL != newPkct) {
              pkctName = newPkct;
              reexec = TRUE;
            }
            if (newPksc = make_lstring_ifneeded(conn, szPkSchemaName, cbPkSchemaName,
                                                ifallupper),
                NULL != newPksc) {
              pkscName = newPksc;
              reexec = TRUE;
            }
            if (newPktb = make_lstring_ifneeded(conn, szPkTableName, cbPkTableName,
                                                ifallupper),
                NULL != newPktb) {
              pktbName = newPktb;
              reexec = TRUE;
            }
            if (newFkct = make_lstring_ifneeded(conn, szFkCatalogName,
                                                cbFkCatalogName, ifallupper),
                NULL != newFkct) {
              fkctName = newFkct;
              reexec = TRUE;
            }
            if (newFksc = make_lstring_ifneeded(conn, szFkSchemaName, cbFkSchemaName,
                                                ifallupper),
                NULL != newFksc) {
              fkscName = newFksc;
              reexec = TRUE;
            }
            if (newFktb = make_lstring_ifneeded(conn, szFkTableName, cbFkTableName,
                                                ifallupper),
                NULL != newFktb) {
              fktbName = newFktb;
              reexec = TRUE;
            }
            if (reexec) {
              /*	ret = WD_ForeignKeys(hstmt, pkctName, cbPkCatalogName,
                      pkscName, cbPkSchemaName, pktbName, cbPkTableName,
                      fkctName, cbFkCatalogName, fkscName, cbFkSchemaName,
                      fktbName, cbFkTableName);*/
              if (newPkct)
                free(newPkct);
              if (newPksc)
                free(newPksc);
              if (newPktb)
                free(newPktb);
              if (newFkct)
                free(newFkct);
              if (newFksc)
                free(newFksc);
              if (newFktb)
                free(newFktb);
            }
          }
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return SQL_ERROR;
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLMoreResults(HSTMT hstmt)
{
	RETCODE	ret = SQL_NO_DATA;

	MYLOG(0, "Entering\n");
	return ret;
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLNativeSql(HDBC hdbc,
			 SQLCHAR *szSqlStrIn,
			 SQLINTEGER cbSqlStrIn,
			 SQLCHAR *szSqlStr,
			 SQLINTEGER cbSqlStrMax,
			 SQLINTEGER *pcbSqlStr)
{
  SQLRETURN rc = SQL_SUCCESS;
        return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_NativeSql(hdbc, szSqlStrIn, cbSqlStrIn, szSqlStr, cbSqlStrMax,
                             pcbSqlStr);
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLNumParams(HSTMT hstmt,
			 SQLSMALLINT *pcpar)
{
  SQLRETURN rc = SQL_SUCCESS;
	RETCODE	ret;
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          MYLOG(0, "Entering\n");
          return WD_NumParams(hstmt, pcpar);
              });
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLPrimaryKeys(HSTMT hstmt,
			   SQLCHAR *szCatalogName,
			   SQLSMALLINT cbCatalogName,
			   SQLCHAR *szSchemaName,
			   SQLSMALLINT cbSchemaName,
			   SQLCHAR *szTableName,
			   SQLSMALLINT cbTableName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLPrimaryKeys";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          RETCODE ret;
          throw DriverException("Unsupported function.", "HYC00");
          StatementClass *stmt = (StatementClass *)hstmt;
          SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
                  *tbName = szTableName;

          MYLOG(0, "Entering\n");
          if (SC_connection_lost_check(stmt, __FUNCTION__))
            return SQL_ERROR;

          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          if (SC_opencheck(stmt, func))
            ret = SQL_ERROR;
          /*else
                  ret = WD_PrimaryKeys(hstmt, ctName, cbCatalogName,
                          scName, cbSchemaName, tbName, cbTableName, 0);*/
          ;
          if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
            BOOL ifallupper = TRUE, reexec = FALSE;
            SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
            ConnectionClass *conn = SC_get_conn(stmt);

            if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier */
              ifallupper = FALSE;
            if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                              ifallupper),
                NULL != newCt) {
              ctName = newCt;
              reexec = TRUE;
            }
            if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                              ifallupper),
                NULL != newSc) {
              scName = newSc;
              reexec = TRUE;
            }
            if (newTb =
                    make_lstring_ifneeded(conn, szTableName, cbTableName, ifallupper),
                NULL != newTb) {
              tbName = newTb;
              reexec = TRUE;
            }
            if (reexec) {
              /*	ret = WD_PrimaryKeys(hstmt, ctName, cbCatalogName,
                      scName, cbSchemaName, tbName, cbTableName, 0);*/
              if (newCt)
                free(newCt);
              if (newSc)
                free(newSc);
              if (newTb)
                free(newTb);
            }
          }
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return SQL_ERROR;
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLProcedureColumns(HSTMT hstmt,
					SQLCHAR *szCatalogName,
					SQLSMALLINT cbCatalogName,
					SQLCHAR *szSchemaName,
					SQLSMALLINT cbSchemaName,
					SQLCHAR *szProcName,
					SQLSMALLINT cbProcName,
					SQLCHAR *szColumnName,
					SQLSMALLINT cbColumnName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLProcedureColumns";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported function.", "HYC00");
          RETCODE ret;
          StatementClass *stmt = (StatementClass *)hstmt;
          SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
                  *prName = szProcName, *clName = szColumnName;
          UWORD flag = 0;

          MYLOG(0, "Entering\n");
          if (SC_connection_lost_check(stmt, __FUNCTION__))
            return SQL_ERROR;

          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          if (stmt->options.metadata_id)
            flag |= PODBC_NOT_SEARCH_PATTERN;
          if (SC_opencheck(stmt, func))
            ret = SQL_ERROR;
          /*	else
                          ret = WD_ProcedureColumns(hstmt, ctName, cbCatalogName,
                                          scName, cbSchemaName, prName, cbProcName,
                                                  clName, cbColumnName, flag);*/
          if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
            BOOL ifallupper = TRUE, reexec = FALSE;
            SQLCHAR *newCt = NULL, *newSc = NULL, *newPr = NULL, *newCl = NULL;
            ConnectionClass *conn = SC_get_conn(stmt);

            if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier */
              ifallupper = FALSE;
            if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                              ifallupper),
                NULL != newCt) {
              ctName = newCt;
              reexec = TRUE;
            }
            if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                              ifallupper),
                NULL != newSc) {
              scName = newSc;
              reexec = TRUE;
            }
            if (newPr =
                    make_lstring_ifneeded(conn, szProcName, cbProcName, ifallupper),
                NULL != newPr) {
              prName = newPr;
              reexec = TRUE;
            }
            if (newCl = make_lstring_ifneeded(conn, szColumnName, cbColumnName,
                                              ifallupper),
                NULL != newCl) {
              clName = newCl;
              reexec = TRUE;
            }
            if (reexec) {
              /*	ret = WD_ProcedureColumns(hstmt, ctName, cbCatalogName,
                              scName, cbSchemaName, prName, cbProcName,
                                      clName, cbColumnName, flag);*/
              if (newCt)
                free(newCt);
              if (newSc)
                free(newSc);
              if (newPr)
                free(newPr);
              if (newCl)
                free(newCl);
            }
          }
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return SQL_ERROR;
              });
}

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLProcedures(HSTMT hstmt,
			  SQLCHAR *szCatalogName,
			  SQLSMALLINT cbCatalogName,
			  SQLCHAR *szSchemaName,
			  SQLSMALLINT cbSchemaName,
			  SQLCHAR *szProcName,
			  SQLSMALLINT cbProcName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLProcedures";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported function.", "HYC00");
          RETCODE ret;
          StatementClass *stmt = (StatementClass *)hstmt;
          SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
                  *prName = szProcName;
          UWORD flag = 0;

          MYLOG(0, "Entering\n");
          if (SC_connection_lost_check(stmt, __FUNCTION__))
            return SQL_ERROR;

          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          if (stmt->options.metadata_id)
            flag |= PODBC_NOT_SEARCH_PATTERN;
          if (SC_opencheck(stmt, func))
            ret = SQL_ERROR;
          /*	else
                          ret = WD_Procedures(hstmt, ctName, cbCatalogName,
                                                   scName, cbSchemaName, prName,
                                                   cbProcName, flag);*/
          if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
            BOOL ifallupper = TRUE, reexec = FALSE;
            SQLCHAR *newCt = NULL, *newSc = NULL, *newPr = NULL;
            ConnectionClass *conn = SC_get_conn(stmt);

            if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier */
              ifallupper = FALSE;
            if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                              ifallupper),
                NULL != newCt) {
              ctName = newCt;
              reexec = TRUE;
            }
            if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                              ifallupper),
                NULL != newSc) {
              scName = newSc;
              reexec = TRUE;
            }
            if (newPr =
                    make_lstring_ifneeded(conn, szProcName, cbProcName, ifallupper),
                NULL != newPr) {
              prName = newPr;
              reexec = TRUE;
            }
            if (reexec) {
              /*		ret = WD_Procedures(hstmt, ctName, cbCatalogName,
                                               scName, cbSchemaName, prName,
                 cbProcName, flag);*/
              if (newCt)
                free(newCt);
              if (newSc)
                free(newSc);
              if (newPr)
                free(newPr);
            }
          }
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return SQL_ERROR;
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetPos(HSTMT hstmt,
		  SQLSETPOSIROW irow,
		  SQLUSMALLINT fOption,
		  SQLUSMALLINT fLock)
{
  SQLRETURN rc = SQL_SUCCESS;
	RETCODE	ret;
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported exception");
          StatementClass *stmt = (StatementClass *)hstmt;

          MYLOG(0, "Entering\n");
          if (SC_connection_lost_check(stmt, __FUNCTION__))
            return SQL_ERROR;

          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          ret = WD_SetPos(hstmt, irow, fOption, fLock);
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return SQL_ERROR;
              });
}

#ifndef	UNICODE_SUPPORTXX
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLTablePrivileges(HSTMT hstmt,
				   SQLCHAR *szCatalogName,
				   SQLSMALLINT cbCatalogName,
				   SQLCHAR *szSchemaName,
				   SQLSMALLINT cbSchemaName,
				   SQLCHAR *szTableName,
				   SQLSMALLINT cbTableName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLTablePrivileges";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported function.", "HYC00");
          RETCODE ret;
          StatementClass *stmt = (StatementClass *)hstmt;
          SQLCHAR *ctName = szCatalogName, *scName = szSchemaName,
                  *tbName = szTableName;
          UWORD flag = 0;

          MYLOG(0, "Entering\n");
          if (SC_connection_lost_check(stmt, __FUNCTION__))
            return SQL_ERROR;

          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          if (stmt->options.metadata_id)
            flag |= PODBC_NOT_SEARCH_PATTERN;
          if (SC_opencheck(stmt, func))
            ret = SQL_ERROR;
          /*	else
                          ret = WD_TablePrivileges(hstmt, ctName, cbCatalogName,
                                  scName, cbSchemaName, tbName, cbTableName, flag);*/
          if (SQL_SUCCESS == ret && theResultIsEmpty(stmt)) {
            BOOL ifallupper = TRUE, reexec = FALSE;
            SQLCHAR *newCt = NULL, *newSc = NULL, *newTb = NULL;
            ConnectionClass *conn = SC_get_conn(stmt);

            if (SC_is_lower_case(stmt, conn)) /* case-insensitive identifier */
              ifallupper = FALSE;
            if (newCt = make_lstring_ifneeded(conn, szCatalogName, cbCatalogName,
                                              ifallupper),
                NULL != newCt) {
              ctName = newCt;
              reexec = TRUE;
            }
            if (newSc = make_lstring_ifneeded(conn, szSchemaName, cbSchemaName,
                                              ifallupper),
                NULL != newSc) {
              scName = newSc;
              reexec = TRUE;
            }
            if (newTb =
                    make_lstring_ifneeded(conn, szTableName, cbTableName, ifallupper),
                NULL != newTb) {
              tbName = newTb;
              reexec = TRUE;
            }
            if (reexec) {
              /*	ret = WD_TablePrivileges(hstmt, ctName, cbCatalogName,
                      scName, cbSchemaName, tbName, cbTableName, 0);*/
              if (newCt)
                free(newCt);
              if (newSc)
                free(newSc);
              if (newTb)
                free(newTb);
            }
          }
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return SQL_ERROR;
              });
}
#endif /* UNICODE_SUPPORTXX */

WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLBindParameter(HSTMT hstmt,
				 SQLUSMALLINT ipar,
				 SQLSMALLINT fParamType,
				 SQLSMALLINT fCType,
				 SQLSMALLINT fSqlType,
				 SQLULEN cbColDef,
				 SQLSMALLINT ibScale,
				 PTR rgbValue,
				 SQLLEN cbValueMax,
				 SQLLEN *pcbValue)
{
  SQLRETURN rc = SQL_SUCCESS;
	RETCODE	ret;
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          throw DriverException("Unsupported function.", "HYC00");
          StatementClass *stmt = (StatementClass *)hstmt;

          MYLOG(0, "Entering\n");
          ENTER_STMT_CS(stmt);
          SC_clear_error(stmt);
          StartRollbackState(stmt);
          ret = WD_BindParameter(hstmt, ipar, fParamType, fCType, fSqlType, cbColDef,
                                 ibScale, rgbValue, cbValueMax, pcbValue);
          ret = DiscardStatementSvp(stmt, ret, FALSE);
          LEAVE_STMT_CS(stmt);
          return ret;
              });
}
