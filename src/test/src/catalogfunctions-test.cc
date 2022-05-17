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
    std::string *err_msg = nullptr;
    EXPECT_TRUE(test_connect(err_msg)) << err_msg;

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

TEST_F(CatalogFunctionsTest, GetTypeInfoTest){
  /* Check for SQLGetTypeInfo */
  rc_ = SQLGetTypeInfo(hstmt_, SQL_VARCHAR);
  CHECK_STMT_RESULT(rc_, "SQLGetTypeInfo failed", hstmt_);
  std::string *err_msg = nullptr;
  std::string exp_result_meta = "TYPE_NAME: VARCHAR(1024) digits: 0, not nullable\n"
                           "DATA_TYPE: SMALLINT(2) digits: 0, not nullable\n"
                           "COLUMN_SIZE: INTEGER(4) digits: 0, nullable\n"
                           "LITERAL_PREFIX: VARCHAR(1024) digits: 0, nullable\n"
                           "LITERAL_SUFFIX: VARCHAR(1024) digits: 0, nullable\n"
                           "CREATE_PARAMS: VARCHAR(1024) digits: 0, nullable\n"
                           "NULLABLE: SMALLINT(2) digits: 0, not nullable\n"
                           "CASE_SENSITIVE: SMALLINT(2) digits: 0, not nullable\n"
                           "SEARCHABLE: SMALLINT(2) digits: 0, not nullable\n"
                           "UNSIGNED_ATTRIBUTE: SMALLINT(2) digits: 0, nullable\n"
                           "FIXED_PREC_SCALE: SMALLINT(2) digits: 0, not nullable\n"
                           "AUTO_UNIQUE_VALUE: SMALLINT(2) digits: 0, nullable\n"
                           "LOCAL_TYPE_NAME: VARCHAR(1024) digits: 0, nullable\n"
                           "MINIMUM_SCALE: SMALLINT(2) digits: 0, nullable\n"
                           "MAXIMUM_SCALE: SMALLINT(2) digits: 0, nullable\n"
                           "SQL_DATA_TYPE: SMALLINT(2) digits: 0, not nullable\n"
                           "SQL_DATETIME_SUB: SMALLINT(2) digits: 0, nullable\n"
                           "NUM_PREC_RADIX: INTEGER(4) digits: 0, nullable\n"
                           "INTERVAL_PRECISION: SMALLINT(2) digits: 0, nullable\n";
  std::string exp_result = "CHARACTER VARYING\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\nVARCHAR\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\nMAP\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\nANY\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\nNULL\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\nUNION\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\nARRAY\t12\t2147483647\t'\t'\tmax length\t1\t0\t3NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t0NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tVARCHARNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\t12NULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXNULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\n";
  EXPECT_STREQ(exp_result_meta.c_str(), print_result_meta(hstmt_, err_msg).c_str());
  EXPECT_STREQ(exp_result.c_str(), get_result(hstmt_, err_msg).c_str());
}

