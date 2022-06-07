/*--------
 * Module:			diagnostic-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */

#include <cstring>
#include <cstdio>

#include "common.h"

class SQLDiagnosticTests : public ::testing::Test {
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
    bool connected{false};
    int return_code_{};
    HSTMT handle_stmt_ = SQL_NULL_HSTMT;

    static void
    AssertSQLGetDiagRecSQLState(SQLSMALLINT handle_type, SQLHANDLE handle, const std::string &expected_sql_state) {
        SQLCHAR actual_sql_state[32];
        SQLCHAR message_text[1024];
        SQLINTEGER native_error;
        SQLSMALLINT text_length;
        SQLRETURN sql_return;
        SQLSMALLINT record_number = 1;

        sql_return = SQLGetDiagRec(handle_type,
                                   handle,
                                   record_number,
                                   actual_sql_state,
                                   &native_error,
                                   message_text,
                                   sizeof(message_text),
                                   &text_length);

        ASSERT_TRUE(SQL_SUCCEEDED(sql_return));
        EXPECT_EQ(reinterpret_cast<const char *>(actual_sql_state), expected_sql_state);
    }

    static void
    AssertSQLGetDiagFieldSQLState(SQLSMALLINT handle_type, SQLHANDLE handle, const std::string &expected_sql_state) {
        SQLCHAR actual_sql_state[32];
        SQLSMALLINT string_length;
        SQLRETURN sql_return;
        SQLSMALLINT record_number = 1;

        sql_return = SQLGetDiagField(handle_type,
                                     handle,
                                     record_number,
                                     SQL_DIAG_SQLSTATE,
                                     actual_sql_state,
                                     sizeof(actual_sql_state),
                                     &string_length);

        ASSERT_TRUE(SQL_SUCCEEDED(sql_return));
        EXPECT_EQ(reinterpret_cast<const char *>(actual_sql_state), expected_sql_state);
    }
};

TEST_F(SQLDiagnosticTests, SQLDiagnosticTest) {
    char buffer[10000];
    const char *invalid_query_sql_state = "HY000";


    /*
     * Execute an erroneous query, and call SQLGetDiagRec twice on the
     * statement. Should get the same result both times; SQLGetDiagRec is
     * not supposed to change the state of the statement.
     */
    return_code_ = SQLExecDirect(handle_stmt_, (SQLCHAR *) "broken query", SQL_NTS);

    CHECK_INVALID_STMT_RESULT(return_code_, "SQLExecDirect was supposed to fail.", handle_stmt_);
    AssertSQLGetDiagRecSQLState(SQL_HANDLE_STMT, handle_stmt_, invalid_query_sql_state);
    AssertSQLGetDiagRecSQLState(SQL_HANDLE_STMT, handle_stmt_, invalid_query_sql_state);

    /*
     * Test a very long invalid query.
     */
    memset(buffer, 'x', sizeof(buffer) - 10);
    sprintf(buffer + sizeof(buffer) - 10, "END");
    return_code_ = SQLExecDirect(handle_stmt_, (SQLCHAR *) buffer, SQL_NTS);

    CHECK_INVALID_STMT_RESULT(return_code_, "SQLExecDirect was supposed to fail.", handle_stmt_);
    AssertSQLGetDiagFieldSQLState(SQL_HANDLE_STMT, handle_stmt_, invalid_query_sql_state);
    AssertSQLGetDiagFieldSQLState(SQL_HANDLE_STMT, handle_stmt_, invalid_query_sql_state);
}
