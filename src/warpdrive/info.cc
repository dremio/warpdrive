/*--------
 * Module:			info.cc
 *
 * Description:		This module contains routines related to
 *					ODBC informational functions.
 *
 * Classes:			n/a
 *
 * API functions:	SQLGetInfo, SQLGetTypeInfo, SQLGetFunctions,
 *					SQLTables, SQLColumns, SQLStatistics, SQLSpecialColumns,
 *					SQLPrimaryKeys, SQLForeignKeys,
 *					SQLProcedureColumns, SQLProcedures,
 *					SQLTablePrivileges, SQLColumnPrivileges(NI)
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */

#include "wdodbc.h"
#include "unicode_support.h"

#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <ctype.h>
#endif

#include "tuple.h"
#include "wdtypes.h"
#include "dlg_specific.h"

#include "environ.h"
#include "connection.h"
#include "statement.h"
#include "qresult.h"
#include "bind.h"
#include "misc.h"
#include "wdtypes.h"
#include "wdapifunc.h"
#include "multibyte.h"
#include "catfunc.h"
#include <odbcabstraction/odbc_impl/ODBCConnection.h>
#include <odbcabstraction/odbc_impl/ODBCStatement.h>
#include <string>


using namespace ODBC;

/*	Trigger related stuff for SQLForeign Keys */
#define TRIGGER_SHIFT 3
#define TRIGGER_MASK   0x03
#define TRIGGER_DELETE 0x01
#define TRIGGER_UPDATE 0x02

static const SQLCHAR *pubstr = (SQLCHAR *) "public";
static const char	*likeop = "like";
static const char	*eqop = "=";

/* group of SQL_CVT_(chars) */
#define WD_CONVERT_CH	SQL_CVT_CHAR | SQL_CVT_VARCHAR | SQL_CVT_LONGVARCHAR
/* group of SQL_CVT_(numbers) */
#define WD_CONVERT_NUM	SQL_CVT_SMALLINT | SQL_CVT_INTEGER |  SQL_CVT_BIGINT | SQL_CVT_REAL | SQL_CVT_FLOAT | SQL_CVT_DOUBLE | SQL_CVT_NUMERIC | SQL_CVT_DECIMAL
/* group of SQL_CVT_(date time) */
#define WD_CONVERT_DT	SQL_CVT_DATE | SQL_CVT_TIME | SQL_CVT_TIMESTAMP
/* group of SQL_CVT_(wchars) */
#ifdef UNICODE_SUPPORT
#define WD_CONVERT_WCH	SQL_CVT_WCHAR | SQL_CVT_WVARCHAR | SQL_CVT_WLONGVARCHAR
#else
#define WD_CONVERT_WCH	0
#endif /* UNICODE_SUPPORT */

#define QR_set_field_info_EN(self, field_num, adtid, adtsize)  (CI_set_field_info(self->fields, field_num, catcn[field_num][is_ODBC2], adtid, adtsize, -1, 0, 0))

const SQLLEN mask_longvarbinary = SQL_CVT_LONGVARBINARY;

RETCODE		SQL_API
WD_GetInfo(HDBC hdbc,
			  SQLUSMALLINT fInfoType,
			  PTR rgbInfoValue,
			  SQLSMALLINT cbInfoValueMax,
			  SQLSMALLINT * pcbInfoValue,
			  UWORD UnicodeOption)
{
	CSTR func = "WD_GetInfo";
	MYLOG(0, "entering...fInfoType=%d\n", fInfoType);

	if (!hdbc) {
		return SQL_INVALID_HANDLE;
	}

    ODBCConnection* conn = reinterpret_cast<ODBCConnection*>(hdbc);
    conn->GetInfo(fInfoType, rgbInfoValue, cbInfoValueMax, pcbInfoValue, UnicodeOption);

	return SQL_SUCCESS;
}

/*
 *	macros for wdtype_xxxx() calls which have WD_ATP_UNSET parameters
 */
#define wdtype_COLUMN_SIZE(conn, wdtype) wdtype_attr_column_size(conn, wdtype, WD_ATP_UNSET, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_TO_CONCISE_TYPE(conn, wdtype) wdtype_attr_to_concise_type(conn, wdtype, WD_ATP_UNSET, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_TO_SQLDESCTYPE(conn, wdtype) wdtype_attr_to_sqldesctype(conn, wdtype, WD_ATP_UNSET, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_BUFFER_LENGTH(conn, wdtype) wdtype_attr_buffer_length(conn, wdtype, WD_ATP_UNSET, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_DECIMAL_DIGITS(conn, wdtype) wdtype_attr_decimal_digits(conn, wdtype, WD_ATP_UNSET, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_TRANSFER_OCTET_LENGTH(conn, wdtype) wdtype_attr_transfer_octet_length(conn, wdtype, WD_ATP_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_TO_NAME(conn, wdtype, auto_increment) wdtype_attr_to_name(conn, wdtype, WD_ATP_UNSET, auto_increment)
#define wdtype_TO_DATETIME_SUB(conn, wdtype) wdtype_attr_to_datetime_sub(conn, wdtype, WD_ATP_UNSET)


RETCODE		SQL_API
WD_GetTypeInfo(HSTMT hstmt,
				  SQLSMALLINT fSqlType)
{
	CSTR func = "WD_GetTypeInfo";
	ODBCStatement* statement = reinterpret_cast<ODBCStatement*>(hstmt);
	statement->GetTypeInfo(fSqlType);
	return SQL_SUCCESS;
}


