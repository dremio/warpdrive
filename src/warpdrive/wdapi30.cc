/*-------
 * Module:			wdapi30.c
 *
 * Description:		This module contains routines related to ODBC 3.0
 *			most of their implementations are temporary
 *			and must be rewritten properly.
 *			2001/07/23	inoue
 *
 * Classes:			n/a
 *
 * API functions:	WD_ColAttribute, WD_GetDiagRec,
			WD_GetConnectAttr, WD_GetStmtAttr,
			WD_SetConnectAttr, WD_SetStmtAttr
 *-------
 */

#include "wdodbc.h"
#include "sqltypes.h"
#include "misc.h"

#include <stdio.h>
#include <string.h>


#include "environ.h"
#include "connection.h"
#include "statement.h"
#include "descriptor.h"
#include "qresult.h"
#include "wdapifunc.h"
#include "loadlib.h"
//#include "dlg_specific.h"

#include "AttributeUtils.h"
#include "ODBCEnvironment.h"
#include "ODBCConnection.h"
#include "ODBCStatement.h"
#include "ODBCDescriptor.h"
#include <odbcabstraction/exceptions.h>

using namespace ODBC;
using namespace driver::odbcabstraction;

/*	SQLError -> SQLDiagRec */
RETCODE		SQL_API
WD_GetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle,
				 SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
				 SQLINTEGER *NativeError, SQLCHAR *MessageText,
				 SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
	RETCODE		ret = SQL_SUCCESS;

	MYLOG(0, "entering type=%d rec=%d buffer=%d\n", HandleType, RecNumber, BufferLength);
        Diagnostics* diagnostics = nullptr;
	switch (HandleType) {
        case SQL_HANDLE_ENV:
          diagnostics = &ODBCEnvironment::of(Handle)->GetDiagnostics();
          break;
        case SQL_HANDLE_DBC:
          diagnostics = &ODBCConnection::of(Handle)->GetDiagnostics();
          break;
        case SQL_HANDLE_STMT:
          diagnostics = &ODBCStatement::of(Handle)->GetDiagnostics();
          break;
        case SQL_HANDLE_DESC:
          diagnostics = &ODBCDescriptor::of(Handle)->GetDiagnostics();
          break;
        default:
          return SQL_INVALID_HANDLE;
        }

        if (RecNumber <= 0) {
          return SQL_ERROR;
        }

        uint32_t zeroBasedRecNumber = RecNumber - 1;
        if (!diagnostics->HasRecord(zeroBasedRecNumber)) {
          return SQL_NO_DATA;
        }

        ret = GetAttributeUTF8(diagnostics->GetMessageText(zeroBasedRecNumber), MessageText, BufferLength, TextLength);
        GetAttributeUTF8(diagnostics->GetSQLState(zeroBasedRecNumber), Sqlstate, static_cast<SQLSMALLINT>(6), static_cast<SQLSMALLINT*>(NULL));
        GetAttribute<SQLINTEGER>(diagnostics->GetNativeError(zeroBasedRecNumber), NativeError, static_cast<SQLSMALLINT>(sizeof(SQLINTEGER)), static_cast<SQLSMALLINT*>(NULL));

	MYLOG(0, "leaving %d\n", ret);
	return ret;
}

/*
 *	Minimal implementation.
 *
 */
