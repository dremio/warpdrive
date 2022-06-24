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

struct SQLColAttributeTestParams {
    std::string m_connectionOptions;
    uint32_t m_odbcVersion;

    friend std::ostream& operator<< (std::ostream& out, const SQLColAttributeTestParams& params);
};

std::ostream& operator<< (std::ostream& out, const SQLColAttributeTestParams& params) {
    out << "{ "
    << "Connection Options: " << params.m_connectionOptions 
    << ", Version: " << params.m_odbcVersion
    << "}";

    return out;
}

class SQLColAttributeTest : public ::testing::TestWithParam<SQLColAttributeTestParams> {
    void SetUp() override {
        const SQLColAttributeTestParams params = GetParam();
        std::string err_msg;
        ASSERT_TRUE(test_connect_ext(params.m_connectionOptions.c_str(), &err_msg, params.m_odbcVersion)) << err_msg;
        connected_ = true;

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

    const SQLColAttributeTestParams params = GetParam();

    return_code_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt_);
    CHECK_CONN_RESULT(return_code_, "failed to allocate stmt handle", conn);

    // Get column attributes of a simple query.

    // NOTE: Original query was: "SELECT '1'::int AS intcol, 'foobar'::text AS textcol, 'varchar string'::varchar as varcharcol, ''::varchar as empty_varchar_col, 'varchar-5-col'::varchar(5) as varchar5col, '5 days'::interval day to second"
    // TODO: add interval type to test concise types
    return_code_ = SQLExecDirect(handle_stmt_,
                                 (SQLCHAR *) "SELECT "
                                 "CAST('1' AS INTEGER) AS intcol, "
                                 "CAST('foobar' AS VARCHAR) AS textcol, "
                                 "CAST('varchar string' AS VARCHAR) as varcharcol, "
                                 "CAST(''AS VARCHAR) as empty_varchar_col, "
                                 "CAST('varchar-5-col' AS VARCHAR(5)) as varchar5col, "
                                 "CAST('2022-10-06' AS TIMESTAMP) as timestampcol"
                                 ,
                                 SQL_NTS);
    CHECK_STMT_RESULT(return_code_, "SQLExecDirect failed", handle_stmt_);

    return_code_ = SQLNumResultCols(handle_stmt_, &num_cols);
    CHECK_STMT_RESULT(return_code_, "SQLNumResultCols failed", handle_stmt_);

    std::vector<std::string> expected_names = {"intcol", "textcol", "varcharcol", "empty_varchar_col", "varchar5col", "timestampcol"};
    std::vector<std::string> expected_type_names = {"INTEGER", "VARCHAR", "VARCHAR", "VARCHAR", "VARCHAR", "SQL_DATETIME"};
    std::vector<long> expected_octet_lengths = {4, 65536, 65536, 65536, 65536, 1024};

    std::vector<SQLULEN> expected_concise_types_odbc_v2 = {SQL_INTEGER, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_TIMESTAMP};
    std::vector<SQLULEN> expected_concise_types_odbc_v3 = {SQL_INTEGER, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_VARCHAR, SQL_TYPE_TIMESTAMP};


    std::vector<SQLULEN> expected_concise_types;
    switch (params.m_odbcVersion) {
        case SQL_OV_ODBC2:
            expected_concise_types = expected_concise_types_odbc_v2;
            break;
        case SQL_OV_ODBC3:
            expected_concise_types = expected_concise_types_odbc_v3;
            break;
        default:
            FAIL() << "Invalid ODBC Version please check the test parameters";
    }

