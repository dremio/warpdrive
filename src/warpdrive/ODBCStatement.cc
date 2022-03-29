/* File:			ODBCStatement.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCStatement.h"

#include "wdodbc.h"
#include "AttributeUtils.h"
#include "ODBCConnection.h"
#include "ODBCDescriptor.h"
#include "sql.h"
#include "sqlext.h"
#include "sqltypes.h"
#include <odbcabstraction/statement.h>
#include <odbcabstraction/exceptions.h>
#include <odbcabstraction/result_set.h>
#include <odbcabstraction/result_set_metadata.h>
#include <odbcabstraction/types.h>
#include <boost/optional.hpp>
#include <utility>
#include <boost/variant.hpp>

using namespace ODBC;
using namespace driver::odbcabstraction;

namespace {
  void DescriptorToHandle(SQLPOINTER output, ODBCDescriptor* descriptor, SQLINTEGER* lenPtr) {
    if (output) {
      SQLHANDLE* outputHandle = static_cast<SQLHANDLE*>(output);
      *outputHandle = reinterpret_cast<SQLHANDLE>(descriptor);
      *lenPtr = sizeof(SQLHANDLE);
    }
  }

  size_t GetLength(const DescriptorRecord& record) {
    switch (record.m_type) {
      case SQL_C_CHAR:
      case SQL_C_WCHAR:
      case SQL_C_BINARY:
        return record.m_length;
      
      case SQL_C_BIT:
      case SQL_C_TINYINT:
      case SQL_C_STINYINT:
      case SQL_C_UTINYINT:
        return sizeof(SQLSCHAR);
      
      case SQL_C_SHORT:
      case SQL_C_SSHORT:
      case SQL_C_USHORT:
        return sizeof(SQLSMALLINT);

      case SQL_C_LONG:
      case SQL_C_SLONG:
      case SQL_C_ULONG:
      case SQL_C_FLOAT:
       return sizeof(SQLINTEGER);

      case SQL_C_SBIGINT:
      case SQL_C_UBIGINT:
      case SQL_C_DOUBLE:
        return sizeof(SQLBIGINT);

      case SQL_C_NUMERIC:
        return sizeof(SQL_NUMERIC_STRUCT);

      case SQL_C_DATE:
      case SQL_C_TYPE_DATE:
        return sizeof(SQL_DATE_STRUCT);
      
      case SQL_C_TIME:
      case SQL_C_TYPE_TIME:
        return sizeof(SQL_TIME_STRUCT);

      case SQL_C_TIMESTAMP:
      case SQL_C_TYPE_TIMESTAMP:
        return sizeof(SQL_TIMESTAMP_STRUCT);

      case SQL_C_INTERVAL_DAY:
      case SQL_C_INTERVAL_DAY_TO_HOUR:
      case SQL_C_INTERVAL_DAY_TO_MINUTE:
      case SQL_C_INTERVAL_DAY_TO_SECOND:
      case SQL_C_INTERVAL_HOUR:
      case SQL_C_INTERVAL_HOUR_TO_MINUTE:
      case SQL_C_INTERVAL_HOUR_TO_SECOND:
      case SQL_C_INTERVAL_MINUTE:
      case SQL_C_INTERVAL_MINUTE_TO_SECOND:
      case SQL_C_INTERVAL_SECOND:
      case SQL_C_INTERVAL_YEAR:
      case SQL_C_INTERVAL_YEAR_TO_MONTH:
      case SQL_C_INTERVAL_MONTH:
        return sizeof(SQL_INTERVAL_STRUCT);
      default:
        return record.m_length;
    }
  }

  SQLSMALLINT getCTypeForSQLType(const DescriptorRecord& record) {
    switch (record.m_conciseType) {
      case SQL_CHAR:
      case SQL_VARCHAR:
      case SQL_LONGVARCHAR:
        return SQL_C_CHAR;

      case SQL_WCHAR:
      case SQL_WVARCHAR:
      case SQL_WLONGVARCHAR:
       return SQL_C_WCHAR;

      case SQL_BINARY:
      case SQL_VARBINARY:
      case SQL_LONGVARBINARY:
        return SQL_C_BINARY;

      case SQL_TINYINT:
        return record.m_unsigned ? SQL_C_UTINYINT : SQL_C_STINYINT;
      
      case SQL_SMALLINT:
        return record.m_unsigned ? SQL_C_USHORT : SQL_C_SSHORT;

      case SQL_INTEGER:
        return record.m_unsigned ? SQL_C_ULONG : SQL_C_SLONG;
      
      case SQL_BIGINT:
        return record.m_unsigned ? SQL_C_UBIGINT : SQL_C_SBIGINT;

      case SQL_REAL:
        return SQL_C_FLOAT;

      case SQL_FLOAT:
      case SQL_DOUBLE:
        return SQL_C_DOUBLE;

      case SQL_DATE:
      case SQL_TYPE_DATE:
        return SQL_C_TYPE_DATE;

      case SQL_TIME:
      case SQL_TYPE_TIME:
        return SQL_C_TYPE_TIME;

      case SQL_TIMESTAMP:
      case SQL_TYPE_TIMESTAMP:
        return SQL_C_TYPE_TIMESTAMP;

      case SQL_C_INTERVAL_DAY:
        return SQL_INTERVAL_DAY;
      case SQL_C_INTERVAL_DAY_TO_HOUR:
        return SQL_INTERVAL_DAY_TO_HOUR;
      case SQL_C_INTERVAL_DAY_TO_MINUTE:
        return SQL_INTERVAL_DAY_TO_MINUTE;
      case SQL_C_INTERVAL_DAY_TO_SECOND:
        return SQL_INTERVAL_DAY_TO_SECOND;
      case SQL_C_INTERVAL_HOUR:
        return SQL_INTERVAL_HOUR;
      case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        return SQL_INTERVAL_HOUR_TO_MINUTE;
      case SQL_C_INTERVAL_HOUR_TO_SECOND:
        return SQL_INTERVAL_HOUR_TO_SECOND;
      case SQL_C_INTERVAL_MINUTE:
        return SQL_INTERVAL_MINUTE;
      case SQL_C_INTERVAL_MINUTE_TO_SECOND:
        return SQL_INTERVAL_MINUTE_TO_SECOND;
      case SQL_C_INTERVAL_SECOND:
        return SQL_INTERVAL_SECOND;
      case SQL_C_INTERVAL_YEAR:
        return SQL_INTERVAL_YEAR;
      case SQL_C_INTERVAL_YEAR_TO_MONTH:
        return SQL_INTERVAL_YEAR_TO_MONTH;
      case SQL_C_INTERVAL_MONTH:
        return SQL_INTERVAL_MONTH;

      default:
        throw DriverException("Unknown type");
    }
  }

  void CopyAttribute(Statement& source, Statement& target, Statement::StatementAttributeId attributeId) {
    auto optionalValue = source.GetAttribute(attributeId);
    if (optionalValue) {
      target.SetAttribute(attributeId, *optionalValue);
    }
  }
}

// Public =========================================================================================
ODBCStatement::ODBCStatement(ODBCConnection& connection, 
  std::shared_ptr<driver::odbcabstraction::Statement> spiStatement) :
  m_connection(connection),
  m_spiStatement(std::move(spiStatement)),
  m_builtInArd(std::make_shared<ODBCDescriptor>(nullptr, true, true, connection.IsOdbc2Connection())),
  m_builtInApd(std::make_shared<ODBCDescriptor>(nullptr, true, true, connection.IsOdbc2Connection())),
  m_ipd(std::make_shared<ODBCDescriptor>(nullptr, false, true, connection.IsOdbc2Connection())),
  m_ird(std::make_shared<ODBCDescriptor>(nullptr, false, false, connection.IsOdbc2Connection())),
  m_currentArd(m_builtInApd.get()),
  m_currentApd(m_builtInApd.get()),
  m_rowNumber(0),
  m_maxRows(0),
  m_isPrepared(false),
  m_hasReachedEndOfResult(false) {
}

void ODBCStatement::CopyAttributesFromConnection(ODBCConnection& connection) {
  ODBCStatement& trackingStatement = connection.GetTrackingStatement();

  // Get abstraction attributes and copy to this m_spiStatement.
  // Possible ODBC attributes are below, but many of these are not supported by warpdrive
  // or ODBCAbstaction:
  // SQL_ATTR_ASYNC_ENABLE:
  // SQL_ATTR_METADATA_ID:
  // SQL_ATTR_CONCURRENCY:
  // SQL_ATTR_CURSOR_TYPE:
  // SQL_ATTR_KEYSET_SIZE:
  // SQL_ATTR_MAX_LENGTH:
  // SQL_ATTR_MAX_ROWS:
  // SQL_ATTR_NOSCAN:
  // SQL_ATTR_QUERY_TIMEOUT:
  // SQL_ATTR_RETRIEVE_DATA:
  // SQL_ATTR_SIMULATE_CURSOR:
  // SQL_ATTR_USE_BOOKMARKS:
  CopyAttribute(*trackingStatement.m_spiStatement, *m_spiStatement, Statement::METADATA_ID);
  CopyAttribute(*trackingStatement.m_spiStatement, *m_spiStatement, Statement::MAX_LENGTH);
  CopyAttribute(*trackingStatement.m_spiStatement, *m_spiStatement, Statement::NOSCAN);
  CopyAttribute(*trackingStatement.m_spiStatement, *m_spiStatement, Statement::QUERY_TIMEOUT);

  // SQL_ATTR_ROW_BIND_TYPE:
  m_currentArd->SetHeaderField(SQL_DESC_BIND_TYPE,
    reinterpret_cast<SQLPOINTER>(static_cast<SQLLEN>(trackingStatement.m_currentArd->GetBoundStructOffset())), 0);
}
    
bool ODBCStatement::isPrepared() const {
  return m_isPrepared;
}

void ODBCStatement::Prepare(const std::string& query) {
  boost::optional<std::shared_ptr<ResultSetMetadata> > metadata = m_spiStatement->Prepare(query);

  if (metadata) {
    m_ird->PopulateFromResultSetMetadata(metadata->get());
  }
  m_isPrepared = true;
}

void ODBCStatement::ExecutePrepared() {
  if (!m_isPrepared) {
    throw DriverException("Function sequence error");
  }

  if (m_spiStatement->ExecutePrepared()) {
    m_currenResult = m_spiStatement->GetResultSet();
    m_ird->PopulateFromResultSetMetadata(m_spiStatement->GetResultSet()->GetMetadata().get());
    m_hasReachedEndOfResult = false;
  }
}

void ODBCStatement::ExecuteDirect(const std::string& query) {
  if (m_spiStatement->Execute(query)) {
    m_currenResult = m_spiStatement->GetResultSet();
    m_ird->PopulateFromResultSetMetadata(m_currenResult->GetMetadata().get());
    m_hasReachedEndOfResult = false;
  }

  // Direct execution wipes out the prepared state.
  m_isPrepared = false;
}

bool ODBCStatement::Fetch(size_t rows) {
  if (m_hasReachedEndOfResult) {
    return false;
  }

  if (m_maxRows) {
    rows = WD_MIN(rows, m_maxRows - m_rowNumber);
  }

  if (m_currentArd->HaveBindingsChanged()) {
    // TODO: Deal handle when offset != bufferlength.

    // Wipe out all bindings in the ResultSet.
    // Note that the number of ARD records can both be more or less
    // than the number of columns.
    SQLULEN bindOffset = m_currentArd->GetBindOffset();
    for (size_t i = 0; i < m_ird->GetRecords().size(); i++) {
      if (i < m_currentArd->GetRecords().size() && m_currentArd->GetRecords()[i].m_isBound) {
        const DescriptorRecord& ardRecord = m_currentArd->GetRecords()[i];
        m_currenResult->BindColumn(i+1, static_cast<CDataType>(ardRecord.m_type), ardRecord.m_precision,
          ardRecord.m_scale, reinterpret_cast<char*>(ardRecord.m_dataPtr) + bindOffset,
          GetLength(ardRecord),
          reinterpret_cast<ssize_t*>(
            reinterpret_cast<char*>(ardRecord.m_indicatorPtr) + bindOffset));
      } else {
        m_currenResult->BindColumn(i+1, CDataType_CHAR /* arbitrary type, not used */, 0, 0, nullptr, 0, nullptr);
      }
    }
    m_currentArd->NotifyBindingsHavePropagated();
  }

  size_t rowsFetched = m_currenResult->Move(rows);
  m_rowNumber += rowsFetched;
  m_hasReachedEndOfResult = rowsFetched != rows;
  return rowsFetched != 0;
}

