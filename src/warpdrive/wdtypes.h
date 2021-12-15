/* File:			wdtypes.h
 *
 * Description:		See "wdtypes.c"
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *
 */

#ifndef __WDTYPES_H__
#define __WDTYPES_H__

#include "wdodbc.h"

/* the type numbers are defined by the OID's of the types' rows */
/* in table WD_type */


#ifdef NOT_USED
#define WD_TYPE_LO				????	/* waiting for permanent type */
#endif

#define	MS_ACCESS_SERIAL		"int identity"
#define WD_TYPE_BOOL			16
#define WD_TYPE_BYTEA			17
#define WD_TYPE_CHAR			18
#define WD_TYPE_NAME			19
#define WD_TYPE_INT8			20
#define WD_TYPE_INT2			21
#define WD_TYPE_INT2VECTOR		22
#define WD_TYPE_INT4			23
#define WD_TYPE_REGPROC			24
#define WD_TYPE_TEXT			25
#define WD_TYPE_OID				26
#define WD_TYPE_TID				27
#define WD_TYPE_XID				28
#define WD_TYPE_CID				29
#define WD_TYPE_OIDVECTOR		30
#define WD_TYPE_XML			142
#define WD_TYPE_XMLARRAY		143
#define WD_TYPE_CIDR			650
#define WD_TYPE_FLOAT4			700
#define WD_TYPE_FLOAT8			701
#define WD_TYPE_ABSTIME			702
#define WD_TYPE_UNKNOWN			705
#define WD_TYPE_MONEY			790
#define WD_TYPE_MACADDR			829
#define WD_TYPE_INET			869
#define WD_TYPE_TEXTARRAY		1009
#define WD_TYPE_BPCHARARRAY		1014
#define WD_TYPE_VARCHARARRAY		1015
#define WD_TYPE_BPCHAR			1042
#define WD_TYPE_VARCHAR			1043
#define WD_TYPE_DATE			1082
#define WD_TYPE_TIME			1083
#define WD_TYPE_TIMESTAMP_NO_TMZONE	1114		/* since 7.2 */
#define WD_TYPE_DATETIME		1184	/* timestamptz */
#define WD_TYPE_INTERVAL		1186
#define WD_TYPE_TIME_WITH_TMZONE	1266		/* since 7.1 */
#define WD_TYPE_TIMESTAMP		1296	/* deprecated since 7.0 */
#define WD_TYPE_BIT			1560
#define WD_TYPE_NUMERIC			1700
#define WD_TYPE_REFCURSOR		1790
#define WD_TYPE_RECORD			2249
#define WD_TYPE_ANY			2276
#define WD_TYPE_VOID			2278
#define WD_TYPE_UUID			2950
#define INTERNAL_ASIS_TYPE		(-9999)

#define TYPE_MAY_BE_ARRAY(type) ((type) == WD_TYPE_XMLARRAY || ((type) >= 1000 && (type) <= 1041))
/* extern Int4 wdtypes_defined[]; */
extern SQLSMALLINT sqlTypes[];

/*	Defines for wdtype_precision */
#define WD_ATP_UNSET				(-3)	/* atttypmod */
#define WD_ADT_UNSET				(-3)	/* adtsize_or_longestlen */
#define WD_UNKNOWNS_UNSET			0 /* UNKNOWNS_AS_MAX */
#define WD_WIDTH_OF_BOOLS_AS_CHAR		5

/*
 *	SQL_INTERVAL support is disabled because I found
 *	some applications which are unhappy with it.
 *
#define	WD_INTERVAL_AS_SQL_INTERVAL
 */

OID		WD_true_type(const ConnectionClass *, OID, OID);
OID		sqltype_to_wdtype(const ConnectionClass *conn, SQLSMALLINT fSqlType);
OID		sqltype_to_bind_wdtype(const ConnectionClass *conn, SQLSMALLINT fSqlType);
const char	*sqltype_to_pgcast(const ConnectionClass *conn, SQLSMALLINT fSqlType);

