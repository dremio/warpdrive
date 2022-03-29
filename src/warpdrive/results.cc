/*
 * Module:			results.c
 *
 * Description:		This module contains functions related to
 *					retrieving result information through the ODBC API.
 *
 * Classes:			n/a
 *
 * API functions:	SQLRowCount, SQLNumResultCols, SQLDescribeCol,
 *					SQLColAttributes, SQLGetData, SQLFetch, SQLExtendedFetch,
 *					SQLMoreResults, SQLSetPos, SQLSetScrollOptions(NI),
 *					SQLSetCursorName, SQLGetCursorName
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */

#include "wdodbc.h"
#include "sql.h"
#include "sqlext.h"

#include <string.h>
#include "misc.h"
#include "dlg_specific.h"
#include "environ.h"
#include "connection.h"
#include "statement.h"
#include "bind.h"
#include "qresult.h"
#include "convert.h"
#include "wdtypes.h"

#include <stdio.h>
#include <limits.h>

#include "wdapifunc.h"

#include "AttributeUtils.h"
#include "ODBCStatement.h"
#include "ODBCDescriptor.h"
#include <odbcabstraction/exceptions.h>

using namespace ODBC;
using namespace driver::odbcabstraction;

/*	Helper macro */
#define getEffectiveOid(conn, fi) WD_true_type((conn), (fi)->columntype, FI_type(fi))
#define	NULL_IF_NULL(a) ((a) ? ((const char *)(a)) : "(null)")


RETCODE		SQL_API
WD_RowCount(HSTMT hstmt,
			   SQLLEN * pcrow)
{
  CSTR func = "WD_RowCount";
  if (pcrow) {
    *pcrow = -1;
  }
  return SQL_SUCCESS;
}

static BOOL
SC_describe_ok(StatementClass *stmt, BOOL build_fi, int col_idx, const char *func)
{
	Int2		num_fields;
	QResultClass *result;
	BOOL		exec_ok = TRUE;

	num_fields = SC_describe(stmt);
	result = SC_get_ExecdOrParsed(stmt);

	MYLOG(0, "entering result = %p, status = %d, numcols = %d\n", result, stmt->status, result != NULL ? QR_NumResultCols(result) : -1);
	/****if ((!result) || ((stmt->status != STMT_FINISHED) && (stmt->status != STMT_PREMATURE))) ****/
	if (!QR_command_maybe_successful(result) || num_fields < 0)
	{
		/* no query has been executed on this statement */
		SC_set_error(stmt, STMT_EXEC_ERROR, "No query has been executed with that handle", func);
		exec_ok = FALSE;
	}
	else if (col_idx >= 0 && col_idx < num_fields)
	{
		OID	reloid = QR_get_relid(result, col_idx);
		IRDFields	*irdflds = SC_get_IRDF(stmt);
		FIELD_INFO	*fi;
		TABLE_INFO	*ti = NULL;

MYLOG(DETAIL_LOG_LEVEL, "build_fi=%d reloid=%u\n", build_fi, reloid);
		if (build_fi && 0 != QR_get_attid(result, col_idx))
			getCOLIfromTI(func, NULL, stmt, reloid, &ti);
MYLOG(DETAIL_LOG_LEVEL, "nfields=%d\n", irdflds->nfields);
		if (irdflds->fi && col_idx < (int) irdflds->nfields)
		{
			fi = irdflds->fi[col_idx];
			if (fi)
			{
				if (ti)
				{
					if (NULL == fi->ti)
						fi->ti = ti;
					if (!FI_is_applicable(fi)
					    && 0 != (ti->flags & TI_COLATTRIBUTE))
						fi->flag |= FIELD_COL_ATTRIBUTE;
				}
				fi->basetype = QR_get_field_type(result, col_idx);
				if (0 == fi->columntype)
					fi->columntype = fi->basetype;
			}
		}
	}
	return exec_ok;
}

/*
 *	This returns the number of columns associated with the database
 *	attached to "hstmt".
 */
RETCODE		SQL_API
WD_NumResultCols(HSTMT hstmt,
					SQLSMALLINT * pccol)
{
  CSTR func = "WD_NumResultCols";
  const ODBCStatement* stmt = reinterpret_cast<ODBCStatement*>(hstmt);
  const std::vector<DescriptorRecord>& records = stmt->GetIRD()->GetRecords();

  GetAttribute<SQLSMALLINT,size_t>(static_cast<SQLSMALLINT>(records.size()), pccol, sizeof(SQLSMALLINT), nullptr);
  return SQL_SUCCESS;
}

#define	USE_FI(fi, unknown) (fi && UNKNOWNS_AS_LONGEST != unknown)

/*
 *	Return information about the database column the user wants
 *	information about.
 */
RETCODE		SQL_API
WD_DescribeCol(HSTMT hstmt,
				  SQLUSMALLINT icol,
				  SQLCHAR * szColName,
				  SQLSMALLINT cbColNameMax,
				  SQLSMALLINT * pcbColName,
				  SQLSMALLINT * pfSqlType,
				  SQLULEN * pcbColDef,
				  SQLSMALLINT * pibScale,
				  SQLSMALLINT * pfNullable)
{
  CSTR func = "WD_DescribeCol";

  if (icol == 0) {
    throw DriverException("InvalidDescriptorIndex");
  }

  const ODBCStatement* stmt = reinterpret_cast<ODBCStatement*>(hstmt);
  const std::vector<DescriptorRecord>& records = stmt->GetIRD()->GetRecords();

  SQLUSMALLINT zeroBasedIndex = icol - 1;
  const DescriptorRecord& record = records[zeroBasedIndex];
  SQLSMALLINT totalColumnNameLen;
  GetAttributeUTF8(record.m_name, szColName, cbColNameMax, &totalColumnNameLen);
  if (pcbColName) {
    *pcbColName = totalColumnNameLen;
  }
  GetAttribute<SQLSMALLINT, size_t>(record.m_type, pfSqlType, sizeof(SQLUSMALLINT), nullptr);
  GetAttribute<SQLUINTEGER, size_t>(record.m_length, pcbColDef, sizeof(SQLULEN), nullptr);
  GetAttribute<SQLSMALLINT, size_t>(record.m_nullable, pfNullable, sizeof(SQLSMALLINT), nullptr);

  return SQL_SUCCESS;
}

/*		Returns result column descriptor information for a result set. */
RETCODE		SQL_API
WD_ColAttributes(HSTMT hstmt,
					SQLUSMALLINT icol,
					SQLUSMALLINT fDescType,
					PTR rgbDesc,
					SQLSMALLINT cbDescMax,
					SQLSMALLINT * pcbDesc,
					SQLLEN * pfDesc)
{
  CSTR func = "WD_ColAttributes";
  SQLRETURN result = SQL_SUCCESS;

  MYLOG(0, "entering..col=%d %d len=%d.\n", icol, fDescType,
				cbDescMax);

  if (icol == 0) {
    throw DriverException("Bookmarks are not supported");
  }

	
  const ODBCStatement* stmt = reinterpret_cast<ODBCStatement*>(hstmt);
  const std::vector<DescriptorRecord>& records = stmt->GetIRD()->GetRecords();
  
  if (fDescType == SQL_DESC_COUNT) {
    if (pfDesc) {
	  *pfDesc = static_cast<SQLLEN>(records.size());
	}
	return SQL_SUCCESS;
  }

  SQLUSMALLINT zeroBasedIndex = icol - 1;
  const DescriptorRecord& record = records[zeroBasedIndex];

  SQLLEN recordNumericValue = 0;
  const std::string* recordStrValue = nullptr;
  switch (fDescType) {
	  case SQL_DESC_AUTO_UNIQUE_VALUE:
	    recordNumericValue = record.m_autoUniqueValue;
		break;
	  case SQL_DESC_CASE_SENSITIVE:
	    recordNumericValue = record.m_caseSensitive;
		break;
	  case SQL_DESC_CONCISE_TYPE:
	    recordNumericValue = record.m_conciseType;
		break;
	  case SQL_DESC_DISPLAY_SIZE:
	  	recordNumericValue = record.m_displaySize;
		break;
	  case SQL_DESC_FIXED_PREC_SCALE:
	    recordNumericValue = record.m_fixedPrecScale;
		break;
	  case SQL_COLUMN_LENGTH:
	  case SQL_DESC_LENGTH:
	    recordNumericValue = record.m_length;
		break;
	  case SQL_DESC_NULLABLE:
	    recordNumericValue = record.m_nullable;
		break;
	  case SQL_DESC_NUM_PREC_RADIX:
	    recordNumericValue = record.m_numPrecRadix;
		break;
	  case SQL_DESC_OCTET_LENGTH:
	    recordNumericValue = record.m_octetLength;
		break;
	  case SQL_COLUMN_PRECISION:
	  case SQL_DESC_PRECISION:
	    recordNumericValue = record.m_precision;
		break;
	  case SQL_COLUMN_SCALE:
	  case SQL_DESC_SCALE:
	    recordNumericValue = record.m_scale;
		break;
	  case SQL_DESC_SEARCHABLE:
	    recordNumericValue = record.m_searchable;
		break;
	  case SQL_DESC_TYPE:
	    recordNumericValue = record.m_type;
		break;
	  case SQL_DESC_UNNAMED:
	    recordNumericValue = record.m_unnamed;
		break;
	  case SQL_DESC_UNSIGNED:
	    recordNumericValue = record.m_unsigned;
		break;
	  case SQL_DESC_UPDATABLE:
	    recordNumericValue = record.m_updatable;
		break;

	  case SQL_DESC_BASE_COLUMN_NAME:
	    recordStrValue = &record.m_baseColumnName;
		break;
	  case SQL_DESC_BASE_TABLE_NAME:
	    recordStrValue = &record.m_baseTableName;
		break;
	  case SQL_DESC_CATALOG_NAME:
	    recordStrValue = &record.m_catalogName;
		break;
	  case SQL_DESC_LABEL:
	    recordStrValue = &record.m_label;
		break;
	  case SQL_DESC_LITERAL_PREFIX:
	    recordStrValue = &record.m_literalPrefix;
		break;
	  case SQL_DESC_LITERAL_SUFFIX:
	    recordStrValue = &record.m_literalSuffix;
		break;
	  case SQL_DESC_LOCAL_TYPE_NAME:
	    recordStrValue = &record.m_localTypeName;
		break;
	  case SQL_DESC_NAME:
	    recordStrValue = &record.m_name;
		break;
	  case SQL_DESC_SCHEMA_NAME:
	    recordStrValue = &record.m_schemaName;
		break;
	  case SQL_DESC_TABLE_NAME:
	    recordStrValue = &record.m_tableName;
		break;
	  case SQL_DESC_TYPE_NAME:
	    recordStrValue = &record.m_typeName;
		break;
	  default:
	    throw DriverException("Invalid descriptor field");
  }

  // Character attribute.
  if (recordStrValue) {
	SQLINTEGER outputLen = 0;
    GetAttributeUTF8(*recordStrValue, rgbDesc, static_cast<SQLINTEGER>(cbDescMax), &outputLen);
	if (pcbDesc) {
	  *pcbDesc = static_cast<SQLSMALLINT>(outputLen);
	}
	return SQL_SUCCESS;
  }

  // Numeric attribute.
  GetAttribute<SQLINTEGER, size_t>(recordNumericValue, pfDesc, sizeof(SQLLEN), nullptr);
  return SQL_SUCCESS;
}


/*	Returns result data for a single column in the current row. */
RETCODE		SQL_API
WD_GetData(HSTMT hstmt,
			  SQLUSMALLINT icol,
			  SQLSMALLINT fCType,
			  PTR rgbValue,
			  SQLLEN cbValueMax,
			  SQLLEN * pcbValue)
{
  CSTR func = "WD_GetData";
  ODBCStatement* stmt = reinterpret_cast<ODBCStatement*>(hstmt);
  if (!stmt->GetData(icol, fCType, rgbValue, cbValueMax, pcbValue)) {
	return SQL_SUCCESS;
  }
  return SQL_SUCCESS_WITH_INFO;
}


/*
 *		Returns data for bound columns in the current row ("hstmt->iCursor"),
 *		advances the cursor.
 */
RETCODE		SQL_API
WD_Fetch(HSTMT hstmt)
{
	CSTR func = "WD_Fetch";
	ODBCStatement* statement = reinterpret_cast<ODBCStatement*>(hstmt);

    SQLULEN numRows = statement->GetARD()->GetArraySize();
	if (!statement->Fetch(numRows)) {
	  return SQL_NO_DATA;
	}
	return SQL_SUCCESS;
}

static RETCODE SQL_API
SC_pos_reload_needed(StatementClass *stmt, SQLULEN req_size, UDWORD flag);

SQLLEN
getNthValid(const QResultClass *res, SQLLEN sta, UWORD orientation,
			SQLULEN nth, SQLLEN *nearest)
{
	SQLLEN	i, num_tuples = QR_get_num_total_tuples(res), nearp;
	SQLULEN count;
	KeySet	*keyset;

	if (!QR_once_reached_eof(res))
		num_tuples = INT_MAX;
	/* Note that the parameter nth is 1-based */
MYLOG(DETAIL_LOG_LEVEL, "get " FORMAT_ULEN "th Valid data from " FORMAT_LEN " to %s [dlt=%d]", nth, sta, orientation == SQL_FETCH_PRIOR ? "backward" : "forward", res->dl_count);
	if (0 == res->dl_count)
	{
		MYPRINTF(DETAIL_LOG_LEVEL, "\n");
		if (SQL_FETCH_PRIOR == orientation)
		{
			if (sta + 1 >= (SQLLEN) nth)
			{
				*nearest = sta + 1 - nth;
				return nth;
			}
			*nearest = -1;
			return -(SQLLEN)(sta + 1);
		}
		else
		{
			nearp = sta - 1 + nth;
			if (nearp < num_tuples)
			{
				*nearest = nearp;
				return nth;
			}
			*nearest = num_tuples;
			return -(SQLLEN)(num_tuples - sta);
		}
	}
	count = 0;
	if (QR_get_cursor(res))
	{
		SQLLEN	*deleted = res->deleted;
		SQLLEN	delsta;

		if (SQL_FETCH_PRIOR == orientation)
		{
			*nearest = sta + 1 - nth;
			delsta = (-1);
			MYPRINTF(DETAIL_LOG_LEVEL, "deleted ");
			for (i = res->dl_count - 1; i >=0 && *nearest <= deleted[i]; i--)
			{
				MYPRINTF(DETAIL_LOG_LEVEL, "[" FORMAT_LEN "]=" FORMAT_LEN " ", i, deleted[i]);
				if (sta >= deleted[i])
				{
					(*nearest)--;
					if (i > delsta)
						delsta = i;
				}
			}
			MYPRINTF(DETAIL_LOG_LEVEL, "nearest=" FORMAT_LEN "\n", *nearest);
			if (*nearest < 0)
			{
				*nearest = -1;
				count = sta - delsta;
			}
			else
				return nth;
		}
		else
		{
			MYPRINTF(DETAIL_LOG_LEVEL, "\n");
			*nearest = sta - 1 + nth;
			delsta = res->dl_count;
			if (!QR_once_reached_eof(res))
				num_tuples = INT_MAX;
			for (i = 0; i < res->dl_count && *nearest >= deleted[i]; i++)
			{
				if (sta <= deleted[i])
				{
					(*nearest)++;
					if (i < delsta)
						delsta = i;
				}
			}
			if (*nearest >= num_tuples)
			{
				*nearest = num_tuples;
				count = *nearest - sta + delsta - res->dl_count;
			}
			else
				return nth;
		}
	}
	else if (SQL_FETCH_PRIOR == orientation)
	{
		for (i = sta, keyset = res->keyset + sta;
			i >= 0; i--, keyset--)
		{
			if (0 == (keyset->status & (CURS_SELF_DELETING | CURS_SELF_DELETED | CURS_OTHER_DELETED)))
			{
				*nearest = i;
MYPRINTF(DETAIL_LOG_LEVEL, " nearest=" FORMAT_LEN "\n", *nearest);
				if (++count == nth)
					return count;
			}
		}
		*nearest = -1;
	}
	else
	{
		for (i = sta, keyset = res->keyset + sta;
			i < num_tuples; i++, keyset++)
		{
			if (0 == (keyset->status & (CURS_SELF_DELETING | CURS_SELF_DELETED | CURS_OTHER_DELETED)))
			{
				*nearest = i;
MYPRINTF(DETAIL_LOG_LEVEL, " nearest=" FORMAT_LEN "\n", *nearest);
				if (++count == nth)
					return count;
			}
		}
		*nearest = num_tuples;
	}
MYPRINTF(DETAIL_LOG_LEVEL, " nearest not found\n");
	return -(SQLLEN)count;
}