RETCODE		SQL_API
WD_GetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
				   SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
				   PTR DiagInfoPtr, SQLSMALLINT BufferLength,
				   SQLSMALLINT *StringLengthPtr)
{
	RETCODE		ret = SQL_ERROR, rtn;
	ConnectionClass	*conn;
	StatementClass	*stmt;
	SQLLEN		rc;
	SQLSMALLINT	pcbErrm;
	ssize_t		rtnlen = -1;
	int		rtnctype = SQL_C_CHAR;

	MYLOG(0, "entering rec=%d\n", RecNumber);
        const Diagnostics* diagnostics;
	switch (HandleType)
	{
		case SQL_HANDLE_ENV:
                  diagnostics = &ODBCEnvironment::of(Handle)->GetDiagnostics();
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
				case SQL_DIAG_SERVER_NAME:
					rtnlen = 0;
					if (DiagInfoPtr && BufferLength > rtnlen)
					{
						ret = SQL_SUCCESS;
						*((char *) DiagInfoPtr) = '\0';
					}
					else
						ret = SQL_SUCCESS_WITH_INFO;
					break;
				case SQL_DIAG_MESSAGE_TEXT:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    return GetAttributeUTF8(diagnostics->GetMessageText(RecNumber-1),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                  } else {
                                    return SQL_NO_DATA;
                                  }
				case SQL_DIAG_NATIVE:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    GetAttribute<SQLINTEGER>(static_cast<SQLINTEGER>(diagnostics->GetNativeError(RecNumber-1)),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                    return SQL_SUCCESS;
                                  } else {
                                    return SQL_NO_DATA;
                                  }
				case SQL_DIAG_NUMBER:
                                  GetAttribute<SQLINTEGER>(diagnostics->GetRecordCount(),
                                                           DiagInfoPtr, BufferLength, StringLengthPtr);
                                  return SQL_SUCCESS;
				case SQL_DIAG_SQLSTATE:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    return GetAttributeUTF8(diagnostics->GetSQLState(RecNumber-1),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                  } else {
                                    return SQL_NO_DATA;
                                  }
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
				case SQL_DIAG_ROW_COUNT:
				case SQL_DIAG_DYNAMIC_FUNCTION:
				case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
					/* options for statement type only */
					break;
			}
			break;
		case SQL_HANDLE_DBC:
			diagnostics = &ODBCConnection::of(Handle)->GetDiagnostics();
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
					rtnlen = 0;
					if (DiagInfoPtr && BufferLength > rtnlen)
					{
						ret = SQL_SUCCESS;
						*((char *) DiagInfoPtr) = '\0';
					}
					else
						ret = SQL_SUCCESS_WITH_INFO;
					break;
				case SQL_DIAG_SERVER_NAME:
					return GetAttributeUTF8(ODBCConnection::of(Handle)->GetDSN(), DiagInfoPtr, BufferLength, StringLengthPtr);
				case SQL_DIAG_MESSAGE_TEXT:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    return GetAttributeUTF8(diagnostics->GetMessageText(RecNumber-1),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                  } else {
                                    return SQL_NO_DATA;
                                  }
                                case SQL_DIAG_NATIVE:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    GetAttribute<SQLINTEGER>(static_cast<SQLINTEGER>(diagnostics->GetNativeError(RecNumber-1)),
                                                             DiagInfoPtr, BufferLength, StringLengthPtr);
                                    return SQL_SUCCESS;
                                  } else {
                                    return SQL_NO_DATA;
                                  }
                                case SQL_DIAG_NUMBER:
                                  GetAttribute<SQLINTEGER>(diagnostics->GetRecordCount(),
                                                           DiagInfoPtr, BufferLength, StringLengthPtr);
                                  return SQL_SUCCESS;
                                case SQL_DIAG_SQLSTATE:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    return GetAttributeUTF8(diagnostics->GetSQLState(RecNumber-1),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                  } else {
                                    return SQL_NO_DATA;
                                  }
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
				case SQL_DIAG_ROW_COUNT:
				case SQL_DIAG_DYNAMIC_FUNCTION:
				case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
					/* options for statement type only */
					break;
			}
			break;
		case SQL_HANDLE_STMT:
                  diagnostics = &ODBCStatement::of(Handle)->GetDiagnostics();
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
					rtnlen = 0;
					if (DiagInfoPtr && BufferLength > rtnlen)
					{
						ret = SQL_SUCCESS;
						*((char *) DiagInfoPtr) = '\0';
					}
					else
						ret = SQL_SUCCESS_WITH_INFO;
					break;
				case SQL_DIAG_SERVER_NAME:
					return GetAttributeUTF8(ODBCStatement::of(Handle)->GetConnection().GetDSN(), DiagInfoPtr, BufferLength, StringLengthPtr);
				case SQL_DIAG_MESSAGE_TEXT:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    return GetAttributeUTF8(diagnostics->GetMessageText(RecNumber-1),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                  } else {
                                    return SQL_NO_DATA;
                                  }
                                case SQL_DIAG_NATIVE:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    GetAttribute<SQLINTEGER>(static_cast<SQLINTEGER>(diagnostics->GetNativeError(RecNumber-1)),
                                                             DiagInfoPtr, BufferLength, StringLengthPtr);
                                    return SQL_SUCCESS;
                                  } else {
                                    return SQL_NO_DATA;
                                  }
                                case SQL_DIAG_NUMBER:
                                  GetAttribute<SQLINTEGER>(diagnostics->GetRecordCount(),
                                                           DiagInfoPtr, BufferLength, StringLengthPtr);
                                  return SQL_SUCCESS;
                                case SQL_DIAG_SQLSTATE:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    return GetAttributeUTF8(diagnostics->GetSQLState(RecNumber-1),
                                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                                  } else {
                                    return SQL_NO_DATA;
                                  }
                                  break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
                                  GetAttribute<SQLINTEGER>(-1,
                                                           DiagInfoPtr, BufferLength, StringLengthPtr);
                                  return SQL_SUCCESS;
				case SQL_DIAG_ROW_COUNT:
                                  GetAttribute<SQLINTEGER>(-1,
                                                           DiagInfoPtr, BufferLength, StringLengthPtr);
                                  return SQL_SUCCESS;
                                case SQL_DIAG_ROW_NUMBER:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    GetAttribute<SQLINTEGER>(-1, DiagInfoPtr,
                                                             BufferLength,
                                                             StringLengthPtr);
                                    return SQL_SUCCESS;
                                  } else {
                                    return SQL_NO_DATA;
                                  }
                                case SQL_DIAG_COLUMN_NUMBER:
                                  if (RecNumber <= 0) {
                                    return SQL_ERROR;
                                  } else if (diagnostics->HasRecord(RecNumber-1)) {
                                    GetAttribute<SQLINTEGER>(-1, DiagInfoPtr,
                                                             BufferLength,
                                                             StringLengthPtr);
                                    return SQL_SUCCESS;
                                  } else {
                                    return SQL_NO_DATA;
                                  }
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
			}
			break;
		case SQL_HANDLE_DESC:
                  diagnostics = &ODBCDescriptor::of(Handle)->GetDiagnostics();
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
					rtnlen = 0;
					if (DiagInfoPtr && BufferLength > rtnlen)
					{
						ret = SQL_SUCCESS;
						*((char *) DiagInfoPtr) = '\0';
					}
					else
						ret = SQL_SUCCESS_WITH_INFO;
					break;
				case SQL_DIAG_SERVER_NAME:
					return GetAttributeUTF8(ODBCDescriptor::of(Handle)->GetConnection().GetDSN(), DiagInfoPtr, BufferLength, StringLengthPtr);
				case SQL_DIAG_MESSAGE_TEXT:
                    if (RecNumber <= 0) {
                    return SQL_ERROR;
                    } else if (diagnostics->HasRecord(RecNumber-1)) {
                    return GetAttributeUTF8(diagnostics->GetMessageText(RecNumber-1),
                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                    } else {
                    ret = SQL_ERROR;
                    }
                    break;
                case SQL_DIAG_NATIVE:
                    if (RecNumber <= 0) {
                    return SQL_ERROR;
                    } else if (diagnostics->HasRecord(RecNumber-1)) {
                    GetAttribute<SQLINTEGER>(static_cast<SQLINTEGER>(diagnostics->GetNativeError(RecNumber-1)),
                                                DiagInfoPtr, BufferLength, StringLengthPtr);
                    return SQL_SUCCESS;
                    } else {
                    ret = SQL_ERROR;
                    }
                    break;
                case SQL_DIAG_NUMBER:
                    GetAttribute<SQLINTEGER>(diagnostics->GetRecordCount(),
                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                    return SQL_SUCCESS;
                case SQL_DIAG_SQLSTATE:
                    if (RecNumber <= 0) {
                    return SQL_ERROR;
                    } else if (diagnostics->HasRecord(RecNumber-1)) {
                    return GetAttributeUTF8(diagnostics->GetSQLState(RecNumber-1),
                                            DiagInfoPtr, BufferLength, StringLengthPtr);
                    } else {
                    ret = SQL_ERROR;
                    }
                    break;
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
				case SQL_DIAG_ROW_COUNT:
				case SQL_DIAG_DYNAMIC_FUNCTION:
				case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
					rtnctype = SQL_C_LONG;
					/* options for statement type only */
					break;
			}
			break;
		default:
			ret = SQL_ERROR;
	}
	if (SQL_C_LONG == rtnctype)
	{
		if (SQL_SUCCESS_WITH_INFO == ret)
			ret = SQL_SUCCESS;
		if (StringLengthPtr)
			*StringLengthPtr = sizeof(SQLINTEGER);
	}
	else if (rtnlen >= 0)
	{
		if (rtnlen >= BufferLength)
		{
			if (SQL_SUCCESS == ret)
				ret = SQL_SUCCESS_WITH_INFO;
			if (BufferLength > 0)
				((char *) DiagInfoPtr) [BufferLength - 1] = '\0';
		}
		if (StringLengthPtr)
			*StringLengthPtr = (SQLSMALLINT) rtnlen;
	}
	MYLOG(0, "leaving %d\n", ret);
	return ret;
}

/*	SQLGetConnectOption -> SQLGetconnectAttr */
RETCODE SQL_API WD_GetConnectAttr(HDBC ConnectionHandle, SQLINTEGER Attribute,
                                  PTR Value, SQLINTEGER BufferLength,
                                  SQLINTEGER *StringLength, bool isUnicode) {
  RETCODE ret = SQL_SUCCESS;

  MYLOG(0, "entering Handle=%p " FORMAT_INTEGER "\n", ConnectionHandle, Attribute);
  ODBCConnection* conn = reinterpret_cast<ODBCConnection*>(ConnectionHandle);
  conn->GetConnectAttr(Attribute, Value, BufferLength, StringLength, isUnicode);
  return ret;
}

