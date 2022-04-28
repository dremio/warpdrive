/* File:			ODBCEnvironment.h
*
* Comments:		See "notice.txt" for copyright and license information.
*
*/
#pragma once

#include "wdodbc.h"
#include <memory>

struct free_deleter {
  void operator()(void* x) {
    std::free(x);
  }
};

struct c_ptr : public std::unique_ptr<char, free_deleter> {
  inline void reallocate(size_t newSize) {
    void* current = release();
    reset(static_cast<char*>(std::realloc(current, newSize)));
  }
};


