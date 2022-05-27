/*--------
 * Module:			select-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
#include "common.h"

TEST(WarpDriveTest, SelectTest)
{
	int rc;
	HSTMT hstmt = SQL_NULL_HSTMT;
  std::string err_msg;
  EXPECT_TRUE(test_connect(&err_msg))<< "SQLDriverConnect failed.";

	rc = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt);
  EXPECT_TRUE(SQL_SUCCEEDED(rc)) << "failed to allocate stmt handle";

	rc = SQLExecDirect(hstmt, (SQLCHAR *) "SELECT 1 UNION ALL SELECT 2", SQL_NTS);

	EXPECT_TRUE(SQL_SUCCEEDED(rc))<<"SQLExecDirect failed";
  auto exp_result1 = "1\n2\n";
  EXPECT_EQ(exp_result1, get_result(hstmt, &err_msg));

	rc = SQLFreeStmt(hstmt, SQL_CLOSE);
  EXPECT_TRUE(SQL_SUCCEEDED(rc)) << "SQLFreeStmt failed";

	/* Result set with 1600 cols */
  std::string exp_result2 = "1";
  std::string sql = "SELECT 1";

	for (int i = 2; i <= 1600; i++)
	{
    sql.append("," + std::to_string(+i));
    exp_result2.append("\t" + std::to_string(+i));
	}
  exp_result2.append("\n");

	rc = SQLExecDirect(hstmt, (SQLCHAR *) sql.c_str(), SQL_NTS);
  EXPECT_TRUE(SQL_SUCCEEDED(rc)) << "SQLExecDirect failed";
  EXPECT_EQ(exp_result2, get_result(hstmt, &err_msg));

  EXPECT_TRUE(test_disconnect(&err_msg));
}