static void
move_cursor_position_if_needed(StatementClass *self, QResultClass *res)
{
	SQLLEN	move_offset;

	/*
	 * The move direction must be initialized to is_not_moving or
	 * is_moving_from_the_last in advance.
	 */
	if (!QR_get_cursor(res))
	{
		QR_stop_movement(res); /* for safety */
		res->move_offset = 0;
		return;
	}
MYLOG(DETAIL_LOG_LEVEL, "BASE=" FORMAT_LEN " numb=" FORMAT_LEN " curr=" FORMAT_LEN " cursT=" FORMAT_LEN "\n", QR_get_rowstart_in_cache(res), res->num_cached_rows, self->currTuple, res->cursTuple);

	/* normal case */
	res->move_offset = 0;
	move_offset = self->currTuple - res->cursTuple;
	if (QR_get_rowstart_in_cache(res) >= 0 &&
	     QR_get_rowstart_in_cache(res) <= res->num_cached_rows)
	{
		QR_set_next_in_cache(res, (QR_get_rowstart_in_cache(res) < 0) ? 0 : QR_get_rowstart_in_cache(res));
		return;
	}
	if (0 == move_offset)
		return;
	if (move_offset > 0)
	{
		QR_set_move_forward(res);
		res->move_offset = move_offset;
	}
	else
	{
		QR_set_move_backward(res);
		res->move_offset = -move_offset;
	}
}
/*
 *	return NO_DATA_FOUND macros
 *	  save_rowset_start or num_tuples must be defined
 */
#define	EXTFETCH_RETURN_BOF(stmt, res) \
{ \
MYLOG(DETAIL_LOG_LEVEL, "RETURN_BOF\n"); \
	SC_set_rowset_start(stmt, -1, TRUE); \
	stmt->currTuple = -1; \
	/* move_cursor_position_if_needed(stmt, res); */ \
	return SQL_NO_DATA_FOUND; \
}
#define	EXTFETCH_RETURN_EOF(stmt, res) \
{ \
MYLOG(DETAIL_LOG_LEVEL, "RETURN_EOF\n"); \
	SC_set_rowset_start(stmt, num_tuples, TRUE); \
	stmt->currTuple = -1; \
	/* move_cursor_position_if_needed(stmt, res); */ \
	return SQL_NO_DATA_FOUND; \
}

/*
 *		This determines whether there are more results sets available for
 *		the "hstmt".
 */
/* CC: return SQL_NO_DATA_FOUND since we do not support multiple result sets */
RETCODE		SQL_API
WD_MoreResults(HSTMT hstmt)
{
	StatementClass	*stmt = (StatementClass *) hstmt;
	QResultClass	*res;
	RETCODE		ret = SQL_SUCCESS;

	MYLOG(0, "entering...\n");
	res = SC_get_Curres(stmt);
	if (res)
	{
		res = QR_nextr(res);
		SC_set_Curres(stmt, res);
	}
	if (res)
	{
		SQLSMALLINT	num_p;
		int errnum = 0, curerr;

		if (stmt->multi_statement < 0)
			WD_NumParams(stmt, &num_p);
		if (stmt->multi_statement > 0)
		{
			const char *cmdstr;

			SC_initialize_cols_info(stmt, FALSE, TRUE);
			stmt->statement_type = STMT_TYPE_UNKNOWN;
			if (cmdstr = QR_get_command(res), NULL != cmdstr)
				stmt->statement_type = statement_type(cmdstr);
			stmt->join_info = 0;
			SC_clear_parse_method(stmt);
		}
		stmt->diag_row_count = res->recent_processed_row_count;
		SC_set_rowset_start(stmt, -1, FALSE);
		stmt->currTuple = -1;

		if (!QR_command_maybe_successful(res))
		{
			ret = SQL_ERROR;
			errnum = STMT_EXEC_ERROR;
		}
		else if (NULL != QR_get_notice(res))
		{
			ret = SQL_SUCCESS_WITH_INFO;
			errnum = STMT_INFO_ONLY;
		}
		if (0 != errnum)
		{
			curerr = SC_get_errornumber(stmt);
			if (0 == curerr ||
			    (0 > curerr && errnum > 0))
				SC_set_errornumber(stmt, errnum);
		}
	}
	else
	{
		WD_FreeStmt(hstmt, SQL_CLOSE);
		ret = SQL_NO_DATA_FOUND;
	}
	MYLOG(0, "leaving %d\n", ret);
	return ret;
}


/*
 *	Stuff for updatable cursors.
 */
static Int2	getNumResultCols(const QResultClass *res)
{
	Int2	res_cols = QR_NumPublicResultCols(res);
	return res_cols;
}
static OID	getOid(const QResultClass *res, SQLLEN index)
{
	return res->keyset[index].oid;
}
static void getTid(const QResultClass *res, SQLLEN index, UInt4 *blocknum, UInt2 *offset)
{
	*blocknum = res->keyset[index].blocknum;
	*offset = res->keyset[index].offset;
}
static void KeySetSet(const TupleField *tuple, int num_fields, int num_key_fields, KeySet *keyset, BOOL statusInit)
{
	if (statusInit)
		keyset->status = 0;
	sscanf(static_cast<const char*>(tuple[num_fields - num_key_fields].value), "(%u,%hu)",
			&keyset->blocknum, &keyset->offset);
	if (num_key_fields > 1)
	{
		const char *oval = static_cast<const char*>(tuple[num_fields - 1].value);

		if ('-' == oval[0])
			sscanf(oval, "%d", &keyset->oid);
		else
			sscanf(oval, "%u", &keyset->oid);
	}
	else
		keyset->oid = 0;
}

static void AddRollback(StatementClass *stmt, QResultClass *res, SQLLEN index, const KeySet *keyset, Int4 dmlcode)
{
	ConnectionClass	*conn = SC_get_conn(stmt);
	Rollback *rollback;

	if (!CC_is_in_trans(conn))
		return;
MYLOG(DETAIL_LOG_LEVEL, "entering " FORMAT_LEN "(%u,%u) %s\n", index, keyset->blocknum, keyset->offset, dmlcode == SQL_ADD ? "ADD" : (dmlcode == SQL_UPDATE ? "UPDATE" : (dmlcode == SQL_DELETE ? "DELETE" : "REFRESH")));
	if (!res->rollback)
	{
		res->rb_count = 0;
		res->rb_alloc = 10;
		rollback = res->rollback = static_cast<Rollback*>(malloc(sizeof(Rollback) * res->rb_alloc));
		if (!rollback)
		{
			res->rb_alloc = res->rb_count = 0;
			return;
		}
	}
	else
	{
		if (res->rb_count >= res->rb_alloc)
		{
			res->rb_alloc *= 2;
			if (1)
		//	if (rollback = realloc(res->rollback, sizeof(Rollback) * res->rb_alloc), !rollback)
			{
				res->rb_alloc = res->rb_count = 0;
				return;
			}
			res->rollback = rollback;
		}
		rollback = res->rollback + res->rb_count;
	}
	rollback->index = index;
	rollback->option = dmlcode;
	rollback->offset = 0;
	rollback->blocknum = 0;
	rollback->oid = 0;
	if (keyset)
	{
		rollback->blocknum = keyset->blocknum;
		rollback->offset = keyset->offset;
		rollback->oid = keyset->oid;
	}

	conn->result_uncommitted = 1;
	res->rb_count++;
}

SQLLEN ClearCachedRows(TupleField *tuple, int num_fields, SQLLEN num_rows)
{
	SQLLEN	i;

	for (i = 0; i < num_fields * num_rows; i++, tuple++)
	{
		if (tuple->value)
		{
MYLOG(DETAIL_LOG_LEVEL, "freeing tuple[" FORMAT_LEN "][" FORMAT_LEN "].value=%p\n", i / num_fields, i % num_fields, tuple->value);
			free(tuple->value);
			tuple->value = NULL;
		}
		tuple->len = -1;
	}
	return i;
}
SQLLEN ReplaceCachedRows(TupleField *otuple, const TupleField *ituple, int num_fields, SQLLEN num_rows)
{
	SQLLEN	i;

MYLOG(DETAIL_LOG_LEVEL, "entering %p num_fields=%d num_rows=" FORMAT_LEN "\n", otuple, num_fields, num_rows);
	for (i = 0; i < num_fields * num_rows; i++, ituple++, otuple++)
	{
		if (otuple->value)
		{
			free(otuple->value);
			otuple->value = NULL;
		}
		if (ituple->value)
{
			otuple->value = strdup(static_cast<const char*>(ituple->value));
MYLOG(DETAIL_LOG_LEVEL, "[" FORMAT_LEN "," FORMAT_LEN "] %s copied\n", i / num_fields, i % num_fields, (const char *) otuple->value);
}
		if (otuple->value)
			otuple->len = ituple->len;
		else
			otuple->len = -1;
	}
	return i;
}

static
int MoveCachedRows(TupleField *otuple, TupleField *ituple, Int2 num_fields, SQLLEN num_rows)
{
	int	i;

MYLOG(DETAIL_LOG_LEVEL, "entering %p num_fields=%d num_rows=" FORMAT_LEN "\n", otuple, num_fields, num_rows);
	for (i = 0; i < num_fields * num_rows; i++, ituple++, otuple++)
	{
		if (otuple->value)
		{
			free(otuple->value);
			otuple->value = NULL;
		}
		if (ituple->value)
		{
			otuple->value = ituple->value;
			ituple->value = NULL;
MYLOG(DETAIL_LOG_LEVEL, "[%d,%d] %s copied\n", i / num_fields, i % num_fields, (const char *) otuple->value);
		}
		otuple->len = ituple->len;
		ituple->len = -1;
	}
	return i;
}

static const char *
ti_quote(StatementClass *stmt, OID tableoid, char *buf, int buf_size)
{
	TABLE_INFO	*ti = stmt->ti[0];
	pgNAME		rNAME;

	if (0 == tableoid || !TI_has_subclass(ti))
		return quote_table(ti->schema_name, ti->table_name, buf, buf_size);
	else if (NAME_IS_VALID(rNAME = TI_From_IH(ti, tableoid)))
		return SAFE_NAME(rNAME);
	else
	{
		char	query[200];
		QResultClass	*res;
		char	*ret = "";

		SPRINTF_FIXED(query, "select relname, nspname from WD_class c, WD_namespace n where c.oid=%u and c.relnamespace=n.oid", tableoid);
		res = CC_send_query(SC_get_conn(stmt), query, NULL, READ_ONLY_QUERY, stmt);
		if (QR_command_maybe_successful(res) &&
		    QR_get_num_cached_tuples(res) == 1)
		{
			pgNAME	schema_name, table_name;

			SET_NAME_DIRECTLY(schema_name, static_cast<char*>(QR_get_value_backend_text(res, 0, 1)));
			SET_NAME_DIRECTLY(table_name, static_cast<char*>(QR_get_value_backend_text(res, 0, 0)));
			ret = quote_table(schema_name, table_name, buf, buf_size);
			TI_Ins_IH(ti, tableoid, ret);
		}
		QR_Destructor(res);
		return ret;
	}
}

static BOOL	tupleExists(StatementClass *stmt, const KeySet *keyset)
{
	PQExpBufferData	selstr = {0};
	const TABLE_INFO	*ti = stmt->ti[0];
	QResultClass	*res;
	RETCODE		ret = FALSE;
	const char *bestqual = GET_NAME(ti->bestqual);
	char table_fqn[256];

	initPQExpBuffer(&selstr);
	printfPQExpBuffer(&selstr,
			 "select 1 from %s where ctid = '(%u,%u)'",
			 ti_quote(stmt, keyset->oid, table_fqn, sizeof(table_fqn)),
			 keyset->blocknum, keyset->offset);
	if (NULL != bestqual && 0 != keyset->oid && !TI_has_subclass(ti))
	{
		appendPQExpBuffer(&selstr, " and ");
		appendPQExpBuffer(&selstr, bestqual, keyset->oid);
	}
	if (PQExpBufferDataBroken(selstr))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in tupleExists()", __FUNCTION__);
		goto cleanup;
	}
	res = CC_send_query(SC_get_conn(stmt), selstr.data, NULL, READ_ONLY_QUERY, NULL);
	if (QR_command_maybe_successful(res) && 1 == res->num_cached_rows)
		ret = TRUE;
	QR_Destructor(res);
cleanup:
	if (!PQExpBufferDataBroken(selstr))
		termPQExpBuffer(&selstr);
	return ret;
}

static BOOL enlargeAdded(QResultClass *res, UInt4 number, const StatementClass *stmt)
{
	UInt4	alloc;
	int	num_fields = res->num_fields;

	alloc = res->ad_alloc;
	if (0 == alloc)
		alloc = number > 10 ? number : 10;
	else
		while (alloc < number)
		{
			alloc *= 2;
		}

	if (alloc <= res->ad_alloc)
		return TRUE;
	QR_REALLOC_return_with_error(res->added_keyset, KeySet, sizeof(KeySet) * alloc, res, "enlargeAdded failed", FALSE);
	if (SQL_CURSOR_KEYSET_DRIVEN != stmt->options.cursor_type)
		QR_REALLOC_return_with_error(res->added_tuples, TupleField, sizeof(TupleField) * num_fields * alloc, res, "enlargeAdded failed 2", FALSE);
	res->ad_alloc = alloc;
	return TRUE;
}
static void AddAdded(StatementClass *stmt, QResultClass *res, SQLLEN index, const TupleField *tuple_added)
{
	KeySet	*added_keyset, *keyset, keys;
	TupleField	*added_tuples = NULL, *tuple;
	UInt4	ad_count;
	Int2	num_fields;

	if (!res)	return;
	num_fields = res->num_fields;
MYLOG(DETAIL_LOG_LEVEL, "entering index=" FORMAT_LEN ", tuple=%p, num_fields=%d\n", index, tuple_added, num_fields);
	ad_count = res->ad_count;
	res->ad_count++;
	if (QR_get_cursor(res))
		index = -(SQLLEN)res->ad_count;
	if (!tuple_added)
		return;
	KeySetSet(tuple_added, num_fields + res->num_key_fields, res->num_key_fields, &keys, TRUE);
	keys.status = SQL_ROW_ADDED;
	if (CC_is_in_trans(SC_get_conn(stmt)))
		keys.status |= CURS_SELF_ADDING;
	else
		keys.status |= CURS_SELF_ADDED;
	AddRollback(stmt, res, index, &keys, SQL_ADD);

	if (!QR_get_cursor(res))
		return;
	if (ad_count > 0 && 0 == res->ad_alloc)
		return;
	if (!enlargeAdded(res, ad_count + 1, stmt))
		return;
	added_keyset = res->added_keyset;
	added_tuples = res->added_tuples;

	keyset = added_keyset + ad_count;
	*keyset = keys;
	if (added_tuples)
	{
		tuple = added_tuples + num_fields * ad_count;
		memset(tuple, 0, sizeof(TupleField) * num_fields);
		ReplaceCachedRows(tuple, tuple_added, num_fields, 1);
	}
}

static	void RemoveAdded(QResultClass *, SQLLEN);
static	void RemoveUpdated(QResultClass *, SQLLEN);
static	void RemoveUpdatedAfterTheKey(QResultClass *, SQLLEN, const KeySet*);
static	void RemoveDeleted(QResultClass *, SQLLEN);

static	void RemoveAdded(QResultClass *res, SQLLEN index)
{
	SQLLEN	rmidx, mv_count;
	Int2	num_fields = res->num_fields;
	KeySet	*added_keyset;
	TupleField	*added_tuples;

	MYLOG(0, "entering index=" FORMAT_LEN "\n", index);
	if (index < 0)
		rmidx = -index - 1;
	else
		rmidx = index - res->num_total_read;
	if (rmidx >= res->ad_count)
		return;
	added_keyset = res->added_keyset + rmidx;
	added_tuples = res->added_tuples + num_fields * rmidx;
	ClearCachedRows(added_tuples, num_fields, 1);
	mv_count = res->ad_count - rmidx - 1;
	if (mv_count > 0)
	{
		memmove(added_keyset, added_keyset + 1, mv_count * sizeof(KeySet));
		memmove(added_tuples, added_tuples + num_fields, mv_count * num_fields * sizeof(TupleField));
	}
	RemoveDeleted(res, index);
	RemoveUpdated(res, index);
	res->ad_count--;
	MYLOG(0, "removed=1 count=%d\n", res->ad_count);
}

static void
CommitAdded(QResultClass *res)
{
	KeySet	*added_keyset;
	int	i;
	UWORD	status;

	MYLOG(0, "entering res=%p\n", res);
	if (!res || !res->added_keyset)	return;
	added_keyset = res->added_keyset;
	for (i = res->ad_count - 1; i >= 0; i--)
	{
		status = added_keyset[i].status;
		if (0 != (status & CURS_SELF_ADDING))
		{
			status |= CURS_SELF_ADDED;
			status &= ~CURS_SELF_ADDING;
		}
		if (0 != (status & CURS_SELF_UPDATING))
		{
			status |= CURS_SELF_UPDATED;
			status &= ~CURS_SELF_UPDATING;
		}
		if (0 != (status & CURS_SELF_DELETING))
		{
			status |= CURS_SELF_DELETED;
			status &= ~CURS_SELF_DELETING;
		}
		if (status != added_keyset[i].status)
		{
MYLOG(DETAIL_LOG_LEVEL, "!!Commit Added=" FORMAT_ULEN "(%d)\n", QR_get_num_total_read(res) + i, i);
			added_keyset[i].status = status;
		}
	}
}

