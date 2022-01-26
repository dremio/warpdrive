/* File:			ODBCDescriptor.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCDescriptor.h"

#include "AttributeUtils.h"
#include "ODBCConnection.h"
#include "ODBCStatement.h"
#include "sql.h"
#include <odbcabstraction/statement.h>
#include <odbcabstraction/exceptions.h>
#include <odbcabstraction/result_set_metadata.h>

using namespace ODBC;
using namespace driver::odbcabstraction;

// Public =========================================================================================
ODBCDescriptor::ODBCDescriptor(ODBCConnection* conn, bool isAppDescriptor, bool isWritable) :
  m_owningConnection(conn),
  m_isAppDescriptor(isAppDescriptor),
  m_isWritable(isWritable),
  m_hasBindingsChanged(true) {

}

void ODBCDescriptor::SetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {
  if (!m_isWritable) {
    throw DriverException("Cannot modify read-only descriptor");
  }

  switch (fieldIdentifier) {
    case SQL_DESC_ALLOC_TYPE:
      throw DriverException("Invalid descriptor field");
    case SQL_DESC_ARRAY_SIZE:
    case SQL_DESC_ARRAY_STATUS_PTR:
    case SQL_DESC_BIND_OFFSET_PTR:
    case SQL_DESC_BIND_TYPE:
    case SQL_DESC_ROWS_PROCESSED_PTR:
      // TODO
      break;
    case SQL_DESC_COUNT: {
      SQLSMALLINT newCount = *reinterpret_cast<SQLSMALLINT*>(value);
      m_records.resize(newCount);
      break;
    }
  }
}

void ODBCDescriptor::SetField(SQLSMALLINT recordNumber, SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {
  if (!m_isWritable) {
    throw DriverException("Cannot modify read-only descriptor");
  }

  if (recordNumber == 0) {
    throw DriverException("Bookmarks are unsupported.");
  }

  if (recordNumber > m_records.size()) {
    throw DriverException("Invalid descriptor index");
  }

  SQLSMALLINT zeroBasedRecord = recordNumber - 1;
  DescriptorRecord& record = m_records[zeroBasedRecord];
  switch (fieldIdentifier) {
    case SQL_DESC_AUTO_UNIQUE_VALUE:
    case SQL_DESC_BASE_COLUMN_NAME:
    case SQL_DESC_BASE_TABLE_NAME:
    case SQL_DESC_CASE_SENSITIVE:
    case SQL_DESC_CATALOG_NAME:
    case SQL_DESC_CONCISE_TYPE:
    case SQL_DESC_DATA_PTR:
    case SQL_DESC_DATETIME_INTERVAL_CODE:
    case SQL_DESC_DATETIME_INTERVAL_PRECISION:
    case SQL_DESC_DISPLAY_SIZE:
    case SQL_DESC_FIXED_PREC_SCALE:
    case SQL_DESC_INDICATOR_PTR:
    case SQL_DESC_LABEL:
    case SQL_DESC_LENGTH:
    case SQL_DESC_LITERAL_PREFIX:
    case SQL_DESC_LITERAL_SUFFIX:
    case SQL_DESC_LOCAL_TYPE_NAME:
    case SQL_DESC_NAME:
    case SQL_DESC_NULLABLE:
    case SQL_DESC_NUM_PREC_RADIX:
    case SQL_DESC_OCTET_LENGTH:
    case SQL_DESC_OCTET_LENGTH_PTR:
    case SQL_DESC_PARAMETER_TYPE:
    case SQL_DESC_PRECISION:
    case SQL_DESC_ROWVER:
    case SQL_DESC_SCALE:
    case SQL_DESC_SCHEMA_NAME:
    case SQL_DESC_SEARCHABLE:
    case SQL_DESC_TABLE_NAME:
    case SQL_DESC_TYPE:
    case SQL_DESC_TYPE_NAME:
    case SQL_DESC_UNNAMED:
    case SQL_DESC_UNSIGNED:
    case SQL_DESC_UPDATABLE:
      break;
  }
}

void ODBCDescriptor::GetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER* outputLength) const {
  switch (fieldIdentifier) {
    case SQL_DESC_ALLOC_TYPE: {
      SQLSMALLINT result;
      if (m_owningConnection) {
        result = SQL_DESC_ALLOC_USER;
      } else {
        result = SQL_DESC_ALLOC_AUTO;
      }
      GetAttribute(result, value, bufferLength, outputLength);
      break;
    }
    case SQL_DESC_ARRAY_SIZE:
    case SQL_DESC_ARRAY_STATUS_PTR:
    case SQL_DESC_BIND_OFFSET_PTR:
    case SQL_DESC_BIND_TYPE:
    case SQL_DESC_ROWS_PROCESSED_PTR:
      // TODO:
      break;
    case SQL_DESC_COUNT: {
      SQLSMALLINT resultCount = m_records.size();
      GetAttribute(resultCount, value, bufferLength, outputLength);
      break;
    }
  }
}

void ODBCDescriptor::GetField(SQLSMALLINT recordNumber, SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER* outputLength) const {
  if (recordNumber == 0) {
    throw DriverException("Bookmarks are unsupported.");
  }

  if (recordNumber > m_records.size()) {
    throw DriverException("Invalid descriptor index");
  }

  // TODO: Restrict fields based on AppDescriptor IPD, and IRD.

  SQLSMALLINT zeroBasedRecord = recordNumber - 1;
  const DescriptorRecord& record = m_records[zeroBasedRecord];
  switch (fieldIdentifier) {
    case SQL_DESC_BASE_COLUMN_NAME:
      GetAttributeUTF8(record.m_baseColumnName, value, bufferLength, outputLength);
      break;
    case SQL_DESC_BASE_TABLE_NAME:
      GetAttributeUTF8(record.m_baseTableName, value, bufferLength, outputLength);
      break;
    case SQL_DESC_CATALOG_NAME:
      GetAttributeUTF8(record.m_catalogName, value, bufferLength, outputLength);
      break;
    case SQL_DESC_LABEL:
      GetAttributeUTF8(record.m_label, value, bufferLength, outputLength);
      break;
    case SQL_DESC_LITERAL_PREFIX:
      GetAttributeUTF8(record.m_literalPrefix, value, bufferLength, outputLength);
      break;
    case SQL_DESC_LITERAL_SUFFIX:
      GetAttributeUTF8(record.m_literalSuffix, value, bufferLength, outputLength);
      break;
    case SQL_DESC_LOCAL_TYPE_NAME:
      GetAttributeUTF8(record.m_localTypeName, value, bufferLength, outputLength);
      break;
    case SQL_DESC_NAME:
      GetAttributeUTF8(record.m_name, value, bufferLength, outputLength);
      break;
    case SQL_DESC_SCHEMA_NAME:
      GetAttributeUTF8(record.m_schemaName, value, bufferLength, outputLength);
      break;
    case SQL_DESC_TABLE_NAME:
      GetAttributeUTF8(record.m_tableName, value, bufferLength, outputLength);
      break;
    case SQL_DESC_TYPE_NAME:
      GetAttributeUTF8(record.m_typeName, value, bufferLength, outputLength);
      break;

    case SQL_DESC_DATA_PTR:
      GetAttribute(record.m_dataPtr, value, bufferLength, outputLength);
      break;
    case SQL_DESC_INDICATOR_PTR:
    case SQL_DESC_OCTET_LENGTH_PTR:
      GetAttribute(record.m_indicatorPtr, value, bufferLength, outputLength);
      break;

    case SQL_DESC_LENGTH:
      GetAttribute(record.m_length, value, bufferLength, outputLength);
      break;
    case SQL_DESC_OCTET_LENGTH:
      GetAttribute(record.m_octetLength, value, bufferLength, outputLength);
      break;

    case SQL_DESC_AUTO_UNIQUE_VALUE:
      GetAttribute(record.m_autoUniqueValue, value, bufferLength, outputLength);
      break;
    case SQL_DESC_CASE_SENSITIVE:
      GetAttribute(record.m_caseSensitive, value, bufferLength, outputLength);
      break;
    case SQL_DESC_DATETIME_INTERVAL_PRECISION:
      GetAttribute(record.m_datetimeIntervalPrecision, value, bufferLength, outputLength);
      break;
    case SQL_DESC_NUM_PREC_RADIX:
      GetAttribute(record.m_numPrecRadix, value, bufferLength, outputLength);
      break;

    case SQL_DESC_CONCISE_TYPE:
      GetAttribute(record.m_conciseType, value, bufferLength, outputLength);
      break;
    case SQL_DESC_DATETIME_INTERVAL_CODE:
      GetAttribute(record.m_datetimeIntervalCode, value, bufferLength, outputLength);
      break;
    case SQL_DESC_DISPLAY_SIZE:
      GetAttribute(record.m_displaySize, value, bufferLength, outputLength);
      break;
    case SQL_DESC_FIXED_PREC_SCALE:
      GetAttribute(record.m_fixedPrecScale, value, bufferLength, outputLength);
      break;
    case SQL_DESC_NULLABLE:
      GetAttribute(record.m_nullable, value, bufferLength, outputLength);
      break;
    case SQL_DESC_PARAMETER_TYPE:
      GetAttribute(record.m_paramType, value, bufferLength, outputLength);
      break;
    case SQL_DESC_PRECISION:
      GetAttribute(record.m_precision, value, bufferLength, outputLength);
      break;
    case SQL_DESC_ROWVER:
      GetAttribute(record.m_rowVer, value, bufferLength, outputLength);
      break;
    case SQL_DESC_SCALE:
      GetAttribute(record.m_scale, value, bufferLength, outputLength);
      break;
    case SQL_DESC_SEARCHABLE:
      GetAttribute(record.m_searchable, value, bufferLength, outputLength);
      break;
    case SQL_DESC_TYPE:
      GetAttribute(record.m_type, value, bufferLength, outputLength);
      break;
    case SQL_DESC_UNNAMED:
      GetAttribute(record.m_unnamed, value, bufferLength, outputLength);
      break;
    case SQL_DESC_UNSIGNED:
      GetAttribute(record.m_unsigned, value, bufferLength, outputLength);
      break;
    case SQL_DESC_UPDATABLE:
      GetAttribute(record.m_updatable, value, bufferLength, outputLength);
      break;
  }
}

SQLSMALLINT ODBCDescriptor::getAllocType() const {
  return m_owningConnection != nullptr ? SQL_DESC_ALLOC_USER : SQL_DESC_ALLOC_AUTO;
}

bool ODBCDescriptor::IsAppDescriptor() const {
  return m_isAppDescriptor;
}

bool ODBCDescriptor::HaveBindingsChanged() const {
  return m_hasBindingsChanged;
}

void ODBCDescriptor::RegisterToStatement(ODBCStatement* statement, bool isApd) {
  if (isApd) {
    m_registeredOnStatementsAsApd.push_back(statement);
  } else {
    m_registeredOnStatementsAsArd.push_back(statement);
  }
}

void ODBCDescriptor::DetachFromStatement(ODBCStatement* statement, bool isApd) {
  auto& vectorToUpdate = isApd ? m_registeredOnStatementsAsApd : m_registeredOnStatementsAsArd;
  auto it = std::find(vectorToUpdate.begin(), vectorToUpdate.end(), statement);
  if (it != vectorToUpdate.end()) {
    vectorToUpdate.erase(it);
  }
}

void ODBCDescriptor::ReleaseDescriptor() {
  for (ODBCStatement* stmt : m_registeredOnStatementsAsApd) {
    stmt->RevertAppDescriptor(true);
  }

  for (ODBCStatement* stmt : m_registeredOnStatementsAsArd) {
    stmt->RevertAppDescriptor(false);
  }

  if (m_owningConnection) {
    m_owningConnection->dropDescriptor(this);
  }
}

void ODBCDescriptor::PopulateFromResultSetMetadata(ResultSetMetadata* rsmd) {
  m_records.assign(rsmd->GetColumnCount(), DescriptorRecord());

  for (size_t i = 0; i < m_records.size(); ++i) {
    size_t oneBasedIndex = i + 1;
    m_records[i].m_baseColumnName = rsmd->GetBaseColumnName(oneBasedIndex);
    m_records[i].m_baseTableName = rsmd->GetBaseTableName(oneBasedIndex);
    m_records[i].m_catalogName = rsmd->GetCatalogName(oneBasedIndex);
    m_records[i].m_label = rsmd->GetColumnLabel(oneBasedIndex);
    m_records[i].m_literalPrefix = rsmd->GetLiteralPrefix(oneBasedIndex);
    m_records[i].m_literalSuffix = rsmd->GetLiteralSuffix(oneBasedIndex);
    m_records[i].m_localTypeName = rsmd->GetLocalTypeName(oneBasedIndex);
    m_records[i].m_name = rsmd->GetName(oneBasedIndex);
    m_records[i].m_schemaName = rsmd->GetSchemaName(oneBasedIndex);
    m_records[i].m_tableName = rsmd->GetTableName(oneBasedIndex);
    m_records[i].m_typeName = rsmd->GetTypeName(oneBasedIndex);
    m_records[i].m_dataPtr = nullptr;
    m_records[i].m_indicatorPtr = nullptr;
    m_records[i].m_displaySize = rsmd->GetColumnDisplaySize(oneBasedIndex);
    m_records[i].m_octetLength = rsmd->GetOctetLength(oneBasedIndex);
    m_records[i].m_length = rsmd->GetLength(oneBasedIndex);
    m_records[i].m_autoUniqueValue = rsmd->IsAutoUnique(oneBasedIndex) ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_caseSensitive = rsmd->IsCaseSensitive(oneBasedIndex)? SQL_TRUE : SQL_FALSE;
    m_records[i].m_datetimeIntervalPrecision; // TODO - update when rsmd adds this
    m_records[i].m_numPrecRadix = rsmd->GetNumPrecRadix(oneBasedIndex);
    m_records[i].m_conciseType = rsmd->GetDataType(oneBasedIndex); // TODO: Check if this needs to be different than datatype.
    m_records[i].m_datetimeIntervalCode; // TODO
    m_records[i].m_fixedPrecScale = rsmd->IsFixedPrecScale(oneBasedIndex) ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_nullable = rsmd->IsNullable(oneBasedIndex);
    m_records[i].m_paramType = SQL_PARAM_INPUT;
    m_records[i].m_precision = rsmd->GetPrecision(oneBasedIndex);
    m_records[i].m_rowVer = SQL_FALSE;
    m_records[i].m_scale = rsmd->GetScale(oneBasedIndex);
    m_records[i].m_searchable = rsmd->IsSearchable(oneBasedIndex);
    m_records[i].m_type = rsmd->GetDataType(oneBasedIndex);
    m_records[i].m_unnamed = m_records[i].m_name.empty() ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_unsigned = rsmd->IsUnsigned(oneBasedIndex) ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_updatable = rsmd->GetUpdatable(oneBasedIndex);
  }
}

const std::vector<DescriptorRecord>& ODBCDescriptor::GetRecords() const {
  return m_records;
}
