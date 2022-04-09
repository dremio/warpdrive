/* File:			ODBCStatement.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include "wdodbc.h"
#include <sql.h>
#include <memory>
#include <string>

namespace driver {
namespace odbcabstraction {
  class Statement;
  class ResultSet;
}
}

namespace ODBC {
  class ODBCConnection;
  class ODBCDescriptor;
}

/**
 * @brief An abstraction over an ODBC connection handle. This also wraps an SPI Connection.
 */
namespace ODBC
{
class ODBCStatement {
  public:
    ODBCStatement(const ODBCStatement&) = delete;
    ODBCStatement& operator=(const ODBCStatement&) = delete;

    ODBCStatement(ODBCConnection& connection, 
      std::shared_ptr<driver::odbcabstraction::Statement> spiStatement);
    
    ~ODBCStatement() = default;

    void Prepare(const std::string& query);
    void ExecutePrepared();
    void ExecuteDirect(const std::string& query);

    /**
     * @brief Returns true if the number of rows fetch was greater than zero.
     */
    bool Fetch(size_t rows);
    bool isPrepared() const;

    void GetAttribute(SQLINTEGER statementAttribute, SQLPOINTER output, SQLLEN bufferSize, SQLLEN* strLenPtr);
    void SetAttribute(SQLINTEGER statementAttribute, SQLPOINTER value, SQLLEN bufferSize);

    void RevertAppDescriptor(bool isApd);

    inline const ODBCDescriptor* GetIRD() const {
      return m_ird.get();
    }

    inline ODBCDescriptor* GetARD() {
      return m_currentArd;
    }

    bool GetData(SQLSMALLINT recordNumber, SQLSMALLINT cType, SQLPOINTER dataPtr, SQLLEN bufferLength, SQLLEN* indicatorPtr);

    /**
     * @brief Closes the cursor. This does _not_ un-prepare the statement or change
     * bindings.
     */
    void closeCursor(bool suppressErrors);

    /**
     * @brief Releases this statement from memory.
     */
    void releaseStatement();

    void GetTables(const std::string* catalog, const std::string* schema, const std::string* table, const std::string* tableType);
    void GetColumns(const std::string* catalog, const std::string* schema, const std::string* table, const std::string* column);
    void GetTypeInfo(SQLSMALLINT dataType);

  private:
    ODBCConnection& m_connection;
    std::shared_ptr<driver::odbcabstraction::Statement> m_spiStatement;
    std::shared_ptr<driver::odbcabstraction::ResultSet> m_currenResult;

    std::shared_ptr<ODBCDescriptor> m_builtInArd;
    std::shared_ptr<ODBCDescriptor> m_builtInApd;
    std::shared_ptr<ODBCDescriptor> m_ipd;
    std::shared_ptr<ODBCDescriptor> m_ird;
    ODBCDescriptor* m_currentArd;
    ODBCDescriptor* m_currentApd;
    bool m_isPrepared;
    bool m_hasReachedEndOfResult;
};
}