static int
AddDeleted(QResultClass *res, SQLULEN index, const KeySet *keyset)
{
	int	i;
	Int2	dl_count, new_alloc;
	SQLLEN	*deleted;
	KeySet	*deleted_keyset;
	UWORD	status;

MYLOG(DETAIL_LOG_LEVEL, "entering " FORMAT_ULEN "\n", index);
	dl_count = res->dl_count;
	res->dl_count++;
	if (!QR_get_cursor(res))
		return TRUE;
	if (!res->deleted)
	{
		dl_count = 0;
		new_alloc = 10;
		QR_MALLOC_return_with_error(res->deleted, SQLLEN, sizeof(SQLLEN) * new_alloc, res, "Deleted index malloc error", FALSE);
		QR_MALLOC_return_with_error(res->deleted_keyset, KeySet, sizeof(KeySet) * new_alloc, res, "Deleted keyset malloc error", FALSE);
		deleted = res->deleted;
		deleted_keyset = res->deleted_keyset;
		res->dl_alloc = new_alloc;
	}
	else
	{
		if (dl_count >= res->dl_alloc)
		{
			new_alloc = res->dl_alloc * 2;
			res->dl_alloc = 0;
			QR_REALLOC_return_with_error(res->deleted, SQLLEN, sizeof(SQLLEN) * new_alloc, res, "Deleted index realloc error", FALSE);
			deleted = res->deleted;
			QR_REALLOC_return_with_error(res->deleted_keyset, KeySet, sizeof(KeySet) * new_alloc, res, "Deleted KeySet realloc error", FALSE);
			deleted_keyset = res->deleted_keyset;
			res->dl_alloc = new_alloc;
		}
		/* sort deleted indexes in ascending order */
		for (i = 0, deleted = res->deleted, deleted_keyset = res->deleted_keyset; i < dl_count; i++, deleted++, deleted_keyset++)
		{
			if (index < *deleted)
				break;
		}
		memmove(deleted + 1, deleted, sizeof(SQLLEN) * (dl_count - i));
		memmove(deleted_keyset + 1, deleted_keyset, sizeof(KeySet) * (dl_count - i));
	}
	*deleted = index;
	*deleted_keyset = *keyset;
	status = keyset->status;
	status &= (~KEYSET_INFO_PUBLIC);
	status |= SQL_ROW_DELETED;
	if (CC_is_in_trans(QR_get_conn(res)))
	{
		status |= CURS_SELF_DELETING;
		QR_get_conn(res)->result_uncommitted = 1;
	}
	else
	{
		status &= ~(CURS_SELF_ADDING | CURS_SELF_UPDATING | CURS_SELF_DELETING);
		status |= CURS_SELF_DELETED;
	}
	deleted_keyset->status = status;
	res->dl_count = dl_count + 1;

	return TRUE;
}

static void
RemoveDeleted(QResultClass *res, SQLLEN index)
{
	int	i, mv_count, rm_count = 0;
	SQLLEN	pidx, midx;
	SQLLEN	*deleted, num_read = QR_get_num_total_read(res);
	KeySet	*deleted_keyset;

	MYLOG(0, "entering index=" FORMAT_LEN "\n", index);
	if (index < 0)
	{
		midx = index;
		pidx = num_read - index - 1;
	}
	else
	{
		pidx = index;
		if (index >= num_read)
			midx = num_read - index - 1;
		else
			midx = index;
	}
	for (i = 0; i < res->dl_count; i++)
	{
		if (pidx == res->deleted[i] ||
		    midx == res->deleted[i])
		{
			mv_count = res->dl_count - i - 1;
			if (mv_count > 0)
			{
				deleted = res->deleted + i;
				deleted_keyset = res->deleted_keyset + i;
				memmove(deleted, deleted + 1, mv_count * sizeof(SQLLEN));
				memmove(deleted_keyset, deleted_keyset + 1, mv_count * sizeof(KeySet));
			}
			res->dl_count--;
			rm_count++;
		}
	}
	MYLOG(0, "removed count=%d,%d\n", rm_count, res->dl_count);
}

static void
CommitDeleted(QResultClass *res)
{
	int	i;
	SQLLEN	*deleted;
	KeySet	*deleted_keyset;
	UWORD	status;

	if (!res->deleted)
		return;

	for (i = 0, deleted = res->deleted, deleted_keyset = res->deleted_keyset; i < res->dl_count; i++, deleted++, deleted_keyset++)
	{
		status = deleted_keyset->status;
		if (0 != (status & CURS_SELF_ADDING))
		{
			status |= CURS_SELF_ADDED;
			status &= ~CURS_SELF_ADDING;
		}
		if (0 != (status & CURS_SELF_UPDATING))
		{
			status |= CURS_SELF_UPDATED;
			status &= ~CURS_SELF_UPDATING;
		}
		if (0 != (status & CURS_SELF_DELETING))
		{
			status |= CURS_SELF_DELETED;
			status &= ~CURS_SELF_DELETING;
		}
		if (status != deleted_keyset->status)
		{
MYLOG(DETAIL_LOG_LEVEL, "Deleted=" FORMAT_LEN "(%d)\n", *deleted, i);
			deleted_keyset->status = status;
		}
	}
}

static BOOL
enlargeUpdated(QResultClass *res, Int4 number, const StatementClass *stmt)
{
	Int2	alloc;

	alloc = res->up_alloc;
	if (0 == alloc)
		alloc = number > 10 ? number : 10;
	else
		while (alloc < number)
		{
			alloc *= 2;
		}
	if (alloc <= res->up_alloc)
		return TRUE;

	QR_REALLOC_return_with_error(res->updated, SQLLEN, sizeof(SQLLEN) * alloc, res, "enlargeUpdated failed", FALSE);
	QR_REALLOC_return_with_error(res->updated_keyset, KeySet, sizeof(KeySet) * alloc, res, "enlargeUpdated failed 2", FALSE);
	if (SQL_CURSOR_KEYSET_DRIVEN != stmt->options.cursor_type)
		QR_REALLOC_return_with_error(res->updated_tuples, TupleField, sizeof(TupleField) * res->num_fields * alloc, res, "enlargeUpdated failed 3", FALSE);
	res->up_alloc = alloc;

	return TRUE;
}

static void
AddUpdated(StatementClass *stmt, SQLLEN index, const KeySet *keyset, const TupleField *tuple_updated)
{
	QResultClass	*res;
	SQLLEN	*updated;
	KeySet	*updated_keyset;
	TupleField	*updated_tuples = NULL,  *tuple;
	/* SQLLEN	res_ridx; */
	UInt2	up_count;
	BOOL	is_in_trans;
	SQLLEN	upd_idx, upd_add_idx;
	Int2	num_fields;
	int	i;
	UWORD	status;

MYLOG(DETAIL_LOG_LEVEL, "entering index=" FORMAT_LEN "\n", index);
	if (!stmt)	return;
	if (res = SC_get_Curres(stmt), !res)	return;
	if (!keyset)	return;
	if (!QR_get_cursor(res))	return;
	up_count = res->up_count;
	if (up_count > 0 && 0 == res->up_alloc)	return;
	num_fields = res->num_fields;
	/*
	res_ridx = GIdx2CacheIdx(index, stmt, res);
	if (res_ridx >= 0 && res_ridx < QR_get_num_cached_tuples(res))
		tuple_updated = res->backend_tuples + res_ridx * num_fields;
	*/
	if (!tuple_updated)
		return;
	upd_idx = -1;
	upd_add_idx = -1;
	updated = res->updated;
	is_in_trans = CC_is_in_trans(SC_get_conn(stmt));
	updated_keyset = res->updated_keyset;
	status = keyset->status;
	status &= (~KEYSET_INFO_PUBLIC);
	status |= SQL_ROW_UPDATED;
	if (is_in_trans)
		status |= CURS_SELF_UPDATING;
	else
	{
		for (i = up_count - 1; i >= 0; i--)
		{
			if (updated[i] == index)
				break;
		}
		if (i >= 0)
			upd_idx = i;
		else
		{
			SQLLEN	num_totals = QR_get_num_total_tuples(res);
			if (index >= num_totals)
				upd_add_idx = num_totals - index;
		}
		status |= CURS_SELF_UPDATED;
		status &= ~(CURS_SELF_ADDING | CURS_SELF_UPDATING | CURS_SELF_DELETING);
	}

	tuple = NULL;
	/* update the corresponding add(updat)ed info */
	if (upd_add_idx >= 0)
	{
		res->added_keyset[upd_add_idx].status = status;
		if (res->added_tuples)
		{
			tuple = res->added_tuples + num_fields * upd_add_idx;
			ClearCachedRows(tuple, num_fields, 1);
		}
	}
	else if (upd_idx >= 0)
	{
		res->updated_keyset[upd_idx].status = status;
		if (res->updated_tuples)
		{
			tuple = res->added_tuples + num_fields * upd_add_idx;
			ClearCachedRows(tuple, num_fields, 1);
		}
	}
	else
	{
		if (!enlargeUpdated(res, res->up_count + 1, stmt))
			return;
		updated = res->updated;
		updated_keyset = res->updated_keyset;
		updated_tuples = res->updated_tuples;
		upd_idx = up_count;
		updated[up_count] = index;
		updated_keyset[up_count] = *keyset;
		updated_keyset[up_count].status = status;
		if (updated_tuples)
		{
			tuple = updated_tuples + num_fields * up_count;
			memset(tuple, 0, sizeof(TupleField) * num_fields);
		}
		res->up_count++;
	}

	if (tuple)
		ReplaceCachedRows(tuple, tuple_updated, num_fields, 1);
	if (is_in_trans)
		SC_get_conn(stmt)->result_uncommitted = 1;
	MYLOG(0, "up_count=%d\n", res->up_count);
}

static void
RemoveUpdated(QResultClass *res, SQLLEN index)
{
	MYLOG(0, "entering index=" FORMAT_LEN "\n", index);
	RemoveUpdatedAfterTheKey(res, index, NULL);
}

static void
RemoveUpdatedAfterTheKey(QResultClass *res, SQLLEN index, const KeySet *keyset)
{
	SQLLEN	*updated, num_read = QR_get_num_total_read(res);
	KeySet	*updated_keyset;
	TupleField	*updated_tuples = NULL;
	SQLLEN	pidx, midx, mv_count;
	int	i, num_fields = res->num_fields, rm_count = 0;

	MYLOG(0, "entering " FORMAT_LEN ",(%u,%u)\n", index, keyset ? keyset->blocknum : 0, keyset ? keyset->offset : 0);
	if (index < 0)
	{
		midx = index;
		pidx = num_read - index - 1;
	}
	else
	{
		pidx = index;
		if (index >= num_read)
			midx = num_read - index - 1;
		else
			midx = index;
	}
	for (i = 0; i < res->up_count; i++)
	{
		updated = res->updated + i;
		if (pidx == *updated ||
		    midx == *updated)
		{
			updated_keyset = res->updated_keyset + i;
			if (keyset &&
			    updated_keyset->blocknum == keyset->blocknum &&
			    updated_keyset->offset == keyset->offset)
				break;
			updated_tuples = NULL;
			if (res->updated_tuples)
			{
				updated_tuples = res->updated_tuples + i * num_fields;
				ClearCachedRows(updated_tuples, num_fields, 1);
			}
			mv_count = res->up_count - i -1;
			if (mv_count > 0)
			{
				memmove(updated, updated + 1, sizeof(SQLLEN) * mv_count);
				memmove(updated_keyset, updated_keyset + 1, sizeof(KeySet) * mv_count);
				if (updated_tuples)
					memmove(updated_tuples, updated_tuples + num_fields, sizeof(TupleField) * num_fields * mv_count);
			}
			res->up_count--;
			rm_count++;
		}
	}
	MYLOG(0, "removed count=%d,%d\n", rm_count, res->up_count);
}

static void
CommitUpdated(QResultClass *res)
{
	KeySet	*updated_keyset;
	int	i;
	UWORD	status;

	MYLOG(0, "entering res=%p\n", res);
	if (!res)	return;
	if (!QR_get_cursor(res))
		return;
	if (res->up_count <= 0)
		return;
	if (updated_keyset = res->updated_keyset, !updated_keyset)
		return;
	for (i = res->up_count - 1; i >= 0; i--)
	{
		status = updated_keyset[i].status;
		if (0 != (status & CURS_SELF_UPDATING))
		{
			status &= ~CURS_SELF_UPDATING;
			status |= CURS_SELF_UPDATED;
		}
		if (0 != (status & CURS_SELF_ADDING))
		{
			status &= ~CURS_SELF_ADDING;
			status |= CURS_SELF_ADDED;
		}
		if (0 != (status & CURS_SELF_DELETING))
		{
			status &= ~CURS_SELF_DELETING;
			status |= CURS_SELF_DELETED;
		}
		if (status != updated_keyset[i].status)
		{
MYLOG(DETAIL_LOG_LEVEL, "!!Commit Updated=" FORMAT_LEN "(%d)\n", res->updated[i], i);
			updated_keyset[i].status = status;
		}
	}
}


static void
DiscardRollback(StatementClass *stmt, QResultClass *res)
{
	int	i;
	SQLLEN	index, kres_ridx;
	UWORD	status;
	Rollback *rollback;
	KeySet	*keyset;
	BOOL	kres_is_valid;

MYLOG(DETAIL_LOG_LEVEL, "entering\n");
	if (QR_get_cursor(res))
	{
		CommitAdded(res);
		CommitUpdated(res);
		CommitDeleted(res);
		return;
	}

	if (0 == res->rb_count || NULL == res->rollback)
		return;
	rollback = res->rollback;
	keyset = res->keyset;
	for (i = 0; i < res->rb_count; i++)
	{
		index = rollback[i].index;
		status = 0;
		kres_is_valid = FALSE;
		if (index >= 0)
		{
			kres_ridx = GIdx2KResIdx(index, stmt, res);
			if (kres_ridx >= 0 && kres_ridx < res->num_cached_keys)
			{
				kres_is_valid = TRUE;
				status = keyset[kres_ridx].status;
			}
		}
		if (kres_is_valid)
		{
			keyset[kres_ridx].status &= ~(CURS_SELF_DELETING | CURS_SELF_UPDATING | CURS_SELF_ADDING);
			keyset[kres_ridx].status |= ((status & (CURS_SELF_DELETING | CURS_SELF_UPDATING | CURS_SELF_ADDING)) << 3);
		}
	}
	free(rollback);
	res->rollback = NULL;
	res->rb_count = res->rb_alloc = 0;
}

static QResultClass *positioned_load(StatementClass *stmt, UInt4 flag, const UInt4 *oidint, const char *tid);

