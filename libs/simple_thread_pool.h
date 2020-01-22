//
// Created by 孙嘉禾 on 2020/1/22.
//

#ifndef TOOLS_LIBS_SIMPLE_THREAD_POOL_H_
#define TOOLS_LIBS_SIMPLE_THREAD_POOL_H_

#include <mutex>
#include <condition_variable>
#include <random>
#include <functional>
#include <queue>
#include <thread>

class FixedThreadPool {
 public:
  typedef std::function<void()> Task;
  explicit FixedThreadPool(size_t num_threads) : data_(std::make_unique<Data>()), num_jobs_(0) {
    for (size_t i = 0; i < num_threads; ++i) {
      threads_.emplace_back([this]() {
        std::unique_lock<std::mutex> lk(data_->mu_);
        for (;;) {
          if (!data_->tasks_.empty()) {
            auto current = std::move(data_->tasks_.front());
            data_->tasks_.pop_front();
            lk.unlock();
            current();
            num_jobs_.fetch_sub(1);
            lk.lock();
          } else if (data_->is_shutdown_) {
            break;
          } else {
            data_->cv_.wait(lk);
          }
        }
      });
    }
  }
  FixedThreadPool() = default;
  ~FixedThreadPool() {
    if (data_) {
      std::lock_guard<std::mutex> lk(data_->mu_);
      data_->is_shutdown_ = true;
    }
    data_->cv_.notify_all();
    for (auto &value: threads_) {
      if (value.joinable())
        value.join();
    }
  }
  template<typename F>
  void Execute(F &&task) {
    {
      std::lock_guard<std::mutex> lk(data_->mu_);
      data_->tasks_.emplace_back(std::forward<F>(task));
      num_jobs_.fetch_add(1);
    }
    data_->cv_.notify_one();
  }
  unsigned NumJobs() {
    return num_jobs_.load();
  }
  void NotifyAll() {
    data_->cv_.notify_all();
  }
 private:
  struct Data {
    std::mutex mu_;
    std::condition_variable cv_;
    bool is_shutdown_ = false;
    std::deque<Task> tasks_;
  };
  std::unique_ptr<Data> data_;
  std::vector<std::thread> threads_;
  std::atomic<unsigned> num_jobs_{};
};

class WorkStealingThreadPool {
 public:
  explicit WorkStealingThreadPool(unsigned num_threads) : num_threads_(num_threads), num_jobs_(0), mt_(dev_()) {
    for (unsigned i = 0; i < num_threads_; ++i) {
      queues_.push_back(std::make_unique<Data>());
      threads_.emplace_back([this](int index) {
        index_ = index;
        local_queue_ = queues_[index_].get();
        while (!done_ && !local_queue_->is_shutdown_) {
          RunTask();
        }
      }, i);
    }
  }
  ~WorkStealingThreadPool() {
    done_ = true;
    for (auto &thread: threads_) {
      thread.join();
    }
  }
  template<typename F>
  void Submit(F &&f) {
    unsigned indices = dis_(mt_) % queues_.size();
    {
      std::lock_guard<std::mutex> lock_guard(queues_[indices]->mu_);
      queues_[indices]->tasks_.emplace_back(f);
    }
    num_jobs_.fetch_add(1);
  }
  unsigned NumJobs() {
    return num_jobs_;
  }
  using Task = std::function<void()>;
 private:
  void RunTask();
  bool PopTaskFromLocalQueue(Task &task);
  bool PopTaskFromOtherThreadQueue(Task &task);
  struct Data {
    std::mutex mu_;
    std::condition_variable cv_;
    bool is_shutdown_ = false;
    std::deque<Task> tasks_;
  };
  std::vector<std::unique_ptr<Data>> queues_;
  static thread_local Data *local_queue_;
  static thread_local unsigned index_;
  std::vector<std::thread> threads_;
  unsigned num_threads_;
  std::atomic<unsigned> num_jobs_;
  std::random_device dev_;
  std::uniform_int_distribution<> dis_;
  std::mt19937 mt_;
  bool done_ = false;
};

#endif //TOOLS_LIBS_SIMPLE_THREAD_POOL_H_
