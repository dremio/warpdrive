/*--------
 * Module:			common.h
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#ifdef WIN32
#include <windows.h>
#else
// #include "config.h"
#endif

#include <gtest/gtest.h>
#include <sql.h>
#include <sqlext.h>
#include <gtest/gtest.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

extern SQLHENV env;
extern SQLHDBC conn;


#define CHECK_CONN_RESULT(rc, msg, hconn) ASSERT_TRUE(SQL_SUCCEEDED(rc)) << format_diagnostic(msg, SQL_HANDLE_DBC, hconn)

#define CHECK_STMT_RESULT(rc, msg, hstmt) ASSERT_TRUE(SQL_SUCCEEDED(rc)) << format_diagnostic(msg, SQL_HANDLE_STMT, hstmt)

extern std::string format_diagnostic(const std::string& msg, SQLSMALLINT htype, SQLHANDLE handle);
extern std::string get_diagnostic(SQLHANDLE handle, SQLSMALLINT htype);
extern void print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle);
extern void print_diag(const std::string& , SQLSMALLINT htype, SQLHANDLE handle);
extern const char *get_test_dsn(void);
extern int  IsAnsi(void);
extern bool test_connect_ext(char *extraparams, std::string *err_msg);
extern bool test_connect(std::string *err_msg);
extern bool test_disconnect(std::string *err_msg);
extern std::string print_result_meta_series(HSTMT hstmt,
									 SQLSMALLINT *colids,
									 SQLSMALLINT numcols,
                   std::string *err_msg);
extern std::string get_result_series(HSTMT hstmt,
                                     SQLSMALLINT *colids,
                                     SQLSMALLINT numcols,
                                     SQLINTEGER rowcount,
                                     std::string *err_msg);
extern std::string print_result_meta(HSTMT hstmt, std::string *err_msg);
extern std::string get_result(HSTMT hstmt, std::string *err_msg);
extern const char *datatype_str(SQLSMALLINT datatype);
extern const char *nullable_str(SQLSMALLINT nullable);
