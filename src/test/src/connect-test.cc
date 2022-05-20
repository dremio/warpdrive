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
  std::string err_msg;
  EXPECT_TRUE(test_disconnect(&err_msg))<<err_msg;
}

TEST(ConnectTest, SQLConnectTest)
{
  SQLRETURN ret;
  SQLCHAR *dsn = (SQLCHAR *) get_test_dsn();

  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

  SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

  SQLAllocHandle(SQL_HANDLE_DBC, env, &conn);

  ret = SQLConnect(conn, dsn, SQL_NTS, NULL, 0, NULL, 0);
  CHECK_CONN_RESULT(ret, "SQLConnect failed", conn);

  /* Clean up */
  std::string err_msg;
  EXPECT_TRUE(test_disconnect(&err_msg))<<err_msg;
}
