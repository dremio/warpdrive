/*--------
 * Module:			wdtypes.cc
 *
 * Description:		This module contains routines for getting information
 *					about the supported Postgres data types.  Only the
 *					function wdtype_to_sqltype() returns an unknown condition.
 *					All other functions return a suitable default so that
 *					even data types that are not directly supported can be
 *					used (it is handled as char data).
 *
 * Classes:			n/a
 *
 * API functions:	none
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */

#include "wdtypes.h"

#include "dlg_specific.h"
#include "statement.h"
#include "connection.h"
#include "environ.h"
#include "qresult.h"

#define	EXPERIMENTAL_CURRENTLY


SQLSMALLINT ansi_to_wtype(const ConnectionClass *self, SQLSMALLINT ansitype)
{
#ifndef	UNICODE_SUPPORT
	return ansitype;
#else
	if (!ALLOW_WCHAR(self))
		return ansitype;
	switch (ansitype)
	{
		case SQL_CHAR:
			return SQL_WCHAR;
		case SQL_VARCHAR:
			return SQL_WVARCHAR;
		case SQL_LONGVARCHAR:
			return SQL_WLONGVARCHAR;
	}
	return ansitype;
#endif /* UNICODE_SUPPORT */
}

Int4		getCharColumnSize(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as);

/*
 * these are the types we support.	all of the wdtype_ functions should
 * return values for each one of these.
 * Even types not directly supported are handled as character types
 * so all types should work (points, etc.)
 */

/*
 * ALL THESE TYPES ARE NO LONGER REPORTED in SQLGetTypeInfo.  Instead, all
 *	the SQL TYPES are reported and mapped to a corresponding Postgres Type
 */

/*
OID wdtypes_defined[][2] = {
			{WD_TYPE_CHAR, 0}
			,{WD_TYPE_CHAR2, 0}
			,{WD_TYPE_CHAR4, 0}
			,{WD_TYPE_CHAR8, 0}
			,{WD_TYPE_CHAR16, 0}
			,{WD_TYPE_NAME, 0}
			,{WD_TYPE_VARCHAR, 0}
			,{WD_TYPE_BPCHAR, 0}
			,{WD_TYPE_DATE, 0}
			,{WD_TYPE_TIME, 0}
			,{WD_TYPE_TIME_WITH_TMZONE, 0}
			,{WD_TYPE_DATETIME, 0}
			,{WD_TYPE_ABSTIME, 0}
			,{WD_TYPE_TIMESTAMP_NO_TMZONE, 0}
			,{WD_TYPE_TIMESTAMP, 0}
			,{WD_TYPE_TEXT, 0}
			,{WD_TYPE_INT2, 0}
			,{WD_TYPE_INT4, 0}
			,{WD_TYPE_FLOAT4, 0}
			,{WD_TYPE_FLOAT8, 0}
			,{WD_TYPE_OID, 0}
			,{WD_TYPE_MONEY, 0}
			,{WD_TYPE_BOOL, 0}
			,{WD_TYPE_BYTEA, 0}
			,{WD_TYPE_NUMERIC, 0}
			,{WD_TYPE_XID, 0}
			,{WD_TYPE_LO_UNDEFINED, 0}
			,{0, 0} };
*/


/*	These are NOW the SQL Types reported in SQLGetTypeInfo.  */
SQLSMALLINT	sqlTypes[] = {
	SQL_BIGINT,
	/* SQL_BINARY, -- Commented out because VarBinary is more correct. */
	SQL_BIT,
	SQL_CHAR,
	SQL_TYPE_DATE,
	SQL_DATE,
	SQL_DECIMAL,
	SQL_DOUBLE,
	SQL_FLOAT,
	SQL_INTEGER,
	SQL_LONGVARBINARY,
	SQL_LONGVARCHAR,
	SQL_NUMERIC,
	SQL_REAL,
	SQL_SMALLINT,
	SQL_TYPE_TIME,
	SQL_TYPE_TIMESTAMP,
	SQL_TIME,
	SQL_TIMESTAMP,
	SQL_TINYINT,
	SQL_VARBINARY,
	SQL_VARCHAR,
#ifdef	UNICODE_SUPPORT
	SQL_WCHAR,
	SQL_WVARCHAR,
	SQL_WLONGVARCHAR,
#endif /* UNICODE_SUPPORT */
	SQL_GUID,
/* AFAIK SQL_INTERVAL types cause troubles in some spplications */
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
	SQL_INTERVAL_MONTH,
	SQL_INTERVAL_YEAR,
	SQL_INTERVAL_YEAR_TO_MONTH,
	SQL_INTERVAL_DAY,
	SQL_INTERVAL_HOUR,
	SQL_INTERVAL_MINUTE,
	SQL_INTERVAL_SECOND,
	SQL_INTERVAL_DAY_TO_HOUR,
	SQL_INTERVAL_DAY_TO_MINUTE,
	SQL_INTERVAL_DAY_TO_SECOND,
	SQL_INTERVAL_HOUR_TO_MINUTE,
	SQL_INTERVAL_HOUR_TO_SECOND,
	SQL_INTERVAL_MINUTE_TO_SECOND,
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */
	0
};

#ifdef ODBCINT64
#define	ALLOWED_C_BIGINT	SQL_C_SBIGINT
/* #define	ALLOWED_C_BIGINT	SQL_C_CHAR */ /* Delphi should be either ? */
#else
#define	ALLOWED_C_BIGINT	SQL_C_CHAR
#endif

OID
WD_true_type(const ConnectionClass *conn, OID type, OID basetype)
{
	if (0 == basetype)
		return type;
	else if (0 == type)
		return basetype;
	else if (type == conn->lobj_type)
		return type;
	return basetype;
}

#define MONTH_BIT 	(1 << 17)
#define YEAR_BIT	(1 << 18)
#define DAY_BIT		(1 << 19)
#define HOUR_BIT	(1 << 26)
#define MINUTE_BIT	(1 << 27)
#define SECOND_BIT	(1 << 28)

static SQLSMALLINT
get_interval_type(Int4 atttypmod, const char **name)
{
MYLOG(0, "entering atttypmod=%x\n", atttypmod);
	if ((-1) == atttypmod)
		return 0;
	if (0 != (YEAR_BIT & atttypmod))
	{
		if (0 != (MONTH_BIT & atttypmod))
		{
			if (name)
				*name = "interval year to month";
			return SQL_INTERVAL_YEAR_TO_MONTH;
		}
		if (name)
			*name = "interval year";
		return SQL_INTERVAL_YEAR;
	}
	else if (0 != (MONTH_BIT & atttypmod))
	{
		if (name)
			*name = "interval month";
		return SQL_INTERVAL_MONTH;
	}
	else if (0 != (DAY_BIT & atttypmod))
	{
		if (0 != (SECOND_BIT & atttypmod))
		{
			if (name)
				*name = "interval day to second";
			return SQL_INTERVAL_DAY_TO_SECOND;
		}
		else if (0 != (MINUTE_BIT & atttypmod))
		{
			if (name)
				*name = "interval day to minute";
			return SQL_INTERVAL_DAY_TO_MINUTE;
		}
		else if (0 != (HOUR_BIT & atttypmod))
		{
			if (name)
				*name = "interval day to hour";
			return SQL_INTERVAL_DAY_TO_HOUR;
		}
		if (name)
			*name = "interval day";
		return SQL_INTERVAL_DAY;
	}
	else if (0 != (HOUR_BIT & atttypmod))
	{
		if (0 != (SECOND_BIT & atttypmod))
		{
			if (name)
				*name = "interval hour to second";
			return SQL_INTERVAL_HOUR_TO_SECOND;
		}
		else if (0 != (MINUTE_BIT & atttypmod))
		{
			if (name)
				*name = "interval hour to minute";
			return SQL_INTERVAL_HOUR_TO_MINUTE;
		}
		if (name)
			*name = "interval hour";
		return SQL_INTERVAL_HOUR;
	}
	else if (0 != (MINUTE_BIT & atttypmod))
	{
		if (0 != (SECOND_BIT & atttypmod))
		{
			if (name)
				*name = "interval minute to second";
			return SQL_INTERVAL_MINUTE_TO_SECOND;
		}
		if (name)
			*name = "interval minute";
		return SQL_INTERVAL_MINUTE;
	}
	else if (0 != (SECOND_BIT & atttypmod))
	{
		if (name)
			*name = "interval second";
		return SQL_INTERVAL_SECOND;
	}

	if (name)
		*name = "interval";
	return 0;
}