static void
UndoRollback(StatementClass *stmt, QResultClass *res, BOOL partial)
{
	Int4	i, rollbp;
	SQLLEN	index, ridx, kres_ridx;
	UWORD	status;
	Rollback *rollback;
	KeySet	*keyset, keys, *wkey = NULL;
	BOOL	curs = (NULL != QR_get_cursor(res)), texist, kres_is_valid;

	if (0 == res->rb_count || NULL == res->rollback)
		return;
	rollback = res->rollback;
	keyset = res->keyset;

	rollbp = 0;
	if (partial)
	{
		SQLLEN	pidx, midx;
		int		rollbps;
		int		doubtp;
		int	j;

		MYLOG(DETAIL_LOG_LEVEL, " ");
		for (i = 0, doubtp = 0; i < res->rb_count; i++)
		{
			keys.status = 0;
			keys.blocknum = rollback[i].blocknum;
			keys.offset = rollback[i].offset;
			keys.oid = rollback[i].oid;
			texist = tupleExists(stmt, &keys);
MYPRINTF(DETAIL_LOG_LEVEL, "texist[%d]=%d", i, texist);
			if (SQL_ADD == rollback[i].option)
			{
				if (texist)
					doubtp = i + 1;
			}
			else if (SQL_REFRESH == rollback[i].option)
			{
				if (texist || doubtp == i)
					doubtp = i + 1;
			}
			else
			{
				if (texist)
					break;
				if (doubtp == i)
					doubtp = i + 1;
			}
MYPRINTF(DETAIL_LOG_LEVEL, " (doubtp=%d)", doubtp);
		}
		rollbp = i;
MYPRINTF(DETAIL_LOG_LEVEL, " doubtp=%d,rollbp=%d\n", doubtp, rollbp);
		do
		{
			rollbps = rollbp;
			for (i = doubtp; i < rollbp; i++)
			{
				index = rollback[i].index;
				if (SQL_ADD == rollback[i].option)
				{
MYLOG(DETAIL_LOG_LEVEL, "index[%d]=" FORMAT_LEN "\n", i, index);
					if (index < 0)
					{
						midx = index;
						pidx = res->num_total_read - index - 1;
					}
					else
					{
						pidx = index;
						midx = res->num_total_read - index - 1;
					}
MYLOG(DETAIL_LOG_LEVEL, "pidx=" FORMAT_LEN ",midx=" FORMAT_LEN "\n", pidx, midx);
					for (j = rollbp - 1; j > i; j--)
					{
						if (rollback[j].index == midx ||
						    rollback[j].index == pidx)
						{
							if (SQL_DELETE == rollback[j].option)
							{
MYLOG(DETAIL_LOG_LEVEL, "delete[%d].index=" FORMAT_LEN "\n", j, rollback[j].index);
								break;
							}
							/*else if (SQL_UPDATE == rollback[j].option)
							{
MYLOG(DETAIL_LOG_LEVEL, "update[%d].index=%d\n", j, rollback[j].index);
								if (IndexExists(stmt, res, rollback + j))
									break;
							}*/
						}
					}
					if (j <= i)
					{
						rollbp = i;
						break;
					}
				}
			}
		} while (rollbp < rollbps);
	}
MYLOG(DETAIL_LOG_LEVEL, "rollbp=%d\n", rollbp);

	for (i = res->rb_count - 1; i >= rollbp; i--)
	{
MYLOG(DETAIL_LOG_LEVEL, "do %d(%d)\n", i, rollback[i].option);
		index = rollback[i].index;
		if (curs)
		{
			if (SQL_ADD == rollback[i].option)
				RemoveAdded(res, index);
			RemoveDeleted(res, index);
			keys.status = 0;
			keys.blocknum = rollback[i].blocknum;
			keys.offset = rollback[i].offset;
			keys.oid = rollback[i].oid;
			/* RemoveUpdatedAfterTheKey(res, index, &keys); is no longer needed? */
			RemoveUpdated(res, index);
		}
		status = 0;
		kres_is_valid = FALSE;
		if (index >= 0)
		{
			kres_ridx = GIdx2KResIdx(index, stmt, res);
			if (kres_ridx >= 0 && kres_ridx < res->num_cached_keys)
			{
				kres_is_valid = TRUE;
				wkey = keyset + kres_ridx;
				status = wkey->status;
			}
		}
MYLOG(DETAIL_LOG_LEVEL, " index=" FORMAT_LEN " status=%hx", index, status);
		if (kres_is_valid)
		{
			QResultClass	*qres;
			Int2		num_fields = res->num_fields;

			ridx = GIdx2CacheIdx(index, stmt, res);
			if (SQL_ADD == rollback[i].option)
			{
				if (ridx >=0 && ridx < res->num_cached_rows)
				{
					TupleField *tuple = res->backend_tuples + res->num_fields * ridx;
					ClearCachedRows(tuple, res->num_fields, 1);
					res->num_cached_rows--;
				}
				res->num_cached_keys--;
				if (!curs)
					res->ad_count--;
			}
			else if (SQL_REFRESH == rollback[i].option)
				continue;
			else
			{
MYPRINTF(DETAIL_LOG_LEVEL, " (%u, %u)", wkey->blocknum,  wkey->offset);
				wkey->blocknum = rollback[i].blocknum;
				wkey->offset = rollback[i].offset;
				wkey->oid = rollback[i].oid;
MYPRINTF(DETAIL_LOG_LEVEL, "->(%u, %u)", wkey->blocknum, wkey->offset);
				wkey->status &= ~KEYSET_INFO_PUBLIC;
				if (SQL_DELETE == rollback[i].option)
					wkey->status &= ~CURS_SELF_DELETING;
				else if (SQL_UPDATE == rollback[i].option)
					wkey->status &= ~CURS_SELF_UPDATING;
				wkey->status |= CURS_NEEDS_REREAD;
				if (ridx >=0 && ridx < res->num_cached_rows)
				{
					char	tidval[32];
					Oid	oid = wkey->oid;

					SPRINTF_FIXED(tidval,
							 "(%u,%u)", wkey->blocknum, wkey->offset);
					//qres = positioned_load(stmt, 0, &oid, tidval);
					if (QR_command_maybe_successful(qres) &&
					    QR_get_num_cached_tuples(qres) == 1)
					{
						MoveCachedRows(res->backend_tuples + num_fields * ridx, qres->backend_tuples, num_fields, 1);
						wkey->status &= ~CURS_NEEDS_REREAD;
					}
					QR_Destructor(qres);
				}
			}
		}
	}
	MYPRINTF(DETAIL_LOG_LEVEL, "\n");
	res->rb_count = rollbp;
	if (0 == rollbp)
	{
		free(rollback);
		res->rollback = NULL;
		res->rb_alloc = 0;
	}
}

void
ProcessRollback(ConnectionClass *conn, BOOL undo, BOOL partial)
{
	int	i;
	StatementClass	*stmt;
	QResultClass	*res;

	for (i = 0; i < conn->num_stmts; i++)
	{
		if (stmt = conn->stmts[i], !stmt)
			continue;
		for (res = SC_get_Result(stmt); res; res = QR_nextr(res))
		{
			if (undo)
				UndoRollback(stmt, res, partial);
			else
				DiscardRollback(stmt, res);
		}
	}
}


#define	LATEST_TUPLE_LOAD	1L
#define	USE_INSERTED_TID	(1L << 1)
static QResultClass *
positioned_load(StatementClass *stmt, UInt4 flag, const UInt4 *oidint, const char *tidval)
{
	CSTR	func = "positioned_load";
	QResultClass *qres = NULL;
	PQExpBufferData	selstr = {0};
	BOOL	latest = ((flag & LATEST_TUPLE_LOAD) != 0);
	TABLE_INFO	*ti = stmt->ti[0];
	const char *bestqual = GET_NAME(ti->bestqual);
	const ssize_t	from_pos = stmt->load_from_pos;
	const char *load_stmt = stmt->load_statement;

MYLOG(DETAIL_LOG_LEVEL, "entering bestitem=%s bestqual=%s\n", SAFE_NAME(ti->bestitem), SAFE_NAME(ti->bestqual));
	initPQExpBuffer(&selstr);
#define	return	DONT_CALL_RETURN_FROM_HERE???
	if (TI_has_subclass(ti))
	{
		OID	tableoid = *oidint;
		char	table_fqn[256];
		const char	*quoted_table;

		quoted_table = ti_quote(stmt, tableoid, table_fqn, sizeof(table_fqn));
		if (tidval)
		{
			if (latest)
			{
				printfPQExpBuffer(&selstr,
					 "%.*sfrom %s where ctid = (select currtid2('%s', '%s'))",
					 (int) from_pos, load_stmt,
					 quoted_table,
					 quoted_table,
					 tidval);
			}
			else
				printfPQExpBuffer(&selstr, "%.*sfrom %s where ctid = '%s'", (int) from_pos, load_stmt, quoted_table, tidval);
		}
		else if ((flag & USE_INSERTED_TID) != 0)
			printfPQExpBuffer(&selstr, "%.*sfrom %s where ctid = (select currtid(0, '(0,0)'))", (int) from_pos, load_stmt, quoted_table);
		/*
		else if (bestitem && oidint)
		{
		}
		*/
		else
		{
			SC_set_error(stmt,STMT_INTERNAL_ERROR, "can't find added and updating row because of the lack of oid", func);
			goto cleanup;
		}
	}
	else
	{
		CSTR	andqual = " and ";
		BOOL	andExist = TRUE;

		if (tidval)
		{
			if (latest)
			{
				char table_fqn[256];

				printfPQExpBuffer(&selstr,
					 "%s where ctid = (select currtid2('%s', '%s'))",
					 load_stmt,
					 ti_quote(stmt, 0, table_fqn, sizeof(table_fqn)),
					 tidval);
			}
			else
				printfPQExpBuffer(&selstr, "%s where ctid = '%s'", load_stmt, tidval);
		}
		else if ((flag & USE_INSERTED_TID) != 0)
			printfPQExpBuffer(&selstr, "%s where ctid = (select currtid(0, '(0,0)'))", load_stmt);
		else if (bestqual)
		{
			andExist = FALSE;
			printfPQExpBuffer(&selstr, "%s where ", load_stmt);
		}
		else
		{
			SC_set_error(stmt,STMT_INTERNAL_ERROR, "can't find added and updating row because of the lack of oid", func);
			goto cleanup;
		}
		if (bestqual && oidint)
		{
			if (andExist)
				appendPQExpBufferStr(&selstr, andqual);
			appendPQExpBuffer(&selstr, bestqual, *oidint);
		}
	}

	if (PQExpBufferDataBroken(selstr))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Could not allocate memory positioned_load()", func);
		goto cleanup;
	}
	MYLOG(0, "selstr=%s\n", selstr.data);
	qres = CC_send_query(SC_get_conn(stmt), selstr.data, NULL, READ_ONLY_QUERY, stmt);
cleanup:
#undef	return
	if (!PQExpBufferDataBroken(selstr))
		termPQExpBuffer(&selstr);
	return qres;
}

BOOL QR_get_last_bookmark(const QResultClass *res, Int4 index, KeySet *keyset)
{
	int	i;

	if (res->dl_count > 0 && res->deleted)
	{
		for (i = 0; i < res->dl_count; i++)
		{
			if (res->deleted[i] == index)
			{
				*keyset = res->deleted_keyset[i];
				return TRUE;
			}
			if (res->deleted[i] > index)
				break;
		}
	}
	if (res->up_count > 0 && res->updated)
	{
		for (i = res->up_count - 1; i >= 0; i--)
		{
			if (res->updated[i] == index)
			{
				*keyset = res->updated_keyset[i];
				return TRUE;
			}
		}
	}
	return FALSE;
}

static RETCODE
SC_pos_reload_with_key(StatementClass *stmt, SQLULEN global_ridx, UInt2 *count, Int4 logKind, const KeySet *keyset)
{
	CSTR		func = "SC_pos_reload_with_key";
	int		res_cols;
	UInt2		rcnt;
	SQLLEN		kres_ridx;
	OID		oidint;
	QResultClass	*res, *qres;
	IRDFields	*irdflds = SC_get_IRDF(stmt);
	RETCODE		ret = SQL_ERROR;
	char		tidval[32];
	BOOL		use_ctid = TRUE;
	BOOL		idx_exist = TRUE;

	MYLOG(0, "entering fi=%p ti=%p\n", irdflds->fi, stmt->ti);
	rcnt = 0;
	if (count)
		*count = 0;
	if (!(res = SC_get_Curres(stmt)))
	{
		SC_set_error(stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_pos_reload.", func);
		return SQL_ERROR;
	}

	kres_ridx = GIdx2KResIdx(global_ridx, stmt, res);
	if (kres_ridx < 0 || kres_ridx >= res->num_cached_keys)
	{
		if (NULL == keyset || keyset->offset == 0)
		{
			SC_set_error(stmt, STMT_ROW_OUT_OF_RANGE, "the target keys are out of the rowset", func);
			return SQL_ERROR;
		}
		idx_exist = FALSE;
	}
	else if (0 != (res->keyset[kres_ridx].status & CURS_SELF_ADDING))
	{
		if (NULL == keyset || keyset->offset == 0)
		{
			use_ctid = FALSE;
			MYLOG(0, "The tuple is currently being added and can't use ctid\n");
		}
	}

	if (SC_update_not_ready(stmt))
		parse_statement(stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(stmt))
	{
		stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", func);
		return SQL_ERROR;
	}
	/* old oid & tid */
	if (idx_exist)
	{
		UInt4	blocknum;
		UInt2	offset;

		if (!(oidint = getOid(res, kres_ridx)))
		{
			if (!strcmp(SAFE_NAME(stmt->ti[0]->bestitem), OID_NAME))
			{
				SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the row was already deleted ?", func);
				return SQL_SUCCESS_WITH_INFO;
			}
		}
		getTid(res, kres_ridx, &blocknum, &offset);
		SPRINTF_FIXED(tidval, "(%u, %u)", blocknum, offset);
	}
	res_cols = getNumResultCols(res);
	if (keyset) /* after or update */
	{
		char tid[32];

		oidint = keyset->oid;
		SPRINTF_FIXED(tid, "(%u,%hu)", keyset->blocknum, keyset->offset);
		qres = positioned_load(stmt, 0, &oidint, tid);
	}
	else
	{
		if (use_ctid)
			qres = positioned_load(stmt, LATEST_TUPLE_LOAD, &oidint, tidval);
		else
			qres = positioned_load(stmt, 0, &oidint, NULL);
		keyset = res->keyset + kres_ridx;
	}
	if (!QR_command_maybe_successful(qres))
	{
		ret = SQL_ERROR;
		SC_replace_error_with_res(stmt, STMT_ERROR_TAKEN_FROM_BACKEND, "positioned_load failed", qres, TRUE);
	}
	else if (rcnt = (UInt2) QR_get_num_cached_tuples(qres), rcnt == 1)
	{
		SQLLEN		res_ridx;

		switch (logKind)
		{
			case 0:
			case SQL_FETCH_BY_BOOKMARK:
				break;
			case SQL_UPDATE:
				AddUpdated(stmt, global_ridx, keyset, qres->tupleField);
				break;
			default:
				AddRollback(stmt, res, global_ridx, keyset, logKind);
		}
		res_ridx = GIdx2CacheIdx(global_ridx, stmt, res);
		if (res_ridx >= 0 && res_ridx < QR_get_num_cached_tuples(res))
		{
			TupleField *tuple_old, *tuple_new;
			int	effective_fields = res_cols;

			tuple_old = res->backend_tuples + res->num_fields * res_ridx;

			QR_set_position(qres, 0);
			tuple_new = qres->tupleField;
			if (SQL_CURSOR_KEYSET_DRIVEN == stmt->options.cursor_type &&
				strcmp(static_cast<const char*>(tuple_new[qres->num_fields - res->num_key_fields].value), tidval))
				res->keyset[kres_ridx].status |= SQL_ROW_UPDATED;
			KeySetSet(tuple_new, qres->num_fields, res->num_key_fields, res->keyset + kres_ridx, FALSE);
			MoveCachedRows(tuple_old, tuple_new, effective_fields, 1);
		}
		if (rcnt > 1)
		{
			ret = SQL_SUCCESS_WITH_INFO;
			SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "more than one row were update/deleted?", func);
		}
		else
			ret = SQL_SUCCESS;
	}
	else
	{
		SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the content was deleted after last fetch", func);
		ret = SQL_SUCCESS_WITH_INFO;
		AddRollback(stmt, res, global_ridx, keyset, logKind);
		if (idx_exist)
		{
			if (stmt->options.cursor_type == SQL_CURSOR_KEYSET_DRIVEN)
			{
				res->keyset[kres_ridx].status |= SQL_ROW_DELETED;
			}
		}
	}
	QR_Destructor(qres);
	if (count)
		*count = rcnt;
	return ret;
}


RETCODE
SC_pos_reload(StatementClass *stmt, SQLULEN global_ridx, UInt2 *count, Int4 logKind)
{
	return SC_pos_reload_with_key(stmt, global_ridx, count, logKind, NULL);
}

