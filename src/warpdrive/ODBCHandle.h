/* File:			ODBCHandle.h
*
* Comments:		See "notice.txt" for copyright and license information.
*
*/

#pragma once

#include <odbcabstraction/diagnostics.h>
#include "wdodbc.h"

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

  template<typename Function>
  inline SQLRETURN execute(Function function) {
    driver::odbcabstraction::Diagnostics& diagnostics = GetDiagnostics();
    try {
      diagnostics.Clear();
      function();
    } catch (const driver::odbcabstraction::DriverException& ex) {
      diagnostics.AddError(ex);
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

    if (diagnostics.HasError()) {
      return SQL_ERROR;
    } else if (diagnostics.HasWarning()) {
      return SQL_SUCCESS_WITH_INFO;
    }
    return SQL_SUCCESS;
  }

  template <typename Function>
  static SQLRETURN ExecuteWithDiagnostics(SQLHANDLE handle, Function func) {
    if (!handle) {
      return SQL_INVALID_HANDLE;
    }
    return reinterpret_cast<Derived*>(handle)->execute(func);
  }

  static Derived* of(SQLHANDLE handle) {
    return reinterpret_cast<Derived*>(handle);
  }
};
}