    for (i = 1; i <= num_cols; i++) {
        char actual_column_name[64];
        SQLLEN actual_concise_type;
        char actual_type_name[64];
        SQLLEN actual_octet_length;

        const std::string expected_column_name = expected_names[i - 1];
        const SQLLEN expected_concise_type = expected_concise_types[i - 1];
        // const std::string expected_type_name = expected_type_names[i - 1];
        const SQLLEN expected_octet_length = expected_octet_lengths[i - 1];

        // SQLColAttribute was only introduced in ODBC 3.0
        if (params.m_odbcVersion != SQL_OV_ODBC2) {
            // Check the column name
            return_code_ = SQLColAttribute(handle_stmt_,
                                        i,
                                        SQL_DESC_LABEL,
                                        actual_column_name,
                                        sizeof(actual_column_name),
                                        nullptr,
                                        nullptr);
            CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed", handle_stmt_);
            EXPECT_EQ(actual_column_name, expected_column_name);

            // Check the concise type
            return_code_ = SQLColAttribute(handle_stmt_,
                                        i,
                                        SQL_DESC_CONCISE_TYPE,
                                        nullptr,
                                        SQL_IS_INTEGER,
                                        nullptr,
                                        &actual_concise_type);
            CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed", handle_stmt_);
            EXPECT_EQ(actual_concise_type, expected_concise_type) << "Failure at " + expected_column_name;
        
        // // Check the type name
        // GTEST_SKIP_("DX-50240: Type names are incorrect");
        // return_code_ = SQLColAttribute(handle_stmt_,
        //                                i,
        //                                SQL_DESC_TYPE_NAME,
        //                                actual_type_name,
        //                                sizeof(actual_type_name),
        //                                nullptr,
        //                                nullptr);
        // CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed to get SQL_DESC_TYPE_NAME", handle_stmt_);
        // EXPECT_EQ(actual_type_name, expected_type_names[i - 1]);

            // // Check the octet length
            // return_code_ = SQLColAttribute(handle_stmt_,
            //                             i,
            //                             SQL_DESC_OCTET_LENGTH,
            //                             nullptr,
            //                             SQL_IS_INTEGER,
            //                             nullptr,
            //                             &actual_octet_length);
            // CHECK_STMT_RESULT(return_code_, "SQLColAttribute failed to get octet length", handle_stmt_);
            // EXPECT_EQ(actual_octet_length, expected_octet_length);
        }

        SQLCHAR actual_column_name_sql_describe[64];
        SQLSMALLINT actual_data_type;
        SQLULEN actual_column_size;
        SQLSMALLINT actual_decimal_size;
        SQLSMALLINT actual_nullable;
        
        return_code_ = SQLDescribeCol(handle_stmt_, i, 
            actual_column_name_sql_describe, sizeof(actual_column_name_sql_describe), nullptr,
            &actual_data_type,
            &actual_column_size,
            &actual_decimal_size,
            &actual_nullable
        );
        CHECK_STMT_RESULT(return_code_, "SQLDescribeCol failed", handle_stmt_);
        EXPECT_EQ((char*)actual_column_name_sql_describe, expected_column_name);
        EXPECT_EQ(actual_data_type, expected_concise_type) << "Failed at column: " + expected_column_name;
        // EXPECT_EQ(actual_column_size, expected_column_size);
        // EXPECT_EQ(actual_decimal_size, expected_decimal_size);
        // EXPECT_EQ(actual_nullable, expected_nullable);
    }
}

INSTANTIATE_TEST_SUITE_P(UnknownSizesTest,
 SQLColAttributeTest,
 ::testing::Values(
        SQLColAttributeTestParams {
            "UnknownSizes=0;MaxVarcharSize=100",
            SQL_OV_ODBC2
        },
        SQLColAttributeTestParams {
            "UnknownSizes=1;MaxVarcharSize=100",
            SQL_OV_ODBC2
        },
        SQLColAttributeTestParams {
            "UnknownSizes=2;MaxVarcharSize=100",
            SQL_OV_ODBC2
        },
        SQLColAttributeTestParams {
            "UnknownSizes=0;MaxVarcharSize=100",
            SQL_OV_ODBC3
        },
        SQLColAttributeTestParams {
            "UnknownSizes=1;MaxVarcharSize=100",
            SQL_OV_ODBC3
        },
        SQLColAttributeTestParams {
            "UnknownSizes=2;MaxVarcharSize=100",
            SQL_OV_ODBC3
        }
 )
);
