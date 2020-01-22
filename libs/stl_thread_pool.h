//
// Created by 孙嘉禾 on 2019/12/26.
//

#ifndef TOOLS_STL_THREAD_POOL_H
#define TOOLS_STL_THREAD_POOL_H

#include <memory>
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
    for (auto &thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }
 private:
  std::vector<std::thread> &threads_;
};

class ThreadPool {
 public:
  using Task = FunctionWrapper;
  static const unsigned kMaxThreads;
  static const unsigned kMaxPoolSize;
  explicit ThreadPool(int num_threads)
      : done_(false), mt_(dev_()), num_threads_(num_threads), joiner(threads_), num_jobs_(0),
      queue_size_(0){
    assert(num_threads <= kMaxThreads);
    request_signals_.reserve(kMaxThreads);
    queues_.reserve(kMaxThreads);
    threads_.reserve(kMaxThreads);
    for (unsigned i = 0; i < num_threads_; ++i) {
      request_signals_.push_back((std::make_unique<bool>(false)));
      queues_.push_back(std::make_unique<WorkStealingQueue>());
      queue_size_.store(queues_.size());
      threads_.emplace_back(&ThreadPool::WorkerThread, this, i);
    }
    dis_ = std::uniform_int_distribution<>(0, kMaxThreads);
  }
  ~ThreadPool() {
    done_ = true;
    cv_.notify_all();
  }
  void Cancel();
  bool StartNewThread();
  bool CancelThread();
//  bool CancelThread() {
//    std::cout << "Current Dont't Supported Delete Threads In Pool, Use"
//                 "Cancel To Destroy ThreadPool And Launch Another ThreadPool !"
//              << std::endl;
//    return false;
//  }
  int NumJobs() { return num_jobs_.load(); }
  int NumThreads() { return num_threads_; }
  void NotifyAll(){cv_.notify_all();}
  template<typename FunctionType, typename... Args>
  std::future<typename std::result_of<FunctionType(Args...)>::type> Submit(FunctionType f, Args...args) {
    using result_type = typename std::result_of<FunctionType(Args...)>::type;
    std::future<result_type> res;
    {
      auto func = std::bind(std::forward<FunctionType>(f), std::forward<Args>(args)...);
      std::packaged_task<result_type()> task(func);
      res = std::move(task.get_future());
      if (pool_work_queue_.Size() > kMaxPoolSize) {
        unsigned indices = dis_(mt_) % queue_size_.load();
        queues_[indices].get()->Push(std::move(task));
      } else {
        pool_work_queue_.Push(std::move(task));
      }
      num_jobs_.fetch_add(1);
    }
    cv_.notify_one();
    return res;
  }
 private:
  void WorkerThread(unsigned my_index);
  void RunPendingTask();
  static bool PopTaskFromLocalQueue(Task &task);
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
  std::atomic<unsigned int> queue_size_;
  std::atomic<int> num_jobs_;
  std::random_device dev_;
  std::uniform_int_distribution<> dis_;
  std::mt19937 mt_;
  std::mutex mu_;
  std::condition_variable cv_;
  std::vector<std::thread> threads_;
  JoinThreads joiner;
};

}

#endif //TOOLS_STL_THREAD_POOL_H