RETCODE		SQL_API
WD_GetFunctions(HDBC hdbc,
				   SQLUSMALLINT fFunction,
				   SQLUSMALLINT * pfExists)
{
	MYLOG(0, "entering...%u\n", fFunction);

	if (fFunction == SQL_API_ALL_FUNCTIONS)
	{
		memset(pfExists, 0, sizeof(pfExists[0]) * 100);

		/* ODBC core functions */
		pfExists[SQL_API_SQLALLOCCONNECT] = TRUE;
		pfExists[SQL_API_SQLALLOCENV] = TRUE;
		pfExists[SQL_API_SQLALLOCSTMT] = TRUE;
		pfExists[SQL_API_SQLBINDCOL] = TRUE;
		pfExists[SQL_API_SQLCANCEL] = TRUE;
		pfExists[SQL_API_SQLCOLATTRIBUTES] = TRUE;
		pfExists[SQL_API_SQLCONNECT] = TRUE;
		pfExists[SQL_API_SQLDESCRIBECOL] = TRUE;	/* partial */
		pfExists[SQL_API_SQLDISCONNECT] = TRUE;
		pfExists[SQL_API_SQLERROR] = TRUE;
		pfExists[SQL_API_SQLEXECDIRECT] = TRUE;
		pfExists[SQL_API_SQLEXECUTE] = TRUE;
		pfExists[SQL_API_SQLFETCH] = TRUE;
		pfExists[SQL_API_SQLFREECONNECT] = TRUE;
		pfExists[SQL_API_SQLFREEENV] = TRUE;
		pfExists[SQL_API_SQLFREESTMT] = TRUE;
		pfExists[SQL_API_SQLGETCURSORNAME] = TRUE;
		pfExists[SQL_API_SQLNUMRESULTCOLS] = TRUE;
		pfExists[SQL_API_SQLPREPARE] = TRUE;		/* complete? */
		pfExists[SQL_API_SQLROWCOUNT] = TRUE;
		pfExists[SQL_API_SQLSETCURSORNAME] = TRUE;
		pfExists[SQL_API_SQLSETPARAM] = FALSE;		/* odbc 1.0 */
		pfExists[SQL_API_SQLTRANSACT] = TRUE;

		/* ODBC level 1 functions */
		pfExists[SQL_API_SQLBINDPARAMETER] = FALSE;
		pfExists[SQL_API_SQLCOLUMNS] = TRUE;
		pfExists[SQL_API_SQLDRIVERCONNECT] = TRUE;
		pfExists[SQL_API_SQLGETCONNECTOPTION] = TRUE;		/* partial */
		pfExists[SQL_API_SQLGETDATA] = TRUE;
		pfExists[SQL_API_SQLGETFUNCTIONS] = TRUE;
		pfExists[SQL_API_SQLGETINFO] = TRUE;
		pfExists[SQL_API_SQLGETSTMTOPTION] = TRUE;	/* partial */
		pfExists[SQL_API_SQLGETTYPEINFO] = TRUE;
		pfExists[SQL_API_SQLPARAMDATA] = TRUE;
		pfExists[SQL_API_SQLPUTDATA] = TRUE;
		pfExists[SQL_API_SQLSETCONNECTOPTION] = TRUE;		/* partial */
		pfExists[SQL_API_SQLSETSTMTOPTION] = TRUE;
		pfExists[SQL_API_SQLSPECIALCOLUMNS] = TRUE;
		pfExists[SQL_API_SQLSTATISTICS] = TRUE;
		pfExists[SQL_API_SQLTABLES] = TRUE;

		/* ODBC level 2 functions */
		pfExists[SQL_API_SQLBROWSECONNECT] = FALSE;
		pfExists[SQL_API_SQLCOLUMNPRIVILEGES] = FALSE;
		pfExists[SQL_API_SQLDATASOURCES] = FALSE;	/* only implemented by
													 * DM */
		pfExists[SQL_API_SQLDESCRIBEPARAM] = FALSE;
		pfExists[SQL_API_SQLDRIVERS] = FALSE;		/* only implemented by
													 * DM */
		pfExists[SQL_API_SQLEXTENDEDFETCH] = TRUE;
		pfExists[SQL_API_SQLFOREIGNKEYS] = TRUE;
		pfExists[SQL_API_SQLMORERESULTS] = TRUE;
		pfExists[SQL_API_SQLNATIVESQL] = TRUE;
		pfExists[SQL_API_SQLNUMPARAMS] = TRUE;
		pfExists[SQL_API_SQLPARAMOPTIONS] = TRUE;
		pfExists[SQL_API_SQLPRIMARYKEYS] = TRUE;
		pfExists[SQL_API_SQLPROCEDURECOLUMNS] = TRUE;
		pfExists[SQL_API_SQLPROCEDURES] = TRUE;
		pfExists[SQL_API_SQLSETPOS] = TRUE;
		pfExists[SQL_API_SQLSETSCROLLOPTIONS] = TRUE;		/* odbc 1.0 */
		pfExists[SQL_API_SQLTABLEPRIVILEGES] = TRUE;
		pfExists[SQL_API_SQLBULKOPERATIONS] = FALSE;
	}
	else
	{
		switch (fFunction)
		{
			case SQL_API_SQLBINDCOL:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLCANCEL:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLCOLATTRIBUTE:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLCONNECT:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLDESCRIBECOL:
				*pfExists = TRUE;
				break;		/* partial */
			case SQL_API_SQLDISCONNECT:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLEXECDIRECT:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLEXECUTE:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLFETCH:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLFREESTMT:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLGETCURSORNAME:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLNUMRESULTCOLS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLPREPARE:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLROWCOUNT:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLSETCURSORNAME:
				*pfExists = TRUE;
				break;

				/* ODBC level 1 functions */
			case SQL_API_SQLBINDPARAMETER:
				*pfExists = FALSE;
				break;
			case SQL_API_SQLCOLUMNS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLDRIVERCONNECT:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLGETDATA:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLGETFUNCTIONS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLGETINFO:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLGETTYPEINFO:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLPARAMDATA:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLPUTDATA:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLSPECIALCOLUMNS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLSTATISTICS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLTABLES:
				*pfExists = TRUE;
				break;

				/* ODBC level 2 functions */
			case SQL_API_SQLBROWSECONNECT:
				*pfExists = FALSE;
				break;
			case SQL_API_SQLCOLUMNPRIVILEGES:
				*pfExists = FALSE;
				break;
			case SQL_API_SQLDATASOURCES:
				*pfExists = FALSE;
				break;		/* only implemented by DM */
			case SQL_API_SQLDESCRIBEPARAM:
				*pfExists = FALSE;
				break;		/* not properly implemented */
			case SQL_API_SQLDRIVERS:
				*pfExists = FALSE;
				break;		/* only implemented by DM */
			case SQL_API_SQLEXTENDEDFETCH:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLFOREIGNKEYS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLMORERESULTS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLNATIVESQL:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLNUMPARAMS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLPRIMARYKEYS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLPROCEDURECOLUMNS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLPROCEDURES:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLSETPOS:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLTABLEPRIVILEGES:
				*pfExists = TRUE;
				break;
			case SQL_API_SQLBULKOPERATIONS:	/* 24 */
				*pfExists = FALSE;
				break;
			case SQL_API_SQLALLOCHANDLE:	/* 1001 */
			case SQL_API_SQLBINDPARAM:	/* 1002 */
			case SQL_API_SQLCLOSECURSOR:	/* 1003 */
			case SQL_API_SQLENDTRAN:	/* 1005 */
			case SQL_API_SQLFETCHSCROLL:	/* 1021 */
			case SQL_API_SQLFREEHANDLE:	/* 1006 */
			case SQL_API_SQLGETCONNECTATTR:	/* 1007 */
			case SQL_API_SQLGETDESCFIELD:	/* 1008 */
			case SQL_API_SQLGETDIAGFIELD:	/* 1010 */
			case SQL_API_SQLGETDIAGREC:	/* 1011 */
			case SQL_API_SQLGETENVATTR:	/* 1012 */
			case SQL_API_SQLGETSTMTATTR:	/* 1014 */
			case SQL_API_SQLSETCONNECTATTR:	/* 1016 */
			case SQL_API_SQLSETDESCFIELD:	/* 1017 */
			case SQL_API_SQLSETENVATTR:	/* 1019 */
			case SQL_API_SQLSETSTMTATTR:	/* 1020 */
				*pfExists = TRUE;
				break;
			case SQL_API_SQLGETDESCREC:	/* 1009 */
			case SQL_API_SQLSETDESCREC:	/* 1018 */
			case SQL_API_SQLCOPYDESC:	/* 1004 */
				*pfExists = FALSE;
				break;
			default:
				*pfExists = FALSE;
				break;
		}
	}
	return SQL_SUCCESS;
}


char *
identifierEscape(const SQLCHAR *src, SQLLEN srclen, const ConnectionClass *conn, char *buf, size_t bufsize, BOOL double_quote)
{
	int	i, outlen;
	UCHAR	tchar;
	char	*dest = NULL, escape_ch = CC_get_escape(conn);
	encoded_str	encstr;

	if (!src || srclen == SQL_NULL_DATA)
		return dest;
	else if (srclen == SQL_NTS)
		srclen = (SQLLEN) strlen((char *) src);
	if (srclen <= 0)
		return dest;
MYLOG(0, "entering in=%s(" FORMAT_LEN ")\n", src, srclen);
	if (NULL != buf && bufsize > 0)
		dest = buf;
	else
	{
		bufsize = 2 * srclen + 1;
		dest = static_cast<char*>(malloc(bufsize));
	}
	if (!dest) return NULL;
	encoded_str_constr(&encstr, conn->ccsc, (char *) src);
	outlen = 0;
	if (double_quote)
		dest[outlen++] = IDENTIFIER_QUOTE;
	for (i = 0, tchar = encoded_nextchar(&encstr); i < srclen && static_cast<size_t>(outlen) < bufsize - 1; i++, tchar = encoded_nextchar(&encstr))
	{
                if (MBCS_NON_ASCII(encstr))
                {
                        dest[outlen++] = tchar;
                        continue;
                }
		if (LITERAL_QUOTE == tchar ||
		    escape_ch == tchar)
			dest[outlen++] = tchar;
		else if (double_quote &&
			 IDENTIFIER_QUOTE == tchar)
			dest[outlen++] = tchar;
		dest[outlen++] = tchar;
	}
	if (double_quote)
		dest[outlen++] = IDENTIFIER_QUOTE;
	dest[outlen] = '\0';
MYLOG(0, "leaving output=%s(%d)\n", dest, outlen);
	return dest;
}

static char *
simpleCatalogEscape(const SQLCHAR *src, SQLLEN srclen, const ConnectionClass *conn)
{
	return identifierEscape(src, srclen, conn, NULL, -1, FALSE);
}

/*
 *	PostgreSQL needs 2 '\\' to escape '_' and '%'.
 */
