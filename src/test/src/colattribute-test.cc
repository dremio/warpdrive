/*--------
 * Module:			colattribute-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */

#include "common.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

class SQLColAttributeTest : public ::testing::TestWithParam<char *> {
    void SetUp() override {
        std::string err_msg;
        ASSERT_TRUE(test_connect_ext(GetParam(), &err_msg)) << err_msg;

        return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
        ASSERT_TRUE(SQL_SUCCEEDED(return_code_)) << "Failed to allocate handle in setup";
    }

    void TearDown() override {
        if (handle_stmt_ != SQL_NULL_HSTMT) {
            return_code_ = SQLFreeStmt(handle_stmt_, SQL_CLOSE);
            CHECK_STMT_RESULT(return_code_, "SQLFreeStmt failed in TearDown", handle_stmt_);
        }
        if (connected_) {
            std::string err_msg;
            ASSERT_TRUE(test_disconnect(&err_msg)) << err_msg;
        }
    }

private:
    bool connected_ {false};

protected:
    int return_code_{};
    HSTMT handle_stmt_ = SQL_NULL_HSTMT;
};

TEST_P(SQLColAttributeTest, ColAttributeTestWithExtraConnOptions) {
    SQLUSMALLINT i;
    SQLSMALLINT num_cols;

    return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
    CHECK_CONN_RESULT(return_code_, "failed to allocate stmt handle", conn);

    // Get column attributes of a simple query.

    // NOTE: Original query was: "SELECT '1'::int AS intcol, 'foobar'::text AS textcol, 'varchar string'::varchar as varcharcol, ''::varchar as empty_varchar_col, 'varchar-5-col'::varchar(5) as varchar5col, '5 days'::interval day to second"
    // TODO: add interval type to test concise types
    return_code_ = SQLExecDirect(handle_stmt_,
                                 (SQLCHAR *) "SELECT CAST('1' AS INTEGER) AS intcol, CAST('foobar' AS VARCHAR) AS textcol, CAST('varchar string' AS VARCHAR) as varcharcol, CAST(''AS VARCHAR) as empty_varchar_col, CAST('varchar-5-col' AS VARCHAR(5)) as varchar5col, CAST('2022-10-06' AS TIMESTAMP) as timestampcol",
                                 SQL_NTS);
    CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", handle_stmt_);

    return_code_ = SQLNumResultCols(handle_stmt_, &num_cols);
    CHECK_STMT_RESULT(return_code_, "SQLNumResultCols failed", handle_stmt_);

    std::vector<std::string> expected_names = {"intcol", "textcol", "varcharcol", "empty_varchar_col", "varchar5col", "timestampcol"};
    std::vector<std::string> expected_type_names = {"INTEGER", "VARCHAR", "VARCHAR", "VARCHAR", "VARCHAR", "SQL_DATETIME"};
    std::vector<long> expected_concise_type = {SQL_INTEGER, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_TYPE_TIMESTAMP};
    std::vector<long> expected_octet_length = {4, 65536, 65536, 65536, 65536};

    for (i = 1; i <= num_cols; i++) {
        char actual_column_name[64];
        long actual_concise_type;
        char actual_type_name[64];
        SQLLEN actual_octet_length;

        // Check the column name
        return_code_ = SQLColAttribute(handle_stmt_,
                                       i,
                                       SQL_DESC_LABEL,
                                       actual_column_name,
                                       sizeof(actual_column_name),
                                       nullptr,
                                       nullptr);
        CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed", handle_stmt_);
        EXPECT_EQ(actual_column_name, expected_names[i - 1]);

        // Check the concise type
        return_code_ = SQLColAttribute(handle_stmt_,
                                       i,
                                       SQL_DESC_CONCISE_TYPE,
                                       nullptr,
                                       SQL_IS_INTEGER,
                                       nullptr,
                                       &actual_concise_type);
        CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed", handle_stmt_);
        EXPECT_EQ(actual_concise_type, expected_concise_type[i - 1]);

        // // Check the type name
        // GTEST_SKIP_("DX-50240: Type names are incorrect");
        // return_code_ = SQLColAttribute(handle_stmt_,
        //                                i,
        //                                SQL_DESC_TYPE_NAME,
        //                                actual_type_name,
        //                                sizeof(actual_type_name),
        //                                nullptr,
        //                                nullptr);
        // CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed", handle_stmt_);
        // EXPECT_EQ(actual_type_name, expected_type_names[i - 1]);

        // // Check the octet length
        // GTEST_SKIP_("DX-52041: Octet length doesn't match.");
        // return_code_ = SQLColAttribute(handle_stmt_,
        //                                i,
        //                                SQL_DESC_OCTET_LENGTH,
        //                                nullptr,
        //                                SQL_IS_INTEGER,
        //                                nullptr,
        //                                &actual_octet_length);
        // CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed", handle_stmt_);
        // EXPECT_EQ(actual_octet_length, expected_octet_length[i - 1]);
    }
}

INSTANTIATE_TEST_SUITE_P

(UnknownSizesTest,
 SQLColAttributeTest,
 ::testing::Values(
         (char *) "UnknownSizes=0;MaxVarcharSize=100",
         (char *) "UnknownSizes=1;MaxVarcharSize=100",
         (char *) "UnknownSizes=2;MaxVarcharSize=100"
 )
);
