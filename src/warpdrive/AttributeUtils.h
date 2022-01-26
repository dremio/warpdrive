/* File:			AttributeUtils.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */

#include <sql.h>
#include <sqlext.h>
#include <algorithm>
#include <memory>

#include "EncodingUtils.h"

#pragma once

namespace ODBC {
  template <typename T>
  inline void GetAttribute(T attributeValue, SQLPOINTER output, SQLINTEGER outputSize, SQLINTEGER* outputLenPtr) {
    if (output) {
      T* typedOutput = reinterpret_cast<T*>(output);
      *typedOutput = attributeValue;
    }

    if (outputLenPtr) {
      *outputLenPtr = sizeof(T);
    }
  }

  inline void GetAttributeUTF8(const std::string& attributeValue, SQLPOINTER output, SQLINTEGER outputSize, SQLINTEGER* outputLenPtr) {
    if (output) {
      size_t outputLen = std::min(static_cast<SQLINTEGER>(attributeValue.size()), outputSize);
      memcpy(output, attributeValue.c_str(), outputLen);
      reinterpret_cast<char*>(output)[outputLen] = '\0';
    }

    if (outputLenPtr) {
      *outputLenPtr = attributeValue.size();
    }
    // TODO: Warn if outputSize is too short.
  }

  inline void GetAttributeSQLWCHAR(const std::string& attributeValue, SQLPOINTER output, SQLINTEGER outputSize, SQLINTEGER* outputLenPtr) {
    size_t result = ConvertToSqlWChar(attributeValue, reinterpret_cast<SQLWCHAR*>(output), outputSize);

    if (outputLenPtr) {
      *outputLenPtr = static_cast<SQLINTEGER>(result);
    }
  }
}
