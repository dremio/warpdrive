/* File:			AttributeUtils.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */

#pragma once

#include <sql.h>
#include <sqlext.h>
#include <algorithm>
#include <memory>
#include <cstring>
#include <odbcabstraction/exceptions.h>
#include <odbcabstraction/diagnostics.h>

#include "EncodingUtils.h"

namespace ODBC {
template <typename T, typename O>
inline void GetAttribute(T attributeValue, SQLPOINTER output, O outputSize,
                         O *outputLenPtr) {
  if (output) {
    T *typedOutput = reinterpret_cast<T *>(output);
    *typedOutput = attributeValue;
  }

  if (outputLenPtr) {
    *outputLenPtr = sizeof(T);
  }
}

template <typename O>
inline SQLRETURN GetAttributeUTF8(const std::string &attributeValue,
                             SQLPOINTER output, O outputSize, O *outputLenPtr) {
  if (output) {
    size_t outputLen =
        WD_MIN(static_cast<O>(attributeValue.size()), outputSize);
    memcpy(output, attributeValue.c_str(), outputLen);
    reinterpret_cast<char *>(output)[outputLen] = '\0';
  }

  if (outputLenPtr) {
    *outputLenPtr = static_cast<O>(attributeValue.size());
  }

  if (outputSize < attributeValue.size() + 1) {
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
}

template <typename O>
inline SQLRETURN GetAttributeUTF8(const std::string &attributeValue,
                                  SQLPOINTER output, O outputSize, O *outputLenPtr, driver::odbcabstraction::Diagnostics& diagnostics) {
  SQLRETURN result = GetAttributeUTF8(attributeValue, output, outputSize, outputLenPtr);
  if (SQL_SUCCESS_WITH_INFO == result) {
    diagnostics.AddTruncationWarning();
  }
  return result;
}

template <typename O>
inline SQLRETURN GetAttributeSQLWCHAR(const std::string &attributeValue,
                                 SQLPOINTER output, O outputSize,
                                 O *outputLenPtr) {
  size_t result = ConvertToSqlWChar(
      attributeValue, reinterpret_cast<SQLWCHAR *>(output), outputSize);

  if (outputLenPtr) {
    *outputLenPtr = static_cast<O>(result);
  }

  if (outputSize < result + sizeof(SQLWCHAR)) {
    return SQL_SUCCESS_WITH_INFO;
  }
  return SQL_SUCCESS;
}

template <typename O>
inline SQLRETURN GetAttributeSQLWCHAR(const std::string &attributeValue,
                                      SQLPOINTER output, O outputSize,
                                      O *outputLenPtr, driver::odbcabstraction::Diagnostics& diagnostics) {
  SQLRETURN result = GetAttributeSQLWCHAR(attributeValue, output, outputSize, outputLenPtr);
  if (SQL_SUCCESS_WITH_INFO == result) {
    diagnostics.AddTruncationWarning();
  }
  return result;
}

template <typename O>
inline SQLRETURN
GetStringAttribute(bool isUnicode, const std::string &attributeValue,
                   SQLPOINTER output, O outputSize, O *outputLenPtr, driver::odbcabstraction::Diagnostics& diagnostics) {
  SQLRETURN result = SQL_SUCCESS;
  if (isUnicode) {
    result = GetAttributeSQLWCHAR(attributeValue, output, outputSize, outputLenPtr);
  } else {
    result = GetAttributeUTF8(attributeValue, output, outputSize, outputLenPtr);
  }

  if (SQL_SUCCESS_WITH_INFO == result) {
    diagnostics.AddTruncationWarning();
  }
  return result;
}

template <typename T>
inline void SetAttribute(SQLPOINTER newValue, T &attributeToWrite) {
  SQLLEN valueAsLen = reinterpret_cast<SQLLEN>(newValue);
  attributeToWrite = static_cast<T>(valueAsLen);
}

template <typename T>
inline void SetPointerAttribute(SQLPOINTER newValue, T &attributeToWrite) {
  attributeToWrite = static_cast<T>(newValue);
}

inline void SetAttributeUTF8(SQLPOINTER newValue, SQLINTEGER inputLength,
                             std::string &attributeToWrite) {
  const char *newValueAsChar = static_cast<const char *>(newValue);
  attributeToWrite.assign(newValueAsChar, inputLength == SQL_NTS
                                              ? strlen(newValueAsChar)
                                              : inputLength);
}

inline void SetAttributeSQLWCHAR(SQLPOINTER newValue,
                                 SQLINTEGER inputLengthInBytes,
                                 std::string &attributeToWrite) {
  SqlWString wstr;
  if (inputLengthInBytes == SQL_NTS) {
    wstr.assign(reinterpret_cast<SqlWChar *>(newValue));
  } else {
    wstr.assign(reinterpret_cast<SqlWChar *>(newValue),
                inputLengthInBytes / sizeof(SqlWChar));
  }
  attributeToWrite = CharToWStrConverter().to_bytes(wstr.c_str());
}

template <typename T>
void CheckIfAttributeIsSetToOnlyValidValue(SQLPOINTER value, T allowed_value) {
  if (static_cast<T>(reinterpret_cast<SQLULEN>(value)) != allowed_value) {
    throw driver::odbcabstraction::DriverException("Optional feature not implemented", "HYC00");
  }
}
}
