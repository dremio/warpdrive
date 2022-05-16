#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>

#ifdef WIN32
#include <windows.h>
#else
// #include "config.h"
#endif

#include <sql.h>
#include <sqlext.h>
#include <gtest/gtest.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

extern SQLHENV env;
extern SQLHDBC conn;


#define CHECK_CONN_RESULT(rc, msg, hconn)	\
	if (!SQL_SUCCEEDED(rc)) \
	{ \
		print_diag(msg, SQL_HANDLE_DBC, hconn);	\
		exit(1);									\
    }


#define CHECK_STMT_RESULT(SQLRETURN rc, char* msg, SQLHANDLE hstmt) \
	ASSERT_TRUE(SQL_SUCCEEDED(rc)) << format_diagnostic(msg, SQL_HANDLE_STMT, hstmt);

extern std::string format_diagnostic(const std::string msg&, SQLSMALLINT htype, SQLHANDLE handle);
extern std::string get_diagnostic(SQLHANDLE handle, SQLSMALLINT htype);
extern void print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle);
extern void print_diag(const std::string& , SQLSMALLINT htype, SQLHANDLE handle);
extern const char *get_test_dsn(void);
extern int  IsAnsi(void);
extern void test_connect_ext(char *extraparams);
extern void test_connect(void);
extern void test_disconnect(void);
extern void print_result_meta_series(HSTMT hstmt,
									 SQLSMALLINT *colids,
									 SQLSMALLINT numcols);
extern void print_result_series(HSTMT hstmt,
								SQLSMALLINT *colids,
								SQLSMALLINT numcols,
								SQLINTEGER rowcount);
extern void print_result_meta(HSTMT hstmt);
extern void print_result(HSTMT hstmt);
extern const char *datatype_str(SQLSMALLINT datatype);
extern const char *nullable_str(SQLSMALLINT nullable);
