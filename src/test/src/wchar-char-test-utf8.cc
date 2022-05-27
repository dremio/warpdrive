#include "common.h"

class Utf8Test : public ::testing::Test {
  void SetUp() override {
    connect_ = test_connect();
    ASSERT_TRUE(connect_);
    return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
    ASSERT_TRUE(SQL_SUCCEEDED(return_code_));
  }

  void TearDown() override {
    return_code_ = SQLFreeStmt(handle_stmt_, SQL_CLOSE);
    CHECK_STMT_RESULT(return_code_, "SQLFreeStmt failed", handle_stmt_);
    std::string *err_msg = nullptr;
    if (connect_){
      ASSERT_TRUE(test_disconnect(err_msg));
    }
  }

protected:
  bool connect_;
  int return_code_{};
  HSTMT handle_stmt_ = SQL_NULL_HSTMT;
};

TEST_F(Utf8Test, testUtf8)
{
	SQLLEN		in, cbParam2;
	SQLINTEGER	cbQueryLen;
	SQLCHAR		chardt[100];

	return_code_ = SQLBindCol(handle_stmt_, 1, SQL_C_CHAR, (SQLPOINTER) chardt, sizeof(chardt), &ind);
	CHECK_STMT_RESULT(return_code_, "SQLBindCol to SQL_C_CHAR failed", handle_stmt_);
  cbParam2 = SQL_NTS;
  std::string str;
  str.append("斉藤浩");
	cbParam2 = str.length();
  str.append("斉藤浩");
	return_code_ = SQLBindParameter(handle_stmt_, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, str.size(), 0, &str, str.size(), &cbParam2);
	CHECK_STMT_RESULT(return_code_, "SQLBindParameter 2 failed", handle_stmt_);

  std::string	query = "select CAST('私は' AS varchar) as c1, CAST('です。貴方は' AS varchar) as c2, CAST('さんですね？  𠀋𡈽𡌛𡑮𡢽𪐷𪗱𪘂' AS varchar) as c3;";
	cbQueryLen = (SQLINTEGER) query.length();
	return_code_ = SQLExecDirect(handle_stmt_, (SQLCHAR *) query.c_str(), cbQueryLen);
	CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed to return SQL_C_CHAR", handle_stmt_);
  SQL_SUCCEEDED(SQLFetch(handle_stmt_));
  std::string expected_output = "私は です。貴方は さんですね？  𠀋𡈽𡌛𡑮𡢽𪐷𪗱𪘂";
  ASSERT_EQ(expected_output, chardt);
	SQLFreeStmt(handle_stmt_, SQL_CLOSE);
}
