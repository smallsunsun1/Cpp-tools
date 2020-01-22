//
// Created by 孙嘉禾 on 2020/1/22.
//

#include "simple_thread_pool.h"

thread_local WorkStealingThreadPool::Data* WorkStealingThreadPool::local_queue_;
thread_local unsigned WorkStealingThreadPool::index_;

bool WorkStealingThreadPool::PopTaskFromLocalQueue(WorkStealingThreadPool::Task &task) {
  if (!local_queue_)
    return false;
  else {
    std::lock_guard<std::mutex> lock_guard(local_queue_->mu_);
    if (local_queue_->tasks_.empty())
      return false;
    task = local_queue_->tasks_.front();
    local_queue_->tasks_.pop_front();
    return true;
  }
}

bool WorkStealingThreadPool::PopTaskFromOtherThreadQueue(WorkStealingThreadPool::Task &task) {
  for (unsigned int i = 0; i < queues_.size(); ++i) {
    unsigned const index = (index_ + i + 1) % queues_.size();
    std::lock_guard<std::mutex> lock_guard(queues_[index]->mu_);
    if (!queues_[index]->tasks_.empty()){
      task = queues_[index]->tasks_.front();
      queues_[index]->tasks_.pop_front();
      return true;
    }
  }
  return false;
}

void WorkStealingThreadPool::RunTask() {
  Task task;
  if (PopTaskFromLocalQueue(task) || PopTaskFromOtherThreadQueue(task)) {
    task();
    num_jobs_.fetch_sub(1);
  } else{
    std::this_thread::yield();
  }
}