static SQLHDESC
descHandleFromStatementHandle(HSTMT StatementHandle, SQLINTEGER descType)
{
	StatementClass	*stmt = (StatementClass *) StatementHandle;

	switch (descType)
	{
		case SQL_ATTR_APP_ROW_DESC:		/* 10010 */
			return (HSTMT) stmt->ard;
		case SQL_ATTR_APP_PARAM_DESC:		/* 10011 */
			return (HSTMT) stmt->apd;
		case SQL_ATTR_IMP_ROW_DESC:		/* 10012 */
			return (HSTMT) stmt->ird;
		case SQL_ATTR_IMP_PARAM_DESC:		/* 10013 */
			return (HSTMT) stmt->ipd;
	}
	return (HSTMT) 0;
}

static  void column_bindings_set(ARDFields *opts, int cols, BOOL maxset)
{
	int	i;

	if (cols == opts->allocated)
		return;
	if (cols > opts->allocated)
	{
		extend_column_bindings(opts, cols);
		return;
	}
	if (maxset)	return;

	for (i = opts->allocated; i > cols; i--)
		reset_a_column_binding(opts, i);
	opts->allocated = cols;
	if (0 == cols)
	{
		free(opts->bindings);
		opts->bindings = NULL;
	}
}

static RETCODE SQL_API
ARDSetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	ARDFields	*opts = &(desc->ardf);
	SQLSMALLINT	row_idx;
	BOOL		unbind = TRUE;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			opts->size_of_rowset = CAST_UPTR(SQLULEN, Value);
			return ret;
		case SQL_DESC_ARRAY_STATUS_PTR:
			opts->row_operation_ptr = static_cast<SQLUSMALLINT*>(Value);
			return ret;
		case SQL_DESC_BIND_OFFSET_PTR:
			opts->row_offset_ptr = static_cast<SQLULEN*>(Value);
			return ret;
		case SQL_DESC_BIND_TYPE:
			opts->bind_size = reinterpret_cast<SQLULEN>(Value);
			return ret;
		case SQL_DESC_COUNT:
			column_bindings_set(opts, static_cast<SQLSMALLINT>(reinterpret_cast<SQLLEN>(Value)), FALSE);
			return ret;

		case SQL_DESC_TYPE:
		case SQL_DESC_DATETIME_INTERVAL_CODE:
		case SQL_DESC_CONCISE_TYPE:
			column_bindings_set(opts, RecNumber, TRUE);
			break;
	}
	if (RecNumber < 0 || RecNumber > opts->allocated)
	{
		DC_set_error(desc, DESC_INVALID_COLUMN_NUMBER_ERROR, "invalid column number");
		return SQL_ERROR;
	}
	if (0 == RecNumber) /* bookmark column */
	{
		BindInfoClass	*bookmark = ARD_AllocBookmark(opts);

		switch (FieldIdentifier)
		{
			case SQL_DESC_DATA_PTR:
				bookmark->buffer = static_cast<char*>(Value);
				break;
			case SQL_DESC_INDICATOR_PTR:
				bookmark->indicator = static_cast<SQLLEN*>(Value);
				break;
			case SQL_DESC_OCTET_LENGTH_PTR:
				bookmark->used = static_cast<SQLLEN*>(Value);
				break;
			default:
				DC_set_error(desc, DESC_INVALID_COLUMN_NUMBER_ERROR, "invalid column number");
				ret = SQL_ERROR;
		}
		return ret;
	}
	row_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_TYPE:
			opts->bindings[row_idx].returntype = static_cast<SQLSMALLINT>(reinterpret_cast<SQLLEN>(Value));
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (opts->bindings[row_idx].returntype)
			{
				case SQL_DATETIME:
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
				switch ((LONG_PTR) Value)
				{
					case SQL_CODE_DATE:
						opts->bindings[row_idx].returntype = SQL_C_TYPE_DATE;
						break;
					case SQL_CODE_TIME:
						opts->bindings[row_idx].returntype = SQL_C_TYPE_TIME;
						break;
					case SQL_CODE_TIMESTAMP:
						opts->bindings[row_idx].returntype = SQL_C_TYPE_TIMESTAMP;
						break;
				}
				break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			opts->bindings[row_idx].returntype = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_DATA_PTR:
			unbind = FALSE;
			opts->bindings[row_idx].buffer = static_cast<char*>(Value);
			break;
		case SQL_DESC_INDICATOR_PTR:
			unbind = FALSE;
			opts->bindings[row_idx].indicator = static_cast<SQLLEN*>(Value);
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			unbind = FALSE;
			opts->bindings[row_idx].used = static_cast<SQLLEN*>(Value);
			break;
		case SQL_DESC_OCTET_LENGTH:
			opts->bindings[row_idx].buflen = CAST_PTR(SQLLEN, Value);
			break;
		case SQL_DESC_PRECISION:
			opts->bindings[row_idx].precision = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_SCALE:
			opts->bindings[row_idx].scale = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		case SQL_DESC_NUM_PREC_RADIX:
		default:ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invalid descriptor identifier");
	}
	if (unbind)
		opts->bindings[row_idx].buffer = NULL;
	return ret;
}

static  void parameter_bindings_set(APDFields *opts, int params, BOOL maxset)
{
	int	i;

	if (params == opts->allocated)
		return;
	if (params > opts->allocated)
	{
		extend_parameter_bindings(opts, params);
		return;
	}
	if (maxset)	return;

	for (i = opts->allocated; i > params; i--)
		reset_a_parameter_binding(opts, i);
	opts->allocated = params;
	if (0 == params)
	{
		free(opts->parameters);
		opts->parameters = NULL;
	}
}

static  void parameter_ibindings_set(IPDFields *opts, int params, BOOL maxset)
{
	int	i;

	if (params == opts->allocated)
		return;
	if (params > opts->allocated)
	{
		extend_iparameter_bindings(opts, params);
		return;
	}
	if (maxset)	return;

	for (i = opts->allocated; i > params; i--)
		reset_a_iparameter_binding(opts, i);
	opts->allocated = params;
	if (0 == params)
	{
		free(opts->parameters);
		opts->parameters = NULL;
	}
}

