/*-------
 * Module:			odbcapi30w.c
 *
 * Description:		This module contains UNICODE routines
 *
 * Classes:			n/a
 *
 * API functions:	SQLColAttributeW, SQLGetStmtAttrW, SQLSetStmtAttrW,
			SQLSetConnectAttrW, SQLGetConnectAttrW,
			SQLGetDescFieldW, SQLGetDescRecW, SQLGetDiagFieldW,
			SQLGetDiagRecW,
 *-------
 */

#include "wdodbc.h"
#include "sqltypes.h"
#include "unicode_support.h"
#include <stdio.h>
#include <string.h>

#include "wdapifunc.h"
#include "connection.h"
#include "statement.h"
#include "misc.h"

extern "C" {
WD_EXPORT_SYMBOL
RETCODE	SQL_API
SQLGetStmtAttrW(SQLHSTMT hstmt,
				SQLINTEGER	fAttribute,
				PTR		rgbValue,
				SQLINTEGER	cbValueMax,
				SQLINTEGER	*pcbValue)
{
	RETCODE	ret = SQL_SUCCESS;
	StatementClass	*stmt = (StatementClass *) hstmt;

	MYLOG(0, "Entering\n");
	ENTER_STMT_CS((StatementClass *) hstmt);
//	ret = WD_GetStmtAttr(hstmt, fAttribute, rgbValue,
//		cbValueMax, pcbValue);
	LEAVE_STMT_CS((StatementClass *) hstmt);
	return ret;
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLSetStmtAttrW(SQLHSTMT hstmt,
				SQLINTEGER	fAttribute,
				PTR		rgbValue,
				SQLINTEGER	cbValueMax)
{
	RETCODE	ret = SQL_SUCCESS;
	StatementClass	*stmt = (StatementClass *) hstmt;

	MYLOG(0, "Entering\n");
	ENTER_STMT_CS(stmt);
//	ret = WD_SetStmtAttr(hstmt, fAttribute, rgbValue,
//		cbValueMax);
	LEAVE_STMT_CS(stmt);
	return ret;
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetConnectAttrW(HDBC hdbc,
				   SQLINTEGER	fAttribute,
				   PTR		rgbValue,
				   SQLINTEGER	cbValueMax,
				   SQLINTEGER	*pcbValue)
{
	RETCODE	ret = SQL_SUCCESS;

	MYLOG(0, "Entering\n");
	ENTER_CONN_CS((ConnectionClass *) hdbc);
//	ret = WD_GetConnectAttr(hdbc, fAttribute, rgbValue,
//		cbValueMax, pcbValue);
	LEAVE_CONN_CS((ConnectionClass *) hdbc);
	return ret;
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLSetConnectAttrW(HDBC hdbc,
				   SQLINTEGER	fAttribute,
				   PTR		rgbValue,
				   SQLINTEGER	cbValue)
{
	RETCODE	ret = SQL_SUCCESS;

	MYLOG(0, "Entering\n");
	ENTER_CONN_CS(conn);
//	ret = WD_SetConnectAttr(hdbc, fAttribute, rgbValue,
//		cbValue);
	LEAVE_CONN_CS(conn);
	return ret;
}

/*      new function */
WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
				 SQLSMALLINT FieldIdentifier, PTR Value,
				 SQLINTEGER BufferLength)
{
	RETCODE	ret;
	SQLLEN	vallen;
        char    *uval = NULL;
	BOOL	val_alloced = FALSE;

	MYLOG(0, "Entering\n");
	if (BufferLength > 0 || SQL_NTS == BufferLength)
	{
		switch (FieldIdentifier)
		{
			case SQL_DESC_BASE_COLUMN_NAME:
			case SQL_DESC_BASE_TABLE_NAME:
			case SQL_DESC_CATALOG_NAME:
			case SQL_DESC_LABEL:
			case SQL_DESC_LITERAL_PREFIX:
			case SQL_DESC_LITERAL_SUFFIX:
			case SQL_DESC_LOCAL_TYPE_NAME:
			case SQL_DESC_NAME:
			case SQL_DESC_SCHEMA_NAME:
			case SQL_DESC_TABLE_NAME:
			case SQL_DESC_TYPE_NAME:
				uval = ucs2_to_utf8(static_cast<SQLWCHAR*>(Value), BufferLength > 0 ? BufferLength / WCLEN : BufferLength, &vallen, FALSE);
				val_alloced = TRUE;
			break;
		}
	}
	if (!val_alloced)
	{
		uval = static_cast<char*>(Value);
		vallen = BufferLength;
	}
	ret = WD_SetDescField(DescriptorHandle, RecNumber, FieldIdentifier,
				uval, (SQLINTEGER) vallen);
	if (val_alloced)
		free(uval);
	return ret;
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetDescFieldW(SQLHDESC hdesc, SQLSMALLINT iRecord, SQLSMALLINT iField,
				 PTR rgbValue, SQLINTEGER cbValueMax,
				 SQLINTEGER *pcbValue)
{
	RETCODE	ret;
	SQLINTEGER		blen = 0, bMax,	*pcbV;
        char    *rgbV = NULL, *rgbVt;

	MYLOG(0, "Entering\n");
	switch (iField)
	{
		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_DESC_BASE_TABLE_NAME:
		case SQL_DESC_CATALOG_NAME:
		case SQL_DESC_LABEL:
		case SQL_DESC_LITERAL_PREFIX:
		case SQL_DESC_LITERAL_SUFFIX:
		case SQL_DESC_LOCAL_TYPE_NAME:
		case SQL_DESC_NAME:
		case SQL_DESC_SCHEMA_NAME:
		case SQL_DESC_TABLE_NAME:
		case SQL_DESC_TYPE_NAME:
			bMax = cbValueMax * 3 / WCLEN;
			rgbV = static_cast<char*>(malloc(bMax + 1));
			pcbV = &blen;
			for (rgbVt = rgbV;; bMax = blen + 1, rgbVt = static_cast<char*>(realloc(rgbV, bMax)))
			{
				if (!rgbVt)
				{
					ret = SQL_ERROR;
					break;
				}
				rgbV = rgbVt;
				ret = WD_GetDescField(hdesc, iRecord, iField, rgbV, bMax, pcbV);
				if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
					break;
			}
			if (SQL_SUCCEEDED(ret))
			{
				blen = (SQLINTEGER) utf8_to_ucs2(rgbV, blen, (SQLWCHAR *) rgbValue, cbValueMax / WCLEN);
				if (SQL_SUCCESS == ret && static_cast<SQLINTEGER>(blen * WCLEN) >= cbValueMax)
				{
					ret = SQL_SUCCESS_WITH_INFO;
					DC_set_error(static_cast<DescriptorClass*>(hdesc), STMT_TRUNCATED, "The buffer was too small for the rgbDesc.");
				}
				if (pcbValue)
					*pcbValue = blen * WCLEN;
			}
			if (rgbV)
				free(rgbV);
			break;
		default:
			rgbV = static_cast<char*>(rgbValue);
			bMax = cbValueMax;
			pcbV = pcbValue;
			ret = WD_GetDescField(hdesc, iRecord, iField, rgbV, bMax, pcbV);
			break;
	}

	return ret;
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetDiagRecW(SQLSMALLINT fHandleType,
			   SQLHANDLE	handle,
			   SQLSMALLINT	iRecord,
			   SQLWCHAR	*szSqlState,
			   SQLINTEGER	*pfNativeError,
			   SQLWCHAR	*szErrorMsg,
			   SQLSMALLINT	cbErrorMsgMax,
			   SQLSMALLINT	*pcbErrorMsg)
{
	RETCODE	ret;
        SQLSMALLINT	buflen, tlen;
        char    qstr_ansi[8], *mtxt = NULL;

	MYLOG(0, "Entering\n");
	buflen = 0;
        if (szErrorMsg && cbErrorMsgMax > 0)
	{
		buflen = cbErrorMsgMax;
                mtxt = static_cast<char*>(malloc(buflen));
	}
	ret = WD_GetDiagRec(fHandleType, handle, iRecord, (SQLCHAR *) qstr_ansi,
						   pfNativeError, (SQLCHAR *) mtxt, buflen, &tlen);
	if (SQL_SUCCEEDED(ret))
	{
		if (szSqlState)
			utf8_to_ucs2(qstr_ansi, -1, szSqlState, 6);
		if (mtxt && tlen <= cbErrorMsgMax)
		{
			SQLULEN ulen = utf8_to_ucs2_lf(mtxt, tlen, FALSE, szErrorMsg, cbErrorMsgMax, TRUE);
			if (ulen == (SQLULEN) -1)
				tlen = (SQLSMALLINT) locale_to_sqlwchar((SQLWCHAR *) szErrorMsg, mtxt, cbErrorMsgMax, FALSE);
			else
				tlen = (SQLSMALLINT) ulen;
			if (tlen >= cbErrorMsgMax)
				ret = SQL_SUCCESS_WITH_INFO;
			else if (tlen < 0)
			{
				char errc[32];

				SPRINTF_FIXED(errc, "Error: SqlState=%s", qstr_ansi);
				tlen = utf8_to_ucs2(errc, -1, szErrorMsg, cbErrorMsgMax);
			}
		}
		if (pcbErrorMsg)
			*pcbErrorMsg = tlen;
	}
	if (mtxt)
		free(mtxt);
	return ret;
}

WD_EXPORT_SYMBOL
SQLRETURN SQL_API
SQLColAttributeW(SQLHSTMT	hstmt,
				 SQLUSMALLINT	iCol,
				 SQLUSMALLINT	iField,
				 SQLPOINTER	pCharAttr,
				 SQLSMALLINT	cbCharAttrMax,
				 SQLSMALLINT	*pcbCharAttr,
#if defined(WITH_UNIXODBC) || defined(_WIN64) || defined(SQLCOLATTRIBUTE_SQLLEN)
				 SQLLEN		*pNumAttr
#else
				 SQLPOINTER	pNumAttr
#endif
	)
{
	CSTR func = "SQLColAttributeW";
	RETCODE	ret;
	SQLSMALLINT	*rgbL, blen = 0, bMax;
        char    *rgbD = NULL, *rgbDt;

	MYLOG(0, "Entering\n");
	switch (iField)
	{
		case SQL_DESC_BASE_COLUMN_NAME:
		case SQL_DESC_BASE_TABLE_NAME:
		case SQL_DESC_CATALOG_NAME:
		case SQL_DESC_LABEL:
		case SQL_DESC_LITERAL_PREFIX:
		case SQL_DESC_LITERAL_SUFFIX:
		case SQL_DESC_LOCAL_TYPE_NAME:
		case SQL_DESC_NAME:
		case SQL_DESC_SCHEMA_NAME:
		case SQL_DESC_TABLE_NAME:
		case SQL_DESC_TYPE_NAME:
		case SQL_COLUMN_NAME:
			bMax = cbCharAttrMax * 3 / WCLEN;
			rgbD = static_cast<char*>(malloc(bMax));
			rgbL = &blen;
			for (rgbDt = rgbD;; bMax = blen + 1, rgbDt = static_cast<char*>(realloc(rgbD, bMax)))
			{
				if (!rgbDt)
				{
					ret = SQL_ERROR;
					break;
				}
				rgbD = rgbDt;
				ret = WD_ColAttributes(hstmt, iCol, iField, rgbD,
					bMax, rgbL, static_cast<SQLLEN*>(pNumAttr));
				if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
					break;
			}
			if (SQL_SUCCEEDED(ret))
			{
				blen = (SQLSMALLINT) utf8_to_ucs2(rgbD, blen, (SQLWCHAR *) pCharAttr, cbCharAttrMax / WCLEN);
				if (SQL_SUCCESS == ret && static_cast<SQLSMALLINT>(blen * WCLEN) >= cbCharAttrMax)
				{
					ret = SQL_SUCCESS_WITH_INFO;
//					SC_set_error(stmt, STMT_TRUNCATED, "The buffer was too small for the pCharAttr.", func);
				}
				if (pcbCharAttr)
					*pcbCharAttr = blen * WCLEN;
			}
			if (rgbD)
				free(rgbD);
			break;
		default:
			rgbD = static_cast<char*>(pCharAttr);
			bMax = cbCharAttrMax;
			rgbL = pcbCharAttr;
			ret = WD_ColAttributes(hstmt, iCol, iField, rgbD,
					bMax, rgbL, static_cast<SQLLEN*>(pNumAttr));
			break;
	}
	LEAVE_STMT_CS(stmt);

	return ret;
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetDiagFieldW(SQLSMALLINT	fHandleType,
				 SQLHANDLE		handle,
				 SQLSMALLINT	iRecord,
				 SQLSMALLINT	fDiagField,
				 SQLPOINTER		rgbDiagInfo,
				 SQLSMALLINT	cbDiagInfoMax,
				 SQLSMALLINT   *pcbDiagInfo)
{
	RETCODE		ret;
	SQLSMALLINT	*rgbL, blen = 0, bMax;
	char	   *rgbD = NULL, *rgbDt;

	MYLOG(0, "Entering Handle=(%u,%p) Rec=%d Id=%d info=(%p,%d)\n", fHandleType,
			handle, iRecord, fDiagField, rgbDiagInfo, cbDiagInfoMax);
	switch (fDiagField)
	{
		case SQL_DIAG_DYNAMIC_FUNCTION:
		case SQL_DIAG_CLASS_ORIGIN:
		case SQL_DIAG_CONNECTION_NAME:
		case SQL_DIAG_MESSAGE_TEXT:
		case SQL_DIAG_SERVER_NAME:
		case SQL_DIAG_SQLSTATE:
		case SQL_DIAG_SUBCLASS_ORIGIN:
			bMax = cbDiagInfoMax * 3 / WCLEN + 1;
			if (rgbD = static_cast<char*>(malloc(bMax)), !rgbD)
				return SQL_ERROR;
			rgbL = &blen;
			for (rgbDt = rgbD;; bMax = blen + 1, rgbDt = static_cast<char*>(realloc(rgbD, bMax)))
			{
				if (!rgbDt)
				{
					free(rgbD);
					return SQL_ERROR;
				}
				rgbD = rgbDt;
				ret = WD_GetDiagField(fHandleType, handle, iRecord, fDiagField, rgbD,
					bMax, rgbL);
				if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
					break;
			}
			if (SQL_SUCCEEDED(ret))
			{
				SQLULEN ulen = (SQLSMALLINT) utf8_to_ucs2_lf(rgbD, blen, FALSE, (SQLWCHAR *) rgbDiagInfo, cbDiagInfoMax / WCLEN, TRUE);
				if (ulen == (SQLULEN) -1)
					blen = (SQLSMALLINT) locale_to_sqlwchar((SQLWCHAR *) rgbDiagInfo, rgbD, cbDiagInfoMax / WCLEN, FALSE);
				else
					blen = (SQLSMALLINT) ulen;
				if (SQL_SUCCESS == ret && blen * WCLEN >= static_cast<unsigned long>(cbDiagInfoMax))
					ret = SQL_SUCCESS_WITH_INFO;
				if (pcbDiagInfo)
				{
					*pcbDiagInfo = blen * WCLEN;
				}
			}
			if (rgbD)
				free(rgbD);
			break;
		default:
			rgbD = static_cast<char*>(rgbDiagInfo);
			bMax = cbDiagInfoMax;
			rgbL = pcbDiagInfo;
			ret = WD_GetDiagField(fHandleType, handle, iRecord, fDiagField, rgbD,
				bMax, rgbL);
			break;
	}

	return ret;
}

/*	new function */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLGetDescRecW(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLWCHAR *Name,
			  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
			  SQLSMALLINT *Type, SQLSMALLINT *SubType,
			  SQLLEN *Length, SQLSMALLINT *Precision,
			  SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
	RETCODE	ret;
	SQLSMALLINT	buflen, nmlen;
	char	*clName = NULL, *clNamet = NULL;

	MYLOG(0, "Entering\n");
	buflen = 0;
	if (BufferLength > 0)
		buflen = BufferLength * 3;
	else if (StringLength)
		buflen = 32;
	if (buflen > 0)
		clNamet = static_cast<char*>(malloc(buflen));
	for (;; buflen = nmlen + 1, clNamet = static_cast<char*>(realloc(clName, buflen)))
	{
		if (!clNamet)
		{
		//	SC_set_error(stmt, STMT_NO_MEMORY_ERROR, "Could not allocate memory for column name", func);
			ret = SQL_ERROR;
			break;
		}
		clName = clNamet;
		ret = WD_GetDescRec(DescriptorHandle, RecNumber,
								(SQLCHAR *) clName, buflen,
								&nmlen, Type, SubType, Length,
			Precision, Scale, Nullable);
		if (SQL_SUCCESS_WITH_INFO != ret || nmlen < buflen)
			break;
	}
	if (SQL_SUCCEEDED(ret))
	{
		SQLLEN	nmcount = nmlen;

		if (nmlen < buflen)
			nmcount = utf8_to_ucs2(clName, nmlen, Name, BufferLength);
		if (SQL_SUCCESS == ret && BufferLength > 0 && nmcount > BufferLength)
		{
			ret = SQL_SUCCESS_WITH_INFO;
		//	SC_set_error(stmt, STMT_TRUNCATED, "Column name too large", func);
		}
		if (StringLength)
			*StringLength = (SQLSMALLINT) nmcount;
	}
	if (clName)
		free(clName);
	return ret;
}

/*	new fucntion */
WD_EXPORT_SYMBOL
RETCODE		SQL_API
SQLSetDescRecW(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLSMALLINT Type,
			  SQLSMALLINT SubType, SQLLEN Length,
			  SQLSMALLINT Precision, SQLSMALLINT Scale,
			  PTR Data, SQLLEN *StringLength,
			  SQLLEN *Indicator)
{
	RETCODE ret;
	MYLOG(0, "Entering\n");
	ret = WD_SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length, Precision, Scale, Data, StringLength, Indicator);
	return ret;
}
}
