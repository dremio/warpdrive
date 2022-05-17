/*--------
 * Module:			common.h
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
// #include "config.h"
#endif

#include <gtest/gtest.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

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

extern void print_diag(char *msg, SQLSMALLINT htype, SQLHANDLE handle);
extern const char *get_test_dsn(void);
extern int  IsAnsi(void);
extern bool test_connect_ext(char *extraparams);
extern bool test_connect();
extern bool test_disconnect(std::string *err_msg);
extern void print_result_meta_series(HSTMT hstmt,
									 SQLSMALLINT *colids,
									 SQLSMALLINT numcols);
extern std::string get_result_series(HSTMT hstmt,
                                     SQLSMALLINT *colids,
                                     SQLSMALLINT numcols,
                                     SQLINTEGER rowcount);
extern void print_result_meta(HSTMT hstmt);
extern std::string get_result(HSTMT hstmt);
extern const char *datatype_str(SQLSMALLINT datatype);
extern const char *nullable_str(SQLSMALLINT nullable);
