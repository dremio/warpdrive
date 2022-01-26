/* File:			TypeUtilities.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */

#include "sql.h"
#include <sql.h>
#include <sqlext.h>

#pragma once

namespace ODBC {
  inline SQLSMALLINT GetSqlTypeForODBCVersion(SQLSMALLINT type, bool isOdbc2x) {
    switch (type) {
      case SQL_DATE:
      case SQL_TYPE_DATE:
        return isOdbc2x ? SQL_DATE : SQL_TYPE_DATE;
      
      case SQL_TIME:
      case SQL_TYPE_TIME:
        return isOdbc2x ? SQL_TIME : SQL_TYPE_TIME;
    
      case SQL_TIMESTAMP:
      case SQL_TYPE_TIMESTAMP:
        return isOdbc2x ? SQL_TIMESTAMP : SQL_TYPE_TIMESTAMP;
    
      default:
        return type;
    }
  }
}