static RETCODE SQL_API
APDSetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	APDFields	*opts = &(desc->apdf);
	SQLSMALLINT	para_idx;
	BOOL		unbind = TRUE;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			opts->paramset_size = static_cast<SQLUINTEGER>(reinterpret_cast<SQLULEN>(Value));
			return ret;
		case SQL_DESC_ARRAY_STATUS_PTR:
			opts->param_operation_ptr = static_cast<SQLUSMALLINT*>(Value);
			return ret;
		case SQL_DESC_BIND_OFFSET_PTR:
			opts->param_offset_ptr = static_cast<SQLULEN*>(Value);
			return ret;
		case SQL_DESC_BIND_TYPE:
			opts->param_bind_type = static_cast<SQLUINTEGER>(reinterpret_cast<SQLULEN>(Value));
			return ret;
		case SQL_DESC_COUNT:
			parameter_bindings_set(opts, static_cast<SQLSMALLINT>(reinterpret_cast<SQLLEN>(Value)), FALSE);
			return ret;

		case SQL_DESC_TYPE:
		case SQL_DESC_DATETIME_INTERVAL_CODE:
		case SQL_DESC_CONCISE_TYPE:
			parameter_bindings_set(opts, RecNumber, TRUE);
			break;
	}
	if (RecNumber <=0)
	{
MYLOG(DETAIL_LOG_LEVEL, "RecN=%d allocated=%d\n", RecNumber, opts->allocated);
		DC_set_error(desc, DESC_BAD_PARAMETER_NUMBER_ERROR,
				"bad parameter number");
		return SQL_ERROR;
	}
	if (RecNumber > opts->allocated)
	{
MYLOG(DETAIL_LOG_LEVEL, "RecN=%d allocated=%d\n", RecNumber, opts->allocated);
		parameter_bindings_set(opts, RecNumber, TRUE);
		/* DC_set_error(desc, DESC_BAD_PARAMETER_NUMBER_ERROR,
				"bad parameter number");
		return SQL_ERROR;*/
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_TYPE:
			opts->parameters[para_idx].CType = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (opts->parameters[para_idx].CType)
			{
				case SQL_DATETIME:
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
				switch ((LONG_PTR) Value)
				{
					case SQL_CODE_DATE:
						opts->parameters[para_idx].CType = SQL_C_TYPE_DATE;
						break;
					case SQL_CODE_TIME:
						opts->parameters[para_idx].CType = SQL_C_TYPE_TIME;
						break;
					case SQL_CODE_TIMESTAMP:
						opts->parameters[para_idx].CType = SQL_C_TYPE_TIMESTAMP;
						break;
				}
				break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			opts->parameters[para_idx].CType = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_DATA_PTR:
			unbind = FALSE;
			opts->parameters[para_idx].buffer = static_cast<char*>(Value);
			break;
		case SQL_DESC_INDICATOR_PTR:
			unbind = FALSE;
			opts->parameters[para_idx].indicator = static_cast<SQLLEN*>(Value);
			break;
		case SQL_DESC_OCTET_LENGTH:
			opts->parameters[para_idx].buflen = CAST_PTR(Int4, Value);
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			unbind = FALSE;
			opts->parameters[para_idx].used = static_cast<SQLLEN*>(Value);
			break;
		case SQL_DESC_PRECISION:
			opts->parameters[para_idx].precision = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_SCALE:
			opts->parameters[para_idx].scale = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		case SQL_DESC_NUM_PREC_RADIX:
		default:ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invaid descriptor identifier");
	}
	if (unbind)
		opts->parameters[para_idx].buffer = NULL;

	return ret;
}

static RETCODE SQL_API
IRDSetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	IRDFields	*opts = &(desc->irdf);

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			opts->rowStatusArray = (SQLUSMALLINT *) Value;
			break;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			opts->rowsFetched = (SQLULEN *) Value;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_COUNT: /* read-only */
		case SQL_DESC_AUTO_UNIQUE_VALUE: /* read-only */
		case SQL_DESC_BASE_COLUMN_NAME: /* read-only */
		case SQL_DESC_BASE_TABLE_NAME: /* read-only */
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_CATALOG_NAME: /* read-only */
		case SQL_DESC_CONCISE_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_CODE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION: /* read-only */
		case SQL_DESC_DISPLAY_SIZE: /* read-only */
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LABEL: /* read-only */
		case SQL_DESC_LENGTH: /* read-only */
		case SQL_DESC_LITERAL_PREFIX: /* read-only */
		case SQL_DESC_LITERAL_SUFFIX: /* read-only */
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME: /* read-only */
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX: /* read-only */
		case SQL_DESC_OCTET_LENGTH: /* read-only */
		case SQL_DESC_PRECISION: /* read-only */
		case SQL_DESC_ROWVER: /* read-only */
		case SQL_DESC_SCALE: /* read-only */
		case SQL_DESC_SCHEMA_NAME: /* read-only */
		case SQL_DESC_SEARCHABLE: /* read-only */
		case SQL_DESC_TABLE_NAME: /* read-only */
		case SQL_DESC_TYPE: /* read-only */
		case SQL_DESC_TYPE_NAME: /* read-only */
		case SQL_DESC_UNNAMED: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		case SQL_DESC_UPDATABLE: /* read-only */
		default:ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invalid descriptor identifier");
	}
	return ret;
}

static RETCODE SQL_API
IPDSetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	IPDFields	*ipdopts = &(desc->ipdf);
	SQLSMALLINT	para_idx;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			ipdopts->param_status_ptr = (SQLUSMALLINT *) Value;
			return ret;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			ipdopts->param_processed_ptr = (SQLULEN *) Value;
			return ret;
		case SQL_DESC_COUNT:
			parameter_ibindings_set(ipdopts, CAST_PTR(SQLSMALLINT, Value), FALSE);
			return ret;
		case SQL_DESC_UNNAMED: /* only SQL_UNNAMED is allowed */
			if (SQL_UNNAMED !=  CAST_PTR(SQLSMALLINT, Value))
			{
				ret = SQL_ERROR;
				DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
					"invalid descriptor identifier");
				return ret;
			}
		case SQL_DESC_NAME:
		case SQL_DESC_TYPE:
		case SQL_DESC_DATETIME_INTERVAL_CODE:
		case SQL_DESC_CONCISE_TYPE:
			parameter_ibindings_set(ipdopts, RecNumber, TRUE);
			break;
	}
	if (RecNumber <= 0 || RecNumber > ipdopts->allocated)
	{
MYLOG(DETAIL_LOG_LEVEL, "RecN=%d allocated=%d\n", RecNumber, ipdopts->allocated);
		DC_set_error(desc, DESC_BAD_PARAMETER_NUMBER_ERROR,
				"bad parameter number");
		return SQL_ERROR;
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_TYPE:
			if (ipdopts->parameters[para_idx].SQLType != CAST_PTR(SQLSMALLINT, Value))
			{
				reset_a_iparameter_binding(ipdopts, RecNumber);
				ipdopts->parameters[para_idx].SQLType = CAST_PTR(SQLSMALLINT, Value);
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_DATETIME:
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
				switch ((LONG_PTR) Value)
				{
					case SQL_CODE_DATE:
						ipdopts->parameters[para_idx].SQLType = SQL_TYPE_DATE;
						break;
					case SQL_CODE_TIME:
						ipdopts->parameters[para_idx].SQLType = SQL_TYPE_TIME;
						break;
					case SQL_CODE_TIMESTAMP:
						ipdopts->parameters[para_idx].SQLType = SQL_TYPE_TIMESTAMP;
						break;
				}
				break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			ipdopts->parameters[para_idx].SQLType = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_NAME:
			if (Value)
				STR_TO_NAME(ipdopts->parameters[para_idx].paramName, static_cast<const char*>(Value));
			else
				NULL_THE_NAME(ipdopts->parameters[para_idx].paramName);
			break;
		case SQL_DESC_PARAMETER_TYPE:
			ipdopts->parameters[para_idx].paramType = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_SCALE:
			ipdopts->parameters[para_idx].decimal_digits = CAST_PTR(SQLSMALLINT, Value);
			break;
		case SQL_DESC_UNNAMED: /* only SQL_UNNAMED is allowed */
			if (SQL_UNNAMED !=  CAST_PTR(SQLSMALLINT, Value))
			{
				ret = SQL_ERROR;
				DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
					"invalid descriptor identifier");
			}
			else
				NULL_THE_NAME(ipdopts->parameters[para_idx].paramName);
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LENGTH:
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX:
		case SQL_DESC_OCTET_LENGTH:
		case SQL_DESC_PRECISION:
		case SQL_DESC_ROWVER: /* read-only */
		case SQL_DESC_TYPE_NAME: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		default:ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invalid descriptor identifier");
	}
	return ret;
}


