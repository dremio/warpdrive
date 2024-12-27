/*
 * Copyright (C) 2020-2022 Dremio Corporation
 *
 * See "license.txt" for license information.
 */

#include <gtest/gtest.h>
#include "common.h"

class SQLStatementFunctionsTest : public ::testing::Test {
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

TEST_F(SQLStatementFunctionsTest, TestExecDirectNormalFlow){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 50 AND 54 LIMIT 5;";
  return_code_= SQLExecDirect(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", hstmt_);

  std::string err_msg;
  std::string exp_result = "50\tCustomer#000000050\t9SzDYlkzxByyJ1QeTI o\t6\t16-658-112-3221\t4266.130000\tMACHINERY \tts. furiously ironic accounts cajole fu\n"
                           "51\tCustomer#000000051\tuR,wEaiTvo4\t12\t22-344-885-4251\t855.870000\tFURNITURE \teposits. furiously regular requests int\n"
                           "52\tCustomer#000000052\t7 QOqGqqSy9jfV51BC71jcHJSD0\t11\t21-186-284-5998\t5630.280000\tHOUSEHOLD \tic platelets use evenly even accounts. \n"
                           "53\tCustomer#000000053\tHnaxHzTfFTZs8MuCpJyTbZ47Cm4wFOOgib\t15\t25-168-852-5363\t4113.640000\tHOUSEHOLD \tar accounts are. even foxes are blithel\n"
                           "54\tCustomer#000000054\t,k4vf 5vECGWFy,hosTE,\t4\t14-776-370-4745\t868.900000\tAUTOMOBILE\tsual, silent accounts. furiously expres\n";
  auto result = get_result(hstmt_, &err_msg);
  EXPECT_EQ(exp_result, result.value());
}

TEST_F(SQLStatementFunctionsTest, TestExecDirectPrepareBeforeDirectExec){
  std::string sqlQueryPrepared = "SELECT * FROM postgres.tpch.customer LIMIT 10";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQueryPrepared.c_str(), sqlQueryPrepared.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 50 AND 54 LIMIT 5;";
  return_code_= SQLExecDirect(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", hstmt_);

  std::string err_msg;
  std::string exp_result = "50\tCustomer#000000050\t9SzDYlkzxByyJ1QeTI o\t6\t16-658-112-3221\t4266.130000\tMACHINERY \tts. furiously ironic accounts cajole fu\n"
                             "51\tCustomer#000000051\tuR,wEaiTvo4\t12\t22-344-885-4251\t855.870000\tFURNITURE \teposits. furiously regular requests int\n"
                             "52\tCustomer#000000052\t7 QOqGqqSy9jfV51BC71jcHJSD0\t11\t21-186-284-5998\t5630.280000\tHOUSEHOLD \tic platelets use evenly even accounts. \n"
                             "53\tCustomer#000000053\tHnaxHzTfFTZs8MuCpJyTbZ47Cm4wFOOgib\t15\t25-168-852-5363\t4113.640000\tHOUSEHOLD \tar accounts are. even foxes are blithel\n"
                             "54\tCustomer#000000054\t,k4vf 5vECGWFy,hosTE,\t4\t14-776-370-4745\t868.900000\tAUTOMOBILE\tsual, silent accounts. furiously expres\n";
  auto result = get_result(hstmt_, &err_msg);
  EXPECT_EQ(exp_result, result.value());
}

TEST_F(SQLStatementFunctionsTest, TestExecDirectCloseCursor){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer LIMIT 5;";
  return_code_= SQLExecDirect(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);
}

TEST_F(SQLStatementFunctionsTest, TestExecDirectEarlyClose){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer LIMIT 5;";
  return_code_= SQLExecDirect(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", hstmt_);

  return_code_ = SQLFetch(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLFetch failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);
}

TEST_F(SQLStatementFunctionsTest, TestNormalFlow){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 50 AND 54 LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  std::string err_msg;
  std::string exp_result = "50\tCustomer#000000050\t9SzDYlkzxByyJ1QeTI o\t6\t16-658-112-3221\t4266.130000\tMACHINERY \tts. furiously ironic accounts cajole fu\n"
                             "51\tCustomer#000000051\tuR,wEaiTvo4\t12\t22-344-885-4251\t855.870000\tFURNITURE \teposits. furiously regular requests int\n"
                             "52\tCustomer#000000052\t7 QOqGqqSy9jfV51BC71jcHJSD0\t11\t21-186-284-5998\t5630.280000\tHOUSEHOLD \tic platelets use evenly even accounts. \n"
                             "53\tCustomer#000000053\tHnaxHzTfFTZs8MuCpJyTbZ47Cm4wFOOgib\t15\t25-168-852-5363\t4113.640000\tHOUSEHOLD \tar accounts are. even foxes are blithel\n"
                             "54\tCustomer#000000054\t,k4vf 5vECGWFy,hosTE,\t4\t14-776-370-4745\t868.900000\tAUTOMOBILE\tsual, silent accounts. furiously expres\n";
  auto result = get_result(hstmt_, &err_msg);
  EXPECT_EQ(exp_result, result.value());
}
TEST_F(SQLStatementFunctionsTest, TestCursorClose){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);
}

TEST_F(SQLStatementFunctionsTest, TestEarlyClose){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  return_code_ = SQLFetch(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLFetch failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);
}

TEST_F(SQLStatementFunctionsTest, TestStatementReuse){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 50 AND 54 LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  std::string err_msg;
  std::string exp_result = "50\tCustomer#000000050\t9SzDYlkzxByyJ1QeTI o\t6\t16-658-112-3221\t4266.130000\tMACHINERY \tts. furiously ironic accounts cajole fu\n"
                             "51\tCustomer#000000051\tuR,wEaiTvo4\t12\t22-344-885-4251\t855.870000\tFURNITURE \teposits. furiously regular requests int\n"
                             "52\tCustomer#000000052\t7 QOqGqqSy9jfV51BC71jcHJSD0\t11\t21-186-284-5998\t5630.280000\tHOUSEHOLD \tic platelets use evenly even accounts. \n"
                             "53\tCustomer#000000053\tHnaxHzTfFTZs8MuCpJyTbZ47Cm4wFOOgib\t15\t25-168-852-5363\t4113.640000\tHOUSEHOLD \tar accounts are. even foxes are blithel\n"
                             "54\tCustomer#000000054\t,k4vf 5vECGWFy,hosTE,\t4\t14-776-370-4745\t868.900000\tAUTOMOBILE\tsual, silent accounts. furiously expres\n";
  auto result = get_result(hstmt_, &err_msg);
  EXPECT_EQ(exp_result, result.value());
}

TEST_F(SQLStatementFunctionsTest, TestStatementReuse2){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 50 AND 54 LIMIT 5;";
  std::string exp_result = "50\tCustomer#000000050\t9SzDYlkzxByyJ1QeTI o\t6\t16-658-112-3221\t4266.130000\tMACHINERY \tts. furiously ironic accounts cajole fu\n"
                             "51\tCustomer#000000051\tuR,wEaiTvo4\t12\t22-344-885-4251\t855.870000\tFURNITURE \teposits. furiously regular requests int\n"
                             "52\tCustomer#000000052\t7 QOqGqqSy9jfV51BC71jcHJSD0\t11\t21-186-284-5998\t5630.280000\tHOUSEHOLD \tic platelets use evenly even accounts. \n"
                             "53\tCustomer#000000053\tHnaxHzTfFTZs8MuCpJyTbZ47Cm4wFOOgib\t15\t25-168-852-5363\t4113.640000\tHOUSEHOLD \tar accounts are. even foxes are blithel\n"
                             "54\tCustomer#000000054\t,k4vf 5vECGWFy,hosTE,\t4\t14-776-370-4745\t868.900000\tAUTOMOBILE\tsual, silent accounts. furiously expres\n";

  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  std::string err_msg;
  auto result = get_result(hstmt_, &err_msg);
  EXPECT_EQ(exp_result, result.value());

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  result = get_result(hstmt_, &err_msg);
  EXPECT_EQ(exp_result, result.value());
}