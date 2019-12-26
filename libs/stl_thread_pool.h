//
// Created by 孙嘉禾 on 2019/12/26.
//

#ifndef TOOLS_STL_THREAD_POOL_H
#define TOOLS_STL_THREAD_POOL_H

#include <thread>
#include <future>
#include <iostream>

#include "macros.h"
#include "thread_safe_structures.h"

namespace sss {

class JoinThreads {
 public:
  explicit JoinThreads(std::vector<std::thread> &threads) : threads_(threads) {}
  ~JoinThreads() {
      for (int i = 0; i < threads_.size(); ++i) {
          if (threads_[i].joinable()) {
              threads_[i].join();
          }
      }
  }
 private:
  std::vector<std::thread> &threads_;
};

class ThreadPool {
 public:
  using Task = FunctionWrapper;
  ThreadPool(int num_threads) : done_(false), num_threads_(num_threads), joiner(threads_) {
      for (unsigned i = 0; i < num_threads_; ++i) {
          queues_.push_back(std::unique_ptr<WorkStealingQueue>(new WorkStealingQueue));
          threads_.push_back(std::thread(&ThreadPool::WorkerThread, this, i));
      }
  }
  ~ThreadPool() { done_ = true; }
  template<typename FunctionType>
  std::future<typename std::result_of<FunctionType()>::type> Submit(FunctionType f) {
      using result_type = typename std::result_of<FunctionType()>::type;
      std::packaged_task<result_type()> task(f);
      std::future<result_type> res(task.get_future());
      if (local_work_queue_) {
          local_work_queue_->Push(std::move(task));
      } else {
          pool_work_queue_.Push(std::move(task));
      }
      return res;
  }
 private:
  void WorkerThread(unsigned my_index);
  void RunPendingTask();
  bool PopTaskFromLocalQueue(Task &task);
  bool PopTaskFromPoolQueue(Task &task);
  bool PopTaskFromOtherThreadQueue(Task &task);
  int num_threads_;
  ThreadSafeQueue<Task> pool_work_queue_;
  std::vector<std::unique_ptr<WorkStealingQueue>> queues_;
  static thread_local WorkStealingQueue *local_work_queue_;
  static thread_local unsigned my_index_;
  std::atomic<bool> done_;
  std::vector<std::thread> threads_;
  JoinThreads joiner;
};

}

#endif //TOOLS_STL_THREAD_POOL_H
