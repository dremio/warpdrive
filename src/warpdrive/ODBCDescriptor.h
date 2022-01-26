/* File:			ODBCDescriptor.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include "sqltypes.h"
#include <sql.h>
#include <memory>
#include <vector>
#include <string>

namespace driver {
namespace odbcabstraction {
  class ResultSetMetadata;
}
}
namespace ODBC {
  class ODBCConnection;
  class ODBCStatement;
}

namespace ODBC
{
  struct DescriptorRecord {
    std::string m_baseColumnName;
    std::string m_baseTableName;
    std::string m_catalogName;
    std::string m_label;
    std::string m_literalPrefix;
    std::string m_literalSuffix;
    std::string m_localTypeName;
    std::string m_name;
    std::string m_schemaName;
    std::string m_tableName;
    std::string m_typeName;
    SQLPOINTER m_dataPtr;
    SQLLEN* m_indicatorPtr;
    SQLLEN m_displaySize;
    SQLLEN m_octetLength;
    SQLULEN m_length;
    SQLINTEGER m_autoUniqueValue;
    SQLINTEGER m_caseSensitive;
    SQLINTEGER m_datetimeIntervalPrecision;
    SQLINTEGER m_numPrecRadix;
    SQLSMALLINT m_conciseType;
    SQLSMALLINT m_datetimeIntervalCode;
    SQLSMALLINT m_fixedPrecScale;
    SQLSMALLINT m_nullable;
    SQLSMALLINT m_paramType;
    SQLSMALLINT m_precision;
    SQLSMALLINT m_rowVer;
    SQLSMALLINT m_scale;
    SQLSMALLINT m_searchable;
    SQLSMALLINT m_type;
    SQLSMALLINT m_unnamed;
    SQLSMALLINT m_unsigned;
    SQLSMALLINT m_updatable;
  };

  class ODBCDescriptor {
    public:
      /**
       * @brief Construct a new ODBCDescriptor object. Link the descriptor to a connection,
       * if applicable. A nullptr should be supplied for conn if the descriptor should not be linked.
       */
      ODBCDescriptor(ODBCConnection* conn, bool isAppDescriptor, bool isWritable);

      void SetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void SetField(SQLSMALLINT recordNumber, SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void GetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER* outputLength) const;
      void GetField(SQLSMALLINT recordNumber, SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength, SQLINTEGER* outputLength) const;
      SQLSMALLINT getAllocType() const;
      bool IsAppDescriptor() const;
      bool HaveBindingsChanged() const;
      void RegisterToStatement(ODBCStatement* statement, bool isApd);
      void DetachFromStatement(ODBCStatement* statement, bool isApd);
      void ReleaseDescriptor();

      void PopulateFromResultSetMetadata(driver::odbcabstraction::ResultSetMetadata* rsmd);

      const std::vector<DescriptorRecord>& GetRecords() const;

    private:
      ODBCConnection* m_owningConnection;
      std::vector<ODBCStatement*> m_registeredOnStatementsAsApd;
      std::vector<ODBCStatement*> m_registeredOnStatementsAsArd;
      std::vector<DescriptorRecord> m_records;
      bool m_isAppDescriptor;
      bool m_isWritable;
      bool m_hasBindingsChanged;
  };
}