static char	*
adjustLikePattern(const SQLCHAR *src, int srclen, const ConnectionClass *conn)
{
	int	i, outlen;
	UCHAR	tchar;
	char	*dest = NULL, escape_in_literal = CC_get_escape(conn);
	BOOL	escape_in = FALSE;
	encoded_str	encstr;

	if (!src || srclen == SQL_NULL_DATA)
		return dest;
	else if (srclen == SQL_NTS)
		srclen = (int) strlen((char *) src);
	/* if (srclen <= 0) */
	if (srclen < 0)
		return dest;
MYLOG(0, "entering in=%.*s(%d)\n", srclen, src, srclen);
	encoded_str_constr(&encstr, conn->ccsc, (char *) src);
	dest = static_cast<char*>(malloc(4 * srclen + 1));
	if (!dest) return NULL;
	for (i = 0, outlen = 0; i < srclen; i++)
	{
		tchar = encoded_nextchar(&encstr);
		if (MBCS_NON_ASCII(encstr))
		{
			dest[outlen++] = tchar;
			continue;
		}
		if (escape_in)
		{
			switch (tchar)
			{
				case '%':
				case '_':
					break;
				default:
					if (SEARCH_PATTERN_ESCAPE == escape_in_literal)
						dest[outlen++] = escape_in_literal;
					dest[outlen++] = SEARCH_PATTERN_ESCAPE;
					break;
			}
		}
		if (tchar == SEARCH_PATTERN_ESCAPE)
		{
			escape_in = TRUE;
			if (SEARCH_PATTERN_ESCAPE == escape_in_literal)
				dest[outlen++] = escape_in_literal; /* insert 1 more LEXER escape */
		}
		else
		{
			escape_in = FALSE;
			if (LITERAL_QUOTE == tchar)
				dest[outlen++] = tchar;
		}
		dest[outlen++] = tchar;
	}
	if (escape_in)
	{
		if (SEARCH_PATTERN_ESCAPE == escape_in_literal)
			dest[outlen++] = escape_in_literal;
		dest[outlen++] = SEARCH_PATTERN_ESCAPE;
	}
	dest[outlen] = '\0';
MYLOG(0, "leaving output=%s(%d)\n", dest, outlen);
	return dest;
}

#define	CSTR_SYS_TABLE	"SYSTEM TABLE"
#define	CSTR_TABLE	"TABLE"
#define	CSTR_VIEW	"VIEW"
#define CSTR_FOREIGN_TABLE "FOREIGN TABLE"
#define CSTR_MATVIEW "MATVIEW"

CSTR	like_op_sp = "like ";
CSTR	like_op_ext = "like E";
CSTR	eq_op_sp =	"= ";
CSTR	eq_op_ext =	"= E";

#define	 IS_VALID_NAME(str) ((str) && (str)[0])

static const char *gen_opestr(const char *orig_opestr, const ConnectionClass * conn)
{
	BOOL	addE = (0 != CC_get_escape(conn) && WD_VERSION_GE(conn, 8.1));

	if (0 == strcmp(orig_opestr, eqop))
		return (addE ? eq_op_ext : eq_op_sp);
	return (addE ? like_op_ext : like_op_sp);
}

/*
 *	If specified schema name == user_name and the current schema is
 *	'public', allowed to use the 'public' schema.
 */
static BOOL
allow_public_schema(ConnectionClass *conn, const SQLCHAR *szSchemaName, SQLSMALLINT cbSchemaName)
{
	const char *user = CC_get_username(conn);
	const char *curschema;
	size_t	userlen = strlen(user);
	size_t	schemalen;

	if (NULL == szSchemaName)
		return FALSE;

	if (SQL_NTS == cbSchemaName)
		schemalen = strlen((char *) szSchemaName);
	else
		schemalen = cbSchemaName;

	if (schemalen != userlen)
		return FALSE;
	if (strnicmp((char *) szSchemaName, user, userlen) != 0)
		return FALSE;

	curschema = CC_get_current_schema(conn);
	if (curschema == NULL)
		return FALSE;

	return stricmp(curschema, (const char *) pubstr) == 0;
}

#define TABLE_IN_RELKIND	"('r', 'v', 'm', 'f', 'p')"

RETCODE		SQL_API
WD_Tables(HSTMT hstmt,
			 const SQLCHAR * szTableQualifier, /* PV X*/
			 SQLSMALLINT cbTableQualifier,
			 const SQLCHAR * szTableOwner, /* PV E*/
			 SQLSMALLINT cbTableOwner,
			 const SQLCHAR * szTableName, /* PV E*/
			 SQLSMALLINT cbTableName,
			 const SQLCHAR * szTableType,
			 SQLSMALLINT cbTableType)
{
	CSTR func = "WD_Tables";
	ODBCStatement* statement = reinterpret_cast<ODBCStatement*>(hstmt);
	
	std::string qualifier;
	if (szTableQualifier) {
	  const char* qualifierCstr = reinterpret_cast<const char*>(szTableQualifier);
	  qualifier = std::string(qualifierCstr, cbTableQualifier == SQL_NTS ? strlen(qualifierCstr) : cbTableQualifier);
	}

	std::string owner;
	if (szTableOwner) {
	  const char* ownerCstr = reinterpret_cast<const char*>(szTableOwner);
	  owner = std::string(ownerCstr, cbTableOwner == SQL_NTS ? strlen(ownerCstr) : cbTableOwner);
	}

	std::string name;
	if (szTableName) {
	  const char* nameCstr = reinterpret_cast<const char*>(szTableName);
	  name = std::string(nameCstr, cbTableName == SQL_NTS ? strlen(nameCstr) : cbTableName);
	}

	std::string type;
	if (szTableType) {
	  const char* typeCstr = reinterpret_cast<const char*>(szTableType);
	  type = std::string(typeCstr, cbTableType == SQL_NTS ? strlen(typeCstr) : cbTableType);
	}

	statement->GetTables(szTableQualifier ? &qualifier : nullptr, 
	  szTableOwner ? &owner : nullptr,
	  szTableName ? &name : nullptr,
	  szTableType ? &type : nullptr);

	return SQL_SUCCESS;
}

/*
 *	macros for wdtype_attr_xxxx() calls which have
 *		WD_ADT_UNSET or WD_UNKNOWNS_UNSET parameters
 */
#define wdtype_ATTR_COLUMN_SIZE(conn, wdtype, atttypmod) wdtype_attr_column_size(conn, wdtype, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_ATTR_TO_CONCISE_TYPE(conn, wdtype, atttypmod) wdtype_attr_to_concise_type(conn, wdtype, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_ATTR_TO_SQLDESCTYPE(conn, wdtype, atttypmod) wdtype_attr_to_sqldesctype(conn, wdtype, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_ATTR_DISPLAY_SIZE(conn, wdtype, atttypmod) wdtype_attr_display_size(conn, wdtype, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_ATTR_BUFFER_LENGTH(conn, wdtype, atttypmod) wdtype_attr_buffer_length(conn, wdtype, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_ATTR_DECIMAL_DIGITS(conn, wdtype, atttypmod) wdtype_attr_decimal_digits(conn, wdtype, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET)
#define wdtype_ATTR_TRANSFER_OCTET_LENGTH(conn, wdtype, atttypmod) wdtype_attr_transfer_octet_length(conn, wdtype, atttypmod, WD_UNKNOWNS_UNSET)

/*
 *	for oid or xmin
 */
static void
add_tuple_for_oid_or_xmin(TupleField *tuple, int ordinal, const char *colname, OID the_type, const char *typname, const ConnectionClass *conn, const char *table_owner, const char *table_name, OID greloid, int attnum, BOOL auto_increment, int table_info)
{
	int	sqltype;
	const int atttypmod = -1;

	set_tuplefield_string(&tuple[COLUMNS_CATALOG_NAME], CurrCat(conn));
	/* see note in SQLTables() */
	set_tuplefield_string(&tuple[COLUMNS_SCHEMA_NAME], GET_SCHEMA_NAME(table_owner));
	set_tuplefield_string(&tuple[COLUMNS_TABLE_NAME], table_name);
	set_tuplefield_string(&tuple[COLUMNS_COLUMN_NAME], colname);
	sqltype = wdtype_ATTR_TO_CONCISE_TYPE(conn, the_type, atttypmod);
	set_tuplefield_int2(&tuple[COLUMNS_DATA_TYPE], sqltype);
	set_tuplefield_string(&tuple[COLUMNS_TYPE_NAME], typname);

	set_tuplefield_int4(&tuple[COLUMNS_PRECISION], wdtype_ATTR_COLUMN_SIZE(conn, the_type, atttypmod));
	set_tuplefield_int4(&tuple[COLUMNS_LENGTH], wdtype_ATTR_BUFFER_LENGTH(conn, the_type, atttypmod));
	set_nullfield_int2(&tuple[COLUMNS_SCALE], wdtype_ATTR_DECIMAL_DIGITS(conn, the_type, atttypmod));
	set_nullfield_int2(&tuple[COLUMNS_RADIX], wdtype_radix(conn, the_type));
	set_tuplefield_int2(&tuple[COLUMNS_NULLABLE], SQL_NO_NULLS);
	set_tuplefield_string(&tuple[COLUMNS_REMARKS], NULL_STRING);
	set_tuplefield_null(&tuple[COLUMNS_COLUMN_DEF]);
	set_tuplefield_int2(&tuple[COLUMNS_SQL_DATA_TYPE], sqltype);
	set_tuplefield_null(&tuple[COLUMNS_SQL_DATETIME_SUB]);
	set_tuplefield_null(&tuple[COLUMNS_CHAR_OCTET_LENGTH]);
	set_tuplefield_int4(&tuple[COLUMNS_ORDINAL_POSITION], ordinal);
	set_tuplefield_string(&tuple[COLUMNS_IS_NULLABLE], "No");
	set_tuplefield_int4(&tuple[COLUMNS_DISPLAY_SIZE], wdtype_ATTR_DISPLAY_SIZE(conn, the_type, atttypmod));
	set_tuplefield_int4(&tuple[COLUMNS_FIELD_TYPE], the_type);
	set_tuplefield_int4(&tuple[COLUMNS_AUTO_INCREMENT], auto_increment);
	set_tuplefield_int2(&tuple[COLUMNS_PHYSICAL_NUMBER], attnum);
	set_tuplefield_int4(&tuple[COLUMNS_TABLE_OID], greloid);
	set_tuplefield_int4(&tuple[COLUMNS_BASE_TYPEID], 0);
	set_tuplefield_int4(&tuple[COLUMNS_ATTTYPMOD], -1);
	set_tuplefield_int4(&tuple[COLUMNS_TABLE_INFO], table_info);
}

