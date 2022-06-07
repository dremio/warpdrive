///
/// Copyright (C) 2020-2022 Dremio Corporation
///
/// See “license.txt” for license information.
///
/// Module:			c_ptr.h
///
/// Description:		Helper class for managing memory allocated using malloc and realloc.
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