static Int4
getCharColumnSizeX(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as)
{
	int		p = -1, maxsize;
	const ConnInfo	*ci = &(conn->connInfo);

	MYLOG(0, "entering type=%d, atttypmod=%d, adtsize_or=%d, unknown = %d\n", type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);

	/* Assign Maximum size based on parameters */
	switch (type)
	{
		case WD_TYPE_TEXT:
			if (ci->drivers.text_as_longvarchar)
				maxsize = ci->drivers.max_longvarchar_size;
			else
				maxsize = ci->drivers.max_varchar_size;
			break;

		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
			maxsize = ci->drivers.max_varchar_size;
			break;

		default:
			if (ci->drivers.unknowns_as_longvarchar)
				maxsize = ci->drivers.max_longvarchar_size;
			else
				maxsize = ci->drivers.max_varchar_size;
			break;
	}
#ifdef	UNICODE_SUPPORT
	if (CC_is_in_unicode_driver(conn) &&
	    isSqlServr() &&
	    maxsize > 4000)
		maxsize = 4000;
#endif /* UNICODE_SUPPORT */

	if (maxsize == TEXT_FIELD_SIZE + 1) /* magic length for testing */
		maxsize = 0;

	/*
	 * Static ColumnSize (i.e., the Maximum ColumnSize of the datatype) This
	 * has nothing to do with a result set.
	 */
MYLOG(DETAIL_LOG_LEVEL, "!!! atttypmod  < 0 ?\n");
	if (atttypmod < 0 && adtsize_or_longestlen < 0)
		return maxsize;

MYLOG(DETAIL_LOG_LEVEL, "!!! adtsize_or_logngest=%d\n", adtsize_or_longestlen);
	p = adtsize_or_longestlen; /* longest */
	/*
	 * Catalog Result Sets -- use assigned column width (i.e., from
	 * set_tuplefield_string)
	 */
MYLOG(DETAIL_LOG_LEVEL, "!!! catalog_result=%d\n", handle_unknown_size_as);
	if (UNKNOWNS_AS_LONGEST == handle_unknown_size_as)
	{
		MYLOG(0, "LONGEST: p = %d\n", p);
		if (p > 0 &&
		    (atttypmod < 0 || atttypmod > p))
			return p;
	}
	if (TYPE_MAY_BE_ARRAY(type))
	{
		if (p > 0)
			return p;
		return maxsize;
	}

	/* Size is unknown -- handle according to parameter */
	if (atttypmod > 0)	/* maybe the length is known */
	{
		return atttypmod;
	}

	/* The type is really unknown */
	switch (handle_unknown_size_as)
	{
		case UNKNOWNS_AS_DONTKNOW:
			return -1;
		case UNKNOWNS_AS_LONGEST:
		case UNKNOWNS_AS_MAX:
			break;
		default:
			return -1;
	}
	if (maxsize <= 0)
		return maxsize;
	switch (type)
	{
		case WD_TYPE_BPCHAR:
		case WD_TYPE_VARCHAR:
		case WD_TYPE_TEXT:
			return maxsize;
	}

	if (p > maxsize)
		maxsize = p;
	return maxsize;
}

/*
 *	Specify when handle_unknown_size_as parameter is unused 
 */
#define	UNUSED_HANDLE_UNKNOWN_SIZE_AS	(-2)

static SQLSMALLINT
getNumericDecimalDigitsX(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longest, int UNUSED_handle_unknown_size_as)
{
	Int4		default_decimal_digits = 6;

	MYLOG(0, "entering type=%d, atttypmod=%d\n", type, atttypmod);

	if (atttypmod < 0 && adtsize_or_longest < 0)
		return default_decimal_digits;

	if (atttypmod > -1)
		return (atttypmod & 0xffff);
	if (adtsize_or_longest <= 0)
		return default_decimal_digits;
	adtsize_or_longest >>= 16; /* extract the scale part */
	return adtsize_or_longest;
}

static Int4	/* PostgreSQL restritiction */
getNumericColumnSizeX(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longest, int handle_unknown_size_as)
{
	Int4	default_column_size = 28;
	const ConnInfo	*ci = &(conn->connInfo);

	MYLOG(0, "entering type=%d, typmod=%d\n", type, atttypmod);

	if (atttypmod > -1)
		return (atttypmod >> 16) & 0xffff;
	switch (ci->numeric_as)
	{
		case SQL_VARCHAR:
			return ci->drivers.max_varchar_size;
		case SQL_LONGVARCHAR:
			return ci->drivers.max_longvarchar_size;
		case SQL_DOUBLE:
			return WD_DOUBLE_DIGITS;
	}
	switch (handle_unknown_size_as)
	{
		case UNKNOWNS_AS_DONTKNOW:
			return SQL_NO_TOTAL;
	}
	if (adtsize_or_longest <= 0)
		return default_column_size;
	adtsize_or_longest %= (1 << 16); /* extract the precision part */
	switch (handle_unknown_size_as)
	{
		case UNKNOWNS_AS_MAX:
			return adtsize_or_longest > default_column_size ? adtsize_or_longest : default_column_size;
		default:
			if (adtsize_or_longest < 10)
				adtsize_or_longest = 10;
	}
	return adtsize_or_longest;
}

static SQLSMALLINT
getTimestampDecimalDigitsX(const ConnectionClass *conn, OID type, int atttypmod)
{
	MYLOG(0, "type=%d, atttypmod=%d\n", type, atttypmod);
	return (atttypmod > -1 ? atttypmod : 6);
}

static SQLSMALLINT
getTimestampColumnSizeX(const ConnectionClass *conn, OID type, int atttypmod)
{
	Int4	fixed, scale;

	MYLOG(0, "entering type=%d, atttypmod=%d\n", type, atttypmod);

	switch (type)
	{
		case WD_TYPE_TIME:
			fixed = 8;
			break;
		case WD_TYPE_TIME_WITH_TMZONE:
			fixed = 11;
			break;
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			fixed = 19;
			break;
		default:
			if (USE_ZONE)
				fixed = 22;
			else
				fixed = 19;
			break;
	}
	scale = getTimestampDecimalDigitsX(conn, type, atttypmod);
	return (scale > 0) ? fixed + 1 + scale : fixed;
}

static SQLSMALLINT
getIntervalDecimalDigits(OID type, int atttypmod)
{
	Int4	prec;

	MYLOG(0, "entering type=%d, atttypmod=%d\n", type, atttypmod);

	if ((atttypmod & SECOND_BIT) == 0)
		return 0;
	return (prec = atttypmod & 0xffff) == 0xffff ? 6 : prec ;
}

