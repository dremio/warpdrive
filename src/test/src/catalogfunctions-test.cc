/*
 * This test is used to check the output and processing of the catalog
 * functions listed as below:
 * - SQLGetTypeInfo
 * - SQLTables
 * - SQLColumns
 * - SQLSpecialColumns
 * - SQLStatistics
 * - SQLPrimaryKeys
 * - SQLForeignKeys
 * - SQLProcedureColumns
 * - SQLTablePrivileges
 * - SQLColumnPrivileges
 * - SQLProcedures
 * - SQLGetInfo
 */
#include <string.h>
#include <stdio.h>

#include "common.h"
class CatalogFunctionsTest : public  ::testing::Test{
public:
  void SetUp() override{
    EXPECT_TRUE(test_connect());

    rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
    EXPECT_TRUE(SQL_SUCCEEDED(rc_));

    SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);
  }
  void TearDown() override{
    rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
    CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
    std::string *err_msg = nullptr;
    EXPECT_TRUE(test_disconnect(err_msg));
  }

protected:
  int			rc_;
  HSTMT		hstmt_ = SQL_NULL_HSTMT;
};

TEST_F(CatalogFunctionsTest, DISABLED_GetTypeInfoTest){
  /* Check for SQLGetTypeInfo */

  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);
  /* Check for SQLGetTypeInfo */
  rc_ = SQLGetTypeInfo(hstmt_, SQL_VARCHAR);
  CHECK_STMT_RESULT(rc_, "SQLGetTypeInfo failed", hstmt_);
  print_result_meta(hstmt_);
  get_result(hstmt_);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, GetTablesTest){
  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLTables(hstmt_, NULL, 0,
                 (SQLCHAR *) "public", SQL_NTS,
                 (SQLCHAR *) "%", SQL_NTS,
                 (SQLCHAR *) "TABLE", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLTables failed", hstmt_);
  print_result_meta(hstmt_);
  get_result(hstmt_);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, GetColumnsTest){
  SQLSMALLINT sql_column_ids[6] = {1, 2, 3, 4, 5, 6};

  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLColumns(hstmt_,
                  NULL, 0,
                  (SQLCHAR *) "", SQL_NTS,
                  (SQLCHAR *) "postgres.tpch.lineitem", SQL_NTS,
                  NULL, 0);
  CHECK_STMT_RESULT(rc_, "SQLColumns failed", hstmt_);
  print_result_meta(hstmt_);
  get_result_series(hstmt_, sql_column_ids, 6, -1);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, SpecialColumnsTest){
  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLSpecialColumns(hstmt_, SQL_ROWVER,
                         NULL, 0,
                         (SQLCHAR *) "public", SQL_NTS,
                         (SQLCHAR *) "postgres.tpch.lineitem", SQL_NTS,
                         SQL_SCOPE_SESSION,
                         SQL_NO_NULLS);
  CHECK_STMT_RESULT(rc_, "SQLSpecialColumns failed", hstmt_);
  print_result_meta(hstmt_);
  get_result(hstmt_);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, ColumnStatisticsTest){
  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLStatistics(hstmt_,
                     NULL, 0,
                     (SQLCHAR *) "public", SQL_NTS,
                     (SQLCHAR *) "postgres.tpch.lineitem", SQL_NTS,
                     0, 0);
  CHECK_STMT_RESULT(rc_, "SQLStatistics failed", hstmt_);
  print_result_meta(hstmt_);
  get_result(hstmt_);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, PrimaryKeysTest){
  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLPrimaryKeys(hstmt_,
                      NULL, 0,
                      (SQLCHAR *) "public", SQL_NTS,
                      (SQLCHAR *) "postgres.tpch.lineitem", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLPrimaryKeys failed", hstmt_);
  print_result_meta(hstmt_);
  get_result(hstmt_);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, ForeignKeysTest){
  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLForeignKeys(hstmt_,
                      NULL, 0,
                      (SQLCHAR *) "public", SQL_NTS,
                      (SQLCHAR *) "testtab1", SQL_NTS,
                      NULL, 0,
                      (SQLCHAR *) "public", SQL_NTS,
                      (SQLCHAR *) "testtab_fk", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLForeignKeys failed", hstmt_);
  print_result_meta(hstmt_);
  get_result(hstmt_);
  rc_ = SQLFreeStmt(hstmt_, SQL_CLOSE);
  CHECK_STMT_RESULT(rc_, "SQLFreeStmt failed", hstmt_);
}

TEST_F(CatalogFunctionsTest, GetInfoTest){
  char		buf[1000];
  SQLSMALLINT	len;

  rc_ = SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt_);
  EXPECT_TRUE(SQL_SUCCEEDED(rc_));

  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  /* Test SQLGetInfo */
  printf("Check for SQLGetInfo\n");
  rc_ = SQLGetInfo(conn, SQL_TABLE_TERM, buf, sizeof(buf), &len);
  CHECK_STMT_RESULT(rc_, "SQLGetInfo failed", hstmt_);
  printf("Term for \"table\": %s\n", buf);
}
