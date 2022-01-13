/* File:			ODBCEnvironment.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCEnvironment.h"

#include <algorithm>
#include "sqlext.h"
#include <odbcabstraction/driver.h>
#include <odbcabstraction/connection.h>
#include <odbcabstraction/types.h>
#include "ODBCConnection.h"

using namespace ODBC;
using namespace driver::odbcabstraction;

// Public =========================================================================================
ODBCEnvironment::ODBCEnvironment(std::shared_ptr<Driver> driver) :
  m_driver(driver),
  m_version(SQL_OV_ODBC2),
  m_connectionPooling(SQL_CP_OFF) {

}

SQLINTEGER ODBCEnvironment::getODBCVersion() const {
    return m_version;
}

void ODBCEnvironment::setODBCVersion(SQLINTEGER version) {
    m_version = version;
}

SQLINTEGER ODBCEnvironment::getConnectionPooling() const {
    return m_connectionPooling;
}

void ODBCEnvironment::setConnectionPooling(SQLINTEGER connectionPooling) {
    m_connectionPooling = connectionPooling;
}

std::shared_ptr<ODBCConnection> ODBCEnvironment::CreateConnection() {
    std::shared_ptr<Connection> spiConnection = m_driver->CreateConnection(m_version == SQL_OV_ODBC2 ? V_2 : V_3);
    std::shared_ptr<ODBCConnection> newConn = std::make_shared<ODBCConnection>(*this, spiConnection);
    m_connections.push_back(newConn);
    return newConn;
}

void ODBCEnvironment::DropConnection(ODBCConnection* conn) {
    auto it = std::find_if(m_connections.begin(), m_connections.end(), 
      [&conn] (const std::shared_ptr<ODBCConnection>& connection) { return connection.get() == conn; });
    if (m_connections.end() != it) {
        m_connections.erase(it);
    }
}
