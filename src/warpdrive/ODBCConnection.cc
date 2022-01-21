/* File:			ODBConnection.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCConnection.h"

#include "ODBCEnvironment.h"
#include "ODBCStatement.h"
#include "odbcinst.h"
#include <iterator>
#include <memory>
#include <odbcabstraction/connection.h>
#include <odbcabstraction/exceptions.h>
#include <odbcabstraction/statement.h>
#include <boost/algorithm/string.hpp>
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
  const boost::xpressive::sregex CONNECTION_STR_REGEX(boost::xpressive::sregex::compile(
    "([^=;]+)=({.+}|[^=;]+|[^;])"));

// Load properties from the given DSN. The properties loaded do _not_ overwrite existing
// entries in the properties.
void loadPropertiesFromDSN(const std::string& dsn, Connection::ConnPropertyMap& properties) {
  const size_t BUFFER_SIZE = 1024 * 10;
  std::vector<char> outputBuffer;
  outputBuffer.resize(BUFFER_SIZE, '\0');
  SQLSetConfigMode(ODBC_BOTH_DSN);
  SQLGetPrivateProfileString(dsn.c_str(), NULL, "", &outputBuffer[0], BUFFER_SIZE, NULL);

  // The output buffer holds the list of keys in a series of NUL-terminated strings.
  // The series is terminated with an empty string (eg a NUL-terminator terminating the last
  // key followed by a NUL terminator after).
  std::vector<std::string> keys;
  size_t pos = 0;
  while (pos < BUFFER_SIZE) {
    std::string key(&outputBuffer[pos]);
    if (key.empty()) {
      break;
    }
    size_t len = key.size();

    // Skip over Driver or DSN keys.
    if (!boost::iequals(key, "DSN") &&
        !boost::iequals(key, "Driver")) {
      keys.emplace_back(std::move(key));
    }
    pos += len + 1;
  }

  for (auto& key : keys) {
    outputBuffer.clear();
    outputBuffer.resize(BUFFER_SIZE, '\0');
    SQLGetPrivateProfileString(dsn.c_str(), key.c_str(), "", &outputBuffer[0], BUFFER_SIZE, NULL);
    std:: string value = std::string(&outputBuffer[0]);
    auto propIter = properties.find(key);
    if (propIter == properties.end()) {
      properties.emplace(std::make_pair(std::move(key), std::move(value)));
    }
  }
}
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

void ODBCConnection::disconnect() {
  if (m_isConnected) {
    m_spiConnection->Close();
    m_isConnected = false;
  }
}

void ODBCConnection::releaseConnection() {
  disconnect();
  m_environment.DropConnection(this);
}

std::shared_ptr<ODBCStatement> ODBCConnection::createStatement() {
  std::shared_ptr<Statement> spiStatement = m_spiConnection->CreateStatement();
  std::shared_ptr<ODBCStatement> statement = std::make_shared<ODBCStatement>(*this, spiStatement);
  m_statements.push_back(statement);
  return statement;
}

void ODBCConnection::dropStatement(ODBCStatement* stmt) {
    auto it = std::find_if(m_statements.begin(), m_statements.end(), 
      [&stmt] (const std::shared_ptr<ODBCStatement>& statement) { return statement.get() == stmt; });
    if (m_statements.end() != it) {
        m_statements.erase(it);
    }
}

// Public Static ===================================================================================
void ODBCConnection::getPropertiesFromConnString(const std::string& connStr,
  Connection::ConnPropertyMap &properties)
{
  const int groups[] = { 1, 2 }; // CONNECTION_STR_REGEX has two groups. key: 1, value: 2
  boost::xpressive::sregex_token_iterator regexIter(connStr.begin(), connStr.end(), 
    CONNECTION_STR_REGEX, groups), end;
  
  bool isDsnFirst = false;
  bool isDriverFirst = false;
  for (auto it = regexIter; end != regexIter; ++regexIter) {
    std::string key = *regexIter;
    std::string value = *++regexIter;

    // If the DSN shows up before driver key, load settings from the DSN.
    // Only load values from the DSN once regardless of how many times the DSN
    // key shows up.
    if (boost::iequals(key, "DSN")) {
      if (!isDriverFirst) {
        if (!isDsnFirst) {
          isDsnFirst = true;
          loadPropertiesFromDSN(value, properties);
        }
      }
      continue;
    } else if (boost::iequals(key, "Driver")) {
      if (!isDsnFirst) {
        isDriverFirst = true;
      }
      continue;
    }

    // Strip wrapping curly braces.
    if (value.size() >= 2 && value[0] == '{' && value[value.size() - 1] == '}') {
      value = value.substr(1, value.size() - 2);
    }

    // Overwrite the existing value. Later copies of the key take precedence,
    // including over entries in the DSN.
    properties[key] = std::move(value);
  }

}