static SQLSMALLINT
getIntervalColumnSize(OID type, int atttypmod)
{
	Int4	ttl, leading_precision = 9, scale;

	MYLOG(0, "entering type=%d, atttypmod=%d\n", type, atttypmod);

	ttl = leading_precision;
	switch (get_interval_type(atttypmod, NULL))
	{
		case 0:
			ttl = 25;
			break;
		case SQL_INTERVAL_YEAR:
			ttl = 16;
			break;
		case SQL_INTERVAL_MONTH:
			ttl = 16;
			break;
		case SQL_INTERVAL_DAY:
			ttl = 16;
			break;
		case SQL_INTERVAL_DAY_TO_SECOND:
		case SQL_INTERVAL_DAY_TO_MINUTE:
		case SQL_INTERVAL_DAY_TO_HOUR:
			ttl = 25;
			break;
		case SQL_INTERVAL_HOUR_TO_SECOND:
		case SQL_INTERVAL_HOUR_TO_MINUTE:
		case SQL_INTERVAL_HOUR:
			ttl = 17;
			break;
		case SQL_INTERVAL_MINUTE_TO_SECOND:
		case SQL_INTERVAL_MINUTE:
			ttl = 15;
			break;
		case SQL_INTERVAL_YEAR_TO_MONTH:
			ttl = 24;
			break;
	}
	scale = getIntervalDecimalDigits(type, atttypmod);
	return (scale > 0) ? ttl + 1 + scale : ttl;
}


