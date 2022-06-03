/*--------
 * Module:			statement-functions-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */

#include <gtest/gtest.h>
#include "common.h"

class SQLStatementFunctionsTest : public ::testing::Test {
  void SetUp() override {
    connected = test_connect();
    ASSERT_TRUE(connected);

    return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
    CHECK_CONN_RESULT(return_code_, "Failed to allocate stmt handle in SetUp:\n", conn);
  }

  void TearDown() override {
    if (handle_stmt_ != SQL_NULL_HSTMT) {
      return_code_ = SQLFreeStmt(handle_stmt_, SQL_CLOSE);
      CHECK_STMT_RESULT(return_code_, "SQLFreeStmt failed in TearDown:\n", handle_stmt_);
    }
    if (connected) {
      std::string err_msg;
      ASSERT_TRUE(test_disconnect(&err_msg));
    }
  }

protected:
  bool connected{false};
  int return_code_{};
  HSTMT handle_stmt_ = SQL_NULL_HSTMT;
};

TEST_F(SQLStatementFunctionsTest, TestNormalFlow){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer";
  return_code_ = SQLPrepare(handle_stmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", handle_stmt_);

  return_code_ = SQLExecute(handle_stmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", handle_stmt_);


}