static RETCODE SQL_API
ARDGetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
			SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLLEN		ival = 0;
	SQLINTEGER	len, rettype = 0;
	PTR		ptr = NULL;
	const ARDFields	*opts = &(desc->ardf);
	SQLSMALLINT	row_idx;

	len = sizeof(SQLINTEGER);
	if (0 == RecNumber) /* bookmark */
	{
		BindInfoClass	*bookmark = opts->bookmark;
		switch (FieldIdentifier)
		{
			case SQL_DESC_DATA_PTR:
				rettype = SQL_IS_POINTER;
				ptr = bookmark ? bookmark->buffer : NULL;
				break;
			case SQL_DESC_INDICATOR_PTR:
				rettype = SQL_IS_POINTER;
				ptr = bookmark ? bookmark->indicator : NULL;
				break;
			case SQL_DESC_OCTET_LENGTH_PTR:
				rettype = SQL_IS_POINTER;
				ptr = bookmark ? bookmark->used : NULL;
				break;
		}
		if (ptr)
		{
			*((void **) Value) = ptr;
			if (StringLength)
				*StringLength = len;
			return ret;
		}
	}
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_BIND_OFFSET_PTR:
		case SQL_DESC_BIND_TYPE:
		case SQL_DESC_COUNT:
			break;
		default:
			if (RecNumber <= 0 || RecNumber > opts->allocated)
			{
				DC_set_error(desc, DESC_INVALID_COLUMN_NUMBER_ERROR,
					"invalid column number");
				return SQL_ERROR;
			}
	}
	row_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			ival = opts->size_of_rowset;
			break;
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->row_operation_ptr;
			break;
		case SQL_DESC_BIND_OFFSET_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->row_offset_ptr;
			break;
		case SQL_DESC_BIND_TYPE:
			ival = opts->bind_size;
			break;
		case SQL_DESC_TYPE:
			rettype = SQL_IS_SMALLINT;
			switch (opts->bindings[row_idx].returntype)
			{
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_DATETIME;
					break;
				default:
					ival = opts->bindings[row_idx].returntype;
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			rettype = SQL_IS_SMALLINT;
			switch (opts->bindings[row_idx].returntype)
			{
				case SQL_C_TYPE_DATE:
					ival = SQL_CODE_DATE;
					break;
				case SQL_C_TYPE_TIME:
					ival = SQL_CODE_TIME;
					break;
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_CODE_TIMESTAMP;
					break;
				default:
					ival = 0;
					break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			rettype = SQL_IS_SMALLINT;
			ival = opts->bindings[row_idx].returntype;
			break;
		case SQL_DESC_DATA_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->bindings[row_idx].buffer;
			break;
		case SQL_DESC_INDICATOR_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->bindings[row_idx].indicator;
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->bindings[row_idx].used;
			break;
		case SQL_DESC_COUNT:
			rettype = SQL_IS_SMALLINT;
			ival = opts->allocated;
			break;
		case SQL_DESC_OCTET_LENGTH:
			ival = opts->bindings[row_idx].buflen;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			rettype = SQL_IS_SMALLINT;
			if (DC_get_embedded(desc))
				ival = SQL_DESC_ALLOC_AUTO;
			else
				ival = SQL_DESC_ALLOC_USER;
			break;
		case SQL_DESC_PRECISION:
			rettype = SQL_IS_SMALLINT;
			ival = opts->bindings[row_idx].precision;
			break;
		case SQL_DESC_SCALE:
			rettype = SQL_IS_SMALLINT;
			ival = opts->bindings[row_idx].scale;
			break;
		case SQL_DESC_NUM_PREC_RADIX:
			ival = 10;
			break;
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		default:
			ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invalid descriptor identifier");
	}
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = sizeof(SQLINTEGER);
			*((SQLINTEGER *) Value) = (SQLINTEGER) ival;
			break;
		case SQL_IS_SMALLINT:
			len = sizeof(SQLSMALLINT);
			*((SQLSMALLINT *) Value) = (SQLSMALLINT) ival;
			break;
		case SQL_IS_POINTER:
			len = sizeof(SQLPOINTER);
			*((void **) Value) = ptr;
			break;
	}

	if (StringLength)
		*StringLength = len;
	return ret;
}

