/*--------
 * Module:			connect-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
#include "common.h"

TEST(ConnectTest, ConnectTest)
{
  EXPECT_TRUE(test_connect())<< "SQLDriverConnect failed.";

  /* Clean up */
  std::string *err_msg = nullptr;
  EXPECT_TRUE(test_disconnect(err_msg))<<err_msg;
}

TEST(ConnectTest, SQLConnectTest)
{
  SQLRETURN ret;
  SQLCHAR *dsn = (SQLCHAR *) get_test_dsn();

  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

  SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

  SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);

  ret = SQLConnect(conn, dsn, SQL_NTS, NULL, 0, NULL, 0);
  CHECK_CONN_RESULT(ret, "SQLTransact failed", conn);

  /* Clean up */
  std::string *err_msg = nullptr;
  EXPECT_TRUE(test_disconnect(err_msg))<<err_msg;
}

/*
 * Test that attributes can be set *before* establishing a connection. (We
 * used to have a bug where it got reset when the per-DSN options were read.)
 */
TEST(ConnectTest, SettingAttributeBeforeConnectTest)
{
	SQLRETURN	ret;
	SQLCHAR		str[1024];
	SQLSMALLINT strl;
	SQLCHAR		dsn[1024];
	SQLULEN		value;
	HSTMT		hstmt = SQL_NULL_HSTMT;

	snprintf((char *) dsn, sizeof(dsn), "DSN=%s", get_test_dsn());

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

	SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);

	printf("Testing that autocommit persists SQLDriverConnect...\n");

	/* Disable autocommit */
	SQLSetConnectAttr(conn,
					  SQL_ATTR_AUTOCOMMIT,
					  (SQLPOINTER) SQL_AUTOCOMMIT_OFF,
					  0);

	/* Connect */
	ret = SQLDriverConnect(conn, NULL, dsn, SQL_NTS,
						   str, sizeof(str), &strl,
						   SQL_DRIVER_COMPLETE);

	CHECK_CONN_RESULT(ret, "SQLDriverConnect failed", conn);

	/*** Test that SQLGetConnectAttr says that it's still disabled. ****/
	value = 0;
	ret = SQLGetConnectAttr(conn,
							SQL_ATTR_AUTOCOMMIT,
							&value,
							0, /* BufferLength, ignored for an int attribute */
							NULL);
	CHECK_CONN_RESULT(ret, "SQLGetConnectAttr failed", conn);

	if (value == SQL_AUTOCOMMIT_ON)
		printf("autocommit is on (should've been off!)\n");
	else if (value == SQL_AUTOCOMMIT_OFF)
		printf("autocommit is still off (correct).\n");
	else
		printf("unexpected autocommit value: %lu\n", (unsigned long) value);

	/*
	 * Test that we're really not autocommitting.
	 *
	 * Insert a row, then rollback, and check that the row is not there
	 * anymore.
	 */
	ret = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
	CHECK_STMT_RESULT(ret, "failed to allocate stmt handle", hstmt);

	ret = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO testtab1 VALUES (10000, 'shouldn''t be here!')", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect failed", hstmt);

	ret = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(ret, "SQLFreeStmt failed", hstmt);

	ret = SQLTransact(SQL_NULL_HENV, conn, SQL_ROLLBACK);
	CHECK_CONN_RESULT(ret, "SQLTransact failed", conn);

	ret = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM testtab1 WHERE id = 10000", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect failed", hstmt);
	print_result_meta(hstmt);

	/* Clean up */
  std::string *err_msg = nullptr;
  EXPECT_TRUE(test_disconnect(err_msg))<<err_msg;
}