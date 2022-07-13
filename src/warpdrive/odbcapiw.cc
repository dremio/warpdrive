/*-------
 * Module:			odbcapiw.c
 *
 * Description:		This module contains UNICODE routines
 *
 * Classes:			n/a
 *
 * API functions:	SQLColumnPrivilegesW, SQLColumnsW,
			SQLConnectW, SQLDataSourcesW, SQLDescribeColW,
			SQLDriverConnectW, SQLExecDirectW,
			SQLForeignKeysW,
			SQLGetCursorNameW, SQLGetInfoW, SQLNativeSqlW,
			SQLPrepareW, SQLPrimaryKeysW, SQLProcedureColumnsW,
			SQLProceduresW, SQLSetCursorNameW,
			SQLSpecialColumnsW, SQLStatisticsW, SQLTablesW,
			SQLTablePrivilegesW, SQLGetTypeInfoW
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *-------
 */

#include "wdodbc.h"
#include "unicode_support.h"
#include <stdio.h>
#include <string.h>

#include "wdapifunc.h"
#include "connection.h"
#include "statement.h"

#include <odbcabstraction/odbc_impl/ODBCConnection.h>
#include <odbcabstraction/odbc_impl/ODBCStatement.h>
#include <odbcabstraction/odbc_impl/ODBCDescriptor.h>
#include "c_ptr.h"

using namespace ODBC;

extern "C" {

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLColumnsW(HSTMT StatementHandle,
			SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
			SQLWCHAR *SchemaName, SQLSMALLINT NameLength2,
			SQLWCHAR *TableName, SQLSMALLINT NameLength3,
			SQLWCHAR *ColumnName, SQLSMALLINT NameLength4)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() {
    c_ptr ctName, scName, tbName, clName;
    SQLLEN nmlen1, nmlen2, nmlen3, nmlen4;
    ctName.reset(ucs2_to_utf8(CatalogName, NameLength1, &nmlen1, FALSE));
    scName.reset(ucs2_to_utf8(SchemaName, NameLength2, &nmlen2, FALSE));
    tbName.reset(ucs2_to_utf8(TableName, NameLength3, &nmlen3, FALSE));
    clName.reset(ucs2_to_utf8(ColumnName, NameLength4, &nmlen4, FALSE));
    return
        WD_Columns(StatementHandle, (SQLCHAR *)ctName.get(), (SQLSMALLINT)nmlen1,
                   (SQLCHAR *)scName.get(), (SQLSMALLINT)nmlen2, (SQLCHAR *)tbName.get(),
                   (SQLSMALLINT)nmlen3, (SQLCHAR *)clName.get(), (SQLSMALLINT)nmlen4);
        });
}


WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLConnectW(HDBC ConnectionHandle,
			SQLWCHAR *ServerName, SQLSMALLINT NameLength1,
			SQLWCHAR *UserName, SQLSMALLINT NameLength2,
			SQLWCHAR *Authentication, SQLSMALLINT NameLength3)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() {
    c_ptr svName, usName, auth;
    SQLLEN nmlen1, nmlen2, nmlen3;
    svName.reset(ucs2_to_utf8(ServerName, NameLength1, &nmlen1, FALSE));
    usName.reset(ucs2_to_utf8(UserName, NameLength2, &nmlen2, FALSE));
    auth.reset(ucs2_to_utf8(Authentication, NameLength3, &nmlen3, FALSE));
    return WD_Connect(ConnectionHandle, (SQLCHAR *)svName.get(), (SQLSMALLINT)nmlen1,
                     (SQLCHAR *)usName.get(), (SQLSMALLINT)nmlen2, (SQLCHAR *)auth.get(),
                     (SQLSMALLINT)nmlen3);
        });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLDriverConnectW(HDBC hdbc,
				  HWND hwnd,
				  SQLWCHAR *szConnStrIn,
				  SQLSMALLINT cbConnStrIn,
				  SQLWCHAR *szConnStrOut,
				  SQLSMALLINT cbConnStrOutMax,
				  SQLSMALLINT *pcbConnStrOut,
				  SQLUSMALLINT fDriverCompletion)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLDriverConnectW";
        return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() {
          c_ptr szIn, szOut;
          SQLSMALLINT maxlen, obuflen = 0;
          SQLLEN inlen;
          SQLSMALLINT olen, *pCSO;
          ODBCConnection *conn = ODBCConnection::of(hdbc);
          RETCODE ret;

          szIn.reset(ucs2_to_utf8(szConnStrIn, cbConnStrIn, &inlen, FALSE));
          maxlen = cbConnStrOutMax;
          pCSO = NULL;
          olen = 0;
          if (maxlen > 0) {
            obuflen = maxlen + 1;
            szOut.reset(static_cast<char *>(malloc(obuflen)));
            if (!szOut) {
              throw std::bad_alloc();
            }
            pCSO = &olen;
          } else if (pcbConnStrOut)
            pCSO = &olen;
          ret = WD_DriverConnect(hdbc, hwnd, (SQLCHAR *)szIn.get(), (SQLSMALLINT)inlen,
                                 (SQLCHAR *)szOut.get(), maxlen, pCSO, fDriverCompletion);
          if (ret != SQL_ERROR && NULL != pCSO) {
            SQLLEN outlen = olen;

            if (olen < obuflen)
              outlen = utf8_to_ucs2(szOut.get(), olen, szConnStrOut, cbConnStrOutMax);
            else
              utf8_to_ucs2(szOut.get(), maxlen, szConnStrOut, cbConnStrOutMax);
            if (outlen >= cbConnStrOutMax && NULL != szConnStrOut &&
                NULL != pcbConnStrOut) {
              MYLOG(DETAIL_LOG_LEVEL, "cbConnstrOutMax=%d pcb=%p\n", cbConnStrOutMax,
                    pcbConnStrOut);
              if (SQL_SUCCESS == ret) {
                conn->GetDiagnostics().AddTruncationWarning();
                ret = SQL_SUCCESS_WITH_INFO;
              }
            }
            if (pcbConnStrOut)
              *pcbConnStrOut = (SQLSMALLINT)outlen;
          }
          return ret;
        });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLBrowseConnectW(HDBC			hdbc,
				  SQLWCHAR	   *szConnStrIn,
				  SQLSMALLINT	cbConnStrIn,
				  SQLWCHAR	   *szConnStrOut,
				  SQLSMALLINT	cbConnStrOutMax,
				  SQLSMALLINT  *pcbConnStrOut)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLBrowseConnectW";
        return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() {
          c_ptr szIn, szOut;
          SQLLEN inlen;
          SQLUSMALLINT obuflen;
          SQLSMALLINT olen;
          RETCODE ret;

          szIn.reset(ucs2_to_utf8(szConnStrIn, cbConnStrIn, &inlen, FALSE));
          obuflen = cbConnStrOutMax + 1;
          szOut.reset(static_cast<char *>(malloc(obuflen)));
          if (szOut)
            ret = WD_BrowseConnect(hdbc, (SQLCHAR *)szIn.get(), (SQLSMALLINT)inlen,
                                   (SQLCHAR *)szOut.get(), cbConnStrOutMax, &olen);
          else {
            ODBCConnection::of(hdbc)->GetDiagnostics().AddTruncationWarning();
            ret = SQL_SUCCESS_WITH_INFO;
          }
          if (ret != SQL_ERROR) {
            SQLLEN outlen = utf8_to_ucs2(szOut.get(), olen, szConnStrOut, cbConnStrOutMax);
            if (pcbConnStrOut)
              *pcbConnStrOut = (SQLSMALLINT)outlen;
          }
          return ret;
              });
}