RETCODE		SQL_API
WD_Columns(HSTMT hstmt,
			  const SQLCHAR * szTableQualifier, /* OA X*/
			  SQLSMALLINT cbTableQualifier,
			  const SQLCHAR * szTableOwner, /* PV E*/
			  SQLSMALLINT cbTableOwner,
			  const SQLCHAR * szTableName, /* PV E*/
			  SQLSMALLINT cbTableName,
			  const SQLCHAR * szColumnName, /* PV E*/
			  SQLSMALLINT cbColumnName)
{
	CSTR func = "WD_Tables";
	ODBCStatement* statement = reinterpret_cast<ODBCStatement*>(hstmt);
	
	std::string qualifier;
	if (szTableQualifier) {
	  const char* qualifierCstr = reinterpret_cast<const char*>(szTableQualifier);
	  qualifier = std::string(qualifierCstr, cbTableQualifier == SQL_NTS ? strlen(qualifierCstr) : cbTableQualifier);
	}

	std::string owner;
	if (szTableOwner) {
	  const char* ownerCstr = reinterpret_cast<const char*>(szTableOwner);
	  owner = std::string(ownerCstr, cbTableOwner == SQL_NTS ? strlen(ownerCstr) : cbTableOwner);
	}

	std::string name;
	if (szTableName) {
	  const char* nameCstr = reinterpret_cast<const char*>(szTableName);
	  name = std::string(nameCstr, cbTableName == SQL_NTS ? strlen(nameCstr) : cbTableName);
	}

	std::string colName;
	if (szColumnName) {
	  const char* colCstr = reinterpret_cast<const char*>(szColumnName);
	  colName = std::string(colCstr, cbColumnName == SQL_NTS ? strlen(colCstr) : cbColumnName);
	}

	statement->GetColumns(szTableQualifier ? &qualifier : nullptr, 
	  szTableOwner ? &owner : nullptr,
	  szTableName ? &name : nullptr,
	  szColumnName ? &colName : nullptr);

	return SQL_SUCCESS;
}


