//
// Created by 孙嘉禾 on 2019/12/26.
//

#include "thread_safe_structures.h"

namespace sss {

void WorkStealingQueue::Push(sss::WorkStealingQueue::data_type data) {
    std::lock_guard<std::mutex> lock(mu_);
    the_queue_.push_front(std::move(data));
}

bool WorkStealingQueue::Empty() const {
    std::lock_guard<std::mutex> lock(mu_);
    return the_queue_.empty();
}

bool WorkStealingQueue::TryPop(sss::WorkStealingQueue::data_type & res) {
    std::lock_guard<std::mutex> lock(mu_);
    if (the_queue_.empty()){
        return false;
    }
    res = std::move(the_queue_.front());
    the_queue_.pop_front();
    return true;
}

bool WorkStealingQueue::TrySteal(sss::WorkStealingQueue::data_type & res) {
    std::lock_guard<std::mutex> lock(mu_);
    if (the_queue_.empty()) {
        return false;
    }
    res = std::move(the_queue_.back());
    the_queue_.pop_back();
    return true;
}

size_t WorkStealingQueue::Size() const {
    std::lock_guard<std::mutex> lock(mu_);
    return the_queue_.size();
}

}