static RETCODE SQL_API
APDGetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
			SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLLEN		ival = 0;
	SQLINTEGER	len, rettype = 0;
	PTR		ptr = NULL;
	const APDFields	*opts = (const APDFields *) &(desc->apdf);
	SQLSMALLINT	para_idx;

	len = sizeof(SQLINTEGER);
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_BIND_OFFSET_PTR:
		case SQL_DESC_BIND_TYPE:
		case SQL_DESC_COUNT:
			break;
		default:if (RecNumber <= 0 || RecNumber > opts->allocated)
			{
MYLOG(DETAIL_LOG_LEVEL, "RecN=%d allocated=%d\n", RecNumber, opts->allocated);
				DC_set_error(desc, DESC_BAD_PARAMETER_NUMBER_ERROR,
					"bad parameter number");
				return SQL_ERROR;
			}
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			rettype = SQL_IS_LEN;
			ival = opts->paramset_size;
			break;
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->param_operation_ptr;
			break;
		case SQL_DESC_BIND_OFFSET_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->param_offset_ptr;
			break;
		case SQL_DESC_BIND_TYPE:
			ival = opts->param_bind_type;
			break;

		case SQL_DESC_TYPE:
			rettype = SQL_IS_SMALLINT;
			switch (opts->parameters[para_idx].CType)
			{
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_DATETIME;
					break;
				default:
					ival = opts->parameters[para_idx].CType;
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			rettype = SQL_IS_SMALLINT;
			switch (opts->parameters[para_idx].CType)
			{
				case SQL_C_TYPE_DATE:
					ival = SQL_CODE_DATE;
					break;
				case SQL_C_TYPE_TIME:
					ival = SQL_CODE_TIME;
					break;
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_CODE_TIMESTAMP;
					break;
				default:
					ival = 0;
					break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			rettype = SQL_IS_SMALLINT;
			ival = opts->parameters[para_idx].CType;
			break;
		case SQL_DESC_DATA_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->parameters[para_idx].buffer;
			break;
		case SQL_DESC_INDICATOR_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->parameters[para_idx].indicator;
			break;
		case SQL_DESC_OCTET_LENGTH:
			ival = opts->parameters[para_idx].buflen;
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->parameters[para_idx].used;
			break;
		case SQL_DESC_COUNT:
			rettype = SQL_IS_SMALLINT;
			ival = opts->allocated;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			rettype = SQL_IS_SMALLINT;
			if (DC_get_embedded(desc))
				ival = SQL_DESC_ALLOC_AUTO;
			else
				ival = SQL_DESC_ALLOC_USER;
			break;
		case SQL_DESC_NUM_PREC_RADIX:
			ival = 10;
			break;
		case SQL_DESC_PRECISION:
			rettype = SQL_IS_SMALLINT;
			ival = opts->parameters[para_idx].precision;
			break;
		case SQL_DESC_SCALE:
			rettype = SQL_IS_SMALLINT;
			ival = opts->parameters[para_idx].scale;
			break;
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		default:ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
					"invalid descriptor identifer");
	}
	switch (rettype)
	{
		case SQL_IS_LEN:
			len = sizeof(SQLLEN);
			*((SQLLEN *) Value) = ival;
			break;
		case 0:
		case SQL_IS_INTEGER:
			len = sizeof(SQLINTEGER);
			*((SQLINTEGER *) Value) = (SQLINTEGER) ival;
			break;
		case SQL_IS_SMALLINT:
			len = sizeof(SQLSMALLINT);
			*((SQLSMALLINT *) Value) = (SQLSMALLINT) ival;
			break;
		case SQL_IS_POINTER:
			len = sizeof(SQLPOINTER);
			*((void **) Value) = ptr;
			break;
	}

	if (StringLength)
		*StringLength = len;
	return ret;
}

static RETCODE SQL_API
IRDGetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
			SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLLEN		ival = 0;
	SQLINTEGER	len = 0, rettype = 0;
	PTR		ptr = NULL;
	BOOL		bCallColAtt = FALSE;
	const IRDFields	*opts = &(desc->irdf);

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->rowStatusArray;
			break;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->rowsFetched;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			rettype = SQL_IS_SMALLINT;
			ival = SQL_DESC_ALLOC_AUTO;
			break;
		case SQL_DESC_COUNT: /* read-only */
		case SQL_DESC_AUTO_UNIQUE_VALUE: /* read-only */
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_CONCISE_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_CODE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION: /* read-only */
		case SQL_DESC_DISPLAY_SIZE: /* read-only */
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LENGTH: /* read-only */
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX: /* read-only */
		case SQL_DESC_OCTET_LENGTH: /* read-only */
		case SQL_DESC_PRECISION: /* read-only */
		case SQL_DESC_ROWVER: /* read-only */
		case SQL_DESC_SCALE: /* read-only */
		case SQL_DESC_SEARCHABLE: /* read-only */
		case SQL_DESC_TYPE: /* read-only */
		case SQL_DESC_UNNAMED: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		case SQL_DESC_UPDATABLE: /* read-only */
			bCallColAtt = TRUE;
			break;
		case SQL_DESC_BASE_COLUMN_NAME: /* read-only */
		case SQL_DESC_BASE_TABLE_NAME: /* read-only */
		case SQL_DESC_CATALOG_NAME: /* read-only */
		case SQL_DESC_LABEL: /* read-only */
		case SQL_DESC_LITERAL_PREFIX: /* read-only */
		case SQL_DESC_LITERAL_SUFFIX: /* read-only */
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME: /* read-only */
		case SQL_DESC_SCHEMA_NAME: /* read-only */
		case SQL_DESC_TABLE_NAME: /* read-only */
		case SQL_DESC_TYPE_NAME: /* read-only */
			rettype = SQL_NTS;
			bCallColAtt = TRUE;
			break;
		default:
			ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invalid descriptor identifier");
	}
	if (bCallColAtt)
	{
		SQLSMALLINT	pcbL;
		StatementClass	*stmt;

		stmt = opts->stmt;
		ret = WD_ColAttributes(stmt, RecNumber,
			FieldIdentifier, Value, (SQLSMALLINT) BufferLength,
				&pcbL, &ival);
		len = pcbL;
	}
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = sizeof(SQLINTEGER);
			*((SQLINTEGER *) Value) = (SQLINTEGER) ival;
			break;
		case SQL_IS_UINTEGER:
			len = sizeof(SQLUINTEGER);
			*((SQLUINTEGER *) Value) = (SQLUINTEGER) ival;
			break;
		case SQL_IS_SMALLINT:
			len = sizeof(SQLSMALLINT);
			*((SQLSMALLINT *) Value) = (SQLSMALLINT) ival;
			break;
		case SQL_IS_POINTER:
			len = sizeof(SQLPOINTER);
			*((void **) Value) = ptr;
			break;
		case SQL_NTS:
			break;
	}

	if (StringLength)
		*StringLength = len;
	return ret;
}