RETCODE		SQL_API
WD_SpecialColumns(HSTMT hstmt,
					 SQLUSMALLINT fColType,
					 const SQLCHAR * szTableQualifier,
					 SQLSMALLINT cbTableQualifier,
					 const SQLCHAR * szTableOwner, /* OA E*/
					 SQLSMALLINT cbTableOwner,
					 const SQLCHAR * szTableName, /* OA(R) E*/
					 SQLSMALLINT cbTableName,
					 SQLUSMALLINT fScope,
					 SQLUSMALLINT fNullable)
{
	return SQL_SUCCESS;
	#if 0
	CSTR func = "WD_SpecialColumns";
	TupleField	*tuple;
	StatementClass *stmt = (StatementClass *) hstmt;
	ConnectionClass *conn;
	QResultClass	*res;
	StatementClass *col_stmt = NULL;
	PQExpBufferData		columns_query = {0};
	char		*escSchemaName = NULL, *escTableName = NULL;
	RETCODE		ret = SQL_ERROR, result;
	char		relhasrules[MAX_INFO_STRING], relkind[8], relhasoids[8];
	BOOL		relisaview;
	SQLSMALLINT	internal_asis_type = SQL_C_CHAR, cbSchemaName;
	const SQLCHAR	*szSchemaName;
	const char *eq_string;
	int		result_cols;

	static const char *catcn[][2] = {
		{"SCOPE", "SCOPE"},
		{"COLUMN_NAME", "COLUMN_NAME"},
		{"DATA_TYPE", "DATA_TYPE"},
		{"TYPE_NAME", "TYPE_NAME"},
		{"COLUMN_SIZE", "PRECISION"},
		{"BUFFER_LENGTH", "LENGTH"},
		{"DECIMAL_DIGITS", "SCALE"},
		{"PSEUDO_COLUMN", "PSEUDO_COLUMN"}};
	EnvironmentClass	*env;
	BOOL is_ODBC2;

	MYLOG(0, "entering...stmt=%p scnm=%p len=%d colType=%d scope=%d\n", stmt, szTableOwner, cbTableOwner, fColType, fScope);

	if (result = SC_initialize_and_recycle(stmt), SQL_SUCCESS != result)
		return result;
	conn = SC_get_conn(stmt);
	env = static_cast<EnvironmentClass_*>(CC_get_env(conn));
	is_ODBC2 = EN_is_odbc2(env);
#ifdef	UNICODE_SUPPORT
	if (CC_is_in_unicode_driver(conn))
		internal_asis_type = INTERNAL_ASIS_TYPE;
#endif /* UNICODE_SUPPORT */

	szSchemaName = szTableOwner;
	cbSchemaName = cbTableOwner;

	escTableName = simpleCatalogEscape(szTableName, cbTableName, conn);
	if (!escTableName)
	{
		SC_set_error(stmt, STMT_INVALID_NULL_ARG, "The table name is required", func);
		return SQL_ERROR;
	}
#define	return	DONT_CALL_RETURN_FROM_HERE???

retry_public_schema:
	if (escSchemaName)
		free(escSchemaName);
	escSchemaName = simpleCatalogEscape(szSchemaName, cbSchemaName, conn);
	eq_string = gen_opestr(eqop, conn);
	initPQExpBuffer(&columns_query);
#define	return	DONT_CALL_RETURN_FROM_HERE???
	/*
	 * Create the query to find out if this is a view or not...
	 */
	appendPQExpBufferStr(&columns_query, "select c.relhasrules, c.relkind");
   if (WD_VERSION_LT(conn, 12.0))
       appendPQExpBufferStr(&columns_query, ", c.relhasoids");
   else
       appendPQExpBufferStr(&columns_query, ", 0 as relhasoids");
	appendPQExpBufferStr(&columns_query, " from WD_catalog.WD_namespace u,"
					" WD_catalog.WD_class c where "
					"u.oid = c.relnamespace");

	/* TableName cannot contain a string search pattern */
	if (escTableName)
		appendPQExpBuffer(&columns_query,
					 " and c.relname %s'%s'", eq_string, escTableName);
	/* SchemaName cannot contain a string search pattern */
	schema_appendPQExpBuffer1(&columns_query, " and u.nspname %s'%.*s'", eq_string, escSchemaName, TABLE_IS_VALID(szTableName, cbTableName), conn);

	result = WD_AllocStmt(conn, (HSTMT *) &col_stmt, 0);
	if (!SQL_SUCCEEDED(result))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Couldn't allocate statement for SQLSpecialColumns result.", func);
		goto cleanup;
	}

	MYLOG(0, "col_stmt = %p\n", col_stmt);

	if (PQExpBufferDataBroken(columns_query))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in WD_SpecialColumns()", func);
		goto cleanup;
	}
	result = WD_ExecDirect(col_stmt, (SQLCHAR *) columns_query.data, SQL_NTS, PODBC_RDONLY);
	if (!SQL_SUCCEEDED(result))
	{
		SC_full_error_copy(stmt, col_stmt, FALSE);
		goto cleanup;
	}

	/* If not found */
	if ((res = SC_get_Result(col_stmt)) &&
	    0 == QR_get_num_total_tuples(res))
	{
		if (allow_public_schema(conn, szSchemaName, cbSchemaName))
		{
			WD_FreeStmt(col_stmt, SQL_DROP);
			col_stmt = NULL;
			szSchemaName = pubstr;
			cbSchemaName = SQL_NTS;
			goto retry_public_schema;
		}

		SC_set_error(stmt, DESC_BAD_PARAMETER_NUMBER_ERROR, "The specified table does not exist", func);
		goto cleanup;
	}

	result = WD_BindCol(col_stmt, 1, internal_asis_type,
					relhasrules, sizeof(relhasrules), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	result = WD_BindCol(col_stmt, 2, internal_asis_type,
					relkind, sizeof(relkind), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}
	relhasoids[0] = '1';
	result = WD_BindCol(col_stmt, 3, internal_asis_type,
				relhasoids, sizeof(relhasoids), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	result = WD_Fetch(col_stmt);
	relisaview = (relkind[0] == 'v');
	WD_FreeStmt(col_stmt, SQL_DROP);
	col_stmt = NULL;

	res = QR_Constructor();
	if (!res)
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Couldn't allocate memory for query.", func);
		goto cleanup;
	}
	SC_set_Result(stmt, res);
	extend_column_bindings(SC_get_ARDF(stmt), 8);

	stmt->catalog_result = TRUE;
	result_cols = NUM_OF_SPECOLS_FIELDS;
	QR_set_num_fields(res, result_cols);
	QR_set_field_info_EN(res, 0, WD_TYPE_INT2, 2);
	QR_set_field_info_EN(res, 1, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, 2, WD_TYPE_INT2, 2);
	QR_set_field_info_EN(res, 3, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, 4, WD_TYPE_INT4, 4);
	QR_set_field_info_EN(res, 5, WD_TYPE_INT4, 4);
	QR_set_field_info_EN(res, 6, WD_TYPE_INT2, 2);
	QR_set_field_info_EN(res, 7, WD_TYPE_INT2, 2);

	if (relisaview)
	{
		/* there's no oid for views */
		if (fColType == SQL_BEST_ROWID)
		{
			ret = SQL_SUCCESS;
			goto cleanup;
		}
		else if (fColType == SQL_ROWVER)
		{
			Int2		the_type = WD_TYPE_TID;
			int	atttypmod = -1;

			tuple = QR_AddNew(res);

			set_tuplefield_null(&tuple[SPECOLS_SCOPE]);
			set_tuplefield_string(&tuple[SPECOLS_COLUMN_NAME], "ctid");
			set_tuplefield_int2(&tuple[SPECOLS_DATA_TYPE], wdtype_ATTR_TO_CONCISE_TYPE(conn, the_type, atttypmod));
			set_tuplefield_string(&tuple[SPECOLS_TYPE_NAME], wdtype_attr_to_name(conn, the_type, atttypmod, FALSE));
			set_tuplefield_int4(&tuple[SPECOLS_COLUMN_SIZE], wdtype_ATTR_COLUMN_SIZE(conn, the_type, atttypmod));
			set_tuplefield_int4(&tuple[SPECOLS_BUFFER_LENGTH], wdtype_ATTR_BUFFER_LENGTH(conn, the_type, atttypmod));
			set_tuplefield_int2(&tuple[SPECOLS_DECIMAL_DIGITS], wdtype_ATTR_DECIMAL_DIGITS(conn, the_type, atttypmod));
			set_tuplefield_int2(&tuple[SPECOLS_PSEUDO_COLUMN], SQL_PC_NOT_PSEUDO);
MYLOG(DETAIL_LOG_LEVEL, "Add ctid\n");
		}
	}
	else
	{
		/* use the oid value for the rowid */
		if (fColType == SQL_BEST_ROWID)
		{
			Int2	the_type = WD_TYPE_OID;
			int	atttypmod = -1;

			if (relhasoids[0] != '1')
			{
				ret = SQL_SUCCESS;
				goto cleanup;
			}
			tuple = QR_AddNew(res);

			set_tuplefield_int2(&tuple[SPECOLS_SCOPE], SQL_SCOPE_SESSION);
			set_tuplefield_string(&tuple[SPECOLS_COLUMN_NAME], OID_NAME);
			set_tuplefield_int2(&tuple[SPECOLS_DATA_TYPE], wdtype_ATTR_TO_CONCISE_TYPE(conn, the_type, atttypmod));
			set_tuplefield_string(&tuple[SPECOLS_TYPE_NAME], wdtype_attr_to_name(conn, the_type, atttypmod, TRUE));
			set_tuplefield_int4(&tuple[SPECOLS_COLUMN_SIZE], wdtype_ATTR_COLUMN_SIZE(conn, the_type, atttypmod));
			set_tuplefield_int4(&tuple[SPECOLS_BUFFER_LENGTH], wdtype_ATTR_BUFFER_LENGTH(conn, the_type, atttypmod));
			set_tuplefield_int2(&tuple[SPECOLS_DECIMAL_DIGITS], wdtype_ATTR_DECIMAL_DIGITS(conn, the_type, atttypmod));
			set_tuplefield_int2(&tuple[SPECOLS_PSEUDO_COLUMN], SQL_PC_PSEUDO);
		}
		else if (fColType == SQL_ROWVER)
		{
			Int2		the_type = WD_TYPE_XID;
			int	atttypmod = -1;

			tuple = QR_AddNew(res);

			set_tuplefield_null(&tuple[SPECOLS_SCOPE]);
			set_tuplefield_string(&tuple[SPECOLS_COLUMN_NAME], XMIN_NAME);
			set_tuplefield_int2(&tuple[SPECOLS_DATA_TYPE], wdtype_ATTR_TO_CONCISE_TYPE(conn, the_type, atttypmod));
			set_tuplefield_string(&tuple[SPECOLS_TYPE_NAME], wdtype_attr_to_name(conn, the_type, atttypmod, FALSE));
			set_tuplefield_int4(&tuple[SPECOLS_COLUMN_SIZE], wdtype_ATTR_COLUMN_SIZE(conn, the_type, atttypmod));
			set_tuplefield_int4(&tuple[SPECOLS_BUFFER_LENGTH], wdtype_ATTR_BUFFER_LENGTH(conn, the_type, atttypmod));
			set_tuplefield_int2(&tuple[SPECOLS_DECIMAL_DIGITS], wdtype_ATTR_DECIMAL_DIGITS(conn, the_type, atttypmod));
			set_tuplefield_int2(&tuple[SPECOLS_PSEUDO_COLUMN], SQL_PC_PSEUDO);
		}
	}
	ret = SQL_SUCCESS;

cleanup:
#undef	return
	if (!SQL_SUCCEEDED(ret) && 0 >= SC_get_errornumber(stmt))
		SC_error_copy(stmt, col_stmt, TRUE);
	if (!PQExpBufferDataBroken(columns_query))
		termPQExpBuffer(&columns_query);
	if (escSchemaName)
		free(escSchemaName);
	if (escTableName)
		free(escTableName);
	stmt->status = STMT_FINISHED;
	stmt->currTuple = -1;
	SC_set_rowset_start(stmt, -1, FALSE);
	SC_set_current_col(stmt, -1);
	if (col_stmt)
		WD_FreeStmt(col_stmt, SQL_DROP);
	MYLOG(0, "leaving  stmt=%p\n", stmt);
	return ret;

	#endif
}