void ODBCStatement::GetStmtAttr(SQLINTEGER statementAttribute,
                                SQLPOINTER output, SQLINTEGER bufferSize,
                                SQLINTEGER *strLenPtr, bool isUnicode) {
  using driver::odbcabstraction::Statement;
  boost::optional<Statement::Attribute> spiAttribute;
  switch (statementAttribute) {
    // Descriptor accessor attributes
    case SQL_ATTR_APP_PARAM_DESC:
      DescriptorToHandle(output, m_currentApd, strLenPtr);
      break;
    case SQL_ATTR_APP_ROW_DESC:
      DescriptorToHandle(output, m_currentArd, strLenPtr);
      break;
    case SQL_ATTR_IMP_PARAM_DESC:
      DescriptorToHandle(output, m_ipd.get(), strLenPtr);
      break;
    case SQL_ATTR_IMP_ROW_DESC:
      DescriptorToHandle(output, m_ird.get(), strLenPtr);
      break;

    // Attributes that are descriptor fields
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
      m_currentApd->GetHeaderField(SQL_DESC_BIND_OFFSET_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_PARAM_BIND_TYPE:
      m_currentApd->GetHeaderField(SQL_DESC_BIND_TYPE, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_PARAM_OPERATION_PTR:
      m_currentApd->GetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_PARAM_STATUS_PTR:
      m_ipd->GetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_PARAMS_PROCESSED_PTR:
      m_ipd->GetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_PARAMSET_SIZE:
      m_currentApd->GetHeaderField(SQL_DESC_ARRAY_SIZE, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_ROW_ARRAY_SIZE:
      m_currentArd->GetHeaderField(SQL_DESC_ARRAY_SIZE, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
      m_currentArd->GetHeaderField(SQL_DESC_BIND_OFFSET_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_ROW_BIND_TYPE:
      m_currentArd->GetHeaderField(SQL_DESC_BIND_TYPE, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_ROW_OPERATION_PTR:
      m_currentArd->GetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_ROW_STATUS_PTR:
      m_ird->GetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_ROWS_FETCHED_PTR:
      m_ird->GetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_ASYNC_ENABLE:
      GetAttribute(static_cast<SQLULEN>(SQL_ASYNC_ENABLE_OFF), output, bufferSize, strLenPtr);
      break;

#ifdef SQL_ATTR_ASYNC_STMT_EVENT
    case SQL_ATTR_ASYNC_STMT_EVENT:
      throw DriverException("Unsupported attribute");
#endif
#ifdef SQL_ATTR_ASYNC_STMT_PCALLBACK
    case SQL_ATTR_ASYNC_STMT_PCALLBACK:
      throw DriverException("Unsupported attribute");
#endif
#ifdef SQL_ATTR_ASYNC_STMT_PCONTEXT
    case SQL_ATTR_ASYNC_STMT_PCONTEXT:
      throw DriverException("Unsupported attribute");
#endif
    case SQL_ATTR_CURSOR_SCROLLABLE:
      GetAttribute(static_cast<SQLULEN>(SQL_NONSCROLLABLE), output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_CURSOR_SENSITIVITY:
      GetAttribute(static_cast<SQLULEN>(SQL_UNSPECIFIED), output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_CURSOR_TYPE:
      GetAttribute(static_cast<SQLULEN>(SQL_CURSOR_FORWARD_ONLY), output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_ENABLE_AUTO_IPD:
      GetAttribute(static_cast<SQLULEN>(SQL_FALSE), output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_FETCH_BOOKMARK_PTR:
      GetAttribute(static_cast<SQLPOINTER>(NULL), output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_KEYSET_SIZE:
      GetAttribute(static_cast<SQLULEN>(0), output, bufferSize, strLenPtr);
      break;

    case SQL_ATTR_ROW_NUMBER:
      GetAttribute(static_cast<SQLULEN>(m_rowNumber), output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_SIMULATE_CURSOR:
      GetAttribute(static_cast<SQLULEN>(SQL_SC_UNIQUE), output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_USE_BOOKMARKS:
      GetAttribute(static_cast<SQLULEN>(SQL_UB_OFF), output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_CONCURRENCY:
      GetAttribute(static_cast<SQLULEN>(SQL_CONCUR_READ_ONLY), output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_MAX_ROWS:
      GetAttribute(static_cast<SQLULEN>(m_maxRows), output, bufferSize, strLenPtr);
      break;
    case SQL_ATTR_RETRIEVE_DATA:
      GetAttribute(static_cast<SQLULEN>(SQL_RD_ON), output, bufferSize, strLenPtr);
      break;

    // Driver-level statement attributes. These are all SQLULEN attributes.
    case SQL_ATTR_MAX_LENGTH:
      spiAttribute = m_spiStatement->GetAttribute(Statement::MAX_LENGTH);
      break;
    case SQL_ATTR_METADATA_ID:
      spiAttribute = m_spiStatement->GetAttribute(Statement::METADATA_ID);
      break;
    case SQL_ATTR_NOSCAN:
      spiAttribute = m_spiStatement->GetAttribute(Statement::NOSCAN);
      break;
    case SQL_ATTR_QUERY_TIMEOUT:
      spiAttribute = m_spiStatement->GetAttribute(Statement::QUERY_TIMEOUT);
      break;
    default:
      throw DriverException("Invalid statement attribute");
  }

  if (spiAttribute) {
    GetAttribute(static_cast<SQLULEN>(boost::get<size_t>(*spiAttribute)),
                 output, bufferSize, strLenPtr);
  }

  // throw DriverException("Invalid statement attribute");
}

void ODBCStatement::SetStmtAttr(SQLINTEGER statementAttribute, SQLPOINTER value,
                                SQLINTEGER bufferSize, bool isUnicode) {
  size_t attributeToWrite = 0;
  bool successfully_written = false;

  switch (statementAttribute) {
    case SQL_ATTR_APP_PARAM_DESC: {
      ODBCDescriptor* desc = static_cast<ODBCDescriptor*>(value);
      if (m_currentApd != desc) {
        if (m_currentApd != m_builtInApd.get()) {
          m_currentApd->DetachFromStatement(this, true);
        }
        m_currentApd = desc;
        if (m_currentApd != m_builtInApd.get()) {
          desc->RegisterToStatement(this, true);
        }
      }
      return;
    }
    case SQL_ATTR_APP_ROW_DESC: {
      ODBCDescriptor* desc = static_cast<ODBCDescriptor*>(value);
      if (m_currentArd != desc) {
        if (m_currentArd != m_builtInArd.get()) {
          m_currentArd->DetachFromStatement(this, false);
        }
        m_currentArd = desc;
        if (m_currentArd != m_builtInArd.get()) {
          desc->RegisterToStatement(this, false);
        }
      }
      return;
    }
    case SQL_ATTR_IMP_PARAM_DESC:
      throw DriverException("Cannot assign impl descriptor.");
    case SQL_ATTR_IMP_ROW_DESC:
      throw DriverException("Cannot assign impl descriptor.");
      // Attributes that are descriptor fields
    case SQL_ATTR_PARAM_BIND_OFFSET_PTR:
      m_currentApd->SetHeaderField(SQL_DESC_BIND_OFFSET_PTR, value, bufferSize);
      return;
    case SQL_ATTR_PARAM_BIND_TYPE:
      m_currentApd->SetHeaderField(SQL_DESC_BIND_TYPE, value, bufferSize);
      return;
    case SQL_ATTR_PARAM_OPERATION_PTR:
      m_currentApd->SetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, value, bufferSize);
      return;
    case SQL_ATTR_PARAM_STATUS_PTR:
      m_ipd->SetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, value, bufferSize);
      return;
    case SQL_ATTR_PARAMS_PROCESSED_PTR:
      m_ipd->SetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, value, bufferSize);
      return;
    case SQL_ATTR_PARAMSET_SIZE:
      m_currentApd->SetHeaderField(SQL_DESC_ARRAY_SIZE, value, bufferSize);
      return;
    case SQL_ATTR_ROW_ARRAY_SIZE:
      m_currentArd->SetHeaderField(SQL_DESC_ARRAY_SIZE, value, bufferSize);
      return;
    case SQL_ATTR_ROW_BIND_OFFSET_PTR:
      m_currentArd->SetHeaderField(SQL_DESC_BIND_OFFSET_PTR, value, bufferSize);
      return;
    case SQL_ATTR_ROW_BIND_TYPE:
      m_currentArd->SetHeaderField(SQL_DESC_BIND_TYPE, value, bufferSize);
      return;
    case SQL_ATTR_ROW_OPERATION_PTR:
      m_currentArd->SetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, value, bufferSize);
      return;
    case SQL_ATTR_ROW_STATUS_PTR:
      m_ird->SetHeaderField(SQL_DESC_ARRAY_STATUS_PTR, value, bufferSize);
      return;
    case SQL_ATTR_ROWS_FETCHED_PTR:
      m_ird->SetHeaderField(SQL_DESC_ROWS_PROCESSED_PTR, value, bufferSize);
      return;

    case SQL_ATTR_ASYNC_ENABLE:
#ifdef SQL_ATTR_ASYNC_STMT_EVENT
    case SQL_ATTR_ASYNC_STMT_EVENT:
      throw DriverException("Unsupported attribute");
#endif
#ifdef SQL_ATTR_ASYNC_STMT_PCALLBACK
    case SQL_ATTR_ASYNC_STMT_PCALLBACK:
      throw DriverException("Unsupported attribute");
#endif
#ifdef SQL_ATTR_ASYNC_STMT_PCONTEXT
    case SQL_ATTR_ASYNC_STMT_PCONTEXT:
      throw DriverException("Unsupported attribute");
#endif
    case SQL_ATTR_CONCURRENCY:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_CONCUR_READ_ONLY));
      return;
    case SQL_ATTR_CURSOR_SCROLLABLE:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_NONSCROLLABLE));
      return;
    case SQL_ATTR_CURSOR_SENSITIVITY:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_UNSPECIFIED));
      return;
    case SQL_ATTR_CURSOR_TYPE:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_CURSOR_FORWARD_ONLY));
      return;
    case SQL_ATTR_ENABLE_AUTO_IPD:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_FALSE));
      return;
    case SQL_ATTR_FETCH_BOOKMARK_PTR:
      if (value != NULL) {
        throw DriverException("Optional feature not implemented");
      }
      return;
    case SQL_ATTR_KEYSET_SIZE:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(0));
      return;
    case SQL_ATTR_ROW_NUMBER:
      throw DriverException("Cannot set read-only attribute");
    case SQL_ATTR_SIMULATE_CURSOR:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_SC_UNIQUE));
      return;
    case SQL_ATTR_USE_BOOKMARKS:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_UB_OFF));
      return;
    case SQL_ATTR_RETRIEVE_DATA:
      CheckIfAttributeIsSetToOnlyValidValue(value, static_cast<SQLULEN>(SQL_TRUE));
      return;

    case SQL_ATTR_MAX_ROWS:
      throw DriverException("Cannot set read-only attribute");

    // Driver-leve statement attributes. These are all size_t attributes
    case SQL_ATTR_MAX_LENGTH:
      SetAttribute(value, attributeToWrite);
      successfully_written = m_spiStatement->SetAttribute(Statement::MAX_LENGTH, attributeToWrite);
      break;
    case SQL_ATTR_METADATA_ID:
      SetAttribute(value, attributeToWrite);
      successfully_written = m_spiStatement->SetAttribute(Statement::METADATA_ID, attributeToWrite);
      break;
    case SQL_ATTR_NOSCAN:
      SetAttribute(value, attributeToWrite);
      successfully_written = m_spiStatement->SetAttribute(Statement::NOSCAN, attributeToWrite);
      break;
    case SQL_ATTR_QUERY_TIMEOUT:
      SetAttribute(value, attributeToWrite);
      successfully_written = m_spiStatement->SetAttribute(Statement::QUERY_TIMEOUT, attributeToWrite);
      break;
    default:
          ;
      //  throw DriverException("Invalid attribute: " + std::to_string(attribute));
  }
  if (!successfully_written) {
    //  throw DriverException("Optional value changed.");
    // Commented out until we can properly report this error.
  }
}

