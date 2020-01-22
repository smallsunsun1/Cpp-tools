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
  FunctionWrapper(F &&f):impl_(std::make_unique<ImplType < F>>(std::forward<F>(f))) {}
  void operator()() { impl_->call(); }
  FunctionWrapper() = default;
  FunctionWrapper(FunctionWrapper &&other) noexcept : impl_(std::move(other.impl_)) {}
  FunctionWrapper &operator=(FunctionWrapper &&other) {
      impl_ = std::move(other.impl_);
      return *this;
  }
 private:
  DISALLOW_COPY_AND_ASSIGN(FunctionWrapper);
  struct ImplBase {
    virtual void call() = 0;
    virtual ~ImplBase() = default;
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
  bool TryPop(T &t) {
      std::lock_guard<std::mutex> lock(mu_);
      if (queue_.empty()) {
          return false;
      }
      t = std::move(queue_.front());
      queue_.pop();
      return true;
  }
  unsigned long Size() const {
    std::lock_guard<std::mutex> lock(mu_);
    return queue_.size();
  }
  bool Empty() const {
      std::lock_guard<std::mutex> lock(mu_);
      return queue_.empty();
  }
  void Push(T &&t) {
      std::lock_guard<std::mutex> lock(mu_);
      queue_.push(std::move(t));
  }
  void Push(T &t) {
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
  WorkStealingQueue() = default;
  void Push(data_type& data);
  void Push(data_type&& data);
  bool Empty() const;
  bool TryPop(data_type &res);
  size_t Size() const;
  bool TrySteal(data_type &res);
 private:
  DISALLOW_COPY_AND_ASSIGN(WorkStealingQueue);
  std::deque<data_type> the_queue_;
  mutable std::mutex mu_;
};

template<typename T>
class NonBlockingThreadSafeQueue {

};

template<typename T>
class LockFreeStack {
 public:
  void Push(const T &data) {
      Node *new_node = new Node(data);
      new_node->next = head_.load();
      while (!head_.compare_exchange_weak(new_node->next, new_node));
  }
  std::shared_ptr<T> Pop() {
      ++threads_in_pop_;
      Node *old_head = head_.load();
      while (old_head && !head_.compare_exchange_weak(old_head, old_head->next));
      std::shared_ptr<T> res;
      if (old_head) {
          res.swap(old_head->data);
      }
      TryReclaim(old_head);  // 回收对应节点
      return res;
  }
 private:
  struct Node {
    std::shared_ptr<T> data;
    Node *next;
    explicit Node(const T &data_) : data(std::make_shared<T>(data_)) {}
  };
  static void DeleteNodes(Node *nodes) {
      while (nodes) {
          Node *next = nodes->next;
          delete nodes;
          nodes = next;
      }
  }
  void ChainPendingNodes(Node* first, Node* last){
      last->next = to_be_deleted_;
      while (!to_be_deleted_.compare_exchange_weak(last->next, first));
  }
  void ChainPendingNodes(Node *nodes) {
      Node* last = nodes;
      while (Node* next = last->next) {
          last = next;
      }
      ChainPendingNodes(nodes, last);
  }
  void ChainPendingNode(Node* n){
      ChainPendingNodes(n, n);
  }
  void TryReclaim(Node *old_head) {
      if (threads_in_pop_ == 1) {
          Node *nodes_to_delete = to_be_deleted_.exchange(nullptr);
          if (!--threads_in_pop_) {
              DeleteNodes(nodes_to_delete);
          } else if (nodes_to_delete){
              ChainPendingNodes(nodes_to_delete);
          }
          delete old_head;
      } else {
          ChainPendingNode(old_head);
          --threads_in_pop_;
      }
  }
  std::atomic<unsigned> threads_in_pop_;
  std::atomic<Node *> head_;
  std::atomic<Node *> to_be_deleted_;
};

}

#endif //TOOLS_THREAD_SAFE_STRUCTURES_H
