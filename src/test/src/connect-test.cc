/*--------
 * Module:			connect-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
#include "common.h"

bool
test_SQLConnect()
{
	SQLRETURN ret;
	SQLCHAR *dsn = (SQLCHAR *) get_test_dsn();

	SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

	SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

	SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);

	printf("Connecting with SQLConnect...");

	ret = SQLConnect(conn, dsn, SQL_NTS, NULL, 0, NULL, 0);
	if (SQL_SUCCEEDED(ret)) {
		printf("connected\n");
	} else {
		print_diag("SQLConnect failed.", SQL_HANDLE_DBC, conn);
	}

	return SQL_SUCCEEDED(ret);
}

/*
 * Test that attributes can be set *before* establishing a connection. (We
 * used to have a bug where it got reset when the per-DSN options were read.)
 */
bool
test_setting_attribute_before_connect()
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
	if (SQL_SUCCEEDED(ret)) {
		printf("connected\n");
	} else {
		print_diag("SQLDriverConnect failed.", SQL_HANDLE_DBC, conn);
		return SQL_SUCCEEDED(ret);
	}

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
	if (!SQL_SUCCEEDED(ret))
	{
		print_diag("failed to allocate stmt handle", SQL_HANDLE_DBC, conn);
		return SQL_SUCCEEDED(ret);
	}

	ret = SQLExecDirect(hstmt, (SQLCHAR *) "INSERT INTO testtab1 VALUES (10000, 'shouldn''t be here!')", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect failed", hstmt);

	ret = SQLFreeStmt(hstmt, SQL_CLOSE);
	CHECK_STMT_RESULT(ret, "SQLFreeStmt failed", hstmt);

	ret = SQLTransact(SQL_NULL_HENV, conn, SQL_ROLLBACK);
	CHECK_CONN_RESULT(ret, "SQLTransact failed", conn);

	ret = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT * FROM testtab1 WHERE id = 10000", SQL_NTS);
	CHECK_STMT_RESULT(ret, "SQLExecDirect failed", hstmt);
	print_result_meta(hstmt);

    return SQL_SUCCEEDED(ret);
}

TEST(ConnectTest, ConnectTest)
{
  EXPECT_TRUE(test_connect())<< "SQLDriverConnect failed.";

  /* Clean up */
  std::string *err_msg = nullptr;
  EXPECT_TRUE(test_disconnect(err_msg))<<err_msg;
}

TEST(ConnectTest, SQLConnectTest)
{
  EXPECT_TRUE(test_SQLConnect())<< "SQLDriverConnect failed.";

  /* Clean up */
  std::string *err_msg = nullptr;
  EXPECT_TRUE(test_disconnect(err_msg))<<err_msg;
}

TEST(ConnectTest, SettingAttributeBeforeConnectTest)
{
  EXPECT_TRUE(test_setting_attribute_before_connect())<< "SQLDriverConnect failed.";

	/* Clean up */
  std::string *err_msg = nullptr;
  EXPECT_TRUE(test_disconnect(err_msg))<<err_msg;
}