/*--------
 * Module:			select-test.cc
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *--------
 */
#include "common.h"

#include <chrono>
#include <atomic>
#include <thread>
#include <future>
#include <random>

namespace {
typedef struct {
  HSTMT statement;
  std::future<void> future;
} Task;

Task TestMultithreading(const std::function<int(void)> &sleep_after_sqlexecdirect, const std::function<int(void)> &sleep_after_sqlfetch,
                   int num_of_rows) {
  HSTMT hstmt = SQL_NULL_HSTMT;
  EXPECT_TRUE(SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, conn, &hstmt))) << "failed to allocate stmt handle";

  std::future<void> future = std::async(std::launch::async, [=]() {
    std::string query = "SELECT 0";
    for (int i = 1; i < num_of_rows; ++i) {
      query += " UNION ALL SELECT " + std::to_string(i * 100);
    }
    int rc = SQLExecDirect(hstmt, (SQLCHAR *) query.c_str(), SQL_NTS);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_after_sqlexecdirect()));

    EXPECT_TRUE(SQL_SUCCEEDED(rc)) << "SQLExecDirect failed";

    int rows = 0;
    while (SQL_SUCCEEDED(SQLFetch(hstmt))) {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_after_sqlfetch()));
      int32_t data;
      EXPECT_TRUE(SQL_SUCCEEDED(SQLGetData(hstmt, 1, SQL_C_LONG, &data, 0, nullptr)));

      EXPECT_EQ(rows * 100, data);
      rows++;
    }
    EXPECT_EQ(num_of_rows, rows);

    rc = SQLFreeStmt(hstmt, SQL_CLOSE);
    EXPECT_TRUE(SQL_SUCCEEDED(rc)) << "SQLFreeStmt failed";
  });

  return {hstmt, std::move(future)};
}
}


class MultithreadingTest : public  ::testing::Test{
public:
  void SetUp() override{
    std::string err_msg;
    connected_ = test_connect(&err_msg);
    EXPECT_TRUE(connected_) << err_msg;
  }

  void TearDown() override{
    std::string err_msg;
    if (connected_){
      EXPECT_TRUE(test_disconnect(&err_msg))<<err_msg;
    }
  }

protected:
  bool connected_;
  int			rc_;
};

TEST_F(MultithreadingTest, SingleConsumer_NoSleeps)
{
  auto sleep_after_sqlexecdirect = [] { return 0; };
  auto sleep_after_sqlfetch = [] { return 0; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 50);
  task.future.wait();
}

TEST_F(MultithreadingTest, SingleConsumer_ShortSleepsDuringQUery)
{
  auto sleep_after_sqlexecdirect = [] { return 100; };
  auto sleep_after_sqlfetch = [] { return 100; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);
  task.future.wait();
}

TEST_F(MultithreadingTest, SingleCOnsumer_LongSleepAfterSqlExecDirect)
{
  auto sleep_after_sqlexecdirect = [] { return 5000; };
  auto sleep_after_sqlfetch = [] { return 00; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);
  task.future.wait();
}

TEST_F(MultithreadingTest, SingleConsumer_LongSleepAfterSqlFetch)
{
  auto sleep_after_sqlexecdirect = [] { return 0; };
  auto sleep_after_sqlfetch = [] { return 1000; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);
  task.future.wait();
}

TEST_F(MultithreadingTest, MultipleConsumers_RandomSleeps)
{
  std::vector<Task> tasks;

  for (int i = 0; i < 50; ++i) {
    auto sleep_after_sqlexecdirect = [=] { return random() % 1000; };
    auto sleep_after_sqlfetch = [=] { return random() % 100; };
    Task task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);

    tasks.push_back(std::move(task));
  }

  for (const auto &task: tasks) {
    task.future.wait();
  }
}
