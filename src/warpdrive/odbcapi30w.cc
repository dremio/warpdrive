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

#include "ODBCConnection.h"
#include "ODBCDescriptor.h"
#include "ODBCEnvironment.h"
#include "ODBCStatement.h"
#include "c_ptr.h"

using namespace ODBC;
using namespace driver::odbcabstraction;

extern "C" {
WD_EXPORT_SYMBOL
RETCODE	SQL_API
SQLGetStmtAttrW(SQLHSTMT hstmt,
				SQLINTEGER	fAttribute,
				PTR		rgbValue,
				SQLINTEGER	cbValueMax,
				SQLINTEGER	*pcbValue)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
    RETCODE ret = SQL_SUCCESS;

    ret =
        WD_GetStmtAttr(hstmt, fAttribute, rgbValue, cbValueMax, pcbValue, true);
    return ret;
        });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLSetStmtAttrW(SQLHSTMT hstmt,
				SQLINTEGER	fAttribute,
				PTR		rgbValue,
				SQLINTEGER	cbValueMax)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {

    MYLOG(0, "Entering\n");
    return WD_SetStmtAttr(hstmt, fAttribute, rgbValue, cbValueMax, true);
        });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetConnectAttrW(HDBC hdbc,
				   SQLINTEGER	fAttribute,
				   PTR		rgbValue,
				   SQLINTEGER	cbValueMax,
				   SQLINTEGER	*pcbValue)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_GetConnectAttr(hdbc, fAttribute, rgbValue, cbValueMax, pcbValue,
                            true);
        });
                                                }
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLSetConnectAttrW(HDBC hdbc,
				   SQLINTEGER	fAttribute,
				   PTR		rgbValue,
				   SQLINTEGER	cbValue)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCConnection::ExecuteWithDiagnostics(hdbc, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering " FORMAT_INTEGER "\n", fAttribute);
    return WD_SetConnectAttr(hdbc, fAttribute, rgbValue, cbValue, true);
  });
}

/*      new function */
WD_EXPORT_SYMBOL
RETCODE  SQL_API
SQLSetDescFieldW(SQLHDESC DescriptorHandle, SQLSMALLINT RecNumber,
				 SQLSMALLINT FieldIdentifier, PTR Value,
				 SQLINTEGER BufferLength)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    SQLLEN vallen;
    c_ptr uval;

    MYLOG(0, "Entering\n");
    if (BufferLength > 0 || SQL_NTS == BufferLength) {
      switch (FieldIdentifier) {
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
        uval.reset(
            ucs2_to_utf8(static_cast<SQLWCHAR *>(Value),
                         BufferLength > 0 ? BufferLength / WCLEN : BufferLength,
                         &vallen, FALSE));
        break;
      }
    }
    return WD_SetDescField(DescriptorHandle, RecNumber, FieldIdentifier, uval ? uval.get() : Value,
                          uval ? (SQLINTEGER)vallen : BufferLength);
        });
}