RETCODE  SQL_API
SQLDataSourcesW(HENV EnvironmentHandle,
				SQLUSMALLINT Direction, SQLWCHAR *ServerName,
				SQLSMALLINT BufferLength1, SQLSMALLINT *NameLength1,
				SQLWCHAR *Description, SQLSMALLINT BufferLength2,
				SQLSMALLINT *NameLength2)
{
	/*
	return WD_DataSources(EnvironmentHandle, Direction, ServerName,
		BufferLength1, NameLength1, Description, BufferLength2,
		NameLength2);
	*/
	return SQL_ERROR;
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLDescribeColW(HSTMT StatementHandle,
				SQLUSMALLINT ColumnNumber, SQLWCHAR *ColumnName,
				SQLSMALLINT BufferLength, SQLSMALLINT *NameLength,
				SQLSMALLINT *DataType, SQLULEN *ColumnSize,
				SQLSMALLINT *DecimalDigits, SQLSMALLINT *Nullable)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLDescribeColW";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          RETCODE ret;
          SQLSMALLINT buflen, nmlen;
          c_ptr clName;

          buflen = 0;
          if (BufferLength > 0)
            buflen = BufferLength * 3;
          else if (NameLength)
            buflen = 32;
          if (buflen > 0)
            clName.reset(static_cast<char *>(malloc(buflen)));
          for (;; buflen = nmlen + 1,
                  clName.reallocate(buflen)) {
            if (!clName) {
              throw std::bad_alloc();
            }
            ret = WD_DescribeCol(StatementHandle, ColumnNumber, (SQLCHAR *)clName.get(),
                                 buflen, &nmlen, DataType, ColumnSize, DecimalDigits,
                                 Nullable);
            if (SQL_SUCCESS_WITH_INFO != ret || nmlen < buflen)
                break;
            else
                // Get rid of any internally-generated truncation warnings.
                ODBCStatement::of(StatementHandle)->GetDiagnostics().Clear();
          }
          if (SQL_SUCCEEDED(ret)) {
            SQLLEN nmcount = nmlen;

            if (nmlen < buflen)
              nmcount = utf8_to_ucs2(clName.get(), nmlen, ColumnName, BufferLength);
            if (SQL_SUCCESS == ret && BufferLength > 0 && nmcount > BufferLength) {
              ret = SQL_SUCCESS_WITH_INFO;
              ODBCStatement::of(StatementHandle)->GetDiagnostics().AddTruncationWarning();
            }
            if (NameLength)
              *NameLength = (SQLSMALLINT)nmcount;
          }
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLExecDirectW(HSTMT StatementHandle,
			   SQLWCHAR *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR	func = "SQLExecDirectW";
	c_ptr	stxt;
	SQLLEN	slen;
	UWORD	flag = 0;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          stxt.reset(wcs_to_utf8(StatementText, TextLength, &slen, FALSE));
          return WD_ExecDirect(StatementHandle, (SQLCHAR *)stxt.get(),
                              (SQLINTEGER)slen, flag);
  });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLGetCursorNameW(HSTMT StatementHandle,
				  SQLWCHAR *CursorName, SQLSMALLINT BufferLength,
				  SQLSMALLINT *NameLength)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLGetCursorNameW";
	RETCODE	ret;
	c_ptr	crName;
	SQLSMALLINT	clen, buflen;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          ODBCStatement* stmt = ODBCStatement::of(StatementHandle);
          if (BufferLength > 0)
            buflen = BufferLength * 3;
          else
            buflen = 32;
          crName.reset(static_cast<char *>(malloc(buflen)));
          for (;; buflen = clen + 1, crName.reallocate(buflen)) {
            if (!crName) {
              ret = SQL_ERROR;
              throw std::bad_alloc();
            }
            ret = WD_GetCursorName(StatementHandle, (SQLCHAR *)crName.get(), buflen,
                                   &clen);
            if (SQL_SUCCESS_WITH_INFO != ret || clen < buflen)
                break;
            else
                // Get rid of any internally-generated truncation warnings.
                ODBCStatement::of(StatementHandle)->GetDiagnostics().Clear();
          }
          if (SQL_SUCCEEDED(ret)) {
            SQLLEN nmcount = clen;

            if (clen < buflen)
              nmcount = utf8_to_ucs2(crName.get(), clen, CursorName, BufferLength);
            if (SQL_SUCCESS == ret && nmcount > BufferLength) {
              stmt->GetDiagnostics().AddTruncationWarning();
              ret = SQL_SUCCESS_WITH_INFO;
            }
            if (NameLength)
              *NameLength = (SQLSMALLINT)nmcount;
          }
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLGetInfoW(HDBC ConnectionHandle,
			SQLUSMALLINT InfoType, PTR InfoValue,
			SQLSMALLINT BufferLength, SQLSMALLINT *StringLength)
{
	MYLOG(0, "Entering\n");
        SQLRETURN rc = SQL_SUCCESS;
        return ODBCConnection::ExecuteWithDiagnostics(ConnectionHandle, rc, [&]() -> SQLRETURN {
          return WD_GetInfo(ConnectionHandle, InfoType, InfoValue, BufferLength,
                           StringLength, TRUE);
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLPrepareW(HSTMT StatementHandle,
			SQLWCHAR *StatementText, SQLINTEGER TextLength)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLPrepareW";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          c_ptr stxt;
          SQLLEN slen;

          MYLOG(0, "Entering\n");
          stxt.reset(ucs2_to_utf8(StatementText, TextLength, &slen, FALSE));
          return WD_Prepare(StatementHandle, (SQLCHAR *)stxt.get(), (SQLINTEGER)slen);
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLSetCursorNameW(HSTMT StatementHandle,
				  SQLWCHAR *CursorName, SQLSMALLINT NameLength)
{
	SQLRETURN	ret;
	c_ptr	crName;
	SQLLEN	nlen;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, ret, [&]() -> SQLRETURN {
          crName.reset(ucs2_to_utf8(CursorName, NameLength, &nlen, FALSE));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          ret = WD_SetCursorName(StatementHandle, (SQLCHAR *)crName.get(),
                                 (SQLSMALLINT)nlen);
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLSpecialColumnsW(HSTMT StatementHandle,
				   SQLUSMALLINT IdentifierType, SQLWCHAR *CatalogName,
				   SQLSMALLINT NameLength1, SQLWCHAR *SchemaName,
				   SQLSMALLINT NameLength2, SQLWCHAR *TableName,
				   SQLSMALLINT NameLength3, SQLUSMALLINT Scope,
				   SQLUSMALLINT Nullable)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLSpecialColumnsW";
	RETCODE	ret;
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          c_ptr ctName, scName, tbName;
          SQLLEN nmlen1, nmlen2, nmlen3;
          ConnectionClass *conn;
          BOOL lower_id = FALSE;

          MYLOG(0, "Entering\n");

          ctName.reset(ucs2_to_utf8(CatalogName, NameLength1, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(SchemaName, NameLength2, &nmlen2, lower_id));
          tbName.reset(ucs2_to_utf8(TableName, NameLength3, &nmlen3, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          /*	else
                          ret = WD_SpecialColumns(StatementHandle, IdentifierType,
                                                                             (SQLCHAR
             *) ctName, (SQLSMALLINT) nmlen1, (SQLCHAR *) scName, (SQLSMALLINT)
             nmlen2, (SQLCHAR *) tbName, (SQLSMALLINT) nmlen3, Scope, Nullable);*/
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLStatisticsW(HSTMT StatementHandle,
			   SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
			   SQLWCHAR *SchemaName, SQLSMALLINT NameLength2,
			   SQLWCHAR *TableName, SQLSMALLINT NameLength3,
			   SQLUSMALLINT Unique, SQLUSMALLINT Reserved)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLStatisticsW";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          RETCODE ret;
          c_ptr ctName, scName, tbName;
          SQLLEN nmlen1, nmlen2, nmlen3;
          BOOL lower_id = FALSE;

          MYLOG(0, "Entering\n");
          ctName.reset(ucs2_to_utf8(CatalogName, NameLength1, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(SchemaName, NameLength2, &nmlen2, lower_id));
          tbName.reset(ucs2_to_utf8(TableName, NameLength3, &nmlen3, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          ret = WD_Statistics(StatementHandle, (SQLCHAR *)ctName.get(),
                              (SQLSMALLINT)nmlen1, (SQLCHAR *)scName.get(),
                              (SQLSMALLINT)nmlen2, (SQLCHAR *)tbName.get(),
                              (SQLSMALLINT)nmlen3, Unique, Reserved);
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLTablesW(HSTMT StatementHandle,
           SQLWCHAR *CatalogName, SQLSMALLINT NameLength1,
           SQLWCHAR *SchemaName, SQLSMALLINT NameLength2,
           SQLWCHAR *TableName, SQLSMALLINT NameLength3,
           SQLWCHAR *TableType, SQLSMALLINT NameLength4)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLTablesW";
	c_ptr	ctName, scName, tbName, tbType;
	SQLLEN	nmlen1, nmlen2, nmlen3, nmlen4;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          ctName.reset(ucs2_to_utf8(CatalogName, NameLength1, &nmlen1, FALSE));
          scName.reset(ucs2_to_utf8(SchemaName, NameLength2, &nmlen2, FALSE));
          tbName.reset(ucs2_to_utf8(TableName, NameLength3, &nmlen3, FALSE));
          tbType.reset(ucs2_to_utf8(TableType, NameLength4, &nmlen4, FALSE));
          return WD_Tables(
              StatementHandle, (SQLCHAR *)ctName.get(), (SQLSMALLINT)nmlen1,
              (SQLCHAR *)scName.get(), (SQLSMALLINT)nmlen2, (SQLCHAR *)tbName.get(),
              (SQLSMALLINT)nmlen3, (SQLCHAR *)tbType.get(), (SQLSMALLINT)nmlen4);
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLColumnPrivilegesW(HSTMT			hstmt,
					 SQLWCHAR	   *szCatalogName,
					 SQLSMALLINT	cbCatalogName,
					 SQLWCHAR	   *szSchemaName,
					 SQLSMALLINT	cbSchemaName,
					 SQLWCHAR	   *szTableName,
					 SQLSMALLINT	cbTableName,
					 SQLWCHAR	   *szColumnName,
					 SQLSMALLINT	cbColumnName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLColumnPrivilegesW";
	c_ptr	ctName, scName, tbName, clName;
	SQLLEN	nmlen1, nmlen2, nmlen3, nmlen4;
	ConnectionClass *conn;
	BOOL	lower_id = FALSE;
	UWORD	flag = 0;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          ctName.reset(ucs2_to_utf8(szCatalogName, cbCatalogName, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(szSchemaName, cbSchemaName, &nmlen2, lower_id));
          tbName.reset(ucs2_to_utf8(szTableName, cbTableName, &nmlen3, lower_id));
          clName.reset(ucs2_to_utf8(szColumnName, cbColumnName, &nmlen4, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          return WD_ColumnPrivileges(hstmt, (SQLCHAR *)ctName.get(), (SQLSMALLINT)nmlen1,
                                    (SQLCHAR *)scName.get(), (SQLSMALLINT)nmlen2,
                                    (SQLCHAR *)tbName.get(), (SQLSMALLINT)nmlen3,
                                    (SQLCHAR *)clName.get(), (SQLSMALLINT)nmlen4, flag);
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLForeignKeysW(HSTMT			hstmt,
				SQLWCHAR	   *szPkCatalogName,
				SQLSMALLINT		cbPkCatalogName,
				SQLWCHAR	   *szPkSchemaName,
				SQLSMALLINT		cbPkSchemaName,
				SQLWCHAR	   *szPkTableName,
				SQLSMALLINT		cbPkTableName,
				SQLWCHAR	   *szFkCatalogName,
				SQLSMALLINT		cbFkCatalogName,
				SQLWCHAR	   *szFkSchemaName,
				SQLSMALLINT		cbFkSchemaName,
				SQLWCHAR	   *szFkTableName,
				SQLSMALLINT		cbFkTableName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLForeignKeysW";
	RETCODE	ret;
	c_ptr	ctName, scName, tbName, fkctName, fkscName, fktbName;
	SQLLEN	nmlen1, nmlen2, nmlen3, nmlen4, nmlen5, nmlen6;
	BOOL	lower_id = FALSE;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          ctName.reset(ucs2_to_utf8(szPkCatalogName, cbPkCatalogName, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(szPkSchemaName, cbPkSchemaName, &nmlen2, lower_id));
          tbName.reset(ucs2_to_utf8(szPkTableName, cbPkTableName, &nmlen3, lower_id));
          fkctName.reset(
              ucs2_to_utf8(szFkCatalogName, cbFkCatalogName, &nmlen4, lower_id));
          fkscName.reset(ucs2_to_utf8(szFkSchemaName, cbFkSchemaName, &nmlen5, lower_id));
          fktbName.reset(ucs2_to_utf8(szFkTableName, cbFkTableName, &nmlen6, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          /*		ret = WD_ForeignKeys(hstmt,
                                                                          (SQLCHAR *)
             ctName, (SQLSMALLINT) nmlen1, (SQLCHAR *) scName, (SQLSMALLINT) nmlen2,
                                                                          (SQLCHAR *)
             tbName, (SQLSMALLINT) nmlen3, (SQLCHAR *) fkctName, (SQLSMALLINT) nmlen4,
                                                                          (SQLCHAR *)
             fkscName, (SQLSMALLINT) nmlen5, (SQLCHAR *) fktbName, (SQLSMALLINT)
             nmlen6);*/
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLNativeSqlW(HDBC			hdbc,
			  SQLWCHAR	   *szSqlStrIn,
			  SQLINTEGER	cbSqlStrIn,
			  SQLWCHAR	   *szSqlStr,
			  SQLINTEGER	cbSqlStrMax,
			  SQLINTEGER   *pcbSqlStr)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLNativeSqlW";
	RETCODE		ret;
	c_ptr		szIn, szOut;
	SQLLEN		slen;
	SQLINTEGER	buflen, olen;

	MYLOG(0, "Entering\n");
        return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() -> SQLRETURN {
          szIn.reset(ucs2_to_utf8(szSqlStrIn, cbSqlStrIn, &slen, FALSE));
          buflen = 3 * cbSqlStrMax;
          if (buflen > 0)
            szOut.reset(static_cast<char *>(malloc(buflen)));
          for (;; buflen = olen + 1, szOut.reallocate(buflen)) {
            if (!szOut) {
              throw std::bad_alloc();
            }
            ret = WD_NativeSql(hdbc, (SQLCHAR *)szIn.get(), (SQLINTEGER)slen,
                               (SQLCHAR *)szOut.get(), buflen, &olen);
            if (SQL_SUCCESS_WITH_INFO != ret || olen < buflen)
                break;
            else
                // Get rid of any internally-generated truncation warnings.
                ODBCStatement::of(hdbc)->GetDiagnostics().Clear();
          }
          if (SQL_SUCCEEDED(ret)) {
            SQLLEN szcount = olen;

            if (olen < buflen)
              szcount = utf8_to_ucs2(szOut.get(), olen, szSqlStr, cbSqlStrMax);
            if (SQL_SUCCESS == ret && szcount > cbSqlStrMax) {
              ret = SQL_SUCCESS_WITH_INFO;
              ODBCConnection::of(hdbc)->GetDiagnostics().AddTruncationWarning();
            }
            if (pcbSqlStr)
              *pcbSqlStr = (SQLINTEGER)szcount;
          }
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLPrimaryKeysW(HSTMT			hstmt,
				SQLWCHAR	   *szCatalogName,
				SQLSMALLINT		cbCatalogName,
				SQLWCHAR	   *szSchemaName,
				SQLSMALLINT		cbSchemaName,
				SQLWCHAR	   *szTableName,
				SQLSMALLINT		cbTableName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLPrimaryKeysW";
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          RETCODE ret;
          c_ptr ctName, scName, tbName;
          SQLLEN nmlen1, nmlen2, nmlen3;
          BOOL lower_id = FALSE;

          MYLOG(0, "Entering\n");
          ctName.reset(ucs2_to_utf8(szCatalogName, cbCatalogName, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(szSchemaName, cbSchemaName, &nmlen2, lower_id));
          tbName.reset(ucs2_to_utf8(szTableName, cbTableName, &nmlen3, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          /*else
                  ret = WD_PrimaryKeys(hstmt,
                                                                  (SQLCHAR *) ctName,
             (SQLSMALLINT) nmlen1, (SQLCHAR *) scName, (SQLSMALLINT) nmlen2,
                                                                  (SQLCHAR *) tbName,
             (SQLSMALLINT) nmlen3, 0);*/
          ;
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLProcedureColumnsW(HSTMT			hstmt,
					 SQLWCHAR	   *szCatalogName,
					 SQLSMALLINT	cbCatalogName,
					 SQLWCHAR	   *szSchemaName,
					 SQLSMALLINT	cbSchemaName,
					 SQLWCHAR	   *szProcName,
					 SQLSMALLINT	cbProcName,
					 SQLWCHAR	   *szColumnName,
					 SQLSMALLINT	cbColumnName)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLProcedureColumnsW";
	RETCODE	ret;
	c_ptr	ctName, scName, prName, clName;
	SQLLEN	nmlen1, nmlen2, nmlen3, nmlen4;
	BOOL	lower_id = FALSE;
	UWORD	flag = 0;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
          ctName.reset(ucs2_to_utf8(szCatalogName, cbCatalogName, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(szSchemaName, cbSchemaName, &nmlen2, lower_id));
          prName.reset(ucs2_to_utf8(szProcName, cbProcName, &nmlen3, lower_id));
          clName.reset(ucs2_to_utf8(szColumnName, cbColumnName, &nmlen4, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          /*	else
                          ret = WD_ProcedureColumns(hstmt,
                                                                                   (SQLCHAR
             *) ctName, (SQLSMALLINT) nmlen1, (SQLCHAR *) scName, (SQLSMALLINT)
             nmlen2, (SQLCHAR *) prName, (SQLSMALLINT) nmlen3, (SQLCHAR *) clName,
             (SQLSMALLINT) nmlen4, flag);*/
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLProceduresW(HSTMT		hstmt,
			   SQLWCHAR	   *szCatalogName,
			   SQLSMALLINT	cbCatalogName,
			   SQLWCHAR	   *szSchemaName,
			   SQLSMALLINT	cbSchemaName,
			   SQLWCHAR	   *szProcName,
			   SQLSMALLINT	cbProcName)
{
	CSTR func = "SQLProceduresW";
	SQLRETURN	ret;
	c_ptr	ctName, scName, prName;
	SQLLEN	nmlen1, nmlen2, nmlen3;
	BOOL	lower_id = FALSE;
	UWORD	flag = 0;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, ret, [&]() -> SQLRETURN {
          ctName.reset(ucs2_to_utf8(szCatalogName, cbCatalogName, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(szSchemaName, cbSchemaName, &nmlen2, lower_id));
          prName.reset(ucs2_to_utf8(szProcName, cbProcName, &nmlen3, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          /*	else
                          ret = WD_Procedures(hstmt,
                                                                     (SQLCHAR *)
             ctName, (SQLSMALLINT) nmlen1, (SQLCHAR *) scName, (SQLSMALLINT) nmlen2,
                                                                     (SQLCHAR *)
             prName, (SQLSMALLINT) nmlen3, flag);*/
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLTablePrivilegesW(HSTMT			hstmt,
					SQLWCHAR	   *szCatalogName,
					SQLSMALLINT		cbCatalogName,
					SQLWCHAR	   *szSchemaName,
					SQLSMALLINT		cbSchemaName,
					SQLWCHAR	   *szTableName,
					SQLSMALLINT		cbTableName)
{
	CSTR func = "SQLTablePrivilegesW";
	SQLRETURN	ret;
	c_ptr	ctName, scName, tbName;
	SQLLEN	nmlen1, nmlen2, nmlen3;
	BOOL	lower_id = FALSE;
	UWORD	flag = 0;

	MYLOG(0, "Entering\n");
        return ODBCStatement::ExecuteWithDiagnostics(hstmt, ret, [&]() -> SQLRETURN {
          ctName.reset(ucs2_to_utf8(szCatalogName, cbCatalogName, &nmlen1, lower_id));
          scName.reset(ucs2_to_utf8(szSchemaName, cbSchemaName, &nmlen2, lower_id));
          tbName.reset(ucs2_to_utf8(szTableName, cbTableName, &nmlen3, lower_id));
          throw driver::odbcabstraction::DriverException("Unsupported function", "HYC00");
          /*	else
                          ret = WD_TablePrivileges(hstmt,
                                                                                  (SQLCHAR
             *) ctName, (SQLSMALLINT) nmlen1, (SQLCHAR *) scName, (SQLSMALLINT)
             nmlen2, (SQLCHAR *) tbName, (SQLSMALLINT) nmlen3, flag);*/
          return ret;
              });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetTypeInfoW(SQLHSTMT	StatementHandle,
				SQLSMALLINT	DataType)
{
  SQLRETURN rc = SQL_SUCCESS;
	CSTR func = "SQLGetTypeInfoW";
        return ODBCStatement::ExecuteWithDiagnostics(StatementHandle, rc, [&]() -> SQLRETURN {
          return WD_GetTypeInfo(StatementHandle, DataType);
              });
}

}
