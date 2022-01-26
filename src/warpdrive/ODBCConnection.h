/* File:			ODBCConnection.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include <sql.h>
#include <memory>
#include <vector>
#include <map>
#include <odbcabstraction/connection.h>

namespace ODBC
{
  class ODBCEnvironment;
  class ODBCDescriptor;
  class ODBCStatement;
}

/**
 * @brief An abstraction over an ODBC connection handle. This also wraps an SPI Connection.
 */
namespace ODBC
{
class ODBCConnection {
  public:
    ODBCConnection(const ODBCConnection&) = delete;
    ODBCConnection& operator=(const ODBCConnection&) = delete;

    ODBCConnection(ODBCEnvironment& environment, 
      std::shared_ptr<driver::odbcabstraction::Connection> spiConnection);
    
    bool isConnected() const;
    void connect(const driver::odbcabstraction::Connection::ConnPropertyMap &properties,
                       std::vector<std::string> &missing_properties);
    
    ~ODBCConnection() = default;

    void disconnect();

    void releaseConnection();

    std::shared_ptr<ODBCStatement> createStatement();
    void dropStatement(ODBCStatement* statement);

    std::shared_ptr<ODBCDescriptor> createDescriptor();
    void dropDescriptor(ODBCDescriptor* descriptor);

    inline bool IsOdbc2Connection() const {
      return m_is2xConnection;
    }

    static void getPropertiesFromConnString(const std::string& connStr, 
      driver::odbcabstraction::Connection::ConnPropertyMap &properties);

  private:
    ODBCEnvironment& m_environment;
    std::shared_ptr<driver::odbcabstraction::Connection> m_spiConnection;
    std::vector<std::shared_ptr<ODBCStatement> > m_statements;
    std::vector<std::shared_ptr<ODBCDescriptor> > m_descriptors;
    const bool m_is2xConnection;
    bool m_isConnected;
};

}