#define INDOPTION_DESC		0x0001	/* values are in reverse order */
RETCODE		SQL_API
WD_Statistics(HSTMT hstmt,
				 const SQLCHAR * szTableQualifier, /* OA X*/
				 SQLSMALLINT cbTableQualifier,
				 const SQLCHAR * szTableOwner, /* OA E*/
				 SQLSMALLINT cbTableOwner,
				 const SQLCHAR * szTableName, /* OA(R) E*/
				 SQLSMALLINT cbTableName,
				 SQLUSMALLINT fUnique,
				 SQLUSMALLINT fAccuracy)
{
	return SQL_SUCCESS;
	#if 0
	CSTR func = "WD_Statistics";
	StatementClass *stmt = (StatementClass *) hstmt;
	ConnectionClass *conn;
	QResultClass	*res;
	PQExpBufferData		index_query = {0};
	RETCODE		ret = SQL_ERROR, result;
	char		*escSchemaName = NULL, *table_name = NULL, *escTableName = NULL;
	char		index_name[MAX_INFO_STRING];
	short		fields_vector[INDEX_KEYS_STORAGE_COUNT + 1];
	short		indopt_vector[INDEX_KEYS_STORAGE_COUNT + 1];
	char		isunique[10],
				isclustered[10],
				ishash[MAX_INFO_STRING];
	SQLLEN		index_name_len, fields_vector_len;
	TupleField	*tuple;
	int			i;
	StatementClass *col_stmt = NULL, *indx_stmt = NULL;
	char		column_name[MAX_INFO_STRING],
			table_schemaname[MAX_INFO_STRING],
				relhasrules[10];
	struct columns_idx {
		int	pnum;
		char	*col_name;
	} *column_names = NULL;
	/* char	  **column_names = NULL; */
	SQLLEN		column_name_len;
	int		total_columns = 0, alcount;
	ConnInfo   *ci;
	char		buf[256];
	SQLSMALLINT	internal_asis_type = SQL_C_CHAR, cbSchemaName, field_number;
	const SQLCHAR *szSchemaName;
	const char *eq_string;
	OID		ioid;
	Int4		relhasoids;

	static const char *catcn[][2] = {
		{"TABLE_CAT", "TABLE_QUALIFIER"},
		{"TABLE_SCHEM", "TABLE_OWNER"},
		{"TABLE_NAME", "TABLE_NAME"},
		{"NON_UNIQUE" , "NON_UNIQUE"},
		{"INDEX_QUALIFIER", "INDEX_QUALIFIER"},
		{"INDEX_NAME", "INDEX_NAME"},
		{"TYPE", "TYPE"},
		{"ORDINAL_POSITION", "SEQ_IN_INDEX"},
		{"COLUMN_NAME", "COLUMN_NAME"},
		{"ASC_OR_DESC", "COLLATION"},
		{"CARDINALITY", "CARDINALITY"},
		{"PAGES", "PAGES"},
		{"FILTER_CONDITION", "FILTER_CONDITION"}};
	EnvironmentClass *env;
	BOOL is_ODBC2;


	MYLOG(0, "entering...stmt=%p scnm=%p len=%d\n", stmt, szTableOwner, cbTableOwner);

	if (result = SC_initialize_and_recycle(stmt), SQL_SUCCESS != result)
		return result;

	table_name = make_string(szTableName, cbTableName, NULL, 0);
	if (!table_name)
	{
		SC_set_error(stmt, STMT_INVALID_NULL_ARG, "The table name is required", func);
		return result;
	}
	conn = SC_get_conn(stmt);
	ci = &(conn->connInfo);
	env = CC_get_env(conn);
	is_ODBC2 = EN_is_odbc2(env);
#ifdef	UNICODE_SUPPORT
	if (CC_is_in_unicode_driver(conn))
		internal_asis_type = INTERNAL_ASIS_TYPE;
#endif /* UNICODE_SUPPORT */

	if (res = QR_Constructor(), !res)
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Couldn't allocate memory for WD_Statistics result.", func);
		free(table_name);
		return SQL_ERROR;
	}
	SC_set_Result(stmt, res);

	/* the binding structure for a statement is not set up until */

	/*
	 * a statement is actually executed, so we'll have to do this
	 * ourselves.
	 */
	extend_column_bindings(SC_get_ARDF(stmt), 13);

	stmt->catalog_result = TRUE;
	/* set the field names */
	QR_set_num_fields(res, NUM_OF_STATS_FIELDS);
	QR_set_field_info_EN(res, STATS_CATALOG_NAME, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, STATS_SCHEMA_NAME, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, STATS_TABLE_NAME, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, STATS_NON_UNIQUE, WD_TYPE_INT2, 2);
	QR_set_field_info_EN(res, STATS_INDEX_QUALIFIER, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, STATS_INDEX_NAME, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, STATS_TYPE, WD_TYPE_INT2, 2);
	QR_set_field_info_EN(res, STATS_SEQ_IN_INDEX, WD_TYPE_INT2, 2);
	QR_set_field_info_EN(res, STATS_COLUMN_NAME, WD_TYPE_VARCHAR, MAX_INFO_STRING);
	QR_set_field_info_EN(res, STATS_COLLATION, WD_TYPE_CHAR, 1);
	QR_set_field_info_EN(res, STATS_CARDINALITY, WD_TYPE_INT4, 4);
	QR_set_field_info_EN(res, STATS_PAGES, WD_TYPE_INT4, 4);
	QR_set_field_info_EN(res, STATS_FILTER_CONDITION, WD_TYPE_VARCHAR, MAX_INFO_STRING);

