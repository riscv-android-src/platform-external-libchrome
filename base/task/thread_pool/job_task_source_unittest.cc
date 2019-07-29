// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/task/thread_pool/job_task_source.h"

#include <utility>

#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/task/thread_pool/test_utils.h"
#include "base/test/bind_test_util.h"
#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

// Verifies the normal flow of running 2 tasks one after the other.
TEST(ThreadPoolJobTaskSourceTest, RunTasks) {
  auto job_task = base::MakeRefCounted<test::MockJobTask>(
      DoNothing(), /* num_tasks_to_run */ 2);
  scoped_refptr<JobTaskSource> task_source =
      job_task->GetJobTaskSource(FROM_HERE, TaskPriority::BEST_EFFORT);

  TaskSource::Transaction task_source_transaction(
      task_source->BeginTransaction());
  {
    auto run_intent = task_source->WillRunTask();
    EXPECT_TRUE(run_intent);
    EXPECT_FALSE(run_intent.IsSaturated());

    auto task = task_source_transaction.TakeTask(&run_intent);
    std::move(task->task).Run();
    EXPECT_TRUE(task_source_transaction.DidProcessTask(std::move(run_intent)));
  }
  {
    auto run_intent = task_source->WillRunTask();
    EXPECT_TRUE(run_intent);
    EXPECT_TRUE(run_intent.IsSaturated());

    // An attempt to run an additional task is not allowed.
    EXPECT_FALSE(task_source->WillRunTask());
    auto task = task_source_transaction.TakeTask(&run_intent);
    EXPECT_FALSE(task_source->WillRunTask());

    std::move(task->task).Run();
    // Returns false because the task source is out of tasks.
    EXPECT_FALSE(task_source_transaction.DidProcessTask(std::move(run_intent)));
  }
}

// Verifies that a job task source doesn't get reenqueued when a task is not
// run.
TEST(ThreadPoolJobTaskSourceTest, SkipTask) {
  auto job_task = base::MakeRefCounted<test::MockJobTask>(
      DoNothing(), /* num_tasks_to_run */ 1);
  scoped_refptr<JobTaskSource> task_source =
      job_task->GetJobTaskSource(FROM_HERE, TaskPriority::BEST_EFFORT);

  TaskSource::Transaction task_source_transaction(
      task_source->BeginTransaction());

  auto run_intent = task_source->WillRunTask();
  EXPECT_TRUE(run_intent);
  EXPECT_TRUE(run_intent.IsSaturated());
  auto task = task_source_transaction.TakeTask(&run_intent);
  EXPECT_FALSE(task_source_transaction.DidProcessTask(
      std::move(run_intent), TaskSource::RunResult::kSkippedAtShutdown));
}

// Verifies that multiple tasks can run in parallel up to |max_concurrency|.
TEST(ThreadPoolJobTaskSourceTest, RunTasksInParallel) {
  auto job_task = base::MakeRefCounted<test::MockJobTask>(
      DoNothing(), /* num_tasks_to_run */ 2);
  scoped_refptr<JobTaskSource> task_source =
      job_task->GetJobTaskSource(FROM_HERE, TaskPriority::BEST_EFFORT);

  TaskSource::Transaction task_source_transaction(
      task_source->BeginTransaction());

  auto run_intent_a = task_source->WillRunTask();
  EXPECT_TRUE(run_intent_a);
  EXPECT_FALSE(run_intent_a.IsSaturated());
  auto task_a = task_source_transaction.TakeTask(&run_intent_a);

  auto run_intent_b = task_source->WillRunTask();
  EXPECT_TRUE(run_intent_b);
  EXPECT_TRUE(run_intent_b.IsSaturated());
  auto task_b = task_source_transaction.TakeTask(&run_intent_b);

  // WillRunTask() should return a null RunIntent once the max concurrency is
  // reached.
  EXPECT_FALSE(task_source->WillRunTask());

  std::move(task_a->task).Run();
  // Adding a task before closing the first run intent should cause the task
  // source to re-enqueue.
  job_task->SetNumTasksToRun(2);
  EXPECT_TRUE(task_source_transaction.DidProcessTask(std::move(run_intent_a)));

  std::move(task_b->task).Run();
  EXPECT_TRUE(task_source_transaction.DidProcessTask(std::move(run_intent_b)));

  auto run_intent_c = task_source->WillRunTask();
  EXPECT_TRUE(run_intent_c);
  EXPECT_TRUE(run_intent_c.IsSaturated());
  auto task_c = task_source_transaction.TakeTask(&run_intent_c);

  std::move(task_c->task).Run();
  EXPECT_FALSE(task_source_transaction.DidProcessTask(std::move(run_intent_c)));
}

TEST(ThreadPoolJobTaskSourceTest, InvalidTakeTask) {
  auto job_task =
      base::MakeRefCounted<test::MockJobTask>(DoNothing(),
                                              /* num_tasks_to_run */ 1);
  scoped_refptr<JobTaskSource> task_source =
      job_task->GetJobTaskSource(FROM_HERE, TaskPriority::BEST_EFFORT);
  TaskSource::Transaction task_source_transaction(
      task_source->BeginTransaction());

  auto run_intent_a = task_source->WillRunTask();
  auto run_intent_b = task_source->WillRunTask();
  EXPECT_FALSE(run_intent_b);
  // Can not be called with an invalid RunIntent.
  EXPECT_DCHECK_DEATH(
      { auto task = task_source_transaction.TakeTask(&run_intent_b); });

  auto task = task_source_transaction.TakeTask(&run_intent_a);
  task_source_transaction.DidProcessTask(std::move(run_intent_a));
}

TEST(ThreadPoolJobTaskSourceTest, InvalidDidProcessTask) {
  auto job_task =
      base::MakeRefCounted<test::MockJobTask>(DoNothing(),
                                              /* num_tasks_to_run */ 1);
  scoped_refptr<JobTaskSource> task_source =
      job_task->GetJobTaskSource(FROM_HERE, TaskPriority::BEST_EFFORT);
  TaskSource::Transaction task_source_transaction(
      task_source->BeginTransaction());

  auto run_intent = task_source->WillRunTask();
  EXPECT_TRUE(run_intent);
  // Can not be called before TakeTask().
  EXPECT_DCHECK_DEATH(
      task_source_transaction.DidProcessTask(std::move(run_intent)));

  auto task = task_source_transaction.TakeTask(&run_intent);
  task_source_transaction.DidProcessTask(std::move(run_intent));
}

}  // namespace internal
}  // namespace base