SQLSMALLINT
wdtype_attr_to_concise_type(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as)
{
	const ConnInfo	*ci = &(conn->connInfo);
	EnvironmentClass *env = (EnvironmentClass *) CC_get_env(conn);
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
	SQLSMALLINT	sqltype;
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */
	BOOL	bLongVarchar, bFixed = FALSE;

	switch (type)
	{
		case WD_TYPE_CHAR:
			return ansi_to_wtype(conn, SQL_CHAR);
		case WD_TYPE_NAME:
		case WD_TYPE_REFCURSOR:
			return ansi_to_wtype(conn, SQL_VARCHAR);

		case WD_TYPE_BPCHAR:
			bFixed = TRUE;
		case WD_TYPE_VARCHAR:
			if (getCharColumnSizeX(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as) > ci->drivers.max_varchar_size)
				bLongVarchar = TRUE;
			else
				bLongVarchar = FALSE;
			return ansi_to_wtype(conn, bLongVarchar ? SQL_LONGVARCHAR : (bFixed ? SQL_CHAR : SQL_VARCHAR));
		case WD_TYPE_TEXT:
			bLongVarchar = ci->drivers.text_as_longvarchar;
			if (bLongVarchar)
			{
				int column_size = getCharColumnSizeX(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
				if (column_size > 0 &&
				    column_size <= ci->drivers.max_varchar_size)
					bLongVarchar = FALSE;
			}
			return ansi_to_wtype(conn, bLongVarchar ? SQL_LONGVARCHAR : SQL_VARCHAR);

		case WD_TYPE_BYTEA:
			if (ci->bytea_as_longvarbinary)
				return SQL_LONGVARBINARY;
			else
				return SQL_VARBINARY;
		case WD_TYPE_LO_UNDEFINED:
			return SQL_LONGVARBINARY;

		case WD_TYPE_INT2:
			return SQL_SMALLINT;

		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
			return SQL_INTEGER;

			/* Change this to SQL_BIGINT for ODBC v3 bjm 2001-01-23 */
		case WD_TYPE_INT8:
			if (ci->int8_as != 0)
				return ci->int8_as;
			if (conn->ms_jet)
				return SQL_NUMERIC; /* maybe a little better than SQL_VARCHAR */
			return SQL_BIGINT;

		case WD_TYPE_NUMERIC:
			if (-1 == atttypmod && DEFAULT_NUMERIC_AS != ci->numeric_as)
				return ci->numeric_as;
			return SQL_NUMERIC;

		case WD_TYPE_FLOAT4:
			return SQL_REAL;
		case WD_TYPE_FLOAT8:
			return SQL_FLOAT;
		case WD_TYPE_DATE:
			if (EN_is_odbc3(env))
				return SQL_TYPE_DATE;
			return SQL_DATE;
		case WD_TYPE_TIME:
			if (EN_is_odbc3(env))
				return SQL_TYPE_TIME;
			return SQL_TIME;
		case WD_TYPE_ABSTIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
		case WD_TYPE_TIMESTAMP:
			if (EN_is_odbc3(env))
				return SQL_TYPE_TIMESTAMP;
			return SQL_TIMESTAMP;
		case WD_TYPE_MONEY:
			return SQL_FLOAT;
		case WD_TYPE_BOOL:
			return ci->drivers.bools_as_char ? SQL_VARCHAR : SQL_BIT;
		case WD_TYPE_XML:
			return ansi_to_wtype(conn, SQL_LONGVARCHAR);
		case WD_TYPE_INET:
		case WD_TYPE_CIDR:
		case WD_TYPE_MACADDR:
			return ansi_to_wtype(conn, SQL_VARCHAR);
		case WD_TYPE_UUID:
			return SQL_GUID;

		case WD_TYPE_INTERVAL:
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
			if (sqltype = get_interval_type(atttypmod, NULL), 0 != sqltype)
				return sqltype;
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */
			return ansi_to_wtype(conn, SQL_VARCHAR);
		case WD_TYPE_BIT:
			if (1 == atttypmod)
				return SQL_BIT;
			else
				return SQL_VARCHAR;

		default:

			/*
			 * first, check to see if 'type' is in list.  If not, look up
			 * with query. Add oid, name to list.  If it's already in
			 * list, just return.
			 */
			/* hack until permanent type is available */
			if (type == conn->lobj_type)
				return SQL_LONGVARBINARY;

			bLongVarchar = ci->drivers.unknowns_as_longvarchar;
			if (bLongVarchar)
			{
				int column_size = getCharColumnSizeX(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
				if (column_size > 0 &&
				    column_size <= ci->drivers.max_varchar_size)
					bLongVarchar = FALSE;
			}
#ifdef	EXPERIMENTAL_CURRENTLY
			return ansi_to_wtype(conn, bLongVarchar ? SQL_LONGVARCHAR : SQL_VARCHAR);
#endif	/* EXPERIMENTAL_CURRENTLY */
			return bLongVarchar ? SQL_LONGVARCHAR : SQL_VARCHAR;
	}
}

SQLSMALLINT
wdtype_attr_to_sqldesctype(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as)
{
	SQLSMALLINT	rettype;

#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
	if (WD_TYPE_INTERVAL == type)
		return SQL_INTERVAL;
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */
	switch (rettype = wdtype_attr_to_concise_type(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as))
	{
		case SQL_TYPE_DATE:
		case SQL_TYPE_TIME:
		case SQL_TYPE_TIMESTAMP:
			return SQL_DATETIME;
	}
	return rettype;
}

SQLSMALLINT
wdtype_attr_to_datetime_sub(const ConnectionClass *conn, OID type, int atttypmod)
{
	SQLSMALLINT	rettype;

	switch (rettype = wdtype_attr_to_concise_type(conn, type, atttypmod, WD_ADT_UNSET, WD_UNKNOWNS_UNSET))
	{
		case SQL_TYPE_DATE:
			return SQL_CODE_DATE;
		case SQL_TYPE_TIME:
			return SQL_CODE_TIME;
		case SQL_TYPE_TIMESTAMP:
			return SQL_CODE_TIMESTAMP;
		case SQL_INTERVAL_MONTH:
		case SQL_INTERVAL_YEAR:
		case SQL_INTERVAL_YEAR_TO_MONTH:
		case SQL_INTERVAL_DAY:
		case SQL_INTERVAL_HOUR:
		case SQL_INTERVAL_MINUTE:
		case SQL_INTERVAL_SECOND:
		case SQL_INTERVAL_DAY_TO_HOUR:
		case SQL_INTERVAL_DAY_TO_MINUTE:
		case SQL_INTERVAL_DAY_TO_SECOND:
		case SQL_INTERVAL_HOUR_TO_MINUTE:
		case SQL_INTERVAL_HOUR_TO_SECOND:
		case SQL_INTERVAL_MINUTE_TO_SECOND:
			return rettype - 100;
	}
	return -1;
}

SQLSMALLINT
wdtype_attr_to_ctype(const ConnectionClass *conn, OID type, int atttypmod)
{
	const ConnInfo	*ci = &(conn->connInfo);
	EnvironmentClass *env = (EnvironmentClass *) CC_get_env(conn);
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
	SQLSMALLINT	ctype;
#endif /* WD_INTERVAL_A_SQL_INTERVAL */

	switch (type)
	{
		case WD_TYPE_INT8:
			if (!conn->ms_jet)
				return ALLOWED_C_BIGINT;
			return SQL_C_CHAR;
		case WD_TYPE_NUMERIC:
			return SQL_C_CHAR;
		case WD_TYPE_INT2:
			return SQL_C_SSHORT;
		case WD_TYPE_OID:
		case WD_TYPE_XID:
			return SQL_C_ULONG;
		case WD_TYPE_INT4:
			return SQL_C_SLONG;
		case WD_TYPE_FLOAT4:
			return SQL_C_FLOAT;
		case WD_TYPE_FLOAT8:
			return SQL_C_DOUBLE;
		case WD_TYPE_DATE:
			if (EN_is_odbc3(env))
				return SQL_C_TYPE_DATE;
			return SQL_C_DATE;
		case WD_TYPE_TIME:
			if (EN_is_odbc3(env))
				return SQL_C_TYPE_TIME;
			return SQL_C_TIME;
		case WD_TYPE_ABSTIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
		case WD_TYPE_TIMESTAMP:
			if (EN_is_odbc3(env))
				return SQL_C_TYPE_TIMESTAMP;
			return SQL_C_TIMESTAMP;
		case WD_TYPE_MONEY:
			return SQL_C_FLOAT;
		case WD_TYPE_BOOL:
			return ci->drivers.bools_as_char ? SQL_C_CHAR : SQL_C_BIT;

		case WD_TYPE_BYTEA:
			return SQL_C_BINARY;
		case WD_TYPE_LO_UNDEFINED:
			return SQL_C_BINARY;
		case WD_TYPE_BPCHAR:
		case WD_TYPE_VARCHAR:
		case WD_TYPE_TEXT:
			return ansi_to_wtype(conn, SQL_C_CHAR);
		case WD_TYPE_UUID:
			if (!conn->ms_jet)
				return SQL_C_GUID;
			return ansi_to_wtype(conn, SQL_C_CHAR);

		case WD_TYPE_INTERVAL:
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
			if (ctype = get_interval_type(atttypmod, NULL), 0 != ctype)
				return ctype;
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */
			return ansi_to_wtype(conn, SQL_CHAR);

		default:
			/* hack until permanent type is available */
			if (type == conn->lobj_type)
				return SQL_C_BINARY;

			/* Experimental, Does this work ? */
#ifdef	EXPERIMENTAL_CURRENTLY
			return ansi_to_wtype(conn, SQL_C_CHAR);
#endif	/* EXPERIMENTAL_CURRENTLY */
			return SQL_C_CHAR;
	}
}

const char *
wdtype_attr_to_name(const ConnectionClass *conn, OID type, int atttypmod, BOOL auto_increment)
{
	const char	*tname = NULL;

	switch (type)
	{
		case WD_TYPE_CHAR:
			return "char";
		case WD_TYPE_INT8:
			return auto_increment ? "bigserial" : "int8";
		case WD_TYPE_NUMERIC:
			return "numeric";
		case WD_TYPE_VARCHAR:
			return "varchar";
		case WD_TYPE_BPCHAR:
			return "char";
		case WD_TYPE_TEXT:
			return "text";
		case WD_TYPE_NAME:
			return "name";
		case WD_TYPE_REFCURSOR:
			return "refcursor";
		case WD_TYPE_INT2:
			return "int2";
		case WD_TYPE_OID:
			return OID_NAME;
		case WD_TYPE_XID:
			return "xid";
		case WD_TYPE_INT4:
MYLOG(DETAIL_LOG_LEVEL, "wdtype_to_name int4\n");
			return auto_increment ? "serial" : "int4";
		case WD_TYPE_FLOAT4:
			return "float4";
		case WD_TYPE_FLOAT8:
			return "float8";
		case WD_TYPE_DATE:
			return "date";
		case WD_TYPE_TIME:
			return "time";
		case WD_TYPE_ABSTIME:
			return "abstime";
		case WD_TYPE_DATETIME:
			return "timestamptz";
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			return "timestamp without time zone";
		case WD_TYPE_TIMESTAMP:
			return "timestamp";
		case WD_TYPE_MONEY:
			return "money";
		case WD_TYPE_BOOL:
			return "bool";
		case WD_TYPE_BYTEA:
			return "bytea";
		case WD_TYPE_XML:
			return "xml";
		case WD_TYPE_MACADDR:
			return "macaddr";
		case WD_TYPE_INET:
			return "inet";
		case WD_TYPE_CIDR:
			return "cidr";
		case WD_TYPE_UUID:
			return "uuid";
		case WD_TYPE_VOID:
			return "void";
		case WD_TYPE_INT2VECTOR:
			return "int2vector";
		case WD_TYPE_OIDVECTOR:
			return "oidvector";
		case WD_TYPE_ANY:
			return "any";
		case WD_TYPE_INTERVAL:
			get_interval_type(atttypmod, &tname);
			return tname;
		case WD_TYPE_LO_UNDEFINED:
			return WD_TYPE_LO_NAME;

		default:
			/* hack until permanent type is available */
			if (type == conn->lobj_type)
				return WD_TYPE_LO_NAME;

			/*
			 * "unknown" can actually be used in alter table because it is
			 * a real PG type!
			 */
			return "unknown";
	}
}


Int4	/* PostgreSQL restriction */
wdtype_attr_column_size(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longest, int handle_unknown_size_as)
{
	const ConnInfo	*ci = &(conn->connInfo);
MYLOG(0, "entering type=%d, atttypmod=%d, adtsize_or=%d, unknown = %d\n", type, atttypmod, adtsize_or_longest, handle_unknown_size_as);

	switch (type)
	{
		case WD_TYPE_CHAR:
			return 1;

		case WD_TYPE_NAME:
		case WD_TYPE_REFCURSOR:
			{
				int	value = 0;
				if (WD_VERSION_GT(conn, 7.4))
				{
					/*
					 * 'conn' argument is marked as const,
					 * because this function just reads
					 * stuff from the already-filled in
					 * fields in the struct. But this is
					 * an exception: CC_get_max_idlen() can
					 * send a SHOW query to the backend to
					 * get the identifier length. Thus cast
					 * away the const here.
					 */
					value = CC_get_max_idlen((ConnectionClass *) conn);
				}
#ifdef	NAME_FIELD_SIZE
				else
					value = NAME_FIELD_SIZE;
#endif /* NAME_FIELD_SIZE */
				if (0 == value)
					value = NAMEDATALEN_V73;
				return value;
			}

		case WD_TYPE_INT2:
			return 5;

		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
			return 10;

		case WD_TYPE_INT8:
			return 19;			/* signed */

		case WD_TYPE_NUMERIC:
			return getNumericColumnSizeX(conn, type, atttypmod, adtsize_or_longest, handle_unknown_size_as);

		case WD_TYPE_MONEY:
			return 10;
		case WD_TYPE_FLOAT4:
			return WD_REAL_DIGITS;

		case WD_TYPE_FLOAT8:
			return WD_DOUBLE_DIGITS;

		case WD_TYPE_DATE:
			return 10;
		case WD_TYPE_TIME:
			return 8;

		case WD_TYPE_ABSTIME:
		case WD_TYPE_TIMESTAMP:
			return 22;
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			/* return 22; */
			return getTimestampColumnSizeX(conn, type, atttypmod);

		case WD_TYPE_BOOL:
			return ci->drivers.bools_as_char ? WD_WIDTH_OF_BOOLS_AS_CHAR : 1;

		case WD_TYPE_MACADDR:
			return 17;

		case WD_TYPE_INET:
		case WD_TYPE_CIDR:
			return sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255/128");
		case WD_TYPE_UUID:
			return sizeof("XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX");

		case WD_TYPE_LO_UNDEFINED:
			return SQL_NO_TOTAL;

		case WD_TYPE_INTERVAL:
			return getIntervalColumnSize(type, atttypmod);

		default:

			if (type == conn->lobj_type)	/* hack until permanent
												 * type is available */
				return SQL_NO_TOTAL;
			if (WD_TYPE_BYTEA == type && ci->bytea_as_longvarbinary)
				return SQL_NO_TOTAL;

			/* Handle Character types and unknown types */
			return getCharColumnSizeX(conn, type, atttypmod, adtsize_or_longest, handle_unknown_size_as);
	}
}

SQLSMALLINT
wdtype_attr_precision(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longest, int handle_unknown_size_as)
{
	switch (type)
	{
		case WD_TYPE_NUMERIC:
			return getNumericColumnSizeX(conn, type, atttypmod, adtsize_or_longest, handle_unknown_size_as);
		case WD_TYPE_TIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			return getTimestampDecimalDigitsX(conn, type, atttypmod);
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
		case WD_TYPE_INTERVAL:
			return getIntervalDecimalDigits(type, atttypmod);
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */
	}
	return -1;
}

Int4
wdtype_attr_display_size(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as)
{
	int	dsize;

	switch (type)
	{
		case WD_TYPE_INT2:
			return 6;

		case WD_TYPE_OID:
		case WD_TYPE_XID:
			return 10;

		case WD_TYPE_INT4:
			return 11;

		case WD_TYPE_INT8:
			return 20;			/* signed: 19 digits + sign */

		case WD_TYPE_NUMERIC:
			dsize = getNumericColumnSizeX(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
			return dsize <= 0 ? dsize : dsize + 2;

		case WD_TYPE_MONEY:
			return 15;			/* ($9,999,999.99) */

		case WD_TYPE_FLOAT4:	/* a sign, WD_REAL_DIGITS digits, a decimal point, the letter E, a sign, and 2 digits */
			return (1 + WD_REAL_DIGITS + 1 + 1 + 3);

		case WD_TYPE_FLOAT8:	/* a sign, WD_DOUBLE_DIGITS digits, a decimal point, the letter E, a sign, and 3 digits */
			return (1 + WD_DOUBLE_DIGITS + 1 + 1 + 1 + 3);

		case WD_TYPE_MACADDR:
			return 17;
		case WD_TYPE_INET:
		case WD_TYPE_CIDR:
			return sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255/128");
		case WD_TYPE_UUID:
			return 36;
		case WD_TYPE_INTERVAL:
			return 30;

			/* Character types use regular precision */
		default:
			return wdtype_attr_column_size(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
	}
}

Int4
wdtype_attr_buffer_length(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as)
{
	int	dsize;

	switch (type)
	{
		case WD_TYPE_INT2:
			return 2; /* sizeof(SQLSMALLINT) */

		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
			return 4; /* sizeof(SQLINTEGER) */

		case WD_TYPE_INT8:
			if (SQL_C_CHAR == wdtype_attr_to_ctype(conn, type, atttypmod))
				return 20;			/* signed: 19 digits + sign */
			return 8; /* sizeof(SQLSBININT) */

		case WD_TYPE_NUMERIC:
			dsize = getNumericColumnSizeX(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
			return dsize <= 0 ? dsize : dsize + 2;

		case WD_TYPE_FLOAT4:
		case WD_TYPE_MONEY:
			return 4; /* sizeof(SQLREAL) */

		case WD_TYPE_FLOAT8:
			return 8; /* sizeof(SQLFLOAT) */

		case WD_TYPE_DATE:
		case WD_TYPE_TIME:
			return 6;		/* sizeof(DATE(TIME)_STRUCT) */

		case WD_TYPE_ABSTIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			return 16;		/* sizeof(TIMESTAMP_STRUCT) */

		case WD_TYPE_MACADDR:
			return 17;
		case WD_TYPE_INET:
		case WD_TYPE_CIDR:
			return sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:255.255.255.255/128");
		case WD_TYPE_UUID:
			return 16;		/* sizeof(SQLGUID) */

			/* Character types use the default precision */
		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
			{
			int	coef = 1;
			Int4	prec = wdtype_attr_column_size(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as), maxvarc;
			if (SQL_NO_TOTAL == prec)
				return prec;
#ifdef	UNICODE_SUPPORT
			if (CC_is_in_unicode_driver(conn))
				return prec * WCLEN;
#endif /* UNICODE_SUPPORT */
			coef = conn->mb_maxbyte_per_char;
			if (coef < 2 && (conn->connInfo).lf_conversion)
				/* CR -> CR/LF */
				coef = 2;
			if (coef == 1)
				return prec;
			maxvarc = conn->connInfo.drivers.max_varchar_size;
			if (prec <= maxvarc && prec * coef > maxvarc)
				return maxvarc;
			return coef * prec;
			}
#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
		case WD_TYPE_INTERVAL:
			return sizeof(SQL_INTERVAL_STRUCT);
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */

		default:
			return wdtype_attr_column_size(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
	}
}

/*
 */
Int4
wdtype_attr_desclength(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int handle_unknown_size_as)
{
	int	dsize;

	switch (type)
	{
		case WD_TYPE_INT2:
			return 2;

		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
			return 4;

		case WD_TYPE_INT8:
			return 20;			/* signed: 19 digits + sign */

		case WD_TYPE_NUMERIC:
			dsize = getNumericColumnSizeX(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
			return dsize <= 0 ? dsize : dsize + 2;

		case WD_TYPE_FLOAT4:
		case WD_TYPE_MONEY:
			return 4;

		case WD_TYPE_FLOAT8:
			return 8;

		case WD_TYPE_DATE:
		case WD_TYPE_TIME:
		case WD_TYPE_ABSTIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
		case WD_TYPE_TIMESTAMP:
		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
			return wdtype_attr_column_size(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
		default:
			return wdtype_attr_column_size(conn, type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
	}
}

Int2
wdtype_attr_decimal_digits(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int UNUSED_handle_unknown_size_as)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_MONEY:
		case WD_TYPE_BOOL:

			/*
			 * Number of digits to the right of the decimal point in
			 * "yyyy-mm=dd hh:mm:ss[.f...]"
			 */
		case WD_TYPE_ABSTIME:
		case WD_TYPE_TIMESTAMP:
			return 0;
		case WD_TYPE_TIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			/* return 0; */
			return getTimestampDecimalDigitsX(conn, type, atttypmod);

		case WD_TYPE_NUMERIC:
			return getNumericDecimalDigitsX(conn, type, atttypmod, adtsize_or_longestlen, UNUSED_handle_unknown_size_as);

#ifdef	WD_INTERVAL_AS_SQL_INTERVAL
		case WD_TYPE_INTERVAL:
			return getIntervalDecimalDigits(type, atttypmod);
#endif /* WD_INTERVAL_AS_SQL_INTERVAL */

		default:
			return -1;
	}
}

Int2
wdtype_attr_scale(const ConnectionClass *conn, OID type, int atttypmod, int adtsize_or_longestlen, int UNUSED_handle_unknown_size_as)
{
	switch (type)
	{
		case WD_TYPE_NUMERIC:
			return getNumericDecimalDigitsX(conn, type, atttypmod, adtsize_or_longestlen, UNUSED_handle_unknown_size_as);
	}
	return -1;
}

Int4
wdtype_attr_transfer_octet_length(const ConnectionClass *conn, OID type, int atttypmod, int handle_unknown_size_as)
{
	int	coef = 1;
	Int4	maxvarc, column_size;

	switch (type)
	{
		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
		case WD_TYPE_TEXT:
		case WD_TYPE_UNKNOWN:
			column_size = wdtype_attr_column_size(conn, type, atttypmod, WD_ADT_UNSET, handle_unknown_size_as);
			if (SQL_NO_TOTAL == column_size)
				return column_size;
#ifdef	UNICODE_SUPPORT
			if (CC_is_in_unicode_driver(conn))
				return column_size * WCLEN;
#endif /* UNICODE_SUPPORT */
			coef = conn->mb_maxbyte_per_char;
			if (coef < 2 && (conn->connInfo).lf_conversion)
				/* CR -> CR/LF */
				coef = 2;
			if (coef == 1)
				return column_size;
			maxvarc = conn->connInfo.drivers.max_varchar_size;
			if (column_size <= maxvarc && column_size * coef > maxvarc)
				return maxvarc;
			return coef * column_size;
		case WD_TYPE_BYTEA:
			return wdtype_attr_column_size(conn, type, atttypmod, WD_ADT_UNSET, handle_unknown_size_as);
		default:
			if (type == conn->lobj_type)
				return wdtype_attr_column_size(conn, type, atttypmod, WD_ADT_UNSET, handle_unknown_size_as);
	}
	return -1;
}



/*
 * This was used when binding a query parameter, to decide which PostgreSQL
 * datatype to send to the server, depending on the SQL datatype that was
 * used in the SQLBindParameter call.
 *
 * However it is inflexible and rather harmful.
 * Now this function always returns 0.
 * We use cast instead to keep regression test results
 * in order to keep regression test results.
 */
OID
sqltype_to_bind_wdtype(const ConnectionClass *conn, SQLSMALLINT fSqlType)
{
	OID		wdtype = 0;

	return wdtype;
}

/*
 * Casting parameters e.g. ?::timestamp is much more flexible
 * than specifying parameter datatype oids determined by
 * sqltype_to_bind_wdtype() via parse message.
 */
const char *
sqltype_to_pgcast(const ConnectionClass *conn, SQLSMALLINT fSqlType)
{
	const char *pgCast = NULL_STRING;

	switch (fSqlType)
	{
		case SQL_BINARY:
		case SQL_VARBINARY:
			pgCast = "::bytea";
			break;
		case SQL_TYPE_DATE:
		case SQL_DATE:
			pgCast = "::date";
			break;
		case SQL_DECIMAL:
		case SQL_NUMERIC:
			pgCast = "::numeric";
			break;
		case SQL_BIGINT:
			pgCast = "::int8";
			break;
		case SQL_INTEGER:
			pgCast = "::int4";
			break;
		case SQL_REAL:
			pgCast = "::float4";
			break;
		case SQL_SMALLINT:
		case SQL_TINYINT:
			pgCast = "::int2";
			break;
		case SQL_TIME:
		case SQL_TYPE_TIME:
			pgCast = "::time";
			break;
		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			pgCast = "::timestamp";
			break;
		case SQL_GUID:
			if (WD_VERSION_GE(conn, 8.3))
				pgCast = "::uuid";
			break;
		case SQL_INTERVAL_MONTH:
		case SQL_INTERVAL_YEAR:
		case SQL_INTERVAL_YEAR_TO_MONTH:
		case SQL_INTERVAL_DAY:
		case SQL_INTERVAL_HOUR:
		case SQL_INTERVAL_MINUTE:
		case SQL_INTERVAL_SECOND:
		case SQL_INTERVAL_DAY_TO_HOUR:
		case SQL_INTERVAL_DAY_TO_MINUTE:
		case SQL_INTERVAL_DAY_TO_SECOND:
		case SQL_INTERVAL_HOUR_TO_MINUTE:
		case SQL_INTERVAL_HOUR_TO_SECOND:
		case SQL_INTERVAL_MINUTE_TO_SECOND:
			pgCast = "::interval";
			break;
	}

	return pgCast;
}

OID
sqltype_to_wdtype(const ConnectionClass *conn, SQLSMALLINT fSqlType)
{
	OID		wdtype;
	const ConnInfo	*ci = &(conn->connInfo);

	wdtype = 0; /* ??? */
	switch (fSqlType)
	{
		case SQL_BINARY:
			wdtype = WD_TYPE_BYTEA;
			break;

		case SQL_CHAR:
			wdtype = WD_TYPE_BPCHAR;
			break;

#ifdef	UNICODE_SUPPORT
		case SQL_WCHAR:
			wdtype = WD_TYPE_BPCHAR;
			break;
#endif /* UNICODE_SUPPORT */

		case SQL_BIT:
			wdtype = WD_TYPE_BOOL;
			break;

		case SQL_TYPE_DATE:
		case SQL_DATE:
			wdtype = WD_TYPE_DATE;
			break;

		case SQL_DOUBLE:
		case SQL_FLOAT:
			wdtype = WD_TYPE_FLOAT8;
			break;

		case SQL_DECIMAL:
		case SQL_NUMERIC:
			wdtype = WD_TYPE_NUMERIC;
			break;

		case SQL_BIGINT:
			wdtype = WD_TYPE_INT8;
			break;

		case SQL_INTEGER:
			wdtype = WD_TYPE_INT4;
			break;

		case SQL_LONGVARBINARY:
			if (ci->bytea_as_longvarbinary)
				wdtype = WD_TYPE_BYTEA;
			else
				wdtype = conn->lobj_type;
			break;

		case SQL_LONGVARCHAR:
			wdtype = ci->drivers.text_as_longvarchar ? WD_TYPE_TEXT : WD_TYPE_VARCHAR;
			break;

#ifdef	UNICODE_SUPPORT
		case SQL_WLONGVARCHAR:
			wdtype = ci->drivers.text_as_longvarchar ? WD_TYPE_TEXT : WD_TYPE_VARCHAR;
			break;
#endif /* UNICODE_SUPPORT */

		case SQL_REAL:
			wdtype = WD_TYPE_FLOAT4;
			break;

		case SQL_SMALLINT:
		case SQL_TINYINT:
			wdtype = WD_TYPE_INT2;
			break;

		case SQL_TIME:
		case SQL_TYPE_TIME:
			wdtype = WD_TYPE_TIME;
			break;

		case SQL_TIMESTAMP:
		case SQL_TYPE_TIMESTAMP:
			wdtype = WD_TYPE_DATETIME;
			break;

		case SQL_VARBINARY:
			wdtype = WD_TYPE_BYTEA;
			break;

		case SQL_VARCHAR:
			wdtype = WD_TYPE_VARCHAR;
			break;

#ifdef	UNICODE_SUPPORT
		case SQL_WVARCHAR:
			wdtype = WD_TYPE_VARCHAR;
			break;
#endif /* UNICODE_SUPPORT */

		case SQL_GUID:
			if (WD_VERSION_GE(conn, 8.3))
				wdtype = WD_TYPE_UUID;
			break;

		case SQL_INTERVAL_MONTH:
		case SQL_INTERVAL_YEAR:
		case SQL_INTERVAL_YEAR_TO_MONTH:
		case SQL_INTERVAL_DAY:
		case SQL_INTERVAL_HOUR:
		case SQL_INTERVAL_MINUTE:
		case SQL_INTERVAL_SECOND:
		case SQL_INTERVAL_DAY_TO_HOUR:
		case SQL_INTERVAL_DAY_TO_MINUTE:
		case SQL_INTERVAL_DAY_TO_SECOND:
		case SQL_INTERVAL_HOUR_TO_MINUTE:
		case SQL_INTERVAL_HOUR_TO_SECOND:
		case SQL_INTERVAL_MINUTE_TO_SECOND:
			wdtype = WD_TYPE_INTERVAL;
			break;
	}

	return wdtype;
}

static int
getAtttypmodEtc(const StatementClass *stmt, int col, int *adtsize_or_longestlen)
{
	int	atttypmod = -1;

	if (NULL != adtsize_or_longestlen)
		*adtsize_or_longestlen = WD_ADT_UNSET;
	if (col >= 0)
	{
		const QResultClass	*res;

		if (res = SC_get_ExecdOrParsed(stmt), NULL != res)
		{
			atttypmod = QR_get_atttypmod(res, col);
			if (NULL != adtsize_or_longestlen)
			{
				if (stmt->catalog_result)
					*adtsize_or_longestlen = QR_get_fieldsize(res, col);
				else
				{
					*adtsize_or_longestlen = QR_get_display_size(res, col);
					if (WD_TYPE_NUMERIC == QR_get_field_type(res, col) &&
					   atttypmod < 0 &&
					   *adtsize_or_longestlen > 0)
					{
						SQLULEN		i;
						size_t		sval, maxscale = 0;
						const char *tval, *sptr;

						for (i = 0; i < res->num_cached_rows; i++)
						{
							tval = static_cast<const char*>(QR_get_value_backend_text(res, i, col));
							if (NULL != tval)
							{
								sptr = strchr(tval, '.');
								if (NULL != sptr)
								{
									sval = strlen(tval) - (sptr + 1 - tval);
									if (sval > maxscale)
										maxscale = sval;
								}
							}
						}
						*adtsize_or_longestlen += (int) (maxscale << 16);
					}
				}
			}
		}
	}
	return atttypmod;
}

/*
 *	There are two ways of calling this function:
 *
 *	1.	When going through the supported PG types (SQLGetTypeInfo)
 *
 *	2.	When taking any type id (SQLColumns, SQLGetData)
 *
 *	The first type will always work because all the types defined are returned here.
 *	The second type will return a default based on global parameter when it does not
 *	know.	This allows for supporting
 *	types that are unknown.  All other pg routines in here return a suitable default.
 */
SQLSMALLINT
wdtype_to_concise_type(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_to_concise_type(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
}

SQLSMALLINT
wdtype_to_sqldesctype(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	adtsize_or_longestlen;
	int	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);

	return wdtype_attr_to_sqldesctype(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, handle_unknown_size_as);
}

SQLSMALLINT
wdtype_to_datetime_sub(const StatementClass *stmt, OID type, int col)
{
	int	atttypmod = getAtttypmodEtc(stmt, col, NULL);

	return wdtype_attr_to_datetime_sub(SC_get_conn(stmt), type, atttypmod);
}

const char *
wdtype_to_name(const StatementClass *stmt, OID type, int col, BOOL auto_increment)
{
	int	atttypmod = getAtttypmodEtc(stmt, col, NULL);

	return wdtype_attr_to_name(SC_get_conn(stmt), type, atttypmod, auto_increment);
}


/*
 *	This corresponds to "precision" in ODBC 2.x.
 *
 *	For WD_TYPE_VARCHAR, WD_TYPE_BPCHAR, WD_TYPE_NUMERIC, SQLColumns will
 *	override this length with the atttypmod length from WD_attribute .
 *
 *	If col >= 0, then will attempt to get the info from the result set.
 *	This is used for functions SQLDescribeCol and SQLColAttributes.
 */
Int4	/* PostgreSQL restriction */
wdtype_column_size(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_column_size(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, stmt->catalog_result ? UNKNOWNS_AS_LONGEST : handle_unknown_size_as);
}

/*
 *	precision in ODBC 3.x.
 */
SQLSMALLINT
wdtype_precision(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_precision(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, stmt->catalog_result ? UNKNOWNS_AS_LONGEST : handle_unknown_size_as);
}


Int4
wdtype_display_size(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_display_size(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, stmt->catalog_result ? UNKNOWNS_AS_LONGEST : handle_unknown_size_as);
}


/*
 *	The length in bytes of data transferred on an SQLGetData, SQLFetch,
 *	or SQLFetchScroll operation if SQL_C_DEFAULT is specified.
 */
Int4
wdtype_buffer_length(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_buffer_length(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, stmt->catalog_result ? UNKNOWNS_AS_LONGEST : handle_unknown_size_as);
}

/*
 */
Int4
wdtype_desclength(const StatementClass *stmt, OID type, int col, int handle_unknown_size_as)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_desclength(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, stmt->catalog_result ? UNKNOWNS_AS_LONGEST : handle_unknown_size_as);
}

#ifdef	NOT_USED
/*
 *	Transfer octet length.
 */
Int4
wdtype_transfer_octet_length(const StatementClass *stmt, OID type, int column_size)
{
	ConnectionClass	*conn = SC_get_conn(stmt);

	int	coef = 1;
	Int4	maxvarc;
	switch (type)
	{
		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
		case WD_TYPE_TEXT:
			if (SQL_NO_TOTAL == column_size)
				return column_size;
#ifdef	UNICODE_SUPPORT
			if (CC_is_in_unicode_driver(conn))
				return column_size * WCLEN;
#endif /* UNICODE_SUPPORT */
			coef = conn->mb_maxbyte_per_char;
			if (coef < 2 && (conn->connInfo).lf_conversion)
				/* CR -> CR/LF */
				coef = 2;
			if (coef == 1)
				return column_size;
			maxvarc = conn->connInfo.drivers.max_varchar_size;
			if (column_size <= maxvarc && column_size * coef > maxvarc)
				return maxvarc;
			return coef * column_size;
		case WD_TYPE_BYTEA:
			return column_size;
		default:
			if (type == conn->lobj_type)
				return column_size;
	}
	return -1;
}
#endif /* NOT_USED */

/*
 *	corrsponds to "min_scale" in ODBC 2.x.
 */
Int2
wdtype_min_decimal_digits(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_MONEY:
		case WD_TYPE_BOOL:
		case WD_TYPE_ABSTIME:
		case WD_TYPE_TIMESTAMP:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
		case WD_TYPE_NUMERIC:
			return 0;
		default:
			return -1;
	}
}

/*
 *	corrsponds to "max_scale" in ODBC 2.x.
 */
Int2
wdtype_max_decimal_digits(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_MONEY:
		case WD_TYPE_BOOL:
		case WD_TYPE_ABSTIME:
		case WD_TYPE_TIMESTAMP:
			return 0;
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
			return 38;
		case WD_TYPE_NUMERIC:
			return getNumericDecimalDigitsX(conn, type, -1, -1, UNUSED_HANDLE_UNKNOWN_SIZE_AS);
		default:
			return -1;
	}
}

/*
 *	corrsponds to "scale" in ODBC 2.x.
 */
Int2
wdtype_decimal_digits(const StatementClass *stmt, OID type, int col)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_decimal_digits(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, UNUSED_HANDLE_UNKNOWN_SIZE_AS);
}

/*
 *	"scale" in ODBC 3.x.
 */
Int2
wdtype_scale(const StatementClass *stmt, OID type, int col)
{
	int	atttypmod, adtsize_or_longestlen;

	atttypmod = getAtttypmodEtc(stmt, col, &adtsize_or_longestlen);
	return wdtype_attr_scale(SC_get_conn(stmt), type, atttypmod, adtsize_or_longestlen, UNUSED_HANDLE_UNKNOWN_SIZE_AS);
}


Int2
wdtype_radix(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_XID:
		case WD_TYPE_OID:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_NUMERIC:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_MONEY:
		case WD_TYPE_FLOAT8:
			return 10;
		default:
			return -1;
	}
}


Int2
wdtype_nullable(const ConnectionClass *conn, OID type)
{
	return SQL_NULLABLE;		/* everything should be nullable */
}


Int2
wdtype_auto_increment(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_MONEY:
		case WD_TYPE_BOOL:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_INT8:
		case WD_TYPE_NUMERIC:

		case WD_TYPE_DATE:
		case WD_TYPE_TIME_WITH_TMZONE:
		case WD_TYPE_TIME:
		case WD_TYPE_ABSTIME:
		case WD_TYPE_DATETIME:
		case WD_TYPE_TIMESTAMP_NO_TMZONE:
		case WD_TYPE_TIMESTAMP:
			return FALSE;

		default:
			return -1;
	}
}


Int2
wdtype_case_sensitive(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_CHAR:

		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
		case WD_TYPE_TEXT:
		case WD_TYPE_NAME:
		case WD_TYPE_REFCURSOR:
			return TRUE;

		default:
			return FALSE;
	}
}


