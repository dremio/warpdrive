/*--------
 * Module:			bindcol-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */

#include "common.h"

class SQLBindColTests : public ::testing::Test {
    void SetUp() override {
        std::string err_msg;
        connected = test_connect(&err_msg);
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
            ASSERT_TRUE(test_disconnect(&err_msg))<<err_msg;
        }
    }

  protected:
    template <typename Function>
    void ExecuteFetchTest(Function fetch_and_validate_function) {
      /*
      * NOTE: in the psqlodbc, we assume that SQL_C_LONG actually means a
      * variable of type SQLINTEGER. They are not the same on platforms where
      * "long" is a 64-bit integer. That seems a bit bogus, but it's too late
      * to change that without breaking applications that depend on it.
      * (on little-endian systems, you won't notice the difference if you reset
      * the high bits to zero before calling SQLBindCol.)
      */
      SQLINTEGER long_value;
      SQLLEN index_long_value;
      char char_value[100];
      SQLLEN index_char_value;
      int row_number = 0;

      return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
      CHECK_CONN_RESULT(return_code_, "failed to allocate stmt handle", conn);

      return_code_ = SQLBindCol(handle_stmt_, 1, SQL_C_LONG, &long_value, 0, &index_long_value);
      CHECK_STMT_RESULT(return_code_, "SQLBindCol failed", handle_stmt_);

      return_code_ = SQLBindCol(handle_stmt_, 2, SQL_C_CHAR, &char_value, sizeof(char_value), &index_char_value);
      CHECK_STMT_RESULT(return_code_, "SQLBindCol failed", handle_stmt_);

      // NOTE: Original query was: "SELECT id, 'foo' || id FROM generate_series(1, 10) id"
      return_code_ = SQLExecDirect(handle_stmt_, (SQLCHAR *)
                                           "SELECT 1, 'foo1' UNION ALL "
                                           "SELECT 2, 'foo2' UNION ALL "
                                           "SELECT 3, 'foo3' UNION ALL "
                                           "SELECT 4, 'foo4' UNION ALL "
                                           "SELECT 5, 'foo5' UNION ALL "
                                           "SELECT 6, 'foo6' UNION ALL "
                                           "SELECT 7, 'foo7' UNION ALL "
                                           "SELECT 8, 'foo8' UNION ALL "
                                           "SELECT 9, 'foo9' UNION ALL "
                                           "SELECT 10, 'foo10'",
                                   SQL_NTS);
      CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", handle_stmt_);

      std::vector<long> expected_index_long_value = {1, 2, 3, 4, 5, 6, 7, 7, 7, 7};
      std::vector<std::string> expected_index_char_value = {"foo1", "foo2", "foo3", "foo3", "foo3",
                                                            "foo6", "foo7", "foo7", "foo7", "foo10"};

      while (true) {
        return_code_ = fetch_and_validate_function(handle_stmt_);
        if (return_code_ == SQL_NO_DATA) break;

        ASSERT_EQ(return_code_, SQL_SUCCESS) << format_diagnostic("SQLFetch failed", SQL_HANDLE_STMT, &handle_stmt_);
        EXPECT_EQ(expected_index_long_value[row_number], (long) long_value);
        EXPECT_EQ(expected_index_char_value[row_number], char_value);

        /*
         * At row 3, unbind the text field. At row 5, bind it again.
         * At row 7, unbind both columns with SQLFreeStmt(SQL_UNBIND).
         * At row 9, bind text field again.
         */
        row_number++;
        if (row_number == 3) {
          return_code_ = SQLBindCol(handle_stmt_, 2, SQL_C_CHAR, nullptr, 0, nullptr);
          CHECK_STMT_RESULT(return_code_, "SQLBindCol failed", handle_stmt_);
        }
        if (row_number == 7) {
          return_code_ = SQLFreeStmt(handle_stmt_, SQL_UNBIND);
          CHECK_STMT_RESULT(return_code_, "SQLFreeStmt(SQL_UNBIND) failed", handle_stmt_);
        }
        if (row_number == 5 || row_number == 9) {
          return_code_ = SQLBindCol(handle_stmt_, 2, SQL_C_CHAR, &char_value, sizeof(char_value), &index_char_value);
          CHECK_STMT_RESULT(return_code_, "SQLBindCol failed", handle_stmt_);
        }
      }
    }

    template <typename Function>
    void ExecuteFetchTest_Multiple_Batches_Single_Fetch(Function fetch_and_validate_function) {
      SQLINTEGER long_value[3];
      SQLLEN index_long_value;

      return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
      CHECK_CONN_RESULT(return_code_, "failed to allocate stmt handle", conn);

      return_code_ = SQLSetStmtAttr(handle_stmt_, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER) 3, 0);
      CHECK_CONN_RESULT(return_code_, "failed set SQL_ATTR_ROW_ARRAY_SIZE", conn);

      return_code_ = SQLBindCol(handle_stmt_, 1, SQL_C_LONG, &long_value, 0, &index_long_value);
      CHECK_STMT_RESULT(return_code_, "SQLBindCol failed", handle_stmt_);

      return_code_ = SQLExecDirect(handle_stmt_, (SQLCHAR *) "SELECT 100 UNION ALL SELECT 200 UNION ALL SELECT 300",
                                   SQL_NTS);
      CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", handle_stmt_);

      return_code_ = fetch_and_validate_function(handle_stmt_);
      EXPECT_EQ(long_value[0], 100);
      EXPECT_EQ(long_value[1], 200);
      EXPECT_EQ(long_value[2], 300);
    }

    bool connected{false};
    int return_code_{};
    HSTMT handle_stmt_ = SQL_NULL_HSTMT;
};