TEST_F(CatalogFunctionsTest, GetTablesTest){
  rc_ = SQLTables(hstmt_, NULL, 0,
                 (SQLCHAR *) "postgres.tpch", SQL_NTS,
                 (SQLCHAR *) "lineitem", SQL_NTS,
                 (SQLCHAR *) "TABLE", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLTables failed", hstmt_);
  std::string *err_msg = nullptr;
  std::string exp_result_meta = "TABLE_CAT: VARCHAR(1024) digits: 0, nullable\nTABLE_SCHEM: VARCHAR(1024) digits: 0, nullable\nTABLE_NAME: VARCHAR(1024) digits: 0, nullable\nTABLE_TYPE: VARCHAR(1024) digits: 0, nullable\nREMARKS: VARCHAR(1024) digits: 0, nullable\n";
  std::string exp_result = "NULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tTABLENULL\tXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\n";
  EXPECT_STREQ(exp_result_meta.c_str(), print_result_meta(hstmt_, err_msg).c_str());
  EXPECT_STREQ(exp_result.c_str(), get_result(hstmt_, err_msg).c_str());
}

TEST_F(CatalogFunctionsTest, GetColumnsTest){
  SQLSMALLINT sql_column_ids[6] = {1, 2, 3, 4, 5, 6};

  rc_ = SQLColumns(hstmt_,
                  NULL, 0,
                  (SQLCHAR *) "postgres.tpch", SQL_NTS,
                  (SQLCHAR *) "lineitem", SQL_NTS,
                  NULL, 0);
  CHECK_STMT_RESULT(rc_, "SQLColumns failed", hstmt_);
  std::string *err_msg = nullptr;
  std::string exp_result_meta = "TABLE_CAT: VARCHAR(1024) digits: 0, nullable\n"
                                "TABLE_SCHEM: VARCHAR(1024) digits: 0, nullable\n"
                                "TABLE_NAME: VARCHAR(1024) digits: 0, nullable\n"
                                "COLUMN_NAME: VARCHAR(1024) digits: 0, nullable\n"
                                "DATA_TYPE: SMALLINT(2) digits: 0, nullable\n"
                                "TYPE_NAME: VARCHAR(1024) digits: 0, nullable\n"
                                "COLUMN_SIZE: INTEGER(4) digits: 0, nullable\n"
                                "BUFFER_LENGTH: INTEGER(4) digits: 0, nullable\n"
                                "DECIMAL_DIGITS: SMALLINT(2) digits: 0, nullable\n"
                                "NUM_PREC_RADIX: SMALLINT(2) digits: 0, nullable\n"
                                "NULLABLE: SMALLINT(2) digits: 0, nullable\n"
                                "REMARKS: VARCHAR(1024) digits: 0, nullable\n"
                                "COLUMN_DEF: VARCHAR(1024) digits: 0, nullable\n"
                                "SQL_DATA_TYPE: SMALLINT(2) digits: 0, nullable\n"
                                "SQL_DATETIME_SUB: SMALLINT(2) digits: 0, nullable\n"
                                "CHAR_OCTET_LENGTH: INTEGER(4) digits: 0, nullable\n"
                                "ORDINAL_POSITION: INTEGER(4) digits: 0, nullable\n"
                                "IS_NULLABLE: VARCHAR(1024) digits: 0, nullable\n";
  std::string exp_result = "NULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_orderkey\t-5\tBIGINT\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_partkey\t4\tINTEGER\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_suppkey\t4\tINTEGER\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_linenumber\t4\tINTEGER\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_quantity\t3\tDECIMAL\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_extendedprice\t3\tDECIMAL\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_discount\t3\tDECIMAL\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_tax\t3\tDECIMAL\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_returnflag\t12\tCHARACTER VARYING\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_linestatus\t12\tCHARACTER VARYING\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_shipdate\t91\tDATE\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_commitdate\t91\tDATE\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_receiptdate\t91\tDATE\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_shipinstruct\t12\tCHARACTER VARYING\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_shipmode\t12\tCHARACTER VARYING\nNULLXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxX\tpostgres.tpch\tlineitem\tl_comment\t12\tCHARACTER VARYING\n";
  EXPECT_STREQ(exp_result_meta.c_str(), print_result_meta(hstmt_, err_msg).c_str());
  EXPECT_STREQ(exp_result.c_str(), get_result_series(hstmt_, sql_column_ids, 6, -1, err_msg).c_str());
}


TEST_F(CatalogFunctionsTest, GetInfoTest){
  char		buf[1000];
  SQLSMALLINT	len;

  /* Test SQLGetInfo */
  rc_ = SQLGetInfo(conn, SQL_ODBC_VER, buf, sizeof(buf), &len);
  CHECK_STMT_RESULT(rc_, "SQLGetInfo failed", hstmt_);
  EXPECT_STREQ("03.52", buf);

  rc_ = SQLGetInfo(conn, SQL_TABLE_TERM, buf, sizeof(buf), &len);
  CHECK_STMT_RESULT(rc_, "SQLGetInfo failed", hstmt_);
  EXPECT_STREQ("table", buf);
}

TEST_F(CatalogFunctionsTest, PrimaryKeysTest){
  rc_ = SQLPrimaryKeys(hstmt_,
                       NULL, 0,
                       (SQLCHAR *) "public", SQL_NTS,
                       (SQLCHAR *) "postgres.tpch.lineitem", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLPrimaryKeys failed", hstmt_);
  std::string *err_msg = nullptr;
  std::cout<<print_result_meta(hstmt_, err_msg);
  get_result(hstmt_, err_msg);
}

TEST_F(CatalogFunctionsTest, ForeignKeysTest){
  SQLExecDirect(hstmt_, (SQLCHAR *) "drop table if exists testtab2", SQL_NTS);

  rc_ = SQLForeignKeys(hstmt_,
                       NULL, 0,
                       (SQLCHAR *) "public", SQL_NTS,
                       (SQLCHAR *) "testtab1", SQL_NTS,
                       NULL, 0,
                       (SQLCHAR *) "public", SQL_NTS,
                       (SQLCHAR *) "testtab_fk", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLForeignKeys failed", hstmt_);
  std::string *err_msg = nullptr;
  std::cout<<print_result_meta(hstmt_, err_msg);
  get_result(hstmt_, err_msg);
}

TEST_F(CatalogFunctionsTest, DISABLED_ColumnStatisticsTest){
  rc_ = SQLStatistics(hstmt_,
                     NULL, 0,
                     (SQLCHAR *) "public", SQL_NTS,
                     (SQLCHAR *) "postgres.tpch.lineitem", SQL_NTS,
                     0, 0);
  CHECK_STMT_RESULT(rc_, "SQLStatistics failed", hstmt_);
  std::string *err_msg = nullptr;
  std::cout<<print_result_meta(hstmt_, err_msg);
  get_result(hstmt_, err_msg);
}