static	const int	pre_fetch_count = 32;
static SQLLEN LoadFromKeyset(StatementClass *stmt, QResultClass * res, int rows_per_fetch, SQLLEN limitrow)
{
	CSTR	func = "LoadFromKeyset";
	ConnectionClass	*conn = SC_get_conn(stmt);
	SQLLEN	i;
	int	j, rowc, rcnt = 0;
	OID	oid;
	UInt4	blocknum;
	SQLLEN	kres_ridx;
	UInt2	offset;
	PQExpBufferData	qval = {0};
	int	keys_per_fetch = 10;

#define	return	DONT_CALL_RETURN_FROM_HERE???
	for (i = SC_get_rowset_start(stmt), kres_ridx = GIdx2KResIdx(i, stmt, res), rowc = 0;; i++, kres_ridx++)
	{
		if (i >= limitrow)
		{
			if (!rowc)
				break;
			if (res->reload_count > 0)
			{
				for (j = rowc; j < keys_per_fetch; j++)
				{
					appendPQExpBufferStr(&qval, j ? ",NULL" : "NULL");
				}
			}
			rowc = -1; /* end of loop */
		}
		if (rowc < 0 || rowc >= keys_per_fetch)
		{
			QResultClass	*qres;

			appendPQExpBufferStr(&qval, ")");
			if (PQExpBufferDataBroken(qval))
			{
				rcnt = -1;
				SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in LoadFromKeyset()", func);
				goto cleanup;
			}
			qres = CC_send_query(conn, qval.data, NULL, CREATE_KEYSET | READ_ONLY_QUERY, stmt);
			if (QR_command_maybe_successful(qres))
			{
				SQLLEN		j, k, l;
				Int2		m;
				TupleField	*tuple, *tuplew;
				UInt4		bln;
				UInt2		off;

				for (j = 0; j < QR_get_num_total_read(qres); j++)
				{
					oid = getOid(qres, j);
					getTid(qres, j, &blocknum, &offset);
					for (k = SC_get_rowset_start(stmt); k < limitrow; k++)
					{
						getTid(res, k, &bln, &off);
						if (oid == getOid(res, k) &&
						    bln == blocknum &&
						    off == offset)
						{
							l = GIdx2CacheIdx(k, stmt, res);
							tuple = res->backend_tuples + res->num_fields * l;
							tuplew = qres->backend_tuples + qres->num_fields * j;
							for (m = 0; m < res->num_fields; m++, tuple++, tuplew++)
							{
								if (tuple->len > 0 && tuple->value)
									free(tuple->value);
								tuple->value = tuplew->value;
								tuple->len = tuplew->len;
								tuplew->value = NULL;
								tuplew->len = -1;
							}
							res->keyset[k].status &= ~CURS_NEEDS_REREAD;
							break;
						}
					}
				}
			}
			else
			{
				SC_set_error(stmt, STMT_EXEC_ERROR, "Data Load Error", func);
				rcnt = -1;
				QR_Destructor(qres);
				break;
			}
			QR_Destructor(qres);
			if (rowc < 0)
				break;
			rowc = 0;
		}
		if (!rowc)
		{
			if (PQExpBufferDataBroken(qval))
			{
				initPQExpBuffer(&qval);

				if (res->reload_count > 0)
					keys_per_fetch = res->reload_count;
				else
				{
					char	planname[32];
					int	j;
					QResultClass	*qres;

					if (rows_per_fetch >= pre_fetch_count * 2)
						keys_per_fetch = pre_fetch_count;
					else
						keys_per_fetch = rows_per_fetch;
					if (!keys_per_fetch)
						keys_per_fetch = 2;
					SPRINTF_FIXED(planname, "_KEYSET_%p", res);
					printfPQExpBuffer(&qval, "PREPARE \"%s\"", planname);
					for (j = 0; j < keys_per_fetch; j++)
					{
						appendPQExpBufferStr(&qval, j ? ",tid" : "(tid");
					}
					appendPQExpBuffer(&qval, ") as %s where ctid in ", stmt->load_statement);
					for (j = 0; j < keys_per_fetch; j++)
					{
						if (j == 0)
							appendPQExpBufferStr(&qval, "($1");
						else
							appendPQExpBuffer(&qval, ",$%d", j + 1);
					}
					appendPQExpBufferStr(&qval, ")");
					if (PQExpBufferDataBroken(qval))
					{
						rcnt = -1;
						SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in LoadFromKeyset()", func);
						goto cleanup;
					}
					qres = CC_send_query(conn, qval.data, NULL, READ_ONLY_QUERY, stmt);
					if (QR_command_maybe_successful(qres))
					{
						res->reload_count = keys_per_fetch;
					}
					else
					{
						SC_set_error(stmt, STMT_EXEC_ERROR, "Prepare for Data Load Error", func);
						rcnt = -1;
						SC_set_Result(stmt, qres);
						break;
					}
					QR_Destructor(qres);
				}
			}
			if (res->reload_count > 0)
			{
				printfPQExpBuffer(&qval, "EXECUTE \"_KEYSET_%p\"(", res);
			}
			else
			{
				printfPQExpBuffer(&qval, "%s where ctid in (", stmt->load_statement);
			}
		}
		if (rcnt >= 0 &&
		    0 != (res->keyset[kres_ridx].status & CURS_NEEDS_REREAD))
		{
			getTid(res, kres_ridx, &blocknum, &offset);
			if (rowc)
				appendPQExpBuffer(&qval, ",'(%u,%u)'", blocknum, offset);
			else
				appendPQExpBuffer(&qval, "'(%u,%u)'", blocknum, offset);
			rowc++;
			rcnt++;
		}
	}
cleanup:
#undef	return
	if (!PQExpBufferDataBroken(qval))
		termPQExpBuffer(&qval);
	return rcnt;
}

static SQLLEN LoadFromKeyset_inh(StatementClass *stmt, QResultClass * res, int rows_per_fetch, SQLLEN limitrow)
{
	ConnectionClass	*conn = SC_get_conn(stmt);
	SQLLEN	i;
	int	j, rowc, rcnt = 0;
	OID	oid, new_oid;
	UInt4	blocknum;
	SQLLEN	kres_ridx;
	UInt2	offset;
	PQExpBufferData	qval = {0};
	int	keys_per_fetch = 10;
	const char *load_stmt = stmt->load_statement;
	const ssize_t	from_pos = stmt->load_from_pos;

MYLOG(0, "entering in rows_per_fetch=%d limitrow=" FORMAT_LEN "\n", rows_per_fetch, limitrow);
	new_oid = 0;
#define	return	DONT_CALL_RETURN_FROM_HERE???
	for (i = SC_get_rowset_start(stmt), kres_ridx = GIdx2KResIdx(i, stmt, res), rowc = 0, oid = 0;; i++, kres_ridx++)
	{
		if (i >= limitrow)
		{
			if (!rowc)
				break;
			rowc = -1; /* end of loop */
		}
		else if (0 == (res->keyset[kres_ridx].status & CURS_NEEDS_REREAD))
			continue;
		else
			new_oid = getOid(res, kres_ridx);
		if (rowc < 0 ||
		    rowc >= keys_per_fetch ||
		    (oid != 0 &&
		     new_oid != oid))
		{
			QResultClass	*qres;

			appendPQExpBufferStr(&qval, ")");
			if (PQExpBufferDataBroken(qval))
			{
				rcnt = -1;
				SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in LoadFromKeyset_inh()", __FUNCTION__);
				goto cleanup;
			}
			qres = CC_send_query(conn, qval.data, NULL, CREATE_KEYSET | READ_ONLY_QUERY, stmt);
			if (QR_command_maybe_successful(qres))
			{
				SQLLEN		k, l;
				Int2		m;
				TupleField	*tuple, *tuplew;
				UInt4		bln;
				UInt2		off;
				OID		tbloid;

				for (j = 0; j < QR_get_num_total_read(qres); j++)
				{
					tbloid = getOid(qres, j);
					getTid(qres, j, &blocknum, &offset);
					for (k = SC_get_rowset_start(stmt); k < limitrow; k++)
					{
						getTid(res, k, &bln, &off);
						if (tbloid == getOid(res, k) &&
						    bln == blocknum &&
						    off == offset)
						{
							l = GIdx2CacheIdx(k, stmt, res);
							tuple = res->backend_tuples + res->num_fields * l;
							tuplew = qres->backend_tuples + qres->num_fields * j;
							for (m = 0; m < res->num_fields; m++, tuple++, tuplew++)
							{
								if (tuple->len > 0 && tuple->value)
									free(tuple->value);
								tuple->value = tuplew->value;
								tuple->len = tuplew->len;
								tuplew->value = NULL;
								tuplew->len = -1;
							}
							res->keyset[k].status &= ~CURS_NEEDS_REREAD;
							break;
						}
					}
				}
			}
			else
			{
				SC_set_error(stmt, STMT_EXEC_ERROR, "Data Load Error", __FUNCTION__);
				rcnt = -1;
				SC_set_Result(stmt, qres);
				break;
			}
			QR_Destructor(qres);
			if (rowc < 0)
				break;
			rowc = 0;
		}
		if (!rowc)
		{
			char table_fqn[256];

			if (PQExpBufferDataBroken(qval))
			{
				if (rows_per_fetch >= pre_fetch_count * 2)
					keys_per_fetch = pre_fetch_count;
				else
					keys_per_fetch = rows_per_fetch;
				if (!keys_per_fetch)
					keys_per_fetch = 2;
				initPQExpBuffer(&qval);
			}
			printfPQExpBuffer(&qval, "%.*sfrom %s where ctid in (", (int) from_pos, load_stmt, ti_quote(stmt, new_oid, table_fqn, sizeof(table_fqn)));
		}
		if (new_oid != oid)
			oid = new_oid;
		getTid(res, kres_ridx, &blocknum, &offset);
		if (rowc)
			appendPQExpBuffer(&qval, ",'(%u,%u)'", blocknum, offset);
		else
			appendPQExpBuffer(&qval, "'(%u,%u)'", blocknum, offset);
		rowc++;
		rcnt++;
	}
cleanup:
#undef	return
	if (!PQExpBufferDataBroken(qval))
		termPQExpBuffer(&qval);
	return rcnt;
}