#define	return	DONT_CALL_RETURN_FROM_HERE???
	szSchemaName = szTableOwner;
	cbSchemaName = cbTableOwner;

	table_schemaname[0] = '\0';
	schema_str(table_schemaname, sizeof(table_schemaname), szSchemaName, cbSchemaName, TABLE_IS_VALID(szTableName, cbTableName), conn);

	/*
	 * we need to get a list of the field names first, so we can return
	 * them later.
	 */
	result = WD_AllocStmt(conn, (HSTMT *) &col_stmt, 0);
	if (!SQL_SUCCEEDED(result))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "WD_AllocStmt failed in WD_Statistics for columns.", func);
		goto cleanup;
	}

	/*
	 * table_name parameter cannot contain a string search pattern.
	 */
	result = WD_Columns(col_stmt,
						   NULL, 0,
						   (SQLCHAR *) table_schemaname, SQL_NTS,
						   (SQLCHAR *) table_name, SQL_NTS,
						   NULL, 0,
						   PODBC_NOT_SEARCH_PATTERN | PODBC_SEARCH_PUBLIC_SCHEMA, 0, 0);

	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}
	result = WD_BindCol(col_stmt, COLUMNS_COLUMN_NAME + 1, internal_asis_type,
						 column_name, sizeof(column_name), &column_name_len);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}
	result = WD_BindCol(col_stmt, COLUMNS_PHYSICAL_NUMBER + 1, SQL_C_SHORT,
			&field_number, sizeof(field_number), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	alcount = 0;
	result = WD_Fetch(col_stmt);
	while (SQL_SUCCEEDED(result))
	{
		if (0 == total_columns)
			WD_GetData(col_stmt, 2, internal_asis_type, table_schemaname, sizeof(table_schemaname), NULL);

		if (total_columns >= alcount)
		{
			if (0 == alcount)
				alcount = 4;
			else
				alcount *= 2;
			SC_REALLOC_gexit_with_error(column_names, struct columns_idx, alcount * sizeof(struct columns_idx), stmt, "Couldn't allocate memory for column names.", (result = SQL_ERROR));
		}
		column_names[total_columns].col_name = strdup(column_name);
		if (!column_names[total_columns].col_name)
		{
			SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Couldn't allocate memory for column name.", func);
			goto cleanup;
		}
		column_names[total_columns].pnum = field_number;
		total_columns++;

		MYLOG(0, "column_name = '%s'\n", column_name);

		result = WD_Fetch(col_stmt);
	}

	if (result != SQL_NO_DATA_FOUND)
	{
		SC_full_error_copy(stmt, col_stmt, FALSE);
		goto cleanup;
	}
	WD_FreeStmt(col_stmt, SQL_DROP);
	col_stmt = NULL;
	if (total_columns == 0)
	{
		/* Couldn't get column names in SQLStatistics.; */
		ret = SQL_SUCCESS;
		goto cleanup;
	}

	/* get a list of indexes on this table */
	result = WD_AllocStmt(conn, (HSTMT *) &indx_stmt, 0);
	if (!SQL_SUCCEEDED(result))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "WD_AllocStmt failed in SQLStatistics for indices.", func);
		goto cleanup;

	}

	/* TableName cannot contain a string search pattern */
	escTableName = simpleCatalogEscape((SQLCHAR *) table_name, SQL_NTS, conn);
	eq_string = gen_opestr(eqop, conn);
	escSchemaName = simpleCatalogEscape((SQLCHAR *) table_schemaname, SQL_NTS, conn);
	initPQExpBuffer(&index_query);
	printfPQExpBuffer(&index_query, "select c.relname, i.indkey, i.indisunique"
		", i.indisclustered, a.amname, c.relhasrules, n.nspname"
		", c.oid, %s, %s"
		" from WD_catalog.WD_index i, WD_catalog.WD_class c,"
		" WD_catalog.WD_class d, WD_catalog.WD_am a,"
		" WD_catalog.WD_namespace n"
		" where d.relname %s'%s'"
		" and n.nspname %s'%s'"
		" and n.oid = d.relnamespace"
		" and d.oid = i.indrelid"
		" and i.indexrelid = c.oid"
		" and c.relam = a.oid order by"
        , WD_VERSION_LT(conn, 12.0) ? "d.relhasoids" : "0"
		, WD_VERSION_GE(conn, 8.3) ? "i.indoption" : "0"
		, eq_string, escTableName, eq_string, escSchemaName);
	appendPQExpBufferStr(&index_query, " i.indisprimary desc,");
	appendPQExpBufferStr(&index_query, " i.indisunique, n.nspname, c.relname");
	if (PQExpBufferDataBroken(index_query))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in WD_Columns()", func);
		goto cleanup;
	}

	result = WD_ExecDirect(indx_stmt, (SQLCHAR *) index_query.data, SQL_NTS, PODBC_RDONLY);
	if (!SQL_SUCCEEDED(result))
	{
		/*
		 * "Couldn't execute index query (w/SQLExecDirect) in
		 * SQLStatistics.";
		 */
		SC_full_error_copy(stmt, indx_stmt, FALSE);
		goto cleanup;
	}

	/* bind the index name column */
	result = WD_BindCol(indx_stmt, 1, internal_asis_type,
						   index_name, MAX_INFO_STRING, &index_name_len);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;

	}
	/* bind the vector column */
	result = WD_BindCol(indx_stmt, 2, SQL_C_DEFAULT,
			fields_vector, sizeof(fields_vector), &fields_vector_len);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;

	}
	/* bind the "is unique" column */
	result = WD_BindCol(indx_stmt, 3, internal_asis_type,
						   isunique, sizeof(isunique), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	/* bind the "is clustered" column */
	result = WD_BindCol(indx_stmt, 4, internal_asis_type,
						   isclustered, sizeof(isclustered), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;

	}

	/* bind the "is hash" column */
	result = WD_BindCol(indx_stmt, 5, internal_asis_type,
						   ishash, sizeof(ishash), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;

	}

	result = WD_BindCol(indx_stmt, 6, internal_asis_type,
					relhasrules, sizeof(relhasrules), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	result = WD_BindCol(indx_stmt, 8, SQL_C_ULONG,
					&ioid, sizeof(ioid), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	result = WD_BindCol(indx_stmt, 9, SQL_C_ULONG,
					&relhasoids, sizeof(relhasoids), NULL);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;
	}

	/* bind the vector column */
	result = WD_BindCol(indx_stmt, 10, SQL_C_DEFAULT,
			indopt_vector, sizeof(fields_vector), &fields_vector_len);
	if (!SQL_SUCCEEDED(result))
	{
		goto cleanup;

	}

	relhasrules[0] = '0';
	result = WD_Fetch(indx_stmt);
	/* fake index of OID */
	if (relhasoids && relhasrules[0] != '1' && atoi(ci->show_oid_column) && atoi(ci->fake_oid_index))
	{
		tuple = QR_AddNew(res);

		/* no table qualifier */
		set_tuplefield_string(&tuple[STATS_CATALOG_NAME], CurrCat(conn));
		/* don't set the table owner, else Access tries to use it */
		set_tuplefield_string(&tuple[STATS_SCHEMA_NAME], GET_SCHEMA_NAME(table_schemaname));
		set_tuplefield_string(&tuple[STATS_TABLE_NAME], table_name);

		/* non-unique index? */
		set_tuplefield_int2(&tuple[STATS_NON_UNIQUE], (Int2) (ci->drivers.unique_index ? FALSE : TRUE));

		/* no index qualifier */
		set_tuplefield_string(&tuple[STATS_INDEX_QUALIFIER], GET_SCHEMA_NAME(table_schemaname));

		SPRINTF_FIXED(buf, "%s_idx_fake_oid", table_name);
		set_tuplefield_string(&tuple[STATS_INDEX_NAME], buf);

		/*
		 * Clustered/HASH index?
		 */
		set_tuplefield_int2(&tuple[STATS_TYPE], (Int2) SQL_INDEX_OTHER);
		set_tuplefield_int2(&tuple[STATS_SEQ_IN_INDEX], (Int2) 1);

		set_tuplefield_string(&tuple[STATS_COLUMN_NAME], OID_NAME);
		set_tuplefield_string(&tuple[STATS_COLLATION], "A");
		set_tuplefield_null(&tuple[STATS_CARDINALITY]);
		set_tuplefield_null(&tuple[STATS_PAGES]);
		set_tuplefield_null(&tuple[STATS_FILTER_CONDITION]);
	}

	while (SQL_SUCCEEDED(result))
	{
		/* If only requesting unique indexs, then just return those. */
		if (fUnique == SQL_INDEX_ALL ||
			(fUnique == SQL_INDEX_UNIQUE && atoi(isunique)))
		{
			int	colcnt, attnum;

			/* add a row in this table for each field in the index */
			colcnt = fields_vector[0];
			for (i = 1; i <= colcnt; i++)
			{
				tuple = QR_AddNew(res);

				/* no table qualifier */
				set_tuplefield_string(&tuple[STATS_CATALOG_NAME], CurrCat(conn));
				/* don't set the table owner, else Access tries to use it */
				set_tuplefield_string(&tuple[STATS_SCHEMA_NAME], GET_SCHEMA_NAME(table_schemaname));
				set_tuplefield_string(&tuple[STATS_TABLE_NAME], table_name);

				/* non-unique index? */
				if (ci->drivers.unique_index)
					set_tuplefield_int2(&tuple[STATS_NON_UNIQUE], (Int2) (atoi(isunique) ? FALSE : TRUE));
				else
					set_tuplefield_int2(&tuple[STATS_NON_UNIQUE], TRUE);

				/* no index qualifier */
				set_tuplefield_string(&tuple[STATS_INDEX_QUALIFIER], GET_SCHEMA_NAME(table_schemaname));
				set_tuplefield_string(&tuple[STATS_INDEX_NAME], index_name);

				/*
				 * Clustered/HASH index?
				 */
				set_tuplefield_int2(&tuple[STATS_TYPE], (Int2)
							   (atoi(isclustered) ? SQL_INDEX_CLUSTERED :
								(!strncmp(ishash, "hash", 4)) ? SQL_INDEX_HASHED : SQL_INDEX_OTHER));
				set_tuplefield_int2(&tuple[STATS_SEQ_IN_INDEX], (Int2) i);

				attnum = fields_vector[i];
				if (OID_ATTNUM == attnum)
				{
					set_tuplefield_string(&tuple[STATS_COLUMN_NAME], OID_NAME);
					MYLOG(0, "column name = oid\n");
				}
				else if (0 == attnum)
				{
					char	cmd[64];

					QResultClass *res;

					SPRINTF_FIXED(cmd, "select WD_get_indexdef(%u, %d, true)", ioid, i);
					res = CC_send_query(conn, cmd, NULL, READ_ONLY_QUERY, stmt);
			//		if (QR_command_maybe_successful(res))
				//		set_tuplefield_string(&tuple[STATS_COLUMN_NAME], QR_get_value_backend_text(res, 0, 0));
					QR_Destructor(res);
				}
				else
				{
					int j, matchidx;
					BOOL	unknownf = TRUE;

					if (attnum > 0)
					{
						for (j = 0; j < total_columns; j++)
						{
							if (attnum == column_names[j].pnum)
							{
								matchidx = j;
								unknownf = FALSE;
								break;
							}
						}
					}
					if (unknownf)
					{
						set_tuplefield_string(&tuple[STATS_COLUMN_NAME], "UNKNOWN");
						MYLOG(0, "column name = UNKNOWN\n");
					}
					else
					{
						set_tuplefield_string(&tuple[STATS_COLUMN_NAME], column_names[matchidx].col_name);
						MYLOG(0, "column name = '%s'\n", column_names[matchidx].col_name);
					}
				}

				if (i <= indopt_vector[0] &&
				    (indopt_vector[i] & INDOPTION_DESC) != 0)
					set_tuplefield_string(&tuple[STATS_COLLATION], "D");
				else
					set_tuplefield_string(&tuple[STATS_COLLATION], "A");
				set_tuplefield_null(&tuple[STATS_CARDINALITY]);
				set_tuplefield_null(&tuple[STATS_PAGES]);
				set_tuplefield_null(&tuple[STATS_FILTER_CONDITION]);
			}
		}

		result = WD_Fetch(indx_stmt);
	}
	if (result != SQL_NO_DATA_FOUND)
	{
		/* "SQLFetch failed in SQLStatistics."; */
		SC_full_error_copy(stmt, indx_stmt, FALSE);
		goto cleanup;
	}
	ret = SQL_SUCCESS;

cleanup:
#undef	return
	/*
	 * also, things need to think that this statement is finished so the
	 * results can be retrieved.
	 */
	stmt->status = STMT_FINISHED;

	if (!SQL_SUCCEEDED(ret) && 0 >= SC_get_errornumber(stmt))
	{
		SC_error_copy(stmt, col_stmt, TRUE);
		if (0 >= SC_get_errornumber(stmt))
			SC_error_copy(stmt, indx_stmt, TRUE);
	}

	if (col_stmt)
		WD_FreeStmt(col_stmt, SQL_DROP);
	if (indx_stmt)
		WD_FreeStmt(indx_stmt, SQL_DROP);
	/* These things should be freed on any error ALSO! */
	if (!PQExpBufferDataBroken(index_query))
		termPQExpBuffer(&index_query);
	if (table_name)
		free(table_name);
	if (escTableName)
		free(escTableName);
	if (escSchemaName)
		free(escSchemaName);
	if (column_names)
	{
		for (i = 0; i < total_columns; i++)
			free(column_names[i].col_name);
		free(column_names);
	}

	/* set up the current tuple pointer for SQLFetch */
	stmt->currTuple = -1;
	SC_set_rowset_start(stmt, -1, FALSE);
	SC_set_current_col(stmt, -1);

	MYLOG(0, "leaving stmt=%p, ret=%d\n", stmt, ret);

	return ret;
	#endif
}


RETCODE		SQL_API
WD_ColumnPrivileges(HSTMT hstmt,
					   const SQLCHAR * szTableQualifier, /* OA X*/
					   SQLSMALLINT cbTableQualifier,
					   const SQLCHAR * szTableOwner, /* OA E*/
					   SQLSMALLINT cbTableOwner,
					   const SQLCHAR * szTableName, /* OA(R) E*/
					   SQLSMALLINT cbTableName,
					   const SQLCHAR * szColumnName, /* PV E*/
					   SQLSMALLINT cbColumnName,
					   UWORD flag)
{
	return SQL_SUCCESS;
	#if 0
	CSTR func = "WD_ColumnPrivileges";
	StatementClass	*stmt = (StatementClass *) hstmt;
	ConnectionClass	*conn = SC_get_conn(stmt);
	RETCODE	ret = SQL_ERROR;
	char	*escSchemaName = NULL, *escTableName = NULL, *escColumnName = NULL;
	const char	*like_or_eq, *op_string, *eq_string;
	PQExpBufferData	column_query = {0};
	BOOL	search_pattern;
	QResultClass	*res = NULL;

	MYLOG(0, "entering...\n");

	/* Neither Access or Borland care about this. */

	if (SC_initialize_and_recycle(stmt) != SQL_SUCCESS)
		return SQL_ERROR;
	escSchemaName = simpleCatalogEscape(szTableOwner, cbTableOwner, conn);
	escTableName = simpleCatalogEscape(szTableName, cbTableName, conn);
	search_pattern = (0 == (flag & PODBC_NOT_SEARCH_PATTERN));
	if (search_pattern)
	{
		like_or_eq = likeop;
		escColumnName = adjustLikePattern(szColumnName, cbColumnName, conn);
	}
	else
	{
		like_or_eq = eqop;
		escColumnName = simpleCatalogEscape(szColumnName, cbColumnName, conn);
	}
	
	#if 0
	initPQExpBuffer(&column_query);
#define	return	DONT_CALL_RETURN_FROM_HERE???
	appendPQExpBufferStr(&column_query, "select '' as TABLE_CAT, table_schema as TABLE_SCHEM,"
			" table_name, column_name, grantor, grantee,"
			" privilege_type as PRIVILEGE, is_grantable from"
			" information_schema.column_privileges where true");
	/* cq_len = strlen(column_query);
	cq_size = sizeof(column_query);
	col_query = column_query; */
	op_string = gen_opestr(like_or_eq, conn);
	eq_string = gen_opestr(eqop, conn);
	if (escSchemaName)
		appendPQExpBuffer(&column_query, " and table_schem %s'%s'", eq_string, escSchemaName);
	if (escTableName)
		appendPQExpBuffer(&column_query, " and table_name %s'%s'", eq_string, escTableName);
	if (escColumnName)
		appendPQExpBuffer(&column_query, " and column_name %s'%s'", op_string, escColumnName);
	if (PQExpBufferDataBroken(column_query))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in WD_ColumnPriviles()", func);
		goto cleanup;
	}
	if (res = CC_send_query(conn, column_query.data, NULL, READ_ONLY_QUERY, stmt), !QR_command_maybe_successful(res))
	{
		SC_set_error(stmt, STMT_EXEC_ERROR, "WD_ColumnPrivileges query error", func);
		goto cleanup;
	}
	#endif
	SC_set_Result(stmt, res);

	/*
	 * also, things need to think that this statement is finished so the
	 * results can be retrieved.
	 */
	extend_column_bindings(SC_get_ARDF(stmt), 8);
	/* set up the current tuple pointer for SQLFetch */
	ret = SQL_SUCCESS;
//cleanup:
#undef return
	if (!SQL_SUCCEEDED(ret))
		QR_Destructor(res);
	/* set up the current tuple pointer for SQLFetch */
	stmt->status = STMT_FINISHED;
	stmt->currTuple = -1;
	SC_set_rowset_start(stmt, -1, FALSE);
//	if (!PQExpBufferDataBroken(column_query))
//		termPQExpBuffer(&column_query);
	if (escSchemaName)
		free(escSchemaName);
	if (escTableName)
		free(escTableName);
	if (escColumnName)
		free(escColumnName);
	return ret;
}


