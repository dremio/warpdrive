/* File:			ODBCDescriptor.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include "wdodbc.h"
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
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
    SQLSMALLINT m_conciseType = SQL_C_DEFAULT;
    SQLSMALLINT m_datetimeIntervalCode;
    SQLSMALLINT m_fixedPrecScale;
    SQLSMALLINT m_nullable;
    SQLSMALLINT m_paramType = SQL_PARAM_INPUT;
    SQLSMALLINT m_precision;
    SQLSMALLINT m_rowVer;
    SQLSMALLINT m_scale;
    SQLSMALLINT m_searchable;
    SQLSMALLINT m_type = SQL_C_DEFAULT;
    SQLSMALLINT m_unnamed;
    SQLSMALLINT m_unsigned;
    SQLSMALLINT m_updatable;
    bool m_isBound = false;

    void CheckConsistency();
  };

  class ODBCDescriptor {
    public:
      /**
       * @brief Construct a new ODBCDescriptor object. Link the descriptor to a connection,
       * if applicable. A nullptr should be supplied for conn if the descriptor should not be linked.
       */
      ODBCDescriptor(ODBCConnection* conn, bool isAppDescriptor, bool isWritable, bool is2xConnection);

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
      std::vector<DescriptorRecord>& GetRecords();

      void BindCol(SQLSMALLINT recordNumber, SQLSMALLINT cType, SQLPOINTER dataPtr, SQLLEN bufferLength, SQLLEN* indicatorPtr);
      void SetDataPtrOnRecord(SQLPOINTER dataPtr, SQLSMALLINT recNumber);

      inline SQLULEN GetBindOffset() {
        return m_bindOffsetPtr ? *m_bindOffsetPtr : 0UL;
      }

      inline SQLULEN GetBoundStructOffset() {
        // If this is SQL_BIND_BY_COLUMN, m_bindType is zero which indicates no offset due to use of a bound struct.
        // If this is non-zero, row-wise binding is being used so the app should set this to sizeof(their struct).
        return m_bindType;
      }

      inline SQLULEN GetArraySize() {
        return m_arraySize;
      }

      void NotifyBindingsHavePropagated() {
        m_hasBindingsChanged = false;
      }

    private:
      std::vector<ODBCStatement*> m_registeredOnStatementsAsApd;
      std::vector<ODBCStatement*> m_registeredOnStatementsAsArd;
      std::vector<DescriptorRecord> m_records;
      ODBCConnection* m_owningConnection;
      SQLUSMALLINT* m_arrayStatusPtr;
      SQLLEN* m_bindOffsetPtr;
      SQLULEN* m_rowsProccessedPtr;
      SQLULEN m_arraySize;
      SQLINTEGER m_bindType;
      SQLSMALLINT m_highestOneBasedBoundRecord;
      const bool m_is2xConnection;
      bool m_isAppDescriptor;
      bool m_isWritable;
      bool m_hasBindingsChanged;
  };
}
