#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

#define CHECK_STMT_RESULT(rc, msg, hstmt)	\
	if (!SQL_SUCCEEDED(rc)) \
	{ \
		print_diag(msg, SQL_HANDLE_STMT, hstmt);	\
		exit(1);									\
    }

#define CHECK_CONN_RESULT(rc, msg, hconn)	\
	if (!SQL_SUCCEEDED(rc)) \
	{ \
		print_diag(msg, SQL_HANDLE_DBC, hconn);	\
		exit(1);									\
    }

#define CHECK_STMT_RESULT_2(rc, msg, hstmt)	\
	ASSERT_TRUE(SQL_SUCCEEDED(rc)) << get_diag_str(msg, SQL_HANDLE_STMT, hstmt);

#define CHECK_CONN_RESULT_2(rc, msg, hconn)	\
	ASSERT_TRUE(SQL_SUCCEEDED(rc)) << get_diag_str(msg, SQL_HANDLE_DBC, hconn);

extern void print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle);
extern std::string get_diag_str(char *extra_message, SQLSMALLINT handle_type, SQLHANDLE handle);
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
