/* File:			ODBCEnvironment.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include "ODBCHandle.h"

#include <sql.h>
#include <memory>
#include <vector>

namespace driver {
namespace odbcabstraction {
  class Driver;
}
}

namespace ODBC {
  class ODBCConnection;
}

/**
 * @brief An abstraction over an ODBC environment handle.
 */
namespace ODBC
{
class ODBCEnvironment : public ODBCHandle<ODBCEnvironment> {
  public:
    ODBCEnvironment(std::shared_ptr<driver::odbcabstraction::Driver> driver);
    driver::odbcabstraction::Diagnostics& GetDiagnostics_Impl();
    SQLINTEGER getODBCVersion() const;
    void setODBCVersion(SQLINTEGER version);
    SQLINTEGER getConnectionPooling() const;
    void setConnectionPooling(SQLINTEGER pooling);
    std::shared_ptr<ODBCConnection> CreateConnection();
    void DropConnection(ODBCConnection* conn);
    ~ODBCEnvironment() = default;

  private:
    std::vector<std::shared_ptr<ODBCConnection> > m_connections;
    std::shared_ptr<driver::odbcabstraction::Driver> m_driver;
    std::unique_ptr<driver::odbcabstraction::Diagnostics> m_diagnostics;
    SQLINTEGER m_version;
    SQLINTEGER m_connectionPooling;
};

}
