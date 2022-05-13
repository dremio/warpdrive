/*
 * Test SQLColAttribute
 */

#include "common.h"
#include <gtest/gtest.h>

class SQLColAttributeTest : public ::testing::TestWithParam<char *> {
public:
    void SetUp() override {
        test_connect_ext(GetParam());
    }

    void TearDown() override {
        test_disconnect();
    }
};

TEST_P(SQLColAttributeTest, ColAttributeTestWithExtraConnOptions) {
    int row_count;
    HSTMT handle_stmt = SQL_NULL_HSTMT;
    SQLUSMALLINT i;
    SQLSMALLINT num_cols;

    row_count = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt);
    CHECK_CONN_RESULT(row_count, "failed to allocate stmt handle", conn);

    /*
     * Get column attributes of a simple query.
     */
    row_count = SQLExecDirect(handle_stmt,
                              (SQLCHAR *) "SELECT '1'::int AS intcol, 'foobar'::text AS textcol, 'varchar string'::varchar as varcharcol, ''::varchar as empty_varchar_col, 'varchar-5-col'::varchar(5) as varchar5col, '5 days'::interval day to second",
                              SQL_NTS);
    CHECK_STMT_RESULT(row_count, "SQLExecDirect failed", handle_stmt);

    row_count = SQLNumResultCols(handle_stmt, &num_cols);
    CHECK_STMT_RESULT(row_count, "SQLNumResultCols failed", handle_stmt);

    for (i = 1; i <= num_cols; i++) {
        char buffer[64];
        SQLLEN number;

        row_count = SQLColAttribute(handle_stmt,
                                    i,
                                    SQL_DESC_LABEL,
                                    buffer,
                                    sizeof(buffer),
                                    nullptr,
                                    nullptr);
        CHECK_STMT_RESULT(row_count, "SQLColAttribute failed", handle_stmt);

        row_count = SQLColAttribute(handle_stmt,
                                    i,
                                    SQL_DESC_OCTET_LENGTH,
                                    nullptr,
                                    SQL_IS_INTEGER,
                                    nullptr,
                                    &number);
        CHECK_STMT_RESULT(row_count, "SQLColAttribute failed", handle_stmt);

        row_count = SQLColAttribute(handle_stmt,
                                    i,
                                    SQL_DESC_TYPE_NAME,
                                    buffer,
                                    sizeof(buffer),
                                    nullptr,
                                    nullptr);
        CHECK_STMT_RESULT(row_count, "SQLColAttribute failed", handle_stmt);
    }
}

INSTANTIATE_TEST_CASE_P

(Test, SQLColAttributeTest,
 ::testing::Values(
         ((char *) "UnknownSizes=0;MaxVarcharSize=100"),
         ((char *) "UnknownSizes=1;MaxVarcharSize=100"),
         ((char *) "UnknownSizes=2;MaxVarcharSize=100"))
);
