//
// Created by 孙嘉禾 on 2019/12/26.
//

#include "stl_thread_pool.h"

namespace sss {

thread_local unsigned ThreadPool::my_index_;
thread_local bool* ThreadPool::request_stop_ = new bool(true);
thread_local WorkStealingQueue *ThreadPool::local_work_queue_;

void ThreadPool::WorkerThread(unsigned my_index) {
    my_index_ = my_index;
    local_work_queue_ = queues_[my_index_].get();
    request_stop_ = request_signals_[my_index_].get();
    while (!done_ && !(*request_stop_)) {
        RunPendingTask();
    }
    while (!local_work_queue_->Empty()) {
        Task task;
        local_work_queue_->TryPop(task);
    }
}

bool ThreadPool::PopTaskFromLocalQueue(sss::ThreadPool::Task &task) {
    bool res = local_work_queue_ && local_work_queue_->TryPop(task);
//    if (res) {
//        std::cout << "ThreadPool::PopTaskFromLocalQueue" << std::endl;
//    }
    return res;
}

bool ThreadPool::PopTaskFromPoolQueue(sss::ThreadPool::Task &task) {
    bool res = pool_work_queue_.TryPop(task);
//    if (res) {
//        std::cout << "ThreadPool::PopTaskFromPoolQueue" << std::endl;
//    }
    return res;
}

bool ThreadPool::PopTaskFromOtherThreadQueue(sss::ThreadPool::Task &task) {
    for (unsigned int i = 0; i < queues_.size(); ++i) {
        unsigned const index = (my_index_ + i + 1) % queues_.size();
        if (queues_[index]->TrySteal(task)) {
//            std::cout << "ThreadPool::PopTaskFromOtherThreadQueue" << std::endl;
            return true;
        }
    }
    return false;
}

void ThreadPool::RunPendingTask() {
    Task task;
    if (PopTaskFromLocalQueue(task) || PopTaskFromPoolQueue(task) || PopTaskFromOtherThreadQueue(task)) {
        task();
    } else {
        std::this_thread::yield();
    }
}

void ThreadPool::Cancel() { done_ = true; }

//bool ThreadPool::StartNewThread(){
//    request_signals_.push_back(std::make_unique<bool>(false));
//    queues_.push_back(std::unique_ptr<WorkStealingQueue>(new WorkStealingQueue));
//    threads_.push_back(std::thread(&ThreadPool::WorkerThread, this, num_threads_));
//    num_threads_ += 1;
//    return true;
//}
//bool ThreadPool::CancelThread() {
//    *request_signals_.back() = true;
//    while (threads_.back().joinable()){}
//    threads_.pop_back();
//    request_signals_.pop_back();
//    num_threads_--;
//    return true;
//}

}