/*
 *	Multibyte support stuff for SQLForeignKeys().
 *	There may be much more effective way in the
 *	future version. The way is very forcible currently.
 */
static BOOL
isMultibyte(const char *str)
{
	for (; *str; str++)
	{
		if ((unsigned char) *str >= 0x80)
			return TRUE;
	}
	return FALSE;
}
static char *
getClientColumnName(ConnectionClass *conn, UInt4 relid, char *serverColumnName, BOOL *nameAlloced)
{
	char		query[1024], saveattnum[16],
			   *ret = serverColumnName;
	const char *eq_string;
	BOOL		continueExec = TRUE,
				bError = FALSE;
	QResultClass *res = NULL;
	UWORD	flag = READ_ONLY_QUERY;

	*nameAlloced = FALSE;
	if (!conn->original_client_encoding || !isMultibyte(serverColumnName))
		return ret;
	if (!conn->server_encoding)
	{
		if (res = CC_send_query(conn, "select getdatabaseencoding()", NULL, flag, NULL), QR_command_maybe_successful(res))
		{
			if (QR_get_num_cached_tuples(res) > 0)
				conn->server_encoding = strdup(static_cast<char*>(QR_get_value_backend_text(res, 0, 0)));
		}
		QR_Destructor(res);
		res = NULL;
	}
	if (!conn->server_encoding)
		return ret;
	SPRINTF_FIXED(query, "SET CLIENT_ENCODING TO '%s'", conn->server_encoding);
	bError = (!QR_command_maybe_successful((res = CC_send_query(conn, query, NULL, flag, NULL))));
	QR_Destructor(res);
	eq_string = gen_opestr(eqop, conn);
	if (!bError && continueExec)
	{
		SPRINTF_FIXED(query, "select attnum from WD_attribute "
			"where attrelid = %u and attname %s'%s'",
			relid, eq_string, serverColumnName);
		if (res = CC_send_query(conn, query, NULL, flag, NULL), QR_command_maybe_successful(res))
		{
			if (QR_get_num_cached_tuples(res) > 0)
			{
				STRCPY_FIXED(saveattnum, static_cast<char*>(QR_get_value_backend_text(res, 0, 0)));
			}
			else
				continueExec = FALSE;
		}
		else
			bError = TRUE;
		QR_Destructor(res);
	}
	continueExec = (continueExec && !bError);
	/* restore the cleint encoding */
	SPRINTF_FIXED(query, "SET CLIENT_ENCODING TO '%s'", conn->original_client_encoding);
	bError = (!QR_command_maybe_successful((res = CC_send_query(conn, query, NULL, flag, NULL))));
	QR_Destructor(res);
	if (bError || !continueExec)
		return ret;
	SPRINTF_FIXED(query, "select attname from WD_attribute where attrelid = %u and attnum = %s", relid, saveattnum);
	if (res = CC_send_query(conn, query, NULL, flag, NULL), QR_command_maybe_successful(res))
	{
		if (QR_get_num_cached_tuples(res) > 0)
		{
			char *tmp;

			tmp = strdup(static_cast<const char*>(QR_get_value_backend_text(res, 0, 0)));
			if (tmp)
			{
				ret = tmp;
				*nameAlloced = TRUE;
			}
		}
	}
	QR_Destructor(res);
	return ret;
	#endif
}