Int2
wdtype_money(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_MONEY:
			return TRUE;
		default:
			return FALSE;
	}
}


Int2
wdtype_searchable(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_CHAR:

		case WD_TYPE_VARCHAR:
		case WD_TYPE_BPCHAR:
		case WD_TYPE_TEXT:
		case WD_TYPE_NAME:
		case WD_TYPE_REFCURSOR:
			return SQL_SEARCHABLE;

		default:
			if (conn && type == conn->lobj_type)
				return SQL_UNSEARCHABLE;
			return SQL_ALL_EXCEPT_LIKE;
	}
}


Int2
wdtype_unsigned(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_OID:
		case WD_TYPE_XID:
			return TRUE;

		case WD_TYPE_INT2:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_NUMERIC:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_MONEY:
			return FALSE;

		default:
			return -1;
	}
}


const char *
wdtype_literal_prefix(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_NUMERIC:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_MONEY:
			return NULL;

		default:
			return "'";
	}
}


const char *
wdtype_literal_suffix(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_INT2:
		case WD_TYPE_OID:
		case WD_TYPE_XID:
		case WD_TYPE_INT4:
		case WD_TYPE_INT8:
		case WD_TYPE_NUMERIC:
		case WD_TYPE_FLOAT4:
		case WD_TYPE_FLOAT8:
		case WD_TYPE_MONEY:
			return NULL;

		default:
			return "'";
	}
}


