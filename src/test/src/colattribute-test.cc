/*
 * Test SQLColAttribute
 */

#include <cstdio>

#include "common.h"
#include <gtest/gtest.h>

class SQLColAttributeTest : public ::testing::TestWithParam<char *> {
};

TEST_P(SQLColAttributeTest, ColAttributeTestWithExtraConnOptions) {
    char *extra_conn_options = GetParam();
    int row_count;
    HSTMT handle_stmt = SQL_NULL_HSTMT;
    SQLUSMALLINT i;
    SQLSMALLINT num_cols;

    printf("Running tests with %s...\n", extra_conn_options);

    /* The behavior of these tests depend on the UnknownSizes parameter */
    test_connect_ext(extra_conn_options);

    row_count = SQLAllocHandle(SQL_HANDLE_STMT, conn, &handle_stmt);
    CHECK_CONN_RESULT_2(row_count, "failed to allocate stmt handle", conn)

    /*
     * Get column attributes of a simple query.
     */
    printf("Testing SQLColAttribute...\n");
    row_count = SQLExecDirect(handle_stmt,
                              (SQLCHAR *) "SELECT '1'::int AS intcol, 'foobar'::text AS textcol, 'varchar string'::varchar as varcharcol, ''::varchar as empty_varchar_col, 'varchar-5-col'::varchar(5) as varchar5col, '5 days'::interval day to second",
                              SQL_NTS);
    CHECK_STMT_RESULT_2(row_count, "SQLExecDirect failed", handle_stmt)

    row_count = SQLNumResultCols(handle_stmt, &num_cols);
    CHECK_STMT_RESULT_2(row_count, "SQLNumResultCols failed", handle_stmt)

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
        CHECK_STMT_RESULT_2(row_count, "SQLColAttribute failed", handle_stmt)
        printf("\n-- Column %d: %s --\n", i, buffer);

        row_count = SQLColAttribute(handle_stmt,
                                    i,
                                    SQL_DESC_OCTET_LENGTH,
                                    nullptr,
                                    SQL_IS_INTEGER,
                                    nullptr,
                                    &number);
        CHECK_STMT_RESULT_2(row_count, "SQLColAttribute failed", handle_stmt)
        printf("SQL_DESC_OCTET_LENGTH: %d\n", (int) number);

        row_count = SQLColAttribute(handle_stmt,
                                    i,
                                    SQL_DESC_TYPE_NAME,
                                    buffer,
                                    sizeof(buffer),
                                    nullptr,
                                    nullptr);
        CHECK_STMT_RESULT_2(row_count, "SQLColAttribute failed", handle_stmt)
        printf("SQL_DESC_TYPE_NAME: %s\n", buffer);
    }

    /* Clean up */
    test_disconnect();
}

INSTANTIATE_TEST_CASE_P

(Test, SQLColAttributeTest,
 ::testing::Values(
         ((char *) "UnknownSizes=0;MaxVarcharSize=100"),
         ((char *) "UnknownSizes=1;MaxVarcharSize=100"),
         ((char *) "UnknownSizes=2;MaxVarcharSize=100"))
);
