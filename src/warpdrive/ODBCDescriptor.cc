/* File:			ODBCDescriptor.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCDescriptor.h"

#include "ODBCConnection.h"
#include "ODBCStatement.h"
#include "sql.h"
#include <odbcabstraction/statement.h>
#include <odbcabstraction/exceptions.h>
#include <boost/optional.hpp>

using namespace ODBC;
using namespace driver::odbcabstraction;

// Public =========================================================================================
ODBCDescriptor::ODBCDescriptor(ODBCConnection* conn, bool isAppDescriptor) :
  m_owningConnection(conn),
  m_isAppDescriptor(isAppDescriptor),
  m_hasBindingsChanged(true) {

}

void ODBCDescriptor::SetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

void ODBCDescriptor::SetField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

void ODBCDescriptor::GetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

void ODBCDescriptor::GetField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

SQLSMALLINT ODBCDescriptor::getAllocType() const {
  return m_owningConnection != nullptr ? SQL_DESC_ALLOC_USER : SQL_DESC_ALLOC_AUTO;
}

bool ODBCDescriptor::IsAppDescriptor() const {
  return m_isAppDescriptor;
}

bool ODBCDescriptor::HaveBindingsChanged() const {
  return m_hasBindingsChanged;
}

void ODBCDescriptor::RegisterToStatement(ODBCStatement* statement, bool isApd) {
  if (isApd) {
    m_registeredOnStatementsAsApd.push_back(statement);
  } else {
    m_registeredOnStatementsAsArd.push_back(statement);
  }
}

void ODBCDescriptor::DetachFromStatement(ODBCStatement* statement, bool isApd) {
  auto& vectorToUpdate = isApd ? m_registeredOnStatementsAsApd : m_registeredOnStatementsAsArd;
  auto it = std::find(vectorToUpdate.begin(), vectorToUpdate.end(), statement);
  if (it != vectorToUpdate.end()) {
    vectorToUpdate.erase(it);
  }
}

void ODBCDescriptor::ReleaseDescriptor() {
  for (ODBCStatement* stmt : m_registeredOnStatementsAsApd) {
    stmt->RevertAppDescriptor(true);
  }

  for (ODBCStatement* stmt : m_registeredOnStatementsAsArd) {
    stmt->RevertAppDescriptor(false);
  }

  if (m_owningConnection) {
    m_owningConnection->dropDescriptor(this);
  }
}

// Private =========================================================================================
void ODBCDescriptor::SetHeaderFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

void ODBCDescriptor::SetFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

void ODBCDescriptor::GetHeaderFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}

void ODBCDescriptor::GetFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength) {

}
