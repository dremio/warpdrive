/*
 * Copyright (C) 2020-2022 Dremio Corporation
 *
 * See "license.txt" for license information.
 */

#include <gtest/gtest.h>
#include "common.h"

class SQLStatementFunctionsTest : public ::testing::Test {
  void SetUp() override {
    connected_ = test_connect();
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
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 536796 AND 536800 LIMIT 5;";
  return_code_= SQLExecDirect(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", hstmt_);

  std::string exp_result = "536796\tCustomer#000536796\tUWHFXVJzZMixLpz00Mk1r,PfP3LT DuLeO\t10\t20-352-489-5961\t5193.040000\tMACHINERY \t beans run after the carefully pending \n"
                           "536797\tCustomer#000536797\t51QNmdLF0ZR\t7\t17-492-119-8512\t3379.690000\tAUTOMOBILE\tkly regular accounts sleep even, regula\n"
                           "536798\tCustomer#000536798\tvoBZynUBhAU\t3\t13-564-584-3770\t7504.490000\tHOUSEHOLD \te carefully pinto beans. slyly regular \n"
                           "536799\tCustomer#000536799\txhD,AtTcEDMnXQroS11O0APgjW3t\t5\t15-958-448-6750\t-37.990000\tMACHINERY \tn pinto beans affix slyly quickly ironi\n"
                           "536800\tCustomer#000536800\t8lnRHiYEDTSNryX\t22\t32-318-475-1634\t8418.260000\tMACHINERY \tickly regular pinto beans cajole furiou\n";
  auto result = get_result(hstmt_);
  EXPECT_EQ(exp_result, result);
}

TEST_F(SQLStatementFunctionsTest, TestExecDirectPrepareBeforeDirectExec){
  std::string sqlQueryPrepared = "SELECT * FROM postgres.tpch.customer LIMIT 10";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQueryPrepared.c_str(), sqlQueryPrepared.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 536796 AND 536800 LIMIT 5;";
  return_code_= SQLExecDirect(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", hstmt_);

  std::string exp_result = "536796\tCustomer#000536796\tUWHFXVJzZMixLpz00Mk1r,PfP3LT DuLeO\t10\t20-352-489-5961\t5193.040000\tMACHINERY \t beans run after the carefully pending \n"
                           "536797\tCustomer#000536797\t51QNmdLF0ZR\t7\t17-492-119-8512\t3379.690000\tAUTOMOBILE\tkly regular accounts sleep even, regula\n"
                           "536798\tCustomer#000536798\tvoBZynUBhAU\t3\t13-564-584-3770\t7504.490000\tHOUSEHOLD \te carefully pinto beans. slyly regular \n"
                           "536799\tCustomer#000536799\txhD,AtTcEDMnXQroS11O0APgjW3t\t5\t15-958-448-6750\t-37.990000\tMACHINERY \tn pinto beans affix slyly quickly ironi\n"
                           "536800\tCustomer#000536800\t8lnRHiYEDTSNryX\t22\t32-318-475-1634\t8418.260000\tMACHINERY \tickly regular pinto beans cajole furiou\n";
  auto result = get_result(hstmt_);
  EXPECT_EQ(exp_result, result);
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

// TODO: reactivate these tests after DX-51635 is fixed
TEST_F(SQLStatementFunctionsTest, DISABLED_TestNormalFlow){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 536796 AND 536800 LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  std::string exp_result = "536796\tCustomer#000536796\tUWHFXVJzZMixLpz00Mk1r,PfP3LT DuLeO\t10\t20-352-489-5961\t5193.040000\tMACHINERY \t beans run after the carefully pending \n"
                           "536797\tCustomer#000536797\t51QNmdLF0ZR\t7\t17-492-119-8512\t3379.690000\tAUTOMOBILE\tkly regular accounts sleep even, regula\n"
                           "536798\tCustomer#000536798\tvoBZynUBhAU\t3\t13-564-584-3770\t7504.490000\tHOUSEHOLD \te carefully pinto beans. slyly regular \n"
                           "536799\tCustomer#000536799\txhD,AtTcEDMnXQroS11O0APgjW3t\t5\t15-958-448-6750\t-37.990000\tMACHINERY \tn pinto beans affix slyly quickly ironi\n"
                           "536800\tCustomer#000536800\t8lnRHiYEDTSNryX\t22\t32-318-475-1634\t8418.260000\tMACHINERY \tickly regular pinto beans cajole furiou\n";
  auto result = get_result(hstmt_);
  EXPECT_EQ(exp_result, result);
}
TEST_F(SQLStatementFunctionsTest, DISABLED_TestCursorClose){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);
}

TEST_F(SQLStatementFunctionsTest, DISABLED_TestEarlyClose){
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

TEST_F(SQLStatementFunctionsTest, DISABLED_TestStatementReuse){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 536796 AND 536800 LIMIT 5;";
  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  std::string exp_result = "536796\tCustomer#000536796\tUWHFXVJzZMixLpz00Mk1r,PfP3LT DuLeO\t10\t20-352-489-5961\t5193.040000\tMACHINERY \t beans run after the carefully pending \n"
                           "536797\tCustomer#000536797\t51QNmdLF0ZR\t7\t17-492-119-8512\t3379.690000\tAUTOMOBILE\tkly regular accounts sleep even, regula\n"
                           "536798\tCustomer#000536798\tvoBZynUBhAU\t3\t13-564-584-3770\t7504.490000\tHOUSEHOLD \te carefully pinto beans. slyly regular \n"
                           "536799\tCustomer#000536799\txhD,AtTcEDMnXQroS11O0APgjW3t\t5\t15-958-448-6750\t-37.990000\tMACHINERY \tn pinto beans affix slyly quickly ironi\n"
                           "536800\tCustomer#000536800\t8lnRHiYEDTSNryX\t22\t32-318-475-1634\t8418.260000\tMACHINERY \tickly regular pinto beans cajole furiou\n";
  auto result = get_result(hstmt_);
  EXPECT_EQ(exp_result, result);
}

TEST_F(SQLStatementFunctionsTest, DISABLED_TestStatementReuse2){
  std::string sqlQuery = "SELECT * FROM postgres.tpch.customer WHERE c_custkey BETWEEN 536796 AND 536800 LIMIT 5;";
  std::string exp_result = "536796\tCustomer#000536796\tUWHFXVJzZMixLpz00Mk1r,PfP3LT DuLeO\t10\t20-352-489-5961\t5193.040000\tMACHINERY \t beans run after the carefully pending \n"
                           "536797\tCustomer#000536797\t51QNmdLF0ZR\t7\t17-492-119-8512\t3379.690000\tAUTOMOBILE\tkly regular accounts sleep even, regula\n"
                           "536798\tCustomer#000536798\tvoBZynUBhAU\t3\t13-564-584-3770\t7504.490000\tHOUSEHOLD \te carefully pinto beans. slyly regular \n"
                           "536799\tCustomer#000536799\txhD,AtTcEDMnXQroS11O0APgjW3t\t5\t15-958-448-6750\t-37.990000\tMACHINERY \tn pinto beans affix slyly quickly ironi\n"
                           "536800\tCustomer#000536800\t8lnRHiYEDTSNryX\t22\t32-318-475-1634\t8418.260000\tMACHINERY \tickly regular pinto beans cajole furiou\n";

  return_code_ = SQLPrepare(hstmt_, (SQLCHAR *) sqlQuery.c_str(), sqlQuery.length());
  CHECK_STMT_RESULT(return_code_, "SQLPrepare failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  auto result = get_result(hstmt_);
  EXPECT_EQ(exp_result, result);

  return_code_ = SQLCloseCursor(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLCloseCursor failed", hstmt_);

  return_code_ = SQLExecute(hstmt_);
  CHECK_STMT_RESULT(return_code_, "SQLExecute failed", hstmt_);

  result = get_result(hstmt_);
  EXPECT_EQ(exp_result, result);
}