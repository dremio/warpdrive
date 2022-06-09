/*
 * Copyright (C) 2020-2022 Dremio Corporation
 *
 * See "license.txt" for license information.
 */

#include "common.h"

class ResultSetMetadataTest : public ::testing::Test {
  void SetUp() override {
    std::string err_msg;
    connected_ = test_connect(&err_msg);
    ASSERT_TRUE(connected_);

    return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
    CHECK_CONN_RESULT(return_code_, "Failed to allocate stmt handle in SetUp:\n", conn);
  }

  void TearDown() override {
    if (hstmt_ != SQL_NULL_HSTMT) {
      return_code_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
      CHECK_STMT_RESULT(return_code_, "SQLFreeStmt failed in TearDown:\n", hstmt_);
    }
    if (connected_) {
      std::string err_msg;
      ASSERT_TRUE(test_disconnect(&err_msg));
    }
  }

protected:
  bool    connected_;
  int			return_code_;
  HSTMT		hstmt_ = SQL_NULL_HSTMT;
};

TEST_F(ResultSetMetadataTest, TestTimeSupportedTypes){
  std::string query = "SELECT TIMESTAMP '2022-01-01 10:10:23.234', DATE '2022-01-01', TIME '12:30:49', TIME '12:30:49.123'";
  return_code_ = SQLExecDirect(hstmt_, (SQLCHAR *) query.c_str(), query.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect Failed",hstmt_);

  std::vector<std::string> exp_result_meta = {"EXPR$0: TYPE_TIMESTAMP(23) digits: 3, nullable",
                                              "EXPR$1: TYPE_DATE(10) digits: 0, nullable",
                                              "EXPR$2: TYPE_TIME(12) digits: 3, nullable",
                                              "EXPR$3: TYPE_TIME(12) digits: 3, nullable"};
  std::string err_msg;
  auto result_meta = print_result_meta(hstmt_, &err_msg).value();
  for (std::string type:exp_result_meta) {
    EXPECT_TRUE(result_meta.find(type) != std::string::npos)<< type <<" isn't in the actual result set metadata.\nActual resultset metadata:\n"<<result_meta;
  }
}
