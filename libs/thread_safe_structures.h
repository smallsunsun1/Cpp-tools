//
// Created by 孙嘉禾 on 2019/12/26.
//

#ifndef TOOLS_THREAD_SAFE_STRUCTURES_H
#define TOOLS_THREAD_SAFE_STRUCTURES_H

#include <mutex>
#include <deque>
#include <queue>
#include "macros.h"

namespace sss {

class FunctionWrapper {
 public:
  template<typename F>
  FunctionWrapper(F &&f):impl_(std::make_unique<ImplType < F>>
  (
  std::move(f)
  )) {}
  void operator()() { impl_->call(); }
  FunctionWrapper() = default;
  FunctionWrapper(FunctionWrapper &&other) : impl_(std::move(other.impl_)) {}
  FunctionWrapper &operator=(FunctionWrapper &&other) {
      impl_ = std::move(other.impl_);
      return *this;
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(FunctionWrapper);
  struct ImplBase {
    virtual void call() = 0;
    virtual ~ImplBase() {}
  };
  std::unique_ptr<ImplBase> impl_;
  template<typename F>
  struct ImplType : ImplBase {
    F f;
    explicit ImplType(F &&f_) : f(std::move(f_)) {}
    void call() override { f(); }
  };
};

template<typename T>
class ThreadSafeQueue {
 public:
  bool TryPop(T& t){
      std::lock_guard<std::mutex> lock(mu_);
      if (queue_.empty()){
          return false;
      }
      t = std::move(queue_.front());
      queue_.pop();
      return true;
  }
  bool Empty() const {
      std::lock_guard<std::mutex> lock(mu_);
      return queue_.empty();
  }
  void Push(T&& t) {
      std::lock_guard<std::mutex> lock(mu_);
      queue_.push(std::move(t));
  }
  void Push(T& t){
      std::lock_guard<std::mutex> lock(mu_);
      queue_.push(std::move(t));
  }
 private:
  std::queue<T> queue_;
  mutable std::mutex mu_;
};

class WorkStealingQueue {
 public:
  using data_type = FunctionWrapper;
  WorkStealingQueue(){}
  void Push(data_type data);
  bool Empty() const;
  bool TryPop(data_type& res);
  bool TrySteal(data_type& res);
 private:
  DISALLOW_COPY_AND_ASSIGN(WorkStealingQueue);
  std::deque<data_type> the_queue_;
  mutable std::mutex mu_;
};



template <typename T>
class NonBlockingThreadSafeQueue {

};

}

#endif //TOOLS_THREAD_SAFE_STRUCTURES_H
