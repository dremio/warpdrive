/* File:			ODBConnection.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCConnection.h"

#include "ODBCEnvironment.h"
#include <iterator>
#include <memory>
#include <odbcabstraction/driver.h>
#include <odbcabstraction/exceptions.h>
#include <boost/xpressive/xpressive.hpp>

using namespace ODBC;
using namespace driver::odbcabstraction;
using driver::odbcabstraction::Connection;
using driver::odbcabstraction::DriverException;

namespace
{
  // Key-value pairs separated by semi-colon.
  // Note that the value can be wrapped in curly braces to escape other significant characters
  // such as semi-colons and equals signs.
  // NOTE: This can be optimized to be built statically.
  const boost::xpressive::sregex CONNECTION_STR_REGEX = boost::xpressive::sregex::compile(
    "([^=;]+)=({.+}|[^=;]+|[^;])");
}

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
  const int groups[] = { 1, 2 }; // CONNECTION_STR_REGEX has two groups. key: 1, value: 2
  boost::xpressive::sregex_token_iterator regexIter(connStr.begin(), connStr.end(), 
    CONNECTION_STR_REGEX, groups), end;

  for (auto it = regexIter; end != regexIter; ++regexIter) {
    std::string key = *regexIter;
    std::string value = *++regexIter;

    // Strip wrapping curly braces.
    if (value.size() >= 2 && value[0] == '{' && value[value.size() - 1] == '}') {
      value = value.substr(1, value.size() - 2);
    }
    properties.emplace(std::make_pair(std::move(key), std::move(value)));
  }
}