static RETCODE	SQL_API
SC_pos_reload_needed(StatementClass *stmt, SQLULEN req_size, UDWORD flag)
{
	CSTR	func = "SC_pos_reload_needed";
	Int4		req_rows_size;
	SQLLEN		i, limitrow;
	UInt2		qcount;
	QResultClass	*res;
	RETCODE		ret = SQL_ERROR;
	SQLLEN		kres_ridx, rowc;
	Int4		rows_per_fetch;
	BOOL		create_from_scratch = (0 != flag);

	MYLOG(0, "entering\n");
#define	return	DONT_CALL_RETURN_FROM_HERE???
	if (!(res = SC_get_Curres(stmt)))
	{
		SC_set_error(stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_pos_reload_needed.", func);
		goto cleanup;
	}
	if (SC_update_not_ready(stmt))
		parse_statement(stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(stmt))
	{
		stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", func);
		goto cleanup;
	}
	rows_per_fetch = 0;
	req_rows_size = QR_get_reqsize(res);
	if (req_size > req_rows_size)
		req_rows_size = (UInt4) req_size;
	if (create_from_scratch)
	{
		rows_per_fetch = ((pre_fetch_count - 1) / req_rows_size + 1) * req_rows_size;
		limitrow = RowIdx2GIdx(rows_per_fetch, stmt);
	}
	else
	{
		limitrow = RowIdx2GIdx(req_rows_size, stmt);
	}
	if (limitrow > res->num_cached_keys)
		limitrow = res->num_cached_keys;
	if (create_from_scratch ||
	    !res->dataFilled)
	{
		ClearCachedRows(res->backend_tuples, res->num_fields, res->num_cached_rows);
		res->dataFilled = FALSE;
	}
	if (!res->dataFilled)
	{
		SQLLEN	brows = GIdx2RowIdx(limitrow, stmt);
		if (brows > res->count_backend_allocated)
		{
			QR_REALLOC_gexit_with_error(res->backend_tuples, TupleField, sizeof(TupleField) * res->num_fields * brows, res, "pos_reload_needed failed", ret = SQL_ERROR);
			res->count_backend_allocated = brows;
		}
		if (brows > 0)
			memset(res->backend_tuples, 0, sizeof(TupleField) * res->num_fields * brows);
		QR_set_num_cached_rows(res, brows);
		QR_set_rowstart_in_cache(res, 0);
		if (SQL_RD_ON != stmt->options.retrieve_data)
		{
			ret = SQL_SUCCESS;
			goto cleanup;
		}
		for (i = SC_get_rowset_start(stmt), kres_ridx = GIdx2KResIdx(i, stmt,res); i < limitrow; i++, kres_ridx++)
		{
			if (0 == (res->keyset[kres_ridx].status & (CURS_SELF_DELETING | CURS_SELF_DELETED | CURS_OTHER_DELETED)))
				res->keyset[kres_ridx].status |= CURS_NEEDS_REREAD;
		}
	}
	if (TI_has_subclass(stmt->ti[0]))
	{
		if (rowc = LoadFromKeyset_inh(stmt, res, rows_per_fetch, limitrow), rowc < 0)
		{
			goto cleanup;
		}
	}
	else if (rowc = LoadFromKeyset(stmt, res, rows_per_fetch, limitrow), rowc < 0)
	{
		goto cleanup;
	}
	for (i = SC_get_rowset_start(stmt), kres_ridx = GIdx2KResIdx(i, stmt, res); i < limitrow; i++)
	{
		if (0 != (res->keyset[kres_ridx].status & CURS_NEEDS_REREAD))
		{
			ret = SC_pos_reload(stmt, i, &qcount, 0);
			if (SQL_ERROR == ret)
			{
				goto cleanup;
			}
			if (SQL_ROW_DELETED == (res->keyset[kres_ridx].status & KEYSET_INFO_PUBLIC))
			{
				res->keyset[kres_ridx].status |= CURS_OTHER_DELETED;
			}
			res->keyset[kres_ridx].status &= ~CURS_NEEDS_REREAD;
		}
	}
	ret = SQL_SUCCESS;
	res->dataFilled = TRUE;

cleanup:
#undef return
	return ret;
}

static RETCODE	SQL_API
SC_pos_newload(StatementClass *stmt, const UInt4 *oidint, BOOL tidRef,
			   const char *tidval)
{
	CSTR	func = "SC_pos_newload";
	int			i;
	QResultClass *res, *qres;
	RETCODE		ret = SQL_ERROR;

	MYLOG(0, "entering ti=%p\n", stmt->ti);
	if (!(res = SC_get_Curres(stmt)))
	{
		SC_set_error(stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_pos_newload.", func);
		return SQL_ERROR;
	}
	if (SC_update_not_ready(stmt))
		parse_statement(stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(stmt))
	{
		stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", func);
		return SQL_ERROR;
	}
	qres = positioned_load(stmt, (tidRef && NULL == tidval) ? USE_INSERTED_TID : 0, oidint, tidRef ? tidval : NULL);
	if (!qres || !QR_command_maybe_successful(qres))
	{
		SC_set_error(stmt, STMT_ERROR_TAKEN_FROM_BACKEND, "positioned_load in pos_newload failed", func);
	}
	else
	{
		SQLLEN	count = QR_get_num_cached_tuples(qres);

		QR_set_position(qres, 0);
		if (count == 1)
		{
			int	effective_fields = res->num_fields;
			ssize_t	tuple_size;
			SQLLEN	num_total_rows, num_cached_rows, kres_ridx;
			BOOL	appendKey = FALSE, appendData = FALSE;
			TupleField *tuple_old, *tuple_new;

			tuple_new = qres->tupleField;
			num_total_rows = QR_get_num_total_tuples(res);

			AddAdded(stmt, res, num_total_rows, tuple_new);
			num_cached_rows = QR_get_num_cached_tuples(res);
			kres_ridx = GIdx2KResIdx(num_total_rows, stmt, res);
			if (QR_haskeyset(res))
			{	if (!QR_get_cursor(res))
				{
					appendKey = TRUE;
					if (num_total_rows == CacheIdx2GIdx(num_cached_rows, stmt, res))
						appendData = TRUE;
					else
					{
MYLOG(DETAIL_LOG_LEVEL, "total " FORMAT_LEN " <> backend " FORMAT_LEN " - base " FORMAT_LEN " + start " FORMAT_LEN " cursor_type=" FORMAT_UINTEGER "\n",
num_total_rows, num_cached_rows,
QR_get_rowstart_in_cache(res), SC_get_rowset_start(stmt), stmt->options.cursor_type);
					}
				}
				else if (kres_ridx >= 0 && kres_ridx < res->cache_size)
				{
					appendKey = TRUE;
					appendData = TRUE;
				}
			}
			if (appendKey)
			{
				if (res->num_cached_keys >= res->count_keyset_allocated)
				{
					if (!res->count_keyset_allocated)
						tuple_size = TUPLE_MALLOC_INC;
					else
						tuple_size = res->count_keyset_allocated * 2;
					QR_REALLOC_return_with_error(res->keyset, KeySet, sizeof(KeySet) * tuple_size, res, "pos_newload failed", SQL_ERROR);
					res->count_keyset_allocated = tuple_size;
				}
				KeySetSet(tuple_new, qres->num_fields, res->num_key_fields, res->keyset + kres_ridx, TRUE);
				res->num_cached_keys++;
			}
			if (appendData)
			{
MYLOG(DETAIL_LOG_LEVEL, "total " FORMAT_LEN " == backend " FORMAT_LEN " - base " FORMAT_LEN " + start " FORMAT_LEN " cursor_type=" FORMAT_UINTEGER "\n",
num_total_rows, num_cached_rows,
QR_get_rowstart_in_cache(res), SC_get_rowset_start(stmt), stmt->options.cursor_type);
				if (num_cached_rows >= res->count_backend_allocated)
				{
					if (!res->count_backend_allocated)
						tuple_size = TUPLE_MALLOC_INC;
					else
						tuple_size = res->count_backend_allocated * 2;
					QR_REALLOC_return_with_error(res->backend_tuples, TupleField, res->num_fields * sizeof(TupleField) * tuple_size, res, "SC_pos_newload failed", SQL_ERROR);
					res->count_backend_allocated = tuple_size;
				}
				tuple_old = res->backend_tuples + res->num_fields * num_cached_rows;
				for (i = 0; i < effective_fields; i++)
				{
					tuple_old[i].len = tuple_new[i].len;
					tuple_new[i].len = -1;
					tuple_old[i].value = tuple_new[i].value;
					tuple_new[i].value = NULL;
				}
				res->num_cached_rows++;
			}
			ret = SQL_SUCCESS;
		}
		else if (0 == count)
			ret = SQL_NO_DATA_FOUND;
		else
		{
			SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the driver cound't identify inserted rows", func);
			ret = SQL_ERROR;
		}
		/* stmt->currTuple = SC_get_rowset_start(stmt) + ridx; */
	}
	QR_Destructor(qres);
	return ret;
}

static RETCODE SQL_API
irow_update(RETCODE ret, StatementClass *stmt, StatementClass *ustmt, SQLULEN global_ridx, const KeySet *old_keyset)
{
	CSTR	func = "irow_update";

	if (ret != SQL_ERROR)
	{
		int			updcnt;
		QResultClass		*tres = SC_get_Curres(ustmt);
		const char *cmdstr = QR_get_command(tres);

		if (cmdstr &&
			sscanf(cmdstr, "UPDATE %d", &updcnt) == 1)
		{
			if (updcnt == 1)
			{
				KeySet	keys;

				if (NULL != tres->backend_tuples &&
				    1 == QR_get_num_cached_tuples(tres))
				{
					KeySetSet(tres->backend_tuples, QR_NumResultCols(tres), QR_NumResultCols(tres), &keys, TRUE);
					if (SQL_SUCCEEDED(ret = SC_pos_reload_with_key(stmt, global_ridx, (UInt2 *) 0, SQL_UPDATE, &keys)))
						AddRollback(stmt, SC_get_Curres(stmt), global_ridx, old_keyset, SQL_UPDATE);
				}
				else
					ret = SQL_ERROR;
			}
			else if (updcnt == 0)
			{
				SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the content was changed before updates", func);
				ret = SQL_SUCCESS_WITH_INFO;
				if (stmt->options.cursor_type == SQL_CURSOR_KEYSET_DRIVEN)
					SC_pos_reload(stmt, global_ridx, (UInt2 *) 0, 0);
			}
			else
				ret = SQL_ERROR;
		}
		else
			ret = SQL_ERROR;
		if (ret == SQL_ERROR && SC_get_errornumber(stmt) == 0)
		{
			SC_set_error(stmt, STMT_ERROR_TAKEN_FROM_BACKEND, "SetPos update return error", func);
		}
	}
	return ret;
}

/* SQL_NEED_DATA callback for SC_pos_update */
typedef struct
{
	BOOL		updyes;
	QResultClass	*res;
	StatementClass	*stmt, *qstmt;
	IRDFields	*irdflds;
	SQLSETPOSIROW		irow;
	SQLULEN		global_ridx;
	KeySet		old_keyset;
}	pup_cdata;
static RETCODE
pos_update_callback(RETCODE retcode, void *para)
{
	RETCODE	ret = retcode;
	pup_cdata *s = (pup_cdata *) para;
	SQLLEN	kres_ridx;
	BOOL	idx_exist = TRUE;

	if (s->updyes)
	{
		MYLOG(0, "entering\n");
		ret = irow_update(ret, s->stmt, s->qstmt, s->global_ridx, &s->old_keyset);
MYLOG(DETAIL_LOG_LEVEL, "irow_update ret=%d,%d\n", ret, SC_get_errornumber(s->qstmt));
		if (ret != SQL_SUCCESS)
			SC_error_copy(s->stmt, s->qstmt, TRUE);
		WD_FreeStmt(s->qstmt, SQL_DROP);
		s->qstmt = NULL;
	}
	s->updyes = FALSE;
	kres_ridx = GIdx2KResIdx(s->global_ridx, s->stmt, s->res);
	if (kres_ridx < 0 || kres_ridx >= s->res->num_cached_keys)
	{
		idx_exist = FALSE;
	}
	if (SQL_SUCCESS == ret && s->res->keyset && idx_exist)
	{
		ConnectionClass	*conn = SC_get_conn(s->stmt);

		if (CC_is_in_trans(conn))
		{
			s->res->keyset[kres_ridx].status |= (SQL_ROW_UPDATED  | CURS_SELF_UPDATING);
		}
		else
			s->res->keyset[kres_ridx].status |= (SQL_ROW_UPDATED  | CURS_SELF_UPDATED);
	}
	if (s->irdflds->rowStatusArray)
	{
		switch (ret)
		{
			case SQL_SUCCESS:
				s->irdflds->rowStatusArray[s->irow] = SQL_ROW_UPDATED;
				break;
			case SQL_NO_DATA_FOUND:
			case SQL_SUCCESS_WITH_INFO:
				s->irdflds->rowStatusArray[s->irow] = SQL_ROW_SUCCESS_WITH_INFO;
				ret = SQL_SUCCESS_WITH_INFO;
				break;
			case SQL_ERROR:
			default:
				s->irdflds->rowStatusArray[s->irow] = SQL_ROW_ERROR;
		}
	}

	return ret;
}
RETCODE
SC_pos_update(StatementClass *stmt,
		  SQLSETPOSIROW irow, SQLULEN global_ridx, const KeySet *keyset)
{
	CSTR	func = "SC_pos_update";
	int			i,
				num_cols,
				upd_cols;
	pup_cdata	s;
	ConnectionClass	*conn;
	ARDFields	*opts = SC_get_ARDF(stmt);
	BindInfoClass *bindings = opts->bindings;
	TABLE_INFO	*ti;
	FIELD_INFO	**fi;
	PQExpBufferData		updstr = {0};
	RETCODE		ret = SQL_ERROR;
	OID	oid;
	UInt4	blocknum;
	UInt2	pgoffset;
	SQLLEN	offset;
	SQLLEN	*used, kres_ridx;
	Int4	bind_size = opts->bind_size;
	BOOL	idx_exist = TRUE;
	char	table_fqn[256];

	s.stmt = stmt;
	s.irow = irow;
	s.global_ridx = global_ridx;
	s.irdflds = SC_get_IRDF(s.stmt);
	fi = s.irdflds->fi;
	if (!(s.res = SC_get_Curres(s.stmt)))
	{
		SC_set_error(s.stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_pos_update.", func);
		return SQL_ERROR;
	}
	MYLOG(0, "entering " FORMAT_POSIROW "+" FORMAT_LEN " fi=%p ti=%p\n", s.irow, QR_get_rowstart_in_cache(s.res), fi, s.stmt->ti);
	if (SC_update_not_ready(stmt))
		parse_statement(s.stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(s.stmt))
	{
		s.stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(s.stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", func);
		return SQL_ERROR;
	}
	kres_ridx = GIdx2KResIdx(s.global_ridx, s.stmt, s.res);
	if (kres_ridx < 0 || kres_ridx >= s.res->num_cached_keys)
	{
		if (NULL == keyset || keyset->offset == 0)
		{
			SC_set_error(s.stmt, STMT_ROW_OUT_OF_RANGE, "the target keys are out of the rowset", func);
			return SQL_ERROR;
		}
		idx_exist = FALSE;
	}
	if (idx_exist)
	{
		if (!(oid = getOid(s.res, kres_ridx)))
		{
			if (!strcmp(SAFE_NAME(stmt->ti[0]->bestitem), OID_NAME))
			{
				SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the row was already deleted ?", func);
				return SQL_ERROR;
			}
		}
		getTid(s.res, kres_ridx, &blocknum, &pgoffset);
		s.old_keyset = s.res->keyset[kres_ridx];
	}
	else
	{
		oid = keyset->oid;
		blocknum = keyset->blocknum;
		pgoffset = keyset->offset;
		s.old_keyset = *keyset;
	}

	ti = s.stmt->ti[0];

	initPQExpBuffer(&updstr);
#define	return	DONT_CALL_RETURN_FROM_HERE???
	printfPQExpBuffer(&updstr,
			 "update %s set", ti_quote(stmt, oid, table_fqn, sizeof(table_fqn)));

	num_cols = s.irdflds->nfields;
	offset = opts->row_offset_ptr ? *opts->row_offset_ptr : 0;
	for (i = upd_cols = 0; i < num_cols; i++)
	{
		if (used = bindings[i].used, used != NULL)
		{
			used = LENADDR_SHIFT(used, offset);
			if (bind_size > 0)
				used = LENADDR_SHIFT(used, bind_size * s.irow);
			else
				used = LENADDR_SHIFT(used, s.irow * sizeof(SQLLEN));
			MYLOG(0, "%d used=" FORMAT_LEN ",%p\n", i, *used, used);
			if (*used != SQL_IGNORE && fi[i]->updatable)
			{
				if (upd_cols)
					appendPQExpBuffer(&updstr,
								 ", \"%s\" = ?", GET_NAME(fi[i]->column_name));
				else
					appendPQExpBuffer(&updstr,
								 " \"%s\" = ?", GET_NAME(fi[i]->column_name));
				upd_cols++;
			}
		}
		else
			MYLOG(0, "%d null bind\n", i);
	}
	conn = SC_get_conn(s.stmt);
	s.updyes = FALSE;
	if (upd_cols > 0)
	{
		HSTMT		hstmt;
		int			j;
		ConnInfo	*ci = &(conn->connInfo);
		APDFields	*apdopts;
		IPDFields	*ipdopts;
		OID		fieldtype = 0;
		const char *bestitem = GET_NAME(ti->bestitem);
		const char *bestqual = GET_NAME(ti->bestqual);
		int	unknown_sizes = ci->drivers.unknown_sizes;

		appendPQExpBuffer(&updstr,
					 " where ctid = '(%u, %u)'",
					 blocknum, pgoffset);
		if (bestqual)
		{
			appendPQExpBuffer(&updstr, " and ");
			appendPQExpBuffer(&updstr, bestqual, oid);
		}
		if (WD_VERSION_GE(conn, 8.2))
		{
			appendPQExpBuffer(&updstr, " returning ctid");
			if (bestitem)
			{
				appendPQExpBuffer(&updstr, ", ");
				appendPQExpBuffer(&updstr, "\"%s\"", bestitem);
			}
		}
		MYLOG(0, "updstr=%s\n", updstr.data);
		if (WD_AllocStmt(conn, &hstmt, 0) != SQL_SUCCESS)
		{
			SC_set_error(s.stmt, STMT_NO_MEMORY_ERROR, "internal AllocStmt error", func);
			goto cleanup;
		}
		s.qstmt = (StatementClass *) hstmt;
		apdopts = SC_get_APDF(s.qstmt);
		apdopts->param_bind_type = opts->bind_size;
		apdopts->param_offset_ptr = opts->row_offset_ptr;
		ipdopts = SC_get_IPDF(s.qstmt);
		SC_set_delegate(s.stmt, s.qstmt);
		extend_iparameter_bindings(ipdopts, num_cols);
		for (i = j = 0; i < num_cols; i++)
		{
			if (used = bindings[i].used, used != NULL)
			{
				used = LENADDR_SHIFT(used, offset);
				if (bind_size > 0)
					used = LENADDR_SHIFT(used, bind_size * s.irow);
				else
					used = LENADDR_SHIFT(used, s.irow * sizeof(SQLLEN));
				MYLOG(0, "%d used=" FORMAT_LEN "\n", i, *used);
				if (*used != SQL_IGNORE && fi[i]->updatable)
				{
					/* fieldtype = QR_get_field_type(s.res, i); */
					fieldtype = getEffectiveOid(conn, fi[i]);
					PIC_set_wdtype(ipdopts->parameters[j], fieldtype);
					WD_BindParameter(hstmt,
						(SQLUSMALLINT) ++j,
						SQL_PARAM_INPUT,
						bindings[i].returntype,
						wdtype_to_concise_type(s.stmt, fieldtype, i, unknown_sizes),
																fi[i]->column_size > 0 ? fi[i]->column_size : wdtype_column_size(s.stmt, fieldtype, i, unknown_sizes),
						(SQLSMALLINT) fi[i]->decimal_digits,
						bindings[i].buffer,
						bindings[i].buflen,
						bindings[i].used);
				}
			}
		}
		s.qstmt->exec_start_row = s.qstmt->exec_end_row = s.irow;
		s.updyes = TRUE;
		if (PQExpBufferDataBroken(updstr))
		{
			SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in SC_pos_updatet()", func);
			goto cleanup;
		}
		ret = WD_ExecDirect(hstmt, (SQLCHAR *) updstr.data, SQL_NTS, 0);
		if (ret == SQL_NEED_DATA)
		{
			pup_cdata *cbdata = (pup_cdata *) malloc(sizeof(pup_cdata));
			if (!cbdata)
			{
				SC_set_error(s.stmt, STMT_NO_MEMORY_ERROR, "Could not allocate memory for cbdata", func);
				ret = SQL_ERROR;
				goto cleanup;
			}
			memcpy(cbdata, &s, sizeof(pup_cdata));
			if (0 == enqueueNeedDataCallback(s.stmt, pos_update_callback, cbdata))
				ret = SQL_ERROR;
			goto cleanup;
		}
		/* else if (ret != SQL_SUCCESS) this is unneccesary
			SC_error_copy(s.stmt, s.qstmt, TRUE); */
	}
	else
	{
		ret = SQL_SUCCESS_WITH_INFO;
		SC_set_error(s.stmt, STMT_INVALID_CURSOR_STATE_ERROR, "update list null", func);
	}

	ret = pos_update_callback(ret, &s);

cleanup:
#undef	return
	if (!PQExpBufferDataBroken(updstr))
		termPQExpBuffer(&updstr);
	return ret;
}
RETCODE
SC_pos_delete(StatementClass *stmt,
		  SQLSETPOSIROW irow, SQLULEN global_ridx, const KeySet *keyset)
{
	CSTR	func = "SC_pos_update";
	UWORD		offset;
	QResultClass *res, *qres;
	ConnectionClass	*conn = SC_get_conn(stmt);
	IRDFields	*irdflds = SC_get_IRDF(stmt);
	PQExpBufferData		dltstr = {0};
	RETCODE		ret;
	SQLLEN		kres_ridx;
	OID		oid;
	UInt4		blocknum, qflag;
	TABLE_INFO	*ti;
	const char	*bestitem;
	const char	*bestqual;
	BOOL		idx_exist = TRUE;
	char		table_fqn[256];

	MYLOG(0, "entering ti=%p\n", stmt->ti);
	if (!(res = SC_get_Curres(stmt)))
	{
		SC_set_error(stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_pos_delete.", func);
		return SQL_ERROR;
	}
	if (SC_update_not_ready(stmt))
		parse_statement(stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(stmt))
	{
		stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", func);
		return SQL_ERROR;
	}
	kres_ridx = GIdx2KResIdx(global_ridx, stmt, res);
	if (kres_ridx < 0 || kres_ridx >= res->num_cached_keys)
	{
		if (NULL == keyset || keyset->offset == 0)
		{
			SC_set_error(stmt, STMT_ROW_OUT_OF_RANGE, "the target keys are out of the rowset", func);
			return SQL_ERROR;
		}
		idx_exist = FALSE;
	}
	ti = stmt->ti[0];
	bestitem = GET_NAME(ti->bestitem);
	bestqual = GET_NAME(ti->bestqual);
	if (idx_exist)
	{
		if (!(oid = getOid(res, kres_ridx)))
		{
			if (bestitem && !strcmp(bestitem, OID_NAME))
			{
				SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the row was already deleted ?", func);
				return SQL_ERROR;
			}
		}
		getTid(res, kres_ridx, &blocknum, &offset);
		keyset = res->keyset + kres_ridx;
	}
	else
	{
		oid = keyset->oid;
		blocknum = keyset->blocknum;
		offset = keyset->offset;
	}
	initPQExpBuffer(&dltstr);
#define	return	DONT_CALL_RETURN_FROM_HERE???
	printfPQExpBuffer(&dltstr,
			 "delete from %s where ctid = '(%u, %u)'",
			 ti_quote(stmt, oid, table_fqn, sizeof(table_fqn)), blocknum, offset);
	if (bestqual && !TI_has_subclass(ti))
	{
		appendPQExpBuffer(&dltstr, " and ");
		appendPQExpBuffer(&dltstr, bestqual, oid);
	}

	if (PQExpBufferDataBroken(dltstr))
	{
		ret = SQL_ERROR;
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in SC_pos_delete()", func);
		goto cleanup;
	}
	MYLOG(0, "dltstr=%s\n", dltstr.data);
	qflag = 0;
        if (stmt->external && !CC_is_in_trans(conn) &&
                 (!CC_does_autocommit(conn)))
		qflag |= GO_INTO_TRANSACTION;
	qres = CC_send_query(conn, dltstr.data, NULL, qflag, stmt);
	ret = SQL_SUCCESS;
	if (QR_command_maybe_successful(qres))
	{
		int			dltcnt;
		const char *cmdstr = QR_get_command(qres);

		if (cmdstr &&
			sscanf(cmdstr, "DELETE %d", &dltcnt) == 1)
		{
			if (dltcnt == 1)
			{
				RETCODE	tret = SC_pos_reload_with_key(stmt, global_ridx, (UInt2 *) 0, SQL_DELETE, keyset);
				if (!SQL_SUCCEEDED(tret))
					ret = tret;
			}
			else if (dltcnt == 0)
			{
				SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the content was changed before deletes", func);
				ret = SQL_SUCCESS_WITH_INFO;
				if (idx_exist && stmt->options.cursor_type == SQL_CURSOR_KEYSET_DRIVEN)
					SC_pos_reload(stmt, global_ridx, (UInt2 *) 0, 0);
			}
			else
				ret = SQL_ERROR;
		}
		else
			ret = SQL_ERROR;
	}
	else
	{
		ret = SQL_ERROR;
		if (qres)
		{
			STRCPY_FIXED(res->sqlstate, qres->sqlstate);
			res->message = qres->message;
			qres->message = NULL;
		}
	}
	if (ret == SQL_ERROR && SC_get_errornumber(stmt) == 0)
	{
		SC_set_error(stmt, STMT_ERROR_TAKEN_FROM_BACKEND, "SetPos delete return error", func);
	}
	if (qres)
		QR_Destructor(qres);
	if (SQL_SUCCESS == ret && keyset)
		AddDeleted(res, global_ridx, keyset);
	if (SQL_SUCCESS == ret && keyset && idx_exist)
	{
		res->keyset[kres_ridx].status &= (~KEYSET_INFO_PUBLIC);
		if (CC_is_in_trans(conn))
		{
			res->keyset[kres_ridx].status |= (SQL_ROW_DELETED | CURS_SELF_DELETING);
		}
		else
			res->keyset[kres_ridx].status |= (SQL_ROW_DELETED | CURS_SELF_DELETED);
MYLOG(DETAIL_LOG_LEVEL, ".status[" FORMAT_ULEN "]=%x\n", global_ridx, res->keyset[kres_ridx].status);
	}
	if (irdflds->rowStatusArray)
	{
		switch (ret)
		{
			case SQL_SUCCESS:
				irdflds->rowStatusArray[irow] = SQL_ROW_DELETED;
				break;
			case SQL_NO_DATA_FOUND:
			case SQL_SUCCESS_WITH_INFO:
				irdflds->rowStatusArray[irow] = SQL_ROW_DELETED; // SQL_ROW_SUCCESS_WITH_INFO;
				ret = SQL_SUCCESS_WITH_INFO;
				break;
			case SQL_ERROR:
			default:
				irdflds->rowStatusArray[irow] = SQL_ROW_ERROR;
				break;
		}
	}

cleanup:
#undef return
	if (!PQExpBufferDataBroken(dltstr))
		termPQExpBuffer(&dltstr);
	return ret;
}

static RETCODE SQL_API
irow_insert(RETCODE ret, StatementClass *stmt, StatementClass *istmt,
			SQLLEN addpos)
{
	CSTR	func = "irow_insert";

	if (ret != SQL_ERROR)
	{
		int		addcnt;
		OID		oid, *poid = NULL;
		ARDFields	*opts = SC_get_ARDF(stmt);
		QResultClass	*ires = SC_get_Curres(istmt), *tres;
		const char *cmdstr;
		BindInfoClass	*bookmark;

		tres = (QR_nextr(ires) ? QR_nextr(ires) : ires);
		cmdstr = QR_get_command(tres);
		if (cmdstr &&
			sscanf(cmdstr, "INSERT %u %d", &oid, &addcnt) == 2 &&
			addcnt == 1)
		{
			RETCODE	qret;
			const char * tidval = NULL;
			char	tidv[32];
			KeySet	keys;

			if (NULL != tres->backend_tuples &&
			    1 == QR_get_num_cached_tuples(tres))
			{
				KeySetSet(tres->backend_tuples, QR_NumResultCols(tres), QR_NumResultCols(tres), &keys, TRUE);
				oid = keys.oid;
				SPRINTF_FIXED(tidv, "(%u,%hu)", keys.blocknum, keys.offset);
				tidval = tidv;
			}
			if (0 != oid)
				poid = &oid;
			qret = SC_pos_newload(stmt, poid, TRUE, tidval);
			if (SQL_ERROR == qret)
				return qret;

			if (SQL_NO_DATA_FOUND == qret)
			{
				qret = SC_pos_newload(stmt, poid, FALSE, NULL);
				if (SQL_ERROR == qret)
					return qret;
			}
			bookmark = opts->bookmark;
			if (bookmark && bookmark->buffer)
			{
				SC_set_current_col(stmt, -1);
				SC_Create_bookmark(stmt, bookmark, stmt->bind_row, addpos, &keys);
			}
		}
		else
		{
			SC_set_error(stmt, STMT_ERROR_TAKEN_FROM_BACKEND, "SetPos insert return error", func);
		}
	}
	return ret;
}

/* SQL_NEED_DATA callback for SC_pos_add */
typedef struct
{
	BOOL		updyes;
	QResultClass	*res;
	StatementClass	*stmt, *qstmt;
	IRDFields	*irdflds;
	SQLSETPOSIROW		irow;
}	padd_cdata;

static RETCODE
pos_add_callback(RETCODE retcode, void *para)
{
	RETCODE	ret = retcode;
	padd_cdata *s = (padd_cdata *) para;
	SQLLEN	addpos;

	if (s->updyes)
	{
		SQLSETPOSIROW	brow_save;

		MYLOG(0, "entering ret=%d\n", ret);
		brow_save = s->stmt->bind_row;
		s->stmt->bind_row = s->irow;
		if (QR_get_cursor(s->res))
			addpos = -(SQLLEN)(s->res->ad_count + 1);
		else
			addpos = QR_get_num_total_tuples(s->res);
		ret = irow_insert(ret, s->stmt, s->qstmt, addpos);
		s->stmt->bind_row = brow_save;
	}
	s->updyes = FALSE;
	SC_setInsertedTable(s->qstmt, ret);
	if (ret != SQL_SUCCESS)
		SC_error_copy(s->stmt, s->qstmt, TRUE);
	WD_FreeStmt((HSTMT) s->qstmt, SQL_DROP);
	s->qstmt = NULL;
	if (SQL_SUCCESS == ret && s->res->keyset)
	{
		SQLLEN	global_ridx = QR_get_num_total_tuples(s->res) - 1;
		ConnectionClass	*conn = SC_get_conn(s->stmt);
		SQLLEN	kres_ridx;
		UWORD	status = SQL_ROW_ADDED;

		if (CC_is_in_trans(conn))
			status |= CURS_SELF_ADDING;
		else
			status |= CURS_SELF_ADDED;
		kres_ridx = GIdx2KResIdx(global_ridx, s->stmt, s->res);
		if (kres_ridx >= 0 && kres_ridx < s->res->num_cached_keys)
		{
			s->res->keyset[kres_ridx].status = status;
		}
	}
	if (s->irdflds->rowStatusArray)
	{
		switch (ret)
		{
			case SQL_SUCCESS:
				s->irdflds->rowStatusArray[s->irow] = SQL_ROW_ADDED;
				break;
			case SQL_NO_DATA_FOUND:
			case SQL_SUCCESS_WITH_INFO:
				s->irdflds->rowStatusArray[s->irow] = SQL_ROW_SUCCESS_WITH_INFO;
				break;
			default:
				s->irdflds->rowStatusArray[s->irow] = SQL_ROW_ERROR;
		}
	}

	return ret;
}

RETCODE
SC_pos_add(StatementClass *stmt,
		   SQLSETPOSIROW irow)
{
	CSTR	func = "SC_pos_add";
	int			num_cols,
				add_cols,
				i;
	HSTMT		hstmt;

	padd_cdata	s;
	ConnectionClass	*conn;
	ConnInfo	*ci;
	ARDFields	*opts = SC_get_ARDF(stmt);
	APDFields	*apdopts;
	IPDFields	*ipdopts;
	BindInfoClass *bindings = opts->bindings;
	FIELD_INFO	**fi = SC_get_IRDF(stmt)->fi;
	PQExpBufferData		addstr = {0};
	RETCODE		ret;
	SQLULEN		offset;
	SQLLEN		*used;
	Int4		bind_size = opts->bind_size;
	OID		fieldtype;
	int		unknown_sizes;
	int		func_cs_count = 0;
	char		table_fqn[256];

	MYLOG(0, "entering fi=%p ti=%p\n", fi, stmt->ti);
	s.stmt = stmt;
	s.irow = irow;
	if (!(s.res = SC_get_Curres(s.stmt)))
	{
		SC_set_error(s.stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_pos_add.", func);
		return SQL_ERROR;
	}
	if (SC_update_not_ready(stmt))
		parse_statement(s.stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(s.stmt))
	{
		s.stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(s.stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", func);
		return SQL_ERROR;
	}
	s.irdflds = SC_get_IRDF(s.stmt);
	num_cols = s.irdflds->nfields;
	conn = SC_get_conn(s.stmt);

	if (WD_AllocStmt(conn, &hstmt, 0) != SQL_SUCCESS)
	{
		SC_set_error(s.stmt, STMT_NO_MEMORY_ERROR, "internal AllocStmt error", func);
		return SQL_ERROR;
	}
	initPQExpBuffer(&addstr);
#define	return	DONT_CALL_RETURN_FROM_HERE???
	printfPQExpBuffer(&addstr,
			 "insert into %s (",
			 ti_quote(s.stmt, 0, table_fqn, sizeof(table_fqn)));
	if (opts->row_offset_ptr)
		offset = *opts->row_offset_ptr;
	else
		offset = 0;
	s.qstmt = (StatementClass *) hstmt;
	apdopts = SC_get_APDF(s.qstmt);
	apdopts->param_bind_type = opts->bind_size;
	apdopts->param_offset_ptr = opts->row_offset_ptr;
	ipdopts = SC_get_IPDF(s.qstmt);
	SC_set_delegate(s.stmt, s.qstmt);
	ci = &(conn->connInfo);
	unknown_sizes = ci->drivers.unknown_sizes;
	extend_iparameter_bindings(ipdopts, num_cols);
	for (i = add_cols = 0; i < num_cols; i++)
	{
		if (used = bindings[i].used, used != NULL)
		{
			used = LENADDR_SHIFT(used, offset);
			if (bind_size > 0)
				used = LENADDR_SHIFT(used, bind_size * s.irow);
			else
				used = LENADDR_SHIFT(used, s.irow * sizeof(SQLLEN));
			MYLOG(0, "%d used=" FORMAT_LEN "\n", i, *used);
			if (*used != SQL_IGNORE && fi[i]->updatable)
			{
				/* fieldtype = QR_get_field_type(s.res, i); */
				fieldtype = getEffectiveOid(conn, fi[i]);
				if (add_cols)
					appendPQExpBuffer(&addstr,
								 ", \"%s\"", GET_NAME(fi[i]->column_name));
				else
					appendPQExpBuffer(&addstr,
								 "\"%s\"", GET_NAME(fi[i]->column_name));
				PIC_set_wdtype(ipdopts->parameters[add_cols], fieldtype);
				WD_BindParameter(hstmt,
					(SQLUSMALLINT) ++add_cols,
					SQL_PARAM_INPUT,
					bindings[i].returntype,
					wdtype_to_concise_type(s.stmt, fieldtype, i, unknown_sizes),
															fi[i]->column_size > 0 ? fi[i]->column_size : wdtype_column_size(s.stmt, fieldtype, i, unknown_sizes),
					(SQLSMALLINT) fi[i]->decimal_digits,
					bindings[i].buffer,
					bindings[i].buflen,
					bindings[i].used);
			}
		}
		else
			MYLOG(0, "%d null bind\n", i);
	}
	s.updyes = FALSE;
	ENTER_INNER_CONN_CS(conn, func_cs_count);
	if (add_cols > 0)
	{
		appendPQExpBuffer(&addstr, ") values (");
		for (i = 0; i < add_cols; i++)
		{
			if (i)
				appendPQExpBuffer(&addstr, ", ?");
			else
				appendPQExpBuffer(&addstr, "?");
		}
		appendPQExpBuffer(&addstr, ")");
		if (WD_VERSION_GE(conn, 8.2))
		{
			TABLE_INFO	*ti = stmt->ti[0];
			const char *bestitem = GET_NAME(ti->bestitem);

			appendPQExpBuffer(&addstr, " returning ctid");
			if (bestitem)
			{
				appendPQExpBuffer(&addstr, ", ");
				appendPQExpBuffer(&addstr, "\"%s\"", bestitem);
			}
		}
		if (PQExpBufferDataBroken(addstr))
		{
			ret = SQL_ERROR;
			SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Out of memory in SC_pos_add()", func);
			goto cleanup;
		}
		MYLOG(0, "addstr=%s\n", addstr.data);
		s.qstmt->exec_start_row = s.qstmt->exec_end_row = s.irow;
		s.updyes = TRUE;
		ret = WD_ExecDirect(hstmt, (SQLCHAR *) addstr.data, SQL_NTS, 0);
		if (ret == SQL_NEED_DATA)
		{
			padd_cdata *cbdata = (padd_cdata *) malloc(sizeof(padd_cdata));
			if (!cbdata)
			{
				SC_set_error(s.stmt, STMT_NO_MEMORY_ERROR, "Could not allocate memory for cbdata", func);
				ret = SQL_ERROR;
			goto cleanup;
			}
			memcpy(cbdata, &s, sizeof(padd_cdata));
			if (0 == enqueueNeedDataCallback(s.stmt, pos_add_callback, cbdata))
				ret = SQL_ERROR;
			goto cleanup;
		}
		/* else if (ret != SQL_SUCCESS) this is unneccesary
			SC_error_copy(s.stmt, s.qstmt, TRUE); */
	}
	else
	{
		ret = SQL_SUCCESS_WITH_INFO;
		SC_set_error(s.stmt, STMT_INVALID_CURSOR_STATE_ERROR, "insert list null", func);
	}

	ret = pos_add_callback(ret, &s);

cleanup:
#undef	return
	CLEANUP_FUNC_CONN_CS(func_cs_count, conn);
	if (!PQExpBufferDataBroken(addstr))
		termPQExpBuffer(&addstr);
	return ret;
}

/*
 *	Stuff for updatable cursors end.
 */

RETCODE
SC_pos_refresh(StatementClass *stmt, SQLSETPOSIROW irow , SQLULEN global_ridx)
{
	RETCODE	ret;
	IRDFields	*irdflds = SC_get_IRDF(stmt);
	/* save the last_fetch_count */
	SQLLEN		last_fetch = stmt->last_fetch_count;
	SQLLEN		last_fetch2 = stmt->last_fetch_count_include_ommitted;
	SQLSETPOSIROW	bind_save = stmt->bind_row;
	BOOL		tuple_reload = FALSE;

	if (stmt->options.cursor_type == SQL_CURSOR_KEYSET_DRIVEN)
		tuple_reload = TRUE;
	else
	{
		QResultClass	*res = SC_get_Curres(stmt);
		if (res && res->keyset)
		{
			SQLLEN kres_ridx = GIdx2KResIdx(global_ridx, stmt, res);
			if (kres_ridx >= 0 && kres_ridx < QR_get_num_cached_tuples(res))
			{
				if (0 != (CURS_NEEDS_REREAD & res->keyset[kres_ridx].status))
					tuple_reload = TRUE;
			}
		}
	}
	if (tuple_reload)
	{
		if (!SQL_SUCCEEDED(ret = SC_pos_reload(stmt, global_ridx, (UInt2 *) 0, 0)))
			return ret;
	}
	stmt->bind_row = irow;
	ret = SC_fetch(stmt);
	/* restore the last_fetch_count */
	stmt->last_fetch_count = last_fetch;
	stmt->last_fetch_count_include_ommitted = last_fetch2;
	stmt->bind_row = bind_save;
	if (irdflds->rowStatusArray)
	{
		switch (ret)
		{
			case SQL_SUCCESS:
				irdflds->rowStatusArray[irow] = SQL_ROW_SUCCESS;
				break;
			case SQL_SUCCESS_WITH_INFO:
				irdflds->rowStatusArray[irow] = SQL_ROW_SUCCESS_WITH_INFO;
				break;
			case SQL_ERROR:
			default:
				irdflds->rowStatusArray[irow] = SQL_ROW_ERROR;
				break;
		}
	}

	return SQL_SUCCESS;
}

/*	SQL_NEED_DATA callback for WD_SetPos */
typedef struct
{
	BOOL		need_data_callback, auto_commit_needed;
	QResultClass	*res;
	StatementClass	*stmt;
	ARDFields	*opts;
	GetDataInfo	*gdata;
	SQLLEN	idx, start_row, end_row, ridx;
	UWORD	fOption;
	SQLSETPOSIROW	irow, nrow, processed;
}	spos_cdata;
static
RETCODE spos_callback(RETCODE retcode, void *para)
{
	CSTR	func = "spos_callback";
	RETCODE	ret;
	spos_cdata *s = (spos_cdata *) para;
	QResultClass	*res;
	ARDFields	*opts;
	ConnectionClass	*conn;
	SQLULEN	global_ridx;
	SQLLEN	kres_ridx, pos_ridx = 0;

	ret = retcode;
	MYLOG(0, "entering %d in\n", s->need_data_callback);
	if (s->need_data_callback)
	{
		s->processed++;
		if (SQL_ERROR != retcode)
		{
			s->nrow++;
			s->idx++;
		}
	}
	else
	{
		s->ridx = -1;
		s->idx = s->nrow = s->processed = 0;
	}
	res = s->res;
	opts = s->opts;
	if (!res || !opts)
	{
		SC_set_error(s->stmt, STMT_SEQUENCE_ERROR, "Passed res or opts for spos_callback is NULL", func);
		return SQL_ERROR;
	}
	s->need_data_callback = FALSE;
	for (; SQL_ERROR != ret && s->nrow <= s->end_row; s->idx++)
	{
		global_ridx = RowIdx2GIdx(s->idx, s->stmt);
		if (SQL_ADD != s->fOption)
		{
			if ((int) global_ridx >= QR_get_num_total_tuples(res))
				break;
			if (res->keyset)
			{
				kres_ridx = GIdx2KResIdx(global_ridx, s->stmt, res);
				if (kres_ridx >= res->num_cached_keys)
					break;
				if (kres_ridx >= 0) /* the row may be deleted and not in the rowset */
				{
					if (0 == (res->keyset[kres_ridx].status & CURS_IN_ROWSET))
						continue;
				}
			}
		}
		if (s->nrow < s->start_row)
		{
			s->nrow++;
			continue;
		}
		s->ridx = s->nrow;
		pos_ridx = s->idx;
		if (0 != s->irow || !opts->row_operation_ptr || opts->row_operation_ptr[s->nrow] == SQL_ROW_PROCEED)
		{
			switch (s->fOption)
			{
				case SQL_UPDATE:
					ret = SC_pos_update(s->stmt, s->nrow, global_ridx, NULL);
					break;
				case SQL_DELETE:
					ret = SC_pos_delete(s->stmt, s->nrow, global_ridx, NULL);
					break;
				case SQL_ADD:
					ret = SC_pos_add(s->stmt, s->nrow);
					break;
				case SQL_REFRESH:
					ret = SC_pos_refresh(s->stmt, s->nrow, global_ridx);
					break;
			}
			if (SQL_NEED_DATA == ret)
			{
				spos_cdata *cbdata = (spos_cdata *) malloc(sizeof(spos_cdata));
				if (!cbdata)
				{
					SC_set_error(s->stmt, STMT_NO_MEMORY_ERROR, "Could not allocate memory for cbdata", func);
					return SQL_ERROR;
				}

				memcpy(cbdata, s, sizeof(spos_cdata));
				cbdata->need_data_callback = TRUE;
				if (0 == enqueueNeedDataCallback(s->stmt, spos_callback, cbdata))
					ret = SQL_ERROR;
				return ret;
			}
			s->processed++;
		}
		if (SQL_ERROR != ret)
			s->nrow++;
	}
	conn = SC_get_conn(s->stmt);
	if (s->auto_commit_needed)
		CC_set_autocommit(conn, TRUE);
	if (s->irow > 0)
	{
		if (SQL_ADD != s->fOption && s->ridx >= 0) /* for SQLGetData */
		{
			s->stmt->currTuple = RowIdx2GIdx(pos_ridx, s->stmt);
			QR_set_position(res, pos_ridx);
		}
	}
	else if (SC_get_IRDF(s->stmt)->rowsFetched)
		*(SC_get_IRDF(s->stmt)->rowsFetched) = s->processed;
	res->recent_processed_row_count = s->stmt->diag_row_count = s->processed;
	if (opts) /* logging */
	{
		MYLOG(DETAIL_LOG_LEVEL, "processed=" FORMAT_POSIROW " ret=%d rowset=" FORMAT_LEN, s->processed, ret, opts->size_of_rowset_odbc2);
		MYPRINTF(DETAIL_LOG_LEVEL, "," FORMAT_LEN "\n", opts->size_of_rowset);
	}

	return ret;
}

/*
 *	This positions the cursor within a rowset, that was positioned using SQLExtendedFetch.
 *	This will be useful (so far) only when using SQLGetData after SQLExtendedFetch.
 */
RETCODE		SQL_API
WD_SetPos(HSTMT hstmt,
			 SQLSETPOSIROW irow,
			 SQLUSMALLINT fOption,
			 SQLUSMALLINT fLock)
{
	CSTR func = "WD_SetPos";
	RETCODE	ret;
	ConnectionClass	*conn;
	SQLLEN		rowsetSize;
	int		i;
	UInt2		gdata_allocated;
	GetDataInfo	*gdata_info;
	GetDataClass	*gdata = NULL;
	spos_cdata	s;

	s.stmt = (StatementClass *) hstmt;
	if (!s.stmt)
	{
		SC_log_error(func, NULL_STRING, NULL);
		return SQL_INVALID_HANDLE;
	}

	s.irow = irow;
	s.fOption = fOption;
	s.auto_commit_needed = FALSE;
	s.opts = SC_get_ARDF(s.stmt);
	gdata_info = SC_get_GDTI(s.stmt);
	gdata = gdata_info->gdata;
	MYLOG(0, "entering fOption=%d irow=" FORMAT_POSIROW " lock=%hu currt=" FORMAT_LEN "\n", s.fOption, s.irow, fLock, s.stmt->currTuple);
	if (s.stmt->options.scroll_concurrency != SQL_CONCUR_READ_ONLY)
		;
	else if (s.fOption != SQL_POSITION && s.fOption != SQL_REFRESH)
	{
		SC_set_error(s.stmt, STMT_NOT_IMPLEMENTED_ERROR, "Only SQL_POSITION/REFRESH is supported for WD_SetPos", func);
		return SQL_ERROR;
	}

	if (!(s.res = SC_get_Curres(s.stmt)))
	{
		SC_set_error(s.stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in WD_SetPos.", func);
		return SQL_ERROR;
	}

	rowsetSize = (s.stmt->transition_status == STMT_TRANSITION_EXTENDED_FETCH ? s.opts->size_of_rowset_odbc2 : s.opts->size_of_rowset);
	if (s.irow == 0) /* bulk operation */
	{
		if (SQL_POSITION == s.fOption)
		{
			SC_set_error(s.stmt, STMT_INVALID_CURSOR_POSITION, "Bulk Position operations not allowed.", func);
			return SQL_ERROR;
		}
		s.start_row = 0;
		s.end_row = rowsetSize - 1;
	}
	else
	{
		if (SQL_ADD != s.fOption && s.irow > s.stmt->last_fetch_count)
		{
			SC_set_error(s.stmt, STMT_ROW_OUT_OF_RANGE, "Row value out of range", func);
			return SQL_ERROR;
		}
		s.start_row = s.end_row = s.irow - 1;
	}

	gdata_allocated = gdata_info->allocated;
MYLOG(0, "num_cols=%d gdatainfo=%d\n", QR_NumPublicResultCols(s.res), gdata_allocated);
	/* Reset for SQLGetData */
	if (gdata)
	{
		for (i = 0; i < gdata_allocated; i++)
			GETDATA_RESET(gdata[i]);
	}
	conn = SC_get_conn(s.stmt);
	switch (s.fOption)
	{
		case SQL_UPDATE:
		case SQL_DELETE:
		case SQL_ADD:
			if (s.auto_commit_needed = CC_does_autocommit(conn), s.auto_commit_needed)
				CC_set_autocommit(conn, FALSE);
			break;
		case SQL_POSITION:
			break;
	}

	s.need_data_callback = FALSE;
#define	return	DONT_CALL_RETURN_FROM_HERE???
	/* StartRollbackState(s.stmt); */
	ret = spos_callback(SQL_SUCCESS, &s);
#undef	return
	if (SQL_SUCCEEDED(ret) && 0 == s.processed)
	{
		SC_set_error(s.stmt, STMT_ROW_OUT_OF_RANGE, "the row was deleted?", func);
		ret = SQL_ERROR;
	}
	MYLOG(0, "leaving %d\n", ret);
	return ret;
}


/*		Sets options that control the behavior of cursors. */
RETCODE		SQL_API
WD_SetScrollOptions(HSTMT hstmt,
					   SQLUSMALLINT fConcurrency,
					   SQLLEN crowKeyset,
					   SQLUSMALLINT crowRowset)
{
	CSTR func = "WD_SetScrollOptions";
	StatementClass *stmt = (StatementClass *) hstmt;

	MYLOG(0, "entering fConcurrency=%d crowKeyset=" FORMAT_LEN " crowRowset=%d\n",
		  fConcurrency, crowKeyset, crowRowset);
	SC_set_error(stmt, STMT_NOT_IMPLEMENTED_ERROR, "SetScroll option not implemeted", func);

	return SQL_ERROR;
}


/*	Set the cursor name on a statement handle */
RETCODE		SQL_API
WD_SetCursorName(HSTMT hstmt,
					const SQLCHAR * szCursor,
					SQLSMALLINT cbCursor)
{
	CSTR func = "WD_SetCursorName";
	StatementClass *stmt = (StatementClass *) hstmt;

	MYLOG(0, "entering hstmt=%p, szCursor=%p, cbCursorMax=%d\n", hstmt, szCursor, cbCursor);

	if (!stmt)
	{
		SC_log_error(func, NULL_STRING, NULL);
		return SQL_INVALID_HANDLE;
	}

	SET_NAME_DIRECTLY(stmt->cursor_name, make_string(szCursor, cbCursor, NULL, 0));
	return SQL_SUCCESS;
}


/*	Return the cursor name for a statement handle */
RETCODE		SQL_API
WD_GetCursorName(HSTMT hstmt,
					SQLCHAR * szCursor,
					SQLSMALLINT cbCursorMax,
					SQLSMALLINT * pcbCursor)
{
	CSTR func = "WD_GetCursorName";
	StatementClass *stmt = (StatementClass *) hstmt;
	size_t		len = 0;
	RETCODE		result;

	MYLOG(0, "entering hstmt=%p, szCursor=%p, cbCursorMax=%d, pcbCursor=%p\n", hstmt, szCursor, cbCursorMax, pcbCursor);

	if (!stmt)
	{
		SC_log_error(func, NULL_STRING, NULL);
		return SQL_INVALID_HANDLE;
	}
	result = SQL_SUCCESS;
	len = strlen(SC_cursor_name(stmt));

	if (szCursor)
	{
		strncpy_null((char *) szCursor, SC_cursor_name(stmt), cbCursorMax);

		if (len >= cbCursorMax)
		{
			result = SQL_SUCCESS_WITH_INFO;
			SC_set_error(stmt, STMT_TRUNCATED, "The buffer was too small for the GetCursorName.", func);
		}
	}

	if (pcbCursor)
		*pcbCursor = (SQLSMALLINT) len;

	/*
	 * Because this function causes no db-access, there's
	 * no need to call DiscardStatementSvp()
	 */

	return result;
}

RETCODE
SC_fetch_by_bookmark(StatementClass *stmt)
{
	UInt2		offset;
	SQLLEN		kres_ridx;
	OID		oidint;
	UInt4		blocknum;
	QResultClass	*res, *qres;
	RETCODE		ret = SQL_ERROR;

	HSTMT		hstmt = NULL;
	StatementClass	*fstmt;
	SQLLEN		size_of_rowset,	cached_rows;
	SQLULEN		cRow;
	UInt2		num_fields;
	ARDFields	*opts = SC_get_ARDF(stmt);
	SQLHDESC	hdesc;
	APDFields	*apdf;
	const int	tidbuflen = 20;
	char 		*tidbuf = NULL, *query = NULL;
	int		i, lodlen;
	BindInfoClass	*bookmark_orig = opts->bookmark;
	TupleField	*otuple, *ituple;
	SQLUSMALLINT	*rowStatusArray;

	MYLOG(0, "entering\n");

	if (!(res = SC_get_Curres(stmt)))
	{
		SC_set_error(stmt, STMT_INVALID_CURSOR_STATE_ERROR, "Null statement result in SC_fetch_by_bookmark.", __FUNCTION__);
		return SQL_ERROR;
	}
	if (SC_update_not_ready(stmt))
		parse_statement(stmt, TRUE);	/* not preferable */
	if (!SC_is_updatable(stmt))
	{
		stmt->options.scroll_concurrency = SQL_CONCUR_READ_ONLY;
		SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "the statement is read-only", __FUNCTION__);
		return SQL_ERROR;
	}
	if (ret = WD_AllocStmt(SC_get_conn(stmt), &hstmt, 0), !SQL_SUCCEEDED(ret))
	{
		SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "internal AllocStmt error", __FUNCTION__);
		return ret;
	}
	size_of_rowset = opts->size_of_rowset;
	SC_MALLOC_gexit_with_error(tidbuf, char, size_of_rowset * tidbuflen, stmt, "Couldn't allocate memory for tidbuf bind.", (ret = SQL_ERROR));
	for (i = 0; i < size_of_rowset; i++)
	{
		WD_BM	WD_bm;
		SQLLEN	bidx;

		WD_bm = SC_Resolve_bookmark(opts, i);
		bidx = WD_bm.index;

MYLOG(0, "i=%d bidx=" FORMAT_LEN " cached=" FORMAT_ULEN "\n", i, bidx, res->num_cached_keys);
		kres_ridx = GIdx2KResIdx(bidx, stmt, res);
		if (kres_ridx < 0 || kres_ridx >= res->num_cached_keys)
		{
			if (WD_bm.keys.offset > 0)
			{
				QR_get_last_bookmark(res, bidx, &WD_bm.keys);
				blocknum = WD_bm.keys.blocknum;
				offset = WD_bm.keys.offset;
				oidint = WD_bm.keys.oid;
			}
			else
			{
				SC_set_error(stmt, STMT_ROW_OUT_OF_RANGE, "the target rows is out of the rowset", __FUNCTION__);
				goto cleanup;
			}
		}
		else
		{
			if (!(oidint = getOid(res, kres_ridx)))
			{
				if (!strcmp(SAFE_NAME(stmt->ti[0]->bestitem), OID_NAME))
				{
					SC_set_error(stmt, STMT_ROW_VERSION_CHANGED, "the row was already deleted ?", __FUNCTION__);
				}
			}
			getTid(res, kres_ridx, &blocknum, &offset);
		}
		snprintf(tidbuf + i * tidbuflen, tidbuflen, "(%u,%u)", blocknum, offset);
		MYLOG(0, "!!!! tidbuf=%s\n", tidbuf + i * tidbuflen);
	}
	if (!SQL_SUCCEEDED(WD_BindParameter(hstmt, 1, SQL_PARAM_INPUT,
			SQL_C_CHAR, SQL_CHAR, tidbuflen, 0,
			tidbuf, tidbuflen, NULL)))
		goto cleanup;
	apdf = SC_get_APDF((StatementClass *) hstmt);
	apdf->paramset_size = size_of_rowset;
	if (!SQL_SUCCEEDED(WD_GetStmtAttr(stmt, SQL_ATTR_APP_ROW_DESC,
                                          (SQLPOINTER)&hdesc, SQL_IS_POINTER,
                                          NULL, false)))
		goto cleanup;
	if (!SQL_SUCCEEDED(WD_SetStmtAttr(hstmt, SQL_ATTR_APP_ROW_DESC,
                                          (SQLPOINTER)hdesc, SQL_IS_POINTER,
                                          false)))
		goto cleanup;

	lodlen = strlen(stmt->load_statement) + 15;
	SC_MALLOC_gexit_with_error(query, char, lodlen, stmt, "Couldn't allocate memory for query buf.", (ret = SQL_ERROR));
	snprintf(query, lodlen, "%s where ctid=?", stmt->load_statement);
	if (!SQL_SUCCEEDED(ret = WD_ExecDirect(hstmt, (SQLCHAR *) query, SQL_NTS, PODBC_RDONLY)))
		goto cleanup;
	/*
	 * Combine multiple results into one
	 */
	fstmt = (StatementClass *) hstmt;
	res = SC_get_Result(fstmt);
	num_fields = QR_NumResultCols(res);
	cached_rows = QR_get_num_cached_tuples(res);
	if (size_of_rowset > res->count_backend_allocated)
	{
		SC_REALLOC_gexit_with_error(res->backend_tuples, TupleField, size_of_rowset * sizeof(TupleField) * num_fields, hstmt, "Couldn't realloc memory for backend.", (ret = SQL_ERROR));
		res->count_backend_allocated = size_of_rowset;
	}
	memset(res->backend_tuples + num_fields * cached_rows, 0, (size_of_rowset - cached_rows) * num_fields * sizeof(TupleField));
	QR_set_num_cached_rows(res, size_of_rowset);
	res->num_total_read = size_of_rowset;
	rowStatusArray = (SC_get_IRDF(stmt))->rowStatusArray;
	for (i = 0, qres = res; i < size_of_rowset && NULL != qres; i++, qres = QR_nextr(qres))
	{
		if (1 == QR_get_num_cached_tuples(qres))
		{
			otuple = res->backend_tuples + i * num_fields;
			ituple = qres->backend_tuples;
			if (otuple != ituple)
				MoveCachedRows(otuple, ituple, num_fields, 1);
			if (NULL != rowStatusArray)
				rowStatusArray[i] = SQL_ROW_SUCCESS;
		}
		else if (NULL != rowStatusArray)
			rowStatusArray[i] = SQL_ROW_DELETED;
	}

	/* Fetch and fill bind info */
	cRow = 0;
	opts->bookmark = NULL;
	//ret = WD_ExtendedFetch(fstmt, SQL_FETCH_NEXT, 0,
	//	&cRow, NULL, 0, size_of_rowset);
	MYLOG(0, "cRow=" FORMAT_ULEN "\n", cRow);

cleanup:
	if (NULL != hstmt)
	{
          WD_SetStmtAttr(hstmt, SQL_ATTR_APP_ROW_DESC, (SQLPOINTER)NULL,
                         SQL_IS_POINTER, false);
		WD_FreeStmt(hstmt, SQL_DROP);
	}
	opts->bookmark = bookmark_orig;
	if (NULL != tidbuf)
		free(tidbuf);
	if (NULL != query)
		free(query);

	return ret;
}
