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
  std::future<void> future;
  std::function<void()> Cancel;
} Task;

Task TestMultithreading(const std::function<int(void)> &sleep_after_sqlexecdirect,
                        const std::function<int(void)> &sleep_after_sqlfetch,
                        int num_of_rows) {
  HSTMT statement = SQL_NULL_HSTMT;
  EXPECT_TRUE(SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, conn, &statement)));

  std::shared_ptr<std::atomic<bool>> was_cancelled(new std::atomic<bool>(false));
  auto Cancel = [=]() {
    *was_cancelled = true;
    EXPECT_TRUE(SQL_SUCCEEDED(SQLCancel(statement)));
  };
  auto Close = [=]() {
    EXPECT_TRUE(SQL_SUCCEEDED(SQLFreeStmt(statement, SQL_CLOSE)));
  };

  std::future<void> future = std::async(std::launch::async, [=]() {
    std::string query = "SELECT 0";
    for (int i = 1; i < num_of_rows; ++i) {
      query += " UNION ALL SELECT " + std::to_string(i * 100);
    }
    EXPECT_TRUE(SQL_SUCCEEDED(SQLExecDirect(statement, (SQLCHAR *) query.c_str(), SQL_NTS)) || *was_cancelled);
    if (*was_cancelled) return Close();

    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_after_sqlexecdirect()));

    int rows = 0;
    while (SQL_SUCCEEDED(SQLFetch(statement))) {
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_after_sqlfetch()));
      int32_t data;
      EXPECT_TRUE(SQL_SUCCEEDED(SQLGetData(statement, 1, SQL_C_LONG, &data, 0, nullptr)) || *was_cancelled);
      if (*was_cancelled) return Close();

      EXPECT_EQ(rows * 100, data);
      rows++;
    }
    if (*was_cancelled) return Close();

    EXPECT_EQ(num_of_rows, rows);

    Close();
  });

  return {std::move(future), Cancel};
}
}


class MultithreadingTest : public ::testing::Test {
public:
  void SetUp() override {
    std::string err_msg;
    connected_ = test_connect(&err_msg);
    EXPECT_TRUE(connected_) << err_msg;
  }

  void TearDown() override {
    std::string err_msg;
    if (connected_) {
      EXPECT_TRUE(test_disconnect(&err_msg)) << err_msg;
    }
  }

protected:
  bool connected_;
};

TEST_F(MultithreadingTest, SingleConsumer_NoSleeps) {
  auto sleep_after_sqlexecdirect = [] { return 0; };
  auto sleep_after_sqlfetch = [] { return 0; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 50);
  task.future.wait();
}

TEST_F(MultithreadingTest, SingleConsumer_ShortSleepsDuringQUery) {
  auto sleep_after_sqlexecdirect = [] { return 100; };
  auto sleep_after_sqlfetch = [] { return 100; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);
  task.future.wait();
}

TEST_F(MultithreadingTest, SingleCOnsumer_LongSleepAfterSqlExecDirect) {
  auto sleep_after_sqlexecdirect = [] { return 5000; };
  auto sleep_after_sqlfetch = [] { return 00; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);
  task.future.wait();
}

TEST_F(MultithreadingTest, SingleConsumer_LongSleepAfterSqlFetch) {
  auto sleep_after_sqlexecdirect = [] { return 0; };
  auto sleep_after_sqlfetch = [] { return 1000; };
  auto task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);
  task.future.wait();
}

TEST_F(MultithreadingTest, MultipleConsumers_RandomSleeps) {
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

TEST_F(MultithreadingTest, MultipleConsumers_RandomSleeps_Cancelling) {
  int number_of_tasks = 50;
  std::vector<Task> tasks(number_of_tasks);
  std::vector<std::atomic<bool>> was_cancelled_vector(number_of_tasks);

  for (int i = 0; i < number_of_tasks; ++i) {
    auto sleep_after_sqlexecdirect = [=] { return random() % 1000; };
    auto sleep_after_sqlfetch = [=] { return random() % 100; };
    Task task = TestMultithreading(sleep_after_sqlexecdirect, sleep_after_sqlfetch, 10);

    tasks[i] = std::move(task);
  }

  for (int i = 0; i < number_of_tasks; ++i) {
    const auto &task = tasks[i];
    if (random() % 2 == 0) {
      task.Cancel();
    }

    task.future.wait();
  }
}
