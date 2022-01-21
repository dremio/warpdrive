/* File:			ODBCStatement.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCStatement.h"

#include "ODBCConnection.h"
#include <odbcabstraction/statement.h>
#include <odbcabstraction/exceptions.h>
#include <boost/optional.hpp>

using namespace ODBC;
using namespace driver::odbcabstraction;

// Public =========================================================================================
ODBCStatement::ODBCStatement(ODBCConnection& connection, 
  std::shared_ptr<driver::odbcabstraction::Statement> spiStatement) :
  m_connection(connection),
  m_spiStatement(spiStatement),
  m_isPrepared(false) {

}
    
bool ODBCStatement::isPrepared() const {
  return m_isPrepared;
}

void ODBCStatement::Prepare(const std::string& query) {
  boost::optional<std::shared_ptr<ResultSetMetadata> > metadata = m_spiStatement->Prepare(query);

  // TODO: Apply the ResultSetMetaData to the IRD as part of wiring result set metadata.
  m_isPrepared = true;
}

void ODBCStatement::ExecutePrepared() {
  if (!m_isPrepared) {
    throw DriverException("Function sequence error");
  }

  m_spiStatement->ExecutePrepared();

  // TODO: Apply the ResultSetMetaData to the IRD as part of wiring result set metadata.
  m_isPrepared = true;
}

void ODBCStatement::ExecuteDirect(const std::string& query) {
  m_spiStatement->Execute(query);

  // Direct execution wipes out the prepared state.
  m_isPrepared = false;
}

bool ODBCStatement::Fetch(size_t rows) {
  // Check ARD if bindings have changed.
  // If they have, apply binding changes to ResultSet.

  // Then call Move(rows) on the ResultSet.
  return false;
}

void ODBCStatement::closeCursor(bool suppressErrors) {
  if (!suppressErrors && !m_currenResult.get()) {
    throw DriverException("Invalid cursor state");
  }
  
  if (m_currenResult.get()) {
    // TODO: call close() on the current ResultSet, but preserve bindings.
  }
}

void ODBCStatement::releaseStatement() {
  closeCursor(true);
  m_connection.dropStatement(this);
}