static RETCODE SQL_API
IPDGetField(DescriptorClass *desc, SQLSMALLINT RecNumber,
			SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
			SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLINTEGER	ival = 0, len = 0, rettype = 0;
	PTR		ptr = NULL;
	const IPDFields	*ipdopts = (const IPDFields *) &(desc->ipdf);
	SQLSMALLINT	para_idx;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_ROWS_PROCESSED_PTR:
		case SQL_DESC_COUNT:
			break;
		default:if (RecNumber <= 0 || RecNumber > ipdopts->allocated)
			{
MYLOG(DETAIL_LOG_LEVEL, "RecN=%d allocated=%d\n", RecNumber, ipdopts->allocated);
				DC_set_error(desc, DESC_BAD_PARAMETER_NUMBER_ERROR,
					"bad parameter number");
				return SQL_ERROR;
			}
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = ipdopts->param_status_ptr;
			break;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			rettype = SQL_IS_POINTER;
			ptr = ipdopts->param_processed_ptr;
			break;
		case SQL_DESC_UNNAMED:
			rettype = SQL_IS_SMALLINT;
			ival = NAME_IS_NULL(ipdopts->parameters[para_idx].paramName) ? SQL_UNNAMED : SQL_NAMED;
			break;
		case SQL_DESC_TYPE:
			rettype = SQL_IS_SMALLINT;
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
					ival = SQL_DATETIME;
					break;
				default:
					ival = ipdopts->parameters[para_idx].SQLType;
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			rettype = SQL_IS_SMALLINT;
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_TYPE_DATE:
					ival = SQL_CODE_DATE;
					break;
				case SQL_TYPE_TIME:
					ival = SQL_CODE_TIME;
					break;
				case SQL_TYPE_TIMESTAMP:
					ival = SQL_CODE_TIMESTAMP;
					break;
				default:
					ival = 0;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			rettype = SQL_IS_SMALLINT;
			ival = ipdopts->parameters[para_idx].SQLType;
			break;
		case SQL_DESC_COUNT:
			rettype = SQL_IS_SMALLINT;
			ival = ipdopts->allocated;
			break;
		case SQL_DESC_PARAMETER_TYPE:
			rettype = SQL_IS_SMALLINT;
			ival = ipdopts->parameters[para_idx].paramType;
			break;
		case SQL_DESC_PRECISION:
			rettype = SQL_IS_SMALLINT;
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
				case SQL_DATETIME:
					ival = ipdopts->parameters[para_idx].decimal_digits;
					break;
			}
			break;
		case SQL_DESC_SCALE:
			rettype = SQL_IS_SMALLINT;
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_NUMERIC:
					ival = ipdopts->parameters[para_idx].decimal_digits;
					break;
			}
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			rettype = SQL_IS_SMALLINT;
			ival = SQL_DESC_ALLOC_AUTO;
			break;
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LENGTH:
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME:
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX:
		case SQL_DESC_OCTET_LENGTH:
		case SQL_DESC_ROWVER: /* read-only */
		case SQL_DESC_TYPE_NAME: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		default:ret = SQL_ERROR;
			DC_set_error(desc, DESC_INVALID_DESCRIPTOR_IDENTIFIER,
				"invalid descriptor identifier");
	}
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = sizeof(SQLINTEGER);
			*((SQLINTEGER *) Value) = ival;
			break;
		case SQL_IS_SMALLINT:
			len = sizeof(SQLSMALLINT);
			*((SQLSMALLINT *) Value) = (SQLSMALLINT) ival;
			break;
		case SQL_IS_POINTER:
			len = sizeof(SQLPOINTER);
			*((void **)Value) = ptr;
			break;
	}

	if (StringLength)
		*StringLength = len;
	return ret;
}

/*	SQLGetStmtOption -> SQLGetStmtAttr */
RETCODE SQL_API WD_GetStmtAttr(HSTMT StatementHandle, SQLINTEGER Attribute,
                               PTR Value, SQLINTEGER BufferLength,
                               SQLINTEGER *StringLength, bool isUnicode) {
    RETCODE ret = SQL_SUCCESS;

    MYLOG(0, "entering Handle=%p " FORMAT_INTEGER "\n", StatementHandle, Attribute);
    ODBCStatement* statement = reinterpret_cast<ODBCStatement*>(StatementHandle);
    statement->GetStmtAttr(Attribute, Value, BufferLength, StringLength, isUnicode);
    return ret;
}

RETCODE		SQL_API
WD_SetConnectAttr(HDBC ConnectionHandle,
                  SQLINTEGER Attribute, PTR Value,
                  SQLINTEGER StringLength,
                  bool isUnicode)
{
  CSTR	func = "WD_SetConnectAttr";
  ODBCConnection *conn = reinterpret_cast<ODBCConnection*>(ConnectionHandle);
  RETCODE	ret = SQL_SUCCESS;

  MYLOG(0, "entering for %p: " FORMAT_INTEGER " %p\n", ConnectionHandle, Attribute, Value);
  conn->SetConnectAttr(Attribute, Value, StringLength, isUnicode);
  return ret;
}