const char *
wdtype_create_params(const ConnectionClass *conn, OID type)
{
	switch (type)
	{
		case WD_TYPE_BPCHAR:
		case WD_TYPE_VARCHAR:
			return "max. length";
		case WD_TYPE_NUMERIC:
			return "precision, scale";
		default:
			return NULL;
	}
}


SQLSMALLINT
sqltype_to_default_ctype(const ConnectionClass *conn, SQLSMALLINT sqltype)
{
	/*
	 * from the table on page 623 of ODBC 2.0 Programmer's Reference
	 * (Appendix D)
	 */
	switch (sqltype)
	{
		case SQL_CHAR:
		case SQL_VARCHAR:
		case SQL_LONGVARCHAR:
		case SQL_DECIMAL:
		case SQL_NUMERIC:
			return SQL_C_CHAR;
		case SQL_BIGINT:
			return ALLOWED_C_BIGINT;

#ifdef	UNICODE_SUPPORT
		case SQL_WCHAR:
		case SQL_WVARCHAR:
		case SQL_WLONGVARCHAR:
			return ansi_to_wtype(conn, SQL_C_CHAR);
#endif /* UNICODE_SUPPORT */

		case SQL_BIT:
			return SQL_C_BIT;

		case SQL_TINYINT:
			return SQL_C_STINYINT;

		case SQL_SMALLINT:
			return SQL_C_SSHORT;

		case SQL_INTEGER:
			return SQL_C_SLONG;

		case SQL_REAL:
			return SQL_C_FLOAT;

		case SQL_FLOAT:
		case SQL_DOUBLE:
			return SQL_C_DOUBLE;

		case SQL_BINARY:
		case SQL_VARBINARY:
		case SQL_LONGVARBINARY:
			return SQL_C_BINARY;

		case SQL_DATE:
			return SQL_C_DATE;

		case SQL_TIME:
			return SQL_C_TIME;

		case SQL_TIMESTAMP:
			return SQL_C_TIMESTAMP;

		case SQL_TYPE_DATE:
			return SQL_C_TYPE_DATE;

		case SQL_TYPE_TIME:
			return SQL_C_TYPE_TIME;

		case SQL_TYPE_TIMESTAMP:
			return SQL_C_TYPE_TIMESTAMP;

		case SQL_GUID:
			if (conn->ms_jet)
				return SQL_C_CHAR;
			else
				return SQL_C_GUID;

		default:
			/* should never happen */
			return SQL_C_CHAR;
	}
}

