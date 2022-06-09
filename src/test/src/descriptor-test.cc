/*--------
 * Module:			descriptor-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *--------
 */
#include "common.h"

class DescriptorTest : public  ::testing::Test{
public:
  void SetUp() override{
    std::string err_msg;
    connected_ = test_connect(&err_msg);
    EXPECT_TRUE(connected_) << err_msg;

    rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
    EXPECT_TRUE(SQL_SUCCEEDED(rc_));
  }
  void TearDown() override{
    if (hstmt_ != SQL_NULL_HSTMT){
      rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
      CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
    }
    std::string err_msg;
    if (connected_){
      EXPECT_TRUE(test_disconnect(&err_msg))<<err_msg;
    }
  }

protected:
  bool connected_;
  int			rc_;
  HSTMT		hstmt_ = SQL_NULL_HSTMT;
};
