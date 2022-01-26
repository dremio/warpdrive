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
  switch (fieldIdentifier) {
    case SQL_DESC_ALLOC_TYPE:
      throw DriverException("Invalid descriptor field");
    case SQL_DESC_ARRAY_SIZE:
    case SQL_DESC_ARRAY_STATUS_PTR:
    case SQL_DESC_BIND_OFFSET_PTR:
    case SQL_DESC_BIND_TYPE:
    case SQL_DESC_ROWS_PROCESSED_PTR:
      if (!m_isWritable) {
        throw DriverException("Cannot modify read-only field");
      }
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

}

void ODBCDescriptor::GetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER* outputLength) {
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

void ODBCDescriptor::GetField(SQLSMALLINT recordNumber, SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER* outputLength) {

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
    m_records[i].m_baseColumnName = rsmd->GetBaseColumnName(i);
    m_records[i].m_baseTableName = rsmd->GetBaseTableName(i);
    m_records[i].m_catalogName = rsmd->GetCatalogName(i);
    m_records[i].m_label = rsmd->GetColumnLabel(i);
    m_records[i].m_literalPrefix = rsmd->GetLiteralPrefix(i);
    m_records[i].m_literalSuffix = rsmd->GetLiteralSuffix(i);
    m_records[i].m_localTypeName = rsmd->GetLocalTypeName(i);
    m_records[i].m_name = rsmd->GetName(i);
    m_records[i].m_schemaName = rsmd->GetSchemaName(i);
    m_records[i].m_tableName = rsmd->GetTableName(i);
    m_records[i].m_typeName = rsmd->GetTypeName(i);
    m_records[i].m_dataPtr = nullptr;
    m_records[i].m_indicatorPtr = nullptr;
    m_records[i].m_displaySize = rsmd->GetColumnDisplaySize(i);
    m_records[i].m_octetLength = rsmd->GetOctetLength(i);
    m_records[i].m_length = rsmd->GetLength(i);
    m_records[i].m_autoUniqueValue = rsmd->IsAutoUnique(i) ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_caseInsensitive = rsmd->IsCaseSensitive(i)? SQL_TRUE : SQL_FALSE;
    m_records[i].m_datetimeIntervalPrecision; // TODO - update when rsmd adds this
    m_records[i].m_numPrecRadix = rsmd->GetNumPrecRadix(i);
    m_records[i].m_conciseType = rsmd->GetDataType(i); // TODO: Check if this needs to be different than datatype.
    m_records[i].m_datetimeIntervalCode; // TODO
    m_records[i].m_fixedPrecScale = rsmd->IsFixedPrecScale(i) ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_nullable = rsmd->IsNullable(i);
    m_records[i].m_paramType = SQL_PARAM_INPUT;
    m_records[i].m_precision = rsmd->GetPrecision(i);
    m_records[i].m_rowVer = SQL_FALSE;
    m_records[i].m_scale = rsmd->GetScale(i);
    m_records[i].m_searchable = rsmd->IsSearchable(i);
    m_records[i].m_type = rsmd->GetDataType(i);
    m_records[i].m_unnamed = m_records[i].m_name.empty() ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_unsigned = rsmd->IsUnsigned(i) ? SQL_TRUE : SQL_FALSE;
    m_records[i].m_updatable = rsmd->GetUpdatable(i);
  }
}

const std::vector<DescriptorRecord>& ODBCDescriptor::GetRecords() const {
  return m_records;
}
