/* File:			ODBCStatement.cc
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#include "ODBCStatement.h"

#include "ODBCConnection.h"
#include "ODBCDescriptor.h"
#include "sql.h"
#include <odbcabstraction/statement.h>
#include <odbcabstraction/exceptions.h>
#include <boost/optional.hpp>

using namespace ODBC;
using namespace driver::odbcabstraction;

namespace {
  void DescriptorToHandle(SQLPOINTER output, ODBCDescriptor* descriptor, SQLLEN* lenPtr) {
    if (output) {
      SQLHANDLE* outputHandle = static_cast<SQLHANDLE*>(output);
      *outputHandle = reinterpret_cast<SQLHANDLE>(descriptor);
      *lenPtr = sizeof(SQLHANDLE);
    }
  }
}

// Public =========================================================================================
ODBCStatement::ODBCStatement(ODBCConnection& connection, 
  std::shared_ptr<driver::odbcabstraction::Statement> spiStatement) :
  m_connection(connection),
  m_spiStatement(spiStatement),
  m_builtInArd(std::make_shared<ODBCDescriptor>(nullptr, true, true)),
  m_builtInApd(std::make_shared<ODBCDescriptor>(nullptr, true, true)),
  m_ipd(std::make_shared<ODBCDescriptor>(nullptr, false, true)),
  m_ird(std::make_shared<ODBCDescriptor>(nullptr, false, false)),
  m_currentArd(m_builtInApd.get()),
  m_currentApd(m_builtInApd.get()),
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

void ODBCStatement::GetAttribute(SQLINTEGER statementAttribute, SQLPOINTER output, SQLLEN bufferSize, SQLLEN* strLenPtr) {
  switch (statementAttribute) {
    case SQL_ATTR_APP_PARAM_DESC:
      DescriptorToHandle(output, m_currentApd, strLenPtr);
      break;
    case SQL_ATTR_APP_ROW_DESC:
      DescriptorToHandle(output, m_currentArd, strLenPtr);
      break;
    case SQL_ATTR_IMP_PARAM_DESC:
      DescriptorToHandle(output, m_ipd.get(), strLenPtr);
      break;
    case SQL_ATTR_IMP_ROW_DESC:
      DescriptorToHandle(output, m_ird.get(), strLenPtr);
      break;
  }
}

void ODBCStatement::SetAttribute(SQLINTEGER statementAttribute, SQLPOINTER value, SQLLEN bufferSize) {
  switch (statementAttribute) {
    case SQL_ATTR_APP_PARAM_DESC: {
        ODBCDescriptor* desc = static_cast<ODBCDescriptor*>(value);
        if (m_currentApd != desc) {
          if (m_currentApd != m_builtInApd.get()) {
            m_currentApd->DetachFromStatement(this, true);
          }
          m_currentApd = desc;
          if (m_currentApd != m_builtInApd.get()) {
            desc->RegisterToStatement(this, true);
          }
        }
        break;
      }
    case SQL_ATTR_APP_ROW_DESC: {
        ODBCDescriptor* desc = static_cast<ODBCDescriptor*>(value);
        if (m_currentArd != desc) {
          if (m_currentArd != m_builtInArd.get()) {
            m_currentArd->DetachFromStatement(this, false);
          }
          m_currentArd = desc;
          if (m_currentArd != m_builtInArd.get()) {
            desc->RegisterToStatement(this, false);
          }
        }
        break;
      }
    case SQL_ATTR_IMP_PARAM_DESC:
      throw DriverException("Cannot assign impl descriptor.");
    case SQL_ATTR_IMP_ROW_DESC:
      throw DriverException("Cannot assign impl descriptor.");
  }
}

void ODBCStatement::RevertAppDescriptor(bool isApd) {
  if (isApd) {
    m_currentApd = m_builtInApd.get();
  } else {
    m_currentArd = m_builtInArd.get();
  }
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
