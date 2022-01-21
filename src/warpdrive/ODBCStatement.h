/* File:			ODBCConnection.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include <sql.h>
#include <memory>

namespace driver {
namespace odbcabstraction {
  class Statement;
  class ResultSet;
}
}

namespace ODBC
{
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
    bool Fetch(size_t rows);
    bool isPrepared() const;

    /**
     * @brief Closes the cursor. This does _not_ un-prepare the statement or change
     * bindings.
     */
    void closeCursor(bool suppressErrors);

    /**
     * @brief Releases this statement from memory.
     */
    void releaseStatement();
  private:
    ODBCConnection& m_connection;
    std::shared_ptr<driver::odbcabstraction::Statement> m_spiStatement;
    std::shared_ptr<driver::odbcabstraction::ResultSet> m_currenResult;
    bool m_isPrepared;
};

}
