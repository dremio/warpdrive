/*
 * Test SQLColAttribute
 */

#include "common.h"
#include <gtest/gtest.h>

class SQLColAttributeTest : public ::testing::TestWithParam<char *> {
    void SetUp() override {
        ASSERT_TRUE(test_connect_ext(GetParam()));

        rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
        ASSERT_TRUE(SQL_SUCCEEDED(rc_));

        SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);
    }

    void TearDown() override {
        rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
        CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
        std::string *err_msg = nullptr;
        ASSERT_TRUE(test_disconnect(err_msg));
    }

protected:
    int rc_{};
    HSTMT hstmt_ = SQL_NULL_HSTMT;
};

TEST_P(SQLColAttributeTest, ColAttributeTestWithExtraConnOptions) {
    int return_code;
    HSTMT handle_stmt = SQL_NULL_HSTMT;
    SQLUSMALLINT i;
    SQLSMALLINT num_cols;

    return_code = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt);
    CHECK_CONN_RESULT(return_code, "failed to allocate stmt handle", conn);

    // Get column attributes of a simple query.

    // CHANGE-NOTE: Original query was: "SELECT '1'::int AS intcol, 'foobar'::text AS textcol, 'varchar string'::varchar as varcharcol, ''::varchar as empty_varchar_col, 'varchar-5-col'::varchar(5) as varchar5col, '5 days'::interval day to second"
    return_code = SQLExecDirect(handle_stmt,
                                (SQLCHAR *) "SELECT CAST('1' AS INTEGER) AS intcol, CAST('foobar' AS VARCHAR) AS textcol, CAST('varchar string' AS VARCHAR) as varcharcol, CAST(''AS VARCHAR) as empty_varchar_col, CAST('varchar-5-col' AS VARCHAR(5)) as varchar5col",
                                SQL_NTS);
    CHECK_STMT_RESULT(return_code, "SQLExecDirect failed", handle_stmt);

    return_code = SQLNumResultCols(handle_stmt, &num_cols);
    CHECK_STMT_RESULT(return_code, "SQLNumResultCols failed", handle_stmt);

    std::vector<std::string> expected_names = {"intcol", "textcol", "varcharcol", "empty_varchar_col", "varchar5col"};
    std::vector<std::string> expected_type_names = {"INTEGER", "VARCHAR", "VARCHAR", "VARCHAR", "VARCHAR"};
    std::vector<long> expected_octet_length = {4, 65536, 65536, 65536, 65536};

    for (i = 1; i <= num_cols; i++) {
        char actual_column_name[64];
        char actual_type_name[64];
        SQLLEN actual_octet_length;

        return_code = SQLColAttribute(handle_stmt,
                                      i,
                                      SQL_DESC_LABEL,
                                      actual_column_name,
                                      sizeof(actual_column_name),
                                      nullptr,
                                      nullptr);
        CHECK_STMT_RESULT(return_code, "SQLColAttribute failed", handle_stmt);
        EXPECT_EQ(actual_column_name, expected_names[i - 1]);

        return_code = SQLColAttribute(handle_stmt,
                                      i,
                                      SQL_DESC_TYPE_NAME,
                                      actual_type_name,
                                      sizeof(actual_type_name),
                                      nullptr,
                                      nullptr);
        CHECK_STMT_RESULT(return_code, "SQLColAttribute failed", handle_stmt);
        EXPECT_EQ(actual_type_name, expected_type_names[i - 1]);

        return_code = SQLColAttribute(handle_stmt,
                                      i,
                                      SQL_DESC_OCTET_LENGTH,
                                      nullptr,
                                      SQL_IS_INTEGER,
                                      nullptr,
                                      &actual_octet_length);
        CHECK_STMT_RESULT(return_code, "SQLColAttribute failed", handle_stmt);
        EXPECT_EQ(actual_octet_length, expected_octet_length[i - 1]);
    }
}

INSTANTIATE_TEST_CASE_P

(UnknownSizesTest,
 SQLColAttributeTest,
 ::testing::Values(
         (char *) "UnknownSizes=0;MaxVarcharSize=100",
         (char *) "UnknownSizes=1;MaxVarcharSize=100",
         (char *) "UnknownSizes=2;MaxVarcharSize=100"
 )
);