void ODBCStatement::RevertAppDescriptor(bool isApd) {
  if (isApd) {
    m_currentApd = m_builtInApd.get();
  } else {
    m_currentArd = m_builtInArd.get();
  }
}

void ODBCStatement::closeCursor(bool suppressErrors) {
  if (!suppressErrors && !m_currenResult.get()) {
    throw DriverException("Invalid cursor state");
  }
  
  if (m_currenResult.get()) {
    m_currenResult->Close();
  }
}

bool ODBCStatement::GetData(SQLSMALLINT recordNumber, SQLSMALLINT cType, SQLPOINTER dataPtr, SQLLEN bufferLength, SQLLEN* indicatorPtr) {
  if (recordNumber == 0) {
    throw DriverException("Bookmarks are not supported");
  }

  SQLSMALLINT evaluatedCType = cType;

  // TODO: Get proper default precision and scale from abstraction.
  int precision = 0;
  int scale = 0;

  if (cType == SQL_ARD_TYPE) {
    // DM should validate the ARD size.
    assert(m_currentArd->GetRecords().size() <= recordNumber);
    const DescriptorRecord& record = m_currentArd->GetRecords()[recordNumber-1];
    evaluatedCType = record.m_conciseType;
    precision = record.m_precision;
    scale = record.m_scale;
  }

  // Note: this is intentionally not an else if, since the type can be SQL_C_DEFAULT in the ARD.
  if (evaluatedCType == SQL_C_DEFAULT) {
    const DescriptorRecord& ardRecord = m_currentArd->GetRecords()[recordNumber-1];
    precision = ardRecord.m_precision;
    scale = ardRecord.m_scale;

    const DescriptorRecord& irdRecord = m_ird->GetRecords()[recordNumber-1];
    evaluatedCType = getCTypeForSQLType(irdRecord);
  }

  return m_currenResult->GetData(recordNumber, static_cast<CDataType>(evaluatedCType), precision,
                       scale, dataPtr, bufferLength, indicatorPtr);
}

