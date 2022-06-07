/*--------
* Module:			catalogfunctions-test.cc
*
* Comments:		See "readme.txt" for copyright and license information.
*                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
*--------
*/

/*
 * This test is used to check the output and processing of the catalog
 * functions listed as below:
 * - SQLGetTypeInfo
 * - SQLTables
 * - SQLColumns
 * - SQLStatistics
 * - SQLPrimaryKeys
 * - SQLForeignKeys
 * - SQLGetInfo
 */
#include "common.h"

class CatalogFunctionsTest : public  ::testing::Test{
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

TEST_F(CatalogFunctionsTest, GetTypeInfoTest){
  /* Check for SQLGetTypeInfo */
  rc_ = SQLGetTypeInfo(hstmt_, SQL_VARCHAR);
  CHECK_STMT_RESULT(rc_, "SQLGetTypeInfo failed", hstmt_);
  std::string err_msg;
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
  std::vector<std::string> exp_result = {"CHARACTER VARYING\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n",
                                         "VARCHAR\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n",
                                         "MAP\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n",
                                         "ANY\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n",
                                         "NULL\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n",
                                         "UNION\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n",
                                         "ARRAY\t12\t2147483647\t'\t'\tmax length\t1\t0\t3\tNULL\t0\tNULL\tVARCHAR\tNULL\tNULL\t12\tNULL\tNULL\tNULL\n"};
  std::string result = get_result(hstmt_, &err_msg).value();
  for (std::string type: exp_result) {
    EXPECT_TRUE(result.find(type) != std::string::npos);
  }
  EXPECT_EQ(exp_result_meta, print_result_meta(hstmt_, &err_msg).value());
}

TEST_F(CatalogFunctionsTest, GetTablesTest){
  rc_ = SQLTables(hstmt_, NULL, 0,
                 (SQLCHAR *) "postgres.tpch", SQL_NTS,
                 (SQLCHAR *) "lineitem", SQL_NTS,
                 (SQLCHAR *) "TABLE", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLTables failed", hstmt_);
  std::string err_msg;
  std::string exp_result_meta = "TABLE_CAT: VARCHAR(1024) digits: 0, nullable\nTABLE_SCHEM: VARCHAR(1024) digits: 0, nullable\nTABLE_NAME: VARCHAR(1024) digits: 0, nullable\nTABLE_TYPE: VARCHAR(1024) digits: 0, nullable\nREMARKS: VARCHAR(1024) digits: 0, nullable\n";
  std::string exp_result = "NULL\tpostgres.tpch\tlineitem\tTABLE\tNULL\n";
  EXPECT_EQ(exp_result_meta, print_result_meta(hstmt_, &err_msg).value());
  EXPECT_EQ(exp_result, get_result(hstmt_, &err_msg).value())<<err_msg;
}

// Tests specific case for SQLTables where SchemaName is SQL_ALL_SCHEMAS and other parameters are empty strings.
// According to the ODBC spec SQLTables should return only the schemas present on source and NULL for all other columns.
TEST_F(CatalogFunctionsTest, GetTablesTest_AllSchemas){
  rc_ = SQLTables(hstmt_, (SQLCHAR *) "", 0,
                  (SQLCHAR *) SQL_ALL_SCHEMAS, 1,
                  (SQLCHAR *) "", 0,
                  (SQLCHAR *) "", 0);
  CHECK_STMT_RESULT(rc_, "SQLTables failed", hstmt_);
  std::string err_msg;

  const std::string &results = get_result(hstmt_, &err_msg).value();

  // Expect that result comes with all columns NULL except for the second one (schema name)
  EXPECT_TRUE(results.find("NULL\tINFORMATION_SCHEMA\tNULL\tNULL\tNULL") != std::string::npos);
}

// Tests that SQLTables function filters by TableType if this argument not empty
TEST_F(CatalogFunctionsTest, GetTablesTest_SpecificTableTypes){
  rc_ = SQLTables(hstmt_, nullptr, 0,
                  nullptr, 0,
                  nullptr, 0,
                  (SQLCHAR *) "SYSTEM_TABLE", 12);
  CHECK_STMT_RESULT(rc_, "SQLTables failed", hstmt_);
  std::string err_msg;

  std::string results = std::move(get_result(hstmt_, &err_msg).value());
  std::stringstream results_ss(results);
  std::string segment;
  std::vector<std::string> lines;
  while(std::getline(results_ss, segment, '\n')) {
    lines.push_back(segment);
  }

  // Expect result to be non-empty.
  EXPECT_TRUE(!lines.empty());
  // Expect SYSTEM_TABLE to be on all returned rows
  for (const auto &line: lines) {
    EXPECT_TRUE(line.find("\tSYSTEM_TABLE\t") != std::string::npos);
  }
}

TEST_F(CatalogFunctionsTest, GetColumnsTest){
  SQLSMALLINT sql_column_ids[6] = {1, 2, 3, 4, 5, 6};

  rc_ = SQLColumns(hstmt_,
                  NULL, 0,
                  (SQLCHAR *) "postgres.tpch", SQL_NTS,
                  (SQLCHAR *) "lineitem", SQL_NTS,
                  NULL, 0);
  CHECK_STMT_RESULT(rc_, "SQLColumns failed", hstmt_);
  std::string err_msg;
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
  std::string exp_result = "NULL\tpostgres.tpch\tlineitem\tl_orderkey\t-5\tBIGINT\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_partkey\t4\tINTEGER\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_suppkey\t4\tINTEGER\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_linenumber\t4\tINTEGER\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_quantity\t3\tDECIMAL\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_extendedprice\t3\tDECIMAL\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_discount\t3\tDECIMAL\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_tax\t3\tDECIMAL\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_returnflag\t12\tCHARACTER VARYING\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_linestatus\t12\tCHARACTER VARYING\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_shipdate\t91\tDATE\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_commitdate\t91\tDATE\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_receiptdate\t91\tDATE\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_shipinstruct\t12\tCHARACTER VARYING\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_shipmode\t12\tCHARACTER VARYING\n"
                           "NULL\tpostgres.tpch\tlineitem\tl_comment\t12\tCHARACTER VARYING\n";
  EXPECT_EQ(exp_result_meta, print_result_meta(hstmt_, &err_msg).value());
  EXPECT_EQ(exp_result, get_result_series(hstmt_, sql_column_ids, 6, -1, &err_msg).value());
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

TEST_F(CatalogFunctionsTest, DISABLED_PrimaryKeysTest){
  rc_ = SQLPrimaryKeys(hstmt_,
                       NULL, 0,
                       (SQLCHAR *) "postgres.tpch", SQL_NTS,
                       (SQLCHAR *) "lineitem", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLPrimaryKeys failed", hstmt_);
  std::string err_msg;
  std::string exp_result_meta = "TABLE_CAT: WVARCHAR(1024) digits: 0, nullable\n"
                                "TABLE_SCHEM: WVARCHAR(1024) digits: 0, nullable\n"
                                "TABLE_NAME: WVARCHAR(1024) digits: 0, not nullable\n"
                                "COLUMN_NAME: WVARCHAR(1024) digits: 0, not nullable\n"
                                "KEY_SEQ: SMALLINT(5) digits: 0, not nullable\n"
                                "PK_NAME: WVARCHAR(1024) digits: 0, nullable\n";
  std::string exp_result = "";
  EXPECT_EQ(exp_result_meta, print_result_meta(hstmt_, &err_msg).value());
  EXPECT_EQ(exp_result, get_result(hstmt_, &err_msg).value());
}

TEST_F(CatalogFunctionsTest, DISABLED_ForeignKeysTest){
  rc_ = SQLForeignKeys(hstmt_,
                       NULL, 0,
                       (SQLCHAR *) "postgres.tpch", SQL_NTS,
                       (SQLCHAR *) "lineitem", SQL_NTS,
                       NULL, 0,
                       (SQLCHAR *) "postgres.tpch", SQL_NTS,
                       (SQLCHAR *) "lineitem", SQL_NTS);
  CHECK_STMT_RESULT(rc_, "SQLForeignKeys failed", hstmt_);
  std::string err_msg;
  std::string exp_result_meta = "PKTABLE_CAT: WVARCHAR(1024) digits: 0, nullable\nPKTABLE_SCHEM: WVARCHAR(1024) digits: 0, nullable\nPKTABLE_NAME: WVARCHAR(1024) digits: 0, not nullable\nPKCOLUMN_NAME: WVARCHAR(1024) digits: 0, not nullable\nFKTABLE_CAT: WVARCHAR(1024) digits: 0, nullable\nFKTABLE_SCHEM: WVARCHAR(1024) digits: 0, nullable\nFKTABLE_NAME: WVARCHAR(1024) digits: 0, not nullable\nFKCOLUMN_NAME: WVARCHAR(1024) digits: 0, not nullable\nKEY_SEQ: SMALLINT(5) digits: 0, not nullable\nUPDATE_RULE: SMALLINT(5) digits: 0, nullable\nDELETE_RULE: SMALLINT(5) digits: 0, nullable\nFK_NAME: WVARCHAR(1024) digits: 0, nullable\nPK_NAME: WVARCHAR(1024) digits: 0, nullable\nDEFERRABILITY: SMALLINT(5) digits: 0, nullable\n";
  std::string exp_result = "";
  EXPECT_EQ(exp_result_meta, print_result_meta(hstmt_, &err_msg).value());
  EXPECT_EQ(exp_result, get_result(hstmt_, &err_msg).value());
}

TEST_F(CatalogFunctionsTest, DISABLED_ColumnStatisticsTest){
  rc_ = SQLStatistics(hstmt_,
                     NULL, 0,
                     (SQLCHAR *) "postgres.tpch", SQL_NTS,
                     (SQLCHAR *) "lineitem", SQL_NTS,
                     0, 0);
  CHECK_STMT_RESULT(rc_, "SQLStatistics failed", hstmt_);
  std::string err_msg;
  std::string exp_result_meta = "TABLE_CAT: WVARCHAR(1024) digits: 0, nullable\n"
                                "TABLE_SCHEM: WVARCHAR(1024) digits: 0, nullable\n"
                                "TABLE_NAME: WVARCHAR(1024) digits: 0, not nullable\n"
                                "NON_UNIQUE: SMALLINT(5) digits: 0, nullable\n"
                                "INDEX_QUALIFIER: WVARCHAR(255) digits: 0, nullable\n"
                                "INDEX_NAME: WVARCHAR(1024) digits: 0, nullable\n"
                                "TYPE: SMALLINT(5) digits: 0, not nullable\n"
                                "ORDINAL_POSITION: INTEGER(10) digits: 0, not nullable\n"
                                "COLUMN_NAME: WVARCHAR(1024) digits: 0, not nullable\n"
                                "ASC_OR_DESC: WCHAR(1) digits: 0, nullable\n"
                                "CARDINALITY: INTEGER(10) digits: 0, nullable\n"
                                "PAGES: INTEGER(10) digits: 0, nullable\n"
                                "FILTER_CONDITION: WVARCHAR(1024) digits: 0, nullable\n";
  std::string exp_result = "";
  EXPECT_EQ(exp_result_meta, print_result_meta(hstmt_, &err_msg).value());
  EXPECT_EQ(exp_result, get_result(hstmt_, &err_msg).value());
}