TEST_F(SQLBindColTests, SQLBindColFetchTest) {
  ExecuteFetchTest([](HSTMT stmt) -> SQLRETURN {
    return SQLFetch(stmt);
    });
}

TEST_F(SQLBindColTests, SQLBindColExtendedFetchTest) {
  ExecuteFetchTest([](HSTMT stmt) -> SQLRETURN {
    SQLULEN rows_fetched = 0;
    SQLUSMALLINT row_status = 0;
    SQLRETURN result = SQLExtendedFetch(stmt, SQL_FETCH_NEXT, 0, &rows_fetched, &row_status);
    if (SQL_SUCCEEDED(result)) {
      EXPECT_EQ(1, rows_fetched);
    } else if (result == SQL_NO_DATA) {
      EXPECT_EQ(0, rows_fetched);
    }
    return result;
  });
}

TEST_F(SQLBindColTests, SQLBindColetchScrollTest) {
  ExecuteFetchTest([](HSTMT stmt) -> SQLRETURN {
    SQLRETURN result = SQLFetchScroll(stmt, SQL_FETCH_NEXT, 0);
    return result;
  });
}

TEST_F(SQLBindColTests, SQLBindColFetchTest_Multiple_Batches_Single_Fetch) {
  ExecuteFetchTest_Multiple_Batches_Single_Fetch([](HSTMT stmt) -> SQLRETURN {
    return SQLFetch(stmt);
  });
}

TEST_F(SQLBindColTests, SQLBindColExtendedFetchTest_Multiple_Batches_Single_Fetch) {
  ExecuteFetchTest_Multiple_Batches_Single_Fetch([](HSTMT stmt) -> SQLRETURN {
    SQLULEN rows_fetched = 0;
    SQLUSMALLINT row_status = 0;
    SQLRETURN result = SQLExtendedFetch(stmt, SQL_FETCH_NEXT, 0, &rows_fetched, &row_status);
    if (SQL_SUCCEEDED(result)) {
      EXPECT_EQ(3, rows_fetched);
    } else if (result == SQL_NO_DATA) {
      EXPECT_EQ(0, rows_fetched);
    }
    return result;
  });
}

TEST_F(SQLBindColTests, SQLBindColetchScrollTest_Multiple_Batches_Single_Fetch) {
  ExecuteFetchTest_Multiple_Batches_Single_Fetch([](HSTMT stmt) -> SQLRETURN {
    SQLRETURN result = SQLFetchScroll(stmt, SQL_FETCH_NEXT, 0);
    return result;
  });
}
