/* File:			ODBConnection.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCConnection.h"

#include "ODBCEnvironment.h"
#include <memory>
#include <odbcabstraction/driver.h>
#include <odbcabstraction/exceptions.h>
#include <algorithm>
#include <boost/tokenizer.hpp>

using namespace ODBC;
using namespace driver::odbcabstraction;
using driver::odbcabstraction::Connection;
using driver::odbcabstraction::DriverException;

// Public =========================================================================================
ODBCConnection::ODBCConnection(ODBCEnvironment& environment, 
  std::shared_ptr<Connection> spiConnection) :
  m_environment(environment),
  m_spiConnection(spiConnection)
{

}

bool ODBCConnection::isConnected() const
{
    return m_isConnected;
}

void ODBCConnection::connect(const Connection::ConnPropertyMap &properties,
  std::vector<std::string> &missing_properties)
{
  if (m_isConnected) {
    throw DriverException("Already connected.");
  }
  m_spiConnection->Connect(properties, missing_properties);
  m_isConnected = true;
}

// Public Static ===================================================================================
void ODBCConnection::getPropertiesFromConnString(const std::string& connStr,
  Connection::ConnPropertyMap &properties)
{
  // TODO: Read from the DSN if a DSN is used. Read the Driver key as well.
  boost::tokenizer<boost::char_separator<char> > strTokenizer(connStr, boost::char_separator<char>(";"));
  for (auto iter = strTokenizer.begin(); strTokenizer.end() != iter; iter++)
  {
    boost::tokenizer<boost::char_separator<char> > kvTokenizer(*iter, boost::char_separator<char>("="));
    auto kv_iter = kvTokenizer.begin();
    std::string key = *kv_iter++;
    Connection::Property value(*kv_iter);
    properties.emplace(std::make_pair(std::move(key), std::move(value)));
  }
}
