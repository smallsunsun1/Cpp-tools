//
// Created by 孙嘉禾 on 2019/12/26.
//

#include "env.h"

namespace sss {

void InterruptFlag::Set() {
    flag_.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lk(set_clear_mutex_);
    if (thread_cond_) {
        thread_cond_->notify_all();
    } else if (thread_cond_any_) {
        thread_cond_any_->notify_all();
    }
}
std::condition_variable_any *& InterruptFlag::Get_CondVar() {
    return thread_cond_any_;
}
std::mutex &InterruptFlag::Get_Mutex() {
    return set_clear_mutex_;
}
bool InterruptFlag::Is_Set() const {
    return flag_.load(std::memory_order_relaxed);
}
void InterruptFlag::Set_Condition_Variable(std::condition_variable &cv) {
    std::lock_guard<std::mutex> lk(set_clear_mutex_);
    thread_cond_ = &cv;
}
void InterruptFlag::Clear_Condition_Variable() {
    std::lock_guard<std::mutex> lk(set_clear_mutex_);
    thread_cond_ = nullptr;
}

thread_local InterruptFlag InterruptibleThread::this_thread_interrupt_flag_;
void InterruptibleThread::Detach() {
    if (this_thread_interrupt_flag_.Is_Set())
        return;
    internal_thread_.detach();
}
void InterruptibleThread::Join() {
    if (this_thread_interrupt_flag_.Is_Set())
        return;
    internal_thread_.join();
}
bool InterruptibleThread::Joinable() const {
    if (this_thread_interrupt_flag_.Is_Set())
        return false;
    return internal_thread_.joinable();
}
template<typename FunctionType, typename ...Args>
InterruptibleThread::InterruptibleThread(FunctionType &&f, Args &&... args) {
    std::promise<InterruptFlag *> p;
    internal_thread_ = std::thread([f, &p]() {
      p.set_value(&this_thread_interrupt_flag_);
      f();
    });
    flag_ = p.get_future().get();
}
void InterruptibleThread::Interrupt() {
    if (flag_) {
        flag_->Set();
    }
}
void InterruptibleThread::Interruption_Point() {
    if (this_thread_interrupt_flag_.Is_Set()) {
        throw ThreadInterrupted();
    }
}
void InterruptibleThread::Interruptible_Wait(std::condition_variable &cv, std::unique_lock<std::mutex> &lk) {
    Interruption_Point();
    this_thread_interrupt_flag_.Set_Condition_Variable(cv);
    Clear_Cv_On_Destruct guard;
    Interruption_Point();
    cv.wait_for(lk, std::chrono::milliseconds(1));
    Interruption_Point();
}
template<typename Predicate>
void InterruptibleThread::Interruptible_Wait(std::condition_variable &cv,
                                             std::unique_lock<std::mutex> &lk,
                                             Predicate pred) {
    Interruption_Point();
    this_thread_interrupt_flag_.Set_Condition_Variable(cv);
    Clear_Cv_On_Destruct guard;
    while (!this_thread_interrupt_flag_.Is_Set() && !pred()) {
        cv.wait_for(lk, std::chrono::milliseconds(1));
    }
    Interruption_Point();
}
template<typename Lockable>
void InterruptibleThread::Wait(std::condition_variable_any &cv, Lockable &lk) {
    struct CustomLock {
      InterruptFlag *self;
      Lockable &lk;
      CustomLock(InterruptFlag *self_, std::condition_variable_any &cond, Lockable &lk_) : self(self_), lk(lk_) {
          self->Get_CondVar() = &cond;
          self->Get_Mutex().lock();
      }
      void Unlock() {
          lk.unlock();
          self->Get_Mutex().unlock();
      }
      void Lock() {
          std::lock(self->Get_Mutex(), lk);
      }
      ~CustomLock(){
          self->Get_CondVar() = nullptr;
          self->Get_Mutex().unlock();
      }
    };
    CustomLock cl(flag_, cv, lk);
    Interruption_Point();
    cv.wait(cl);
    Interruption_Point();
}

}

