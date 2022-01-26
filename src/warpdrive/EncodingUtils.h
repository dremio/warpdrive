/* File:			EncodingUtils.h
 *
 * Comments:		See "notice.txt" for copyright and license information.
 *
 */

#include <sql.h>
#include <sqlext.h>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <memory>
#include <string>

#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

namespace ODBC {
  #ifdef WITH_IODBC
  typedef char32_t SqlWChar;
  typedef std::u32string SqlWString;
  typedef std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> CharToWStrConverter;
  #else
  typedef char16_t SqlWChar;
  typedef std::u16string SqlWString;
  typedef std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> CharToWStrConverter;
  #endif

  // Return the number of bytes required for the conversion.
  inline size_t ConvertToSqlWChar(const std::string& str, SQLWCHAR* buffer, SQLLEN bufferSizeInBytes) {
    SqlWString wstr = CharToWStrConverter().from_bytes(str);
    SQLLEN valueLengthInBytes = wstr.size() * sizeof(SqlWChar);

    if (buffer) {
      memcpy(buffer, wstr.data(), std::min(static_cast<SQLLEN>(wstr.size() * sizeof(SqlWChar)), bufferSizeInBytes));

      // Write a NUL terminator
      if (bufferSizeInBytes > valueLengthInBytes + sizeof(SqlWChar)) {
        reinterpret_cast<SqlWChar*>(buffer)[wstr.size()] = '\0';
      } else {
        SQLLEN numCharsWritten = bufferSizeInBytes / sizeof(SqlWChar);
        // If we failed to even write one char, the buffer is too small to hold a NUL-terminator.
        if (numCharsWritten > 0) {
          reinterpret_cast<SqlWChar*>(buffer)[numCharsWritten-1] = '\0';
        }
      }
    }
    return valueLengthInBytes + sizeof(SqlWChar);
  }

  // Return the number of bytes required for the conversion.
  inline size_t ConvertToSqlWChar(const SQLCHAR* str, SQLLEN numChars, SQLWCHAR* buffer, SQLLEN bufferSizeInBytes) {
    if (numChars != SQL_NTS) {
      return ConvertToSqlWChar(std::string(reinterpret_cast<const char*>(str), numChars), buffer, bufferSizeInBytes);
    }
    return ConvertToSqlWChar(std::string(reinterpret_cast<const char*>(str)), buffer, bufferSizeInBytes);
  }
    
  // Return the number of bytes required for the conversion.
  inline size_t ConvertToSqlChar(const SQLWCHAR* str, SQLLEN numChars, SQLCHAR* buffer, SQLLEN bufferSizeInBytes) {
    SqlWString wstr;
    if (numChars != SQL_NTS) {
      wstr = SqlWString(reinterpret_cast<const SqlWChar*>(str), numChars);
    } else {
      wstr = SqlWString(reinterpret_cast<const SqlWChar*>(str));
    }

    std::string converted = CharToWStrConverter().to_bytes(wstr.c_str());
    if (buffer) {
      memcpy(buffer, converted.c_str(), std::min(bufferSizeInBytes, static_cast<SQLLEN>(converted.size())));
      if (bufferSizeInBytes > converted.size() + 1) {
        buffer[bufferSizeInBytes] = '\0';   
      } else {
        buffer[converted.size()] = '0';
      }
    }
    return converted.size() + 1;
  }
}