Int4
ctype_length(SQLSMALLINT ctype)
{
	switch (ctype)
	{
		case SQL_C_SSHORT:
		case SQL_C_SHORT:
			return sizeof(SWORD);

		case SQL_C_USHORT:
			return sizeof(UWORD);

		case SQL_C_SLONG:
		case SQL_C_LONG:
			return sizeof(SDWORD);

		case SQL_C_ULONG:
			return sizeof(UDWORD);

		case SQL_C_FLOAT:
			return sizeof(SFLOAT);

		case SQL_C_DOUBLE:
			return sizeof(SDOUBLE);

		case SQL_C_BIT:
			return sizeof(UCHAR);

		case SQL_C_STINYINT:
		case SQL_C_TINYINT:
			return sizeof(SCHAR);

		case SQL_C_UTINYINT:
			return sizeof(UCHAR);

		case SQL_C_DATE:
		case SQL_C_TYPE_DATE:
			return sizeof(DATE_STRUCT);

		case SQL_C_TIME:
		case SQL_C_TYPE_TIME:
			return sizeof(TIME_STRUCT);

		case SQL_C_TIMESTAMP:
		case SQL_C_TYPE_TIMESTAMP:
			return sizeof(TIMESTAMP_STRUCT);

		case SQL_C_GUID:
			return sizeof(SQLGUID);
		case SQL_C_INTERVAL_YEAR:
		case SQL_C_INTERVAL_MONTH:
		case SQL_C_INTERVAL_YEAR_TO_MONTH:
		case SQL_C_INTERVAL_DAY:
		case SQL_C_INTERVAL_HOUR:
		case SQL_C_INTERVAL_DAY_TO_HOUR:
		case SQL_C_INTERVAL_MINUTE:
		case SQL_C_INTERVAL_DAY_TO_MINUTE:
		case SQL_C_INTERVAL_HOUR_TO_MINUTE:
		case SQL_C_INTERVAL_SECOND:
		case SQL_C_INTERVAL_DAY_TO_SECOND:
		case SQL_C_INTERVAL_HOUR_TO_SECOND:
		case SQL_C_INTERVAL_MINUTE_TO_SECOND:
			return sizeof(SQL_INTERVAL_STRUCT);
		case SQL_C_NUMERIC:
			return sizeof(SQL_NUMERIC_STRUCT);
		case SQL_C_SBIGINT:
		case SQL_C_UBIGINT:
			return sizeof(SQLBIGINT);

		case SQL_C_BINARY:
		case SQL_C_CHAR:
#ifdef	UNICODE_SUPPORT
		case SQL_C_WCHAR:
#endif /* UNICODE_SUPPORT */
			return 0;

		default:				/* should never happen */
			return 0;
	}
}