void ODBCStatement::releaseStatement() {
  closeCursor(true);
  m_connection.dropStatement(this);
}

void ODBCStatement::GetTables(const std::string* catalog, const std::string* schema, const std::string* table, const std::string* tableType) {
  closeCursor(true);
  if (m_connection.IsOdbc2Connection()) {
    m_currenResult = m_spiStatement->GetTables_V2(catalog, schema, table, tableType);
  } else {
    m_currenResult = m_spiStatement->GetTables_V3(catalog, schema, table, tableType);
  }
  m_ird->PopulateFromResultSetMetadata(m_currenResult->GetMetadata().get());
  m_hasReachedEndOfResult = false;

  // Direct execution wipes out the prepared state.
  m_isPrepared = false;
}

void ODBCStatement::GetColumns(const std::string* catalog, const std::string* schema, const std::string* table, const std::string* column) {
  closeCursor(true);
  if (m_connection.IsOdbc2Connection()) {
    m_currenResult = m_spiStatement->GetColumns_V2(catalog, schema, table, column);
  } else {
    m_currenResult = m_spiStatement->GetColumns_V3(catalog, schema, table, column);
  }
  m_ird->PopulateFromResultSetMetadata(m_currenResult->GetMetadata().get());
  m_hasReachedEndOfResult = false;

  // Direct execution wipes out the prepared state.
  m_isPrepared = false;
}

void ODBCStatement::GetTypeInfo(SQLSMALLINT dataType) {
  closeCursor(true);
  m_currenResult = m_spiStatement->GetTypeInfo(dataType);

  m_ird->PopulateFromResultSetMetadata(m_currenResult->GetMetadata().get());
  m_hasReachedEndOfResult = false;

  // Direct execution wipes out the prepared state.
  m_isPrepared = false;
}
