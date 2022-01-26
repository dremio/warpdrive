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
  void GetAttribute(T attributeValue, SQLPOINTER output, SQLINTEGER outputSize, SQLINTEGER* outputLenPtr) {
    if (output) {
      T* typedOutput = reinterpret_cast<T*>(output);
      *typedOutput = attributeValue;
    }

    if (outputLenPtr) {
      *outputLenPtr = sizeof(T);
    }
  }

  void GetAttributeUTF8(const std::string& attributeValue, SQLPOINTER output, SQLINTEGER outputSize, SQLINTEGER* outputLenPtr) {
    if (output) {
      memcpy(output, attributeValue.c_str(), std::min(static_cast<SQLINTEGER>(attributeValue.size()), outputSize));
    }

    if (outputLenPtr) {
      *outputLenPtr = attributeValue.size() + 1;
    }
    // TODO: Warn if outputSize is too short.
  }

  void GetAttributeSQLWCHAR(const std::string& attributeValue, SQLPOINTER output, SQLINTEGER outputSize, SQLINTEGER* outputLenPtr) {
    size_t result = ConvertToSqlWChar(attributeValue, reinterpret_cast<SQLWCHAR*>(output), outputSize);

    if (outputLenPtr) {
      *outputLenPtr = static_cast<SQLINTEGER>(result);
    }
  }
}
