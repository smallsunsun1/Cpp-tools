//
// Created by 孙嘉禾 on 2019/12/26.
//

#ifndef TOOLS_STL_THREAD_POOL_H
#define TOOLS_STL_THREAD_POOL_H

#include <thread>
#include <future>
#include <iostream>
#include <random>

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
  ThreadPool(int num_threads) : done_(false), mt_(dev_()), num_threads_(num_threads), joiner(threads_) {
      for (unsigned i = 0; i < num_threads_; ++i) {
          request_signals_.push_back((std::make_unique<bool>(false)));
          queues_.push_back(std::unique_ptr<WorkStealingQueue>(new WorkStealingQueue));
          threads_.push_back(std::thread(&ThreadPool::WorkerThread, this, i));
      }
      queues_.push_back(std::unique_ptr<WorkStealingQueue>(new WorkStealingQueue));
      request_signals_.push_back((std::make_unique<bool>(false)));
      local_work_queue_ = queues_.back().get();
      dis_ = std::move(std::uniform_int_distribution<>(0, queues_.size() - 1));
  }
  ~ThreadPool() { done_ = true; }
  bool StartNewThread();
  bool CancelThread();
  void Cancel();

  template<typename FunctionType, typename... Args>
  std::future<typename std::result_of<FunctionType(Args...)>::type> Submit(FunctionType f, Args...args) {
      using result_type = typename std::result_of<FunctionType(Args...)>::type;
      auto func = std::bind(std::forward<FunctionType>(f), std::forward<Args>(args)...);
      std::packaged_task<result_type()> task(func);
      std::future<result_type> res(task.get_future());
      if (!pool_work_queue_.Empty()) {
//          std::cout << "local queue" << std::endl;
          int indices = dis_(mt_);
          queues_[indices].get()->Push(std::move(task));
//            queues_[indices].get()->Push([&task]{task();});
      } else {
//          std::cout << "here we are" << std::endl;
          pool_work_queue_.Push(std::move(task));
//            pool_work_queue_.Push([&task]{task();});
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
  std::vector<std::unique_ptr<bool>> request_signals_;
  static thread_local WorkStealingQueue *local_work_queue_;
  static thread_local unsigned my_index_;
  static thread_local bool *request_stop_;
  std::atomic<bool> done_;
  std::random_device dev_;
  std::uniform_int_distribution<> dis_;
  std::mt19937 mt_;
  std::vector<std::thread> threads_;
  JoinThreads joiner;
};

}

#endif //TOOLS_STL_THREAD_POOL_H