SQLSMALLINT	wdtype_to_concise_type(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as);
SQLSMALLINT	wdtype_to_sqldesctype(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as);
SQLSMALLINT	wdtype_to_datetime_sub(const StatementClass *stmt, OID type, int col);
const char	*wdtype_to_name(const StatementClass *stmt, OID type, int col, BOOL auto_increment);

SQLSMALLINT	wdtype_attr_to_concise_type(const ConnectionClass *conn, OID type, int typmod, int adtsize_or_longestlen,int handle_unknown_size_as);
SQLSMALLINT	wdtype_attr_to_sqldesctype(const ConnectionClass *conn, OID type, int typmod, int adtsize_or_longestlen, int handle_unknown_size_as);
SQLSMALLINT	wdtype_attr_to_datetime_sub(const ConnectionClass *conn, OID type, int typmod);
SQLSMALLINT	wdtype_attr_to_ctype(const ConnectionClass *conn, OID type, int typmod);
const char	*wdtype_attr_to_name(const ConnectionClass *conn, OID type, int typmod, BOOL auto_increment);
Int4		wdtype_attr_column_size(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longest, int handle_unknown_size_as);
Int4		wdtype_attr_buffer_length(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as);
Int4		wdtype_attr_display_size(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as);
Int2		wdtype_attr_decimal_digits(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as);
Int4		wdtype_attr_transfer_octet_length(const ConnectionClass *conn, OID type, int atttypmod, int handle_unknown_size_as);
SQLSMALLINT	wdtype_attr_precision(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longest, int handle_unknown_size_as);
Int4		wdtype_attr_desclength(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as);
Int2		wdtype_attr_scale(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as);

/*	These functions can use static numbers or result sets(col parameter) */
Int4		wdtype_column_size(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as); /* corresponds to "precision" in ODBC 2.x */
SQLSMALLINT	wdtype_precision(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as); /* "precsion in ODBC 3.x */
/* the following size/length are of Int4 due to PG restriction */
Int4		wdtype_display_size(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as);
Int4		wdtype_buffer_length(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as);
Int4		wdtype_desclength(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as);
// Int4		wdtype_transfer_octet_length(const ConnectionClass *conn, OID type, int column_size);

SQLSMALLINT	wdtype_decimal_digits(const StatementClass *stmt, OID type, int col); /* corresponds to "scale" in ODBC 2.x */
SQLSMALLINT	wdtype_min_decimal_digits(const ConnectionClass *conn, OID type); /* corresponds to "min_scale" in ODBC 2.x */
SQLSMALLINT	wdtype_max_decimal_digits(const ConnectionClass *conn, OID type); /* corresponds to "max_scale" in ODBC 2.x */
SQLSMALLINT	wdtype_scale(const StatementClass *stmt, OID type, int col); /* ODBC 3.x " */
Int2		wdtype_radix(const ConnectionClass *conn, OID type);
Int2		wdtype_nullable(const ConnectionClass *conn, OID type);
Int2		wdtype_auto_increment(const ConnectionClass *conn, OID type);
Int2		wdtype_case_sensitive(const ConnectionClass *conn, OID type);
Int2		wdtype_money(const ConnectionClass *conn, OID type);
Int2		wdtype_searchable(const ConnectionClass *conn, OID type);
Int2		wdtype_unsigned(const ConnectionClass *conn, OID type);
const char	*wdtype_literal_prefix(const ConnectionClass *conn, OID type);
const char	*wdtype_literal_suffix(const ConnectionClass *conn, OID type);
const char	*wdtype_create_params(const ConnectionClass *conn, OID type);

SQLSMALLINT	sqltype_to_default_ctype(const ConnectionClass *stmt, SQLSMALLINT sqltype);
Int4		ctype_length(SQLSMALLINT ctype);

SQLSMALLINT	ansi_to_wtype(const ConnectionClass *self, SQLSMALLINT ansitype);

#define	USE_ZONE	FALSE
#endif