/*	new function */
RETCODE		SQL_API
WD_GetDescField(SQLHDESC DescriptorHandle,
				   SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
				   PTR Value, SQLINTEGER BufferLength,
				   SQLINTEGER *StringLength)
{
	CSTR func = "WD_GetDescField";
	RETCODE		ret = SQL_SUCCESS;

	MYLOG(0, "entering h=%p rec=" FORMAT_SMALLI " field=" FORMAT_SMALLI " blen=" FORMAT_INTEGER "\n", DescriptorHandle, RecNumber, FieldIdentifier, BufferLength);

	ODBCDescriptor* desc = reinterpret_cast<ODBCDescriptor*>(DescriptorHandle);
	desc->GetField(RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
	return ret;
}

RETCODE		SQL_API
WD_GetDescRec(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLCHAR *Name,
			  SQLSMALLINT BufferLength, SQLSMALLINT *StringLength,
			  SQLSMALLINT *Type, SQLSMALLINT *SubType,
			  SQLLEN *Length, SQLSMALLINT *Precision,
			  SQLSMALLINT *Scale, SQLSMALLINT *Nullable)
{
  CSTR func = "WD_GetDescRec";
  RETCODE		ret = SQL_SUCCESS;

  if (RecNumber == 0) {
    throw DriverException("InvalidDescriptorIndex");
  }

  ODBCDescriptor* desc = reinterpret_cast<ODBCDescriptor*>(DescriptorHandle);
  const std::vector<DescriptorRecord>& records = desc->GetRecords();

  SQLUSMALLINT zeroBasedIndex = RecNumber - 1;
  const DescriptorRecord& record = records[zeroBasedIndex];
  SQLINTEGER totalColumnNameLen;
  GetAttributeUTF8(record.m_name, Name, static_cast<SQLINTEGER>(BufferLength), &totalColumnNameLen,
                   desc->GetDiagnostics());
  if (StringLength) {
    *StringLength = static_cast<SQLSMALLINT>(totalColumnNameLen);
  }
  GetAttribute<SQLSMALLINT, size_t>(record.m_type, Type, sizeof(SQLSMALLINT), nullptr);
  GetAttribute<SQLSMALLINT, size_t>(record.m_datetimeIntervalCode, SubType, sizeof(SQLSMALLINT), nullptr);
  GetAttribute<SQLINTEGER, size_t>(record.m_length, Length, sizeof(SQLLEN), nullptr);
  GetAttribute<SQLSMALLINT, size_t>(record.m_precision, Precision, sizeof(SQLSMALLINT), nullptr);
  GetAttribute<SQLSMALLINT, size_t>(record.m_scale, Scale, sizeof(SQLSMALLINT), nullptr);
  GetAttribute<SQLSMALLINT, size_t>(record.m_nullable, Nullable, sizeof(SQLSMALLINT), nullptr);

  return SQL_SUCCESS;
}

/*	new function */
RETCODE		SQL_API
WD_SetDescField(SQLHDESC DescriptorHandle,
				   SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
				   PTR Value, SQLINTEGER BufferLength)
{
  CSTR func = "WD_SetDescField";
  RETCODE		ret = SQL_SUCCESS;
  ODBCDescriptor *desc = reinterpret_cast<ODBCDescriptor*>(DescriptorHandle);

  MYLOG(0, "entering h=%p rec=" FORMAT_SMALLI " field=" FORMAT_SMALLI " val=%p," FORMAT_INTEGER "\n", DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
  desc->SetField(RecNumber, FieldIdentifier, Value, BufferLength);
  return ret;
}

RETCODE SQL_API WD_SetDescRec(SQLHDESC DescriptorHandle,
			  SQLSMALLINT RecNumber, SQLSMALLINT Type,
			  SQLSMALLINT SubType, SQLLEN Length,
			  SQLSMALLINT Precision, SQLSMALLINT Scale,
			  PTR Data, SQLLEN *StringLength,
			  SQLLEN *Indicator)
{
  CSTR func = "WD_SetDescField";
  RETCODE		ret = SQL_SUCCESS;
  ODBCDescriptor *desc = reinterpret_cast<ODBCDescriptor*>(DescriptorHandle);

  if (RecNumber == 0) {
    throw DriverException("InvalidDescriptorIndex");
  }

  std::vector<DescriptorRecord>& records = desc->GetRecords();

  SQLUSMALLINT zeroBasedIndex = RecNumber - 1;
  DescriptorRecord& record = records[zeroBasedIndex];

  record.m_type = Type;
  record.m_datetimeIntervalCode = SubType;
  record.m_length = Length;
  record.m_precision = Precision;
  record.m_scale = Scale;
  record.m_indicatorPtr = StringLength;
  record.m_indicatorPtr = Indicator;
  desc->SetDataPtrOnRecord(Data, RecNumber);

  return SQL_SUCCESS;
}
			  
/*	SQLSet(Param/Scroll/Stmt)Option -> SQLSetStmtAttr */
RETCODE SQL_API WD_SetStmtAttr(HSTMT StatementHandle, SQLINTEGER Attribute,
                               PTR Value, SQLINTEGER StringLength,
                               bool isUnicode) {
  RETCODE ret = SQL_SUCCESS;

  MYLOG(0, "entering Handle=%p " FORMAT_INTEGER "\n", StatementHandle, Attribute);
  ODBCStatement* statement = reinterpret_cast<ODBCStatement*>(StatementHandle);
  statement->SetStmtAttr(Attribute, Value, StringLength, isUnicode);
  return ret;
}

/*	SQL_NEED_DATA callback for WD_BulkOperations */
typedef struct
{
	StatementClass	*stmt;
	SQLSMALLINT	operation;
	char		need_data_callback;
	char		auto_commit_needed;
	ARDFields	*opts;
	int		idx, processed;
}	bop_cdata;

static
RETCODE	bulk_ope_callback(RETCODE retcode, void *para)
{
	CSTR func = "bulk_ope_callback";
	RETCODE	ret = retcode;
	bop_cdata *s = (bop_cdata *) para;
	SQLULEN		global_idx;
	ConnectionClass	*conn;
	QResultClass	*res;
	IRDFields	*irdflds;
	WD_BM		WD_bm;

	if (s->need_data_callback)
	{
		MYLOG(0, "entering in\n");
		s->processed++;
		s->idx++;
	}
	else
	{
		s->idx = s->processed = 0;
	}
	s->need_data_callback = FALSE;
	res = SC_get_Curres(s->stmt);
	for (; SQL_ERROR != ret && s->idx < s->opts->size_of_rowset; s->idx++)
	{
		if (SQL_ADD != s->operation)
		{
			WD_bm = SC_Resolve_bookmark(s->opts, s->idx);
			QR_get_last_bookmark(res, s->idx, &WD_bm.keys);
			global_idx = WD_bm.index;
		}
		/* Note opts->row_operation_ptr is ignored */
		switch (s->operation)
		{
			case SQL_ADD:
				ret = SC_pos_add(s->stmt, (UWORD) s->idx);
				break;
			case SQL_UPDATE_BY_BOOKMARK:
				ret = SC_pos_update(s->stmt, (UWORD) s->idx, global_idx, &(WD_bm.keys));
				break;
			case SQL_DELETE_BY_BOOKMARK:
				ret = SC_pos_delete(s->stmt, (UWORD) s->idx, global_idx, &(WD_bm.keys));
				break;
		}
		if (SQL_NEED_DATA == ret)
		{
			bop_cdata *cbdata = (bop_cdata *) malloc(sizeof(bop_cdata));
			if (!cbdata)
			{
				SC_set_error(s->stmt, STMT_NO_MEMORY_ERROR, "Couldn't allocate memory for cbdata.", func);
				return SQL_ERROR;
			}
			memcpy(cbdata, s, sizeof(bop_cdata));
			cbdata->need_data_callback = TRUE;
			if (0 == enqueueNeedDataCallback(s->stmt, bulk_ope_callback, cbdata))
				ret = SQL_ERROR;
			return ret;
		}
		s->processed++;
	}
	conn = SC_get_conn(s->stmt);
	if (s->auto_commit_needed)
		CC_set_autocommit(conn, TRUE);
	irdflds = SC_get_IRDF(s->stmt);
	if (irdflds->rowsFetched)
		*(irdflds->rowsFetched) = s->processed;

	if (res)
		res->recent_processed_row_count = s->stmt->diag_row_count = s->processed;
	return ret;
}

RETCODE	SQL_API
WD_BulkOperations(HSTMT hstmt, SQLSMALLINT operationX)
{
	CSTR func = "WD_BulkOperations";
	bop_cdata	s;
	RETCODE		ret;
	ConnectionClass	*conn;
	BindInfoClass	*bookmark;

	MYLOG(0, "entering operation = %d\n", operationX);
	s.stmt = (StatementClass *) hstmt;
	s.operation = operationX;
	SC_clear_error(s.stmt);
	s.opts = SC_get_ARDF(s.stmt);

	s.auto_commit_needed = FALSE;
	if (SQL_FETCH_BY_BOOKMARK != s.operation)
	{
		conn = SC_get_conn(s.stmt);
		if (s.auto_commit_needed = (char) CC_does_autocommit(conn), s.auto_commit_needed)
			CC_set_autocommit(conn, FALSE);
	}
	if (SQL_ADD != s.operation)
	{
		if (!(bookmark = s.opts->bookmark) || !(bookmark->buffer))
		{
			SC_set_error(s.stmt, DESC_INVALID_OPTION_IDENTIFIER, "bookmark isn't specified", func);
			return SQL_ERROR;
		}
	}

	/* StartRollbackState(s.stmt); */
	if (SQL_FETCH_BY_BOOKMARK == operationX)
		ret = SC_fetch_by_bookmark(s.stmt);
	else
	{
		s.need_data_callback = FALSE;
		ret = bulk_ope_callback(SQL_SUCCESS, &s);
	}
	return ret;
}
