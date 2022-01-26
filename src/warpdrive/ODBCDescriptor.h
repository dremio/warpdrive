/* File:			ODBCDescriptor.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */
#pragma once

#include <sql.h>
#include <memory>
#include <vector>

namespace ODBC {
  class ODBCConnection;
  class ODBCStatement;
}

namespace ODBC
{
  class ODBCDescriptor {
    public:
      /**
       * @brief Construct a new ODBCDescriptor object. Link the descriptor to a connection,
       * if applicable. A nullptr should be supplied for conn if the descriptor should not be linked.
       */
      ODBCDescriptor(ODBCConnection* conn, bool isAppDescriptor);

      void SetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void SetField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void GetHeaderField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void GetField(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      SQLSMALLINT getAllocType() const;
      bool IsAppDescriptor() const;
      bool HaveBindingsChanged() const;
      void RegisterToStatement(ODBCStatement* statement, bool isApd);
      void DetachFromStatement(ODBCStatement* statement, bool isApd);
      void ReleaseDescriptor();

    private:
      void SetHeaderFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void SetFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);      
      void GetHeaderFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);
      void GetFieldInternal(SQLSMALLINT fieldIdentifier, SQLPOINTER value, SQLINTEGER bufferLength);

      ODBCConnection* m_owningConnection;
      std::vector<ODBCStatement*> m_registeredOnStatementsAsApd;
      std::vector<ODBCStatement*> m_registeredOnStatementsAsArd;
      bool m_isAppDescriptor;
      bool m_hasBindingsChanged;
  };
}
