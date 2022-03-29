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

#include "EncodingUtils.h"

namespace ODBC {
  template <typename T, typename O>
  inline void GetAttribute(T attributeValue, SQLPOINTER output, O outputSize, O* outputLenPtr) {
    if (output) {
      T* typedOutput = reinterpret_cast<T*>(output);
      *typedOutput = attributeValue;
    }

    if (outputLenPtr) {
      *outputLenPtr = sizeof(T);
    }
  }

  template <typename O>
  inline void GetAttributeUTF8(const std::string& attributeValue, SQLPOINTER output, O outputSize, O* outputLenPtr) {
    if (output) {
      size_t outputLen = std::min(static_cast<O>(attributeValue.size()), outputSize);
      memcpy(output, attributeValue.c_str(), outputLen);
      reinterpret_cast<char*>(output)[outputLen] = '\0';
    }

    if (outputLenPtr) {
      *outputLenPtr = static_cast<O>(attributeValue.size());
    }
    // TODO: Warn if outputSize is too short.
  }

  template <typename O>
  inline void GetAttributeSQLWCHAR(const std::string& attributeValue, SQLPOINTER output, O outputSize, O* outputLenPtr) {
    size_t result = ConvertToSqlWChar(attributeValue, reinterpret_cast<SQLWCHAR*>(output), outputSize);

    if (outputLenPtr) {
      *outputLenPtr = static_cast<O>(result);
    }
  }

  template <typename O>
  inline void GetStringAttribute(bool isUnicode, const std::string& attributeValue, SQLPOINTER output, O outputSize, O* outputLenPtr) {
    if (isUnicode) {
      GetAttributeSQLWCHAR(attributeValue, output, outputSize, outputLenPtr);
    } else {
      GetAttributeUTF8(attributeValue, output, outputSize, outputLenPtr);
    }
  }

  template<typename T>
  inline void SetAttribute(SQLPOINTER newValue, T& attributeToWrite) {
    SQLLEN valueAsLen = reinterpret_cast<SQLLEN>(newValue);
    attributeToWrite = static_cast<T>(valueAsLen);
  }

  template<typename T>
  inline void SetPointerAttribute(SQLPOINTER newValue, T& attributeToWrite) {
    attributeToWrite = static_cast<T>(newValue);
  }

  inline void SetAttributeUTF8(SQLPOINTER newValue, SQLINTEGER inputLength, std::string& attributeToWrite) {
    const char* newValueAsChar = static_cast<const char*>(newValue);
    attributeToWrite.assign(newValueAsChar, inputLength == SQL_NTS ? strlen(newValueAsChar) : inputLength);
  }
}
