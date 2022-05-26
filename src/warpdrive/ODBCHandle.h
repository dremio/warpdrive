/* File:			ODBCHandle.h
*
* Comments:		See "notice.txt" for copyright and license information.
*
*/

#pragma once

#include <odbcabstraction/diagnostics.h>
#include "wdodbc.h"

#include <functional>

/**
 * @brief An abstraction over a generic ODBC handle.
 */
namespace ODBC {

template <typename Derived>
class ODBCHandle {

public:
  inline driver::odbcabstraction::Diagnostics& GetDiagnostics() {
    return static_cast<Derived*>(this)->GetDiagnostics_Impl();
  }

  inline driver::odbcabstraction::Diagnostics& GetDiagnostics_Impl() {
    throw std::runtime_error("Illegal state -- diagnostics requested on invalid handle");
  }

  template <typename Function>
  inline SQLRETURN execute(SQLRETURN rc, Function function) {
    try {
      GetDiagnostics().Clear();
      rc = function();
    } catch (const driver::odbcabstraction::DriverException& ex) {
      GetDiagnostics().AddError(ex);
    } catch (const std::bad_alloc& ex) {
      GetDiagnostics().AddError(
        driver::odbcabstraction::DriverException("A memory allocation error occurred.", "HY001"));
    } catch (const std::exception& ex) {
      GetDiagnostics().AddError(
        driver::odbcabstraction::DriverException(ex.what()));
    } catch (...) {
      GetDiagnostics().AddError(
        driver::odbcabstraction::DriverException("An unknown error occurred."));
    }

    if (GetDiagnostics().HasError()) {
      return SQL_ERROR;
    } if (SQL_SUCCEEDED(rc) && GetDiagnostics().HasWarning()) {
      return SQL_SUCCESS_WITH_INFO;
    }
    return rc;
  }

  template <typename Function>
  static inline SQLRETURN ExecuteWithDiagnostics(SQLHANDLE handle, SQLRETURN rc, Function func) {
    if (!handle) {
      return SQL_INVALID_HANDLE;
    }
    return reinterpret_cast<Derived*>(handle)->execute(rc, func);
  }

  static Derived* of(SQLHANDLE handle) {
    return reinterpret_cast<Derived*>(handle);
  }
};
}
