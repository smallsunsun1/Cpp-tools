//
// Created by 孙嘉禾 on 2019/12/26.
//

#include "stl_thread_pool.h"

#include <memory>

namespace sss {

thread_local unsigned ThreadPool::my_index_;
thread_local bool* ThreadPool::request_stop_;
thread_local WorkStealingQueue *ThreadPool::local_work_queue_;
const int ThreadPool::kMaxThreads = std::thread::hardware_concurrency() * 2;

void ThreadPool::WorkerThread(unsigned my_index) {
    my_index_ = my_index;
    local_work_queue_ = queues_[my_index_].get();
    request_stop_ = request_signals_[my_index_].get();
    while (!done_ && !(*request_stop_)) {
        RunPendingTask();
    }
}

bool ThreadPool::PopTaskFromLocalQueue(sss::ThreadPool::Task &task) {
    bool res = local_work_queue_ && local_work_queue_->TryPop(task);
    return res;
}

bool ThreadPool::PopTaskFromPoolQueue(sss::ThreadPool::Task &task) {
    bool res = pool_work_queue_.TryPop(task);
    return res;
}

bool ThreadPool::PopTaskFromOtherThreadQueue(sss::ThreadPool::Task &task) {
    int queue_size = queue_size_.load();
    for (unsigned int i = 0; i < queue_size; ++i) {
        unsigned const index = (my_index_ + i + 1) % queue_size;
        if (queues_[index]->TrySteal(task)) {
            return true;
        }
    }
    return false;
}

void ThreadPool::RunPendingTask() {
    Task task;
    if (PopTaskFromLocalQueue(task) || PopTaskFromPoolQueue(task) || PopTaskFromOtherThreadQueue(task)) {
        task();
        num_jobs_.fetch_sub(1);
    } else {
      std::this_thread::yield();
        std::unique_lock<std::mutex> unique_lock(mu_);
        if (num_jobs_.load() == 0){
          cv_.wait(unique_lock);
        } else {
          unique_lock.unlock();
          std::this_thread::yield();
        }
    }
}

void ThreadPool::Cancel() { done_ = true; cv_.notify_all();}

bool ThreadPool::StartNewThread(){
    assert(num_threads_ < kMaxThreads);
    request_signals_.push_back(std::make_unique<bool>(false));
    queues_.push_back(std::make_unique<WorkStealingQueue>());
    queue_size_.store(queues_.size());
    threads_.emplace_back(&ThreadPool::WorkerThread, this, num_threads_);
    num_threads_++;
    return true;
}
//bool ThreadPool::CancelThread() {
//    *request_signals_.back() = true;
//    while (threads_.back().joinable()){}
//    threads_.pop_back();
//    queues_.pop_back();
//    request_signals_.pop_back();
//    num_threads_--;
//    return true;
//}

}
