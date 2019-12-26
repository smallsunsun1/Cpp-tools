//
// Created by 孙嘉禾 on 2019/12/26.
//

#include "stl_thread_pool.h"

namespace sss{

thread_local unsigned ThreadPool::my_index_;
thread_local WorkStealingQueue* ThreadPool::local_work_queue_ = nullptr;

void ThreadPool::WorkerThread(unsigned my_index) {
    my_index_ = my_index;
    local_work_queue_ = queues_[my_index_].get();
    while (!done_) {
        RunPendingTask();
    }
}

bool ThreadPool::PopTaskFromLocalQueue(sss::ThreadPool::Task &task) {
    return local_work_queue_ && local_work_queue_->TryPop(task);
}

bool ThreadPool::PopTaskFromPoolQueue(sss::ThreadPool::Task &task) {
    return pool_work_queue_.TryPop(task);
}

bool ThreadPool::PopTaskFromOtherThreadQueue(sss::ThreadPool::Task &task) {
    for (unsigned int i = 0; i < queues_.size(); ++i) {
        unsigned const index = (my_index_ + i + 1) % queues_.size();
        if (queues_[index]->TrySteal(task)){
            return true;
        }
    }
    return false;
}

void ThreadPool::RunPendingTask() {
    Task task;
    if (PopTaskFromLocalQueue(task) || PopTaskFromPoolQueue(task) || PopTaskFromOtherThreadQueue(task)){
        task();
    } else {
        std::this_thread::yield();
    }
}

//template <typename FunctionType>
//std::future<typename std::result_of<FunctionType()>::type> ThreadPool::Submit(FunctionType f) {
//    using result_type = typename std::result_of<FunctionType()>::type;
//    std::packaged_task<result_type()> task(f);
//    std::future<result_type> res(task.get_future());
//    if (local_work_queue_) {
//        local_work_queue_->Push(std::move(task));
//    } else {
//        pool_work_queue_.Push(std::move(task));
//    }
//    return res;
//}

}