WD_EXPORT_SYMBOL
RETCODE SQL_API
SQLGetDescFieldW(SQLHDESC hdesc, SQLSMALLINT iRecord, SQLSMALLINT iField,
				 PTR rgbValue, SQLINTEGER cbValueMax,
				 SQLINTEGER *pcbValue)
{
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(hdesc, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    SQLINTEGER blen = 0, bMax, *pcbV;
    c_ptr rgbV;

    MYLOG(0, "Entering\n");
    switch (iField) {
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
      rgbV.reset(static_cast<char *>(malloc(bMax + 1)));
      pcbV = &blen;
      for (;;
           bMax = blen + 1, rgbV.reallocate(bMax)) {
        if (!rgbV) {
          throw std::bad_alloc();
        }
        ret = WD_GetDescField(hdesc, iRecord, iField, rgbV.get(), bMax, pcbV);
        if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
            break;
        else
            // Get rid of any internally-generated truncation warnings.
            ODBCStatement::of(hdesc)->GetDiagnostics().Clear();
      }
      if (SQL_SUCCEEDED(ret)) {
        blen = (SQLINTEGER)utf8_to_ucs2(rgbV.get(), blen, (SQLWCHAR *)rgbValue,
                                        cbValueMax / WCLEN);
        if (SQL_SUCCESS == ret &&
            static_cast<SQLINTEGER>(blen * WCLEN) >= cbValueMax) {
          ret = SQL_SUCCESS_WITH_INFO;
          ODBCDescriptor::of(hdesc)->GetDiagnostics().AddTruncationWarning();
        }
        if (pcbValue)
          *pcbValue = blen * WCLEN;
      }
      break;
    default:
      ret = WD_GetDescField(hdesc, iRecord, iField, rgbValue, cbValueMax, pcbValue);
      break;
    }

    return ret;
        });
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
        char    qstr_ansi[8];
        c_ptr mtxt;

	MYLOG(0, "Entering\n");
	buflen = 0;
        if (szErrorMsg && cbErrorMsgMax > 0)
	{
		buflen = cbErrorMsgMax;
                mtxt.reset(static_cast<char*>(malloc(buflen)));
	}
	ret = WD_GetDiagRec(fHandleType, handle, iRecord, (SQLCHAR *) qstr_ansi,
						   pfNativeError, (SQLCHAR *) mtxt.get(), buflen, &tlen);
	if (SQL_SUCCEEDED(ret))
	{
		if (szSqlState)
			utf8_to_ucs2(qstr_ansi, -1, szSqlState, 6);
		if (mtxt && tlen <= cbErrorMsgMax)
		{
			SQLULEN ulen = utf8_to_ucs2_lf(mtxt.get(), tlen, FALSE, szErrorMsg, cbErrorMsgMax, TRUE);
			if (ulen == (SQLULEN) -1)
				tlen = (SQLSMALLINT) locale_to_sqlwchar((SQLWCHAR *) szErrorMsg, mtxt.get(), cbErrorMsgMax, FALSE);
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
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCStatement::ExecuteWithDiagnostics(hstmt, rc, [&]() -> SQLRETURN {
    CSTR func = "SQLColAttributeW";
    RETCODE ret;
    SQLSMALLINT *rgbL, blen = 0, bMax;
    c_ptr rgbD;

    MYLOG(0, "Entering\n");
    switch (iField) {
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
      rgbD.reset(static_cast<char *>(malloc(bMax)));
      rgbL = &blen;
      for (;;
           bMax = blen + 1, rgbD.reallocate(bMax)) {
        if (!rgbD) {
          ret = SQL_ERROR;
          throw std::bad_alloc();
        }
        ret = WD_ColAttributes(hstmt, iCol, iField, rgbD.get(), bMax, rgbL,
                               static_cast<SQLLEN *>(pNumAttr));
        if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
            break;
        else
            // Get rid of any internally-generated truncation warnings.
            ODBCStatement::of(hstmt)->GetDiagnostics().Clear();
      }
      if (SQL_SUCCEEDED(ret)) {
        blen = (SQLSMALLINT)utf8_to_ucs2(rgbD.get(), blen, (SQLWCHAR *)pCharAttr,
                                         cbCharAttrMax / WCLEN);
        if (SQL_SUCCESS == ret &&
            static_cast<SQLSMALLINT>(blen * WCLEN) >= cbCharAttrMax) {
          ret = SQL_SUCCESS_WITH_INFO;
          ODBCStatement::of(hstmt)->GetDiagnostics().AddTruncationWarning();
        }
        if (pcbCharAttr)
          *pcbCharAttr = blen * WCLEN;
      }
      break;
    default:
      ret = WD_ColAttributes(hstmt, iCol, iField, pCharAttr, cbCharAttrMax, pcbCharAttr,
                             static_cast<SQLLEN *>(pNumAttr));
      break;
    }

    return ret;
        });
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
  // Note: Ensure the diagnostics manager is empty at the end of this function.
  RETCODE ret = SQL_SUCCESS;
  try {
    SQLSMALLINT *rgbL, blen = 0, bMax;
    c_ptr rgbD;

    MYLOG(0, "Entering Handle=(%u,%p) Rec=%d Id=%d info=(%p,%d)\n", fHandleType,
          handle, iRecord, fDiagField, rgbDiagInfo, cbDiagInfoMax);

    switch (fDiagField) {
    case SQL_DIAG_DYNAMIC_FUNCTION:
    case SQL_DIAG_CLASS_ORIGIN:
    case SQL_DIAG_CONNECTION_NAME:
    case SQL_DIAG_MESSAGE_TEXT:
    case SQL_DIAG_SERVER_NAME:
    case SQL_DIAG_SQLSTATE:
    case SQL_DIAG_SUBCLASS_ORIGIN:
      bMax = cbDiagInfoMax * 3 / WCLEN + 1;
      rgbD.reset(static_cast<char *>(malloc(bMax)));
      if (!rgbD)
        return SQL_ERROR;
      rgbL = &blen;
      for (;; bMax = blen + 1, rgbD.reallocate(bMax)) {
        if (!rgbD) {
          return SQL_ERROR;
        }
        ret = WD_GetDiagField(fHandleType, handle, iRecord, fDiagField,
                              rgbD.get(), bMax, rgbL);
        if (SQL_SUCCESS_WITH_INFO != ret || blen < bMax)
          break;
      }
      if (SQL_SUCCEEDED(ret)) {
        SQLULEN ulen = (SQLSMALLINT)utf8_to_ucs2_lf(
            rgbD.get(), blen, FALSE, (SQLWCHAR *)rgbDiagInfo, cbDiagInfoMax / WCLEN,
            TRUE);
        if (ulen == (SQLULEN)-1)
          blen = (SQLSMALLINT)locale_to_sqlwchar((SQLWCHAR *)rgbDiagInfo, rgbD.get(),
                                                 cbDiagInfoMax / WCLEN, FALSE);
        else
          blen = (SQLSMALLINT)ulen;
        if (SQL_SUCCESS == ret &&
            blen * WCLEN >= static_cast<unsigned long>(cbDiagInfoMax))
          ret = SQL_SUCCESS_WITH_INFO;
        if (pcbDiagInfo) {
          *pcbDiagInfo = blen * WCLEN;
        }
      }
      break;
    default:
      ret = WD_GetDiagField(fHandleType, handle, iRecord, fDiagField, rgbDiagInfo,
                            cbDiagInfoMax, pcbDiagInfo);
      break;
    }
  } catch (const std::exception& e) {
    MYLOG(0, "Error when getting diagnostics: %s", e.what());
    ret = SQL_ERROR;
  } catch (...) {
    MYLOG(0, "Unknown error when getting diagnostics.");
    ret = SQL_ERROR;
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
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    RETCODE ret;
    SQLSMALLINT buflen, nmlen;
    c_ptr clName;

    MYLOG(0, "Entering\n");
    buflen = 0;
    if (BufferLength > 0)
      buflen = BufferLength * 3;
    else if (StringLength)
      buflen = 32;
    if (buflen > 0)
      clName.reset(static_cast<char *>(malloc(buflen)));
    for (;; buflen = nmlen + 1,
            clName.reallocate(buflen)) {
      if (!clName) {
        ret = SQL_ERROR;
        throw std::bad_alloc();
      }
      ret = WD_GetDescRec(DescriptorHandle, RecNumber, (SQLCHAR *)clName.get(),
                          buflen, &nmlen, Type, SubType, Length, Precision,
                          Scale, Nullable);
      if (SQL_SUCCESS_WITH_INFO != ret || nmlen < buflen)
        break;
    }
    if (SQL_SUCCEEDED(ret)) {
      SQLLEN nmcount = nmlen;

      if (nmlen < buflen)
        nmcount = utf8_to_ucs2(clName.get(), nmlen, Name, BufferLength);
      if (SQL_SUCCESS == ret && BufferLength > 0 && nmcount > BufferLength) {
        ret = SQL_SUCCESS_WITH_INFO;
        ODBCDescriptor::of(DescriptorHandle)->GetDiagnostics().AddTruncationWarning();
      }
      if (StringLength)
        *StringLength = (SQLSMALLINT)nmcount;
    }
    return ret;
        });
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
  SQLRETURN rc = SQL_SUCCESS;
  return ODBCDescriptor::ExecuteWithDiagnostics(DescriptorHandle, rc, [&]() -> SQLRETURN {
    MYLOG(0, "Entering\n");
    return WD_SetDescRec(DescriptorHandle, RecNumber, Type, SubType, Length,
                        Precision, Scale, Data, StringLength, Indicator);
        });
}
