//
// Created by 孙嘉禾 on 2019/12/25.
//

#ifndef TOOLS_ENV_H
#define TOOLS_ENV_H

#include <thread>
#include <functional>
#include <atomic>
#include <exception>
#include <future>
#include <condition_variable>

namespace sss {

class ThreadInterrupted : public std::exception {
 public:
  ThreadInterrupted() {}
  const char* what() const noexcept override {
      return "Thread Interrupted Exception!";
  }
  ~ThreadInterrupted() {}
};

class InterruptFlag {
 public:
  InterruptFlag() : thread_cond_(nullptr), thread_cond_any_(nullptr) {}
  void Set();
  bool Is_Set() const;
  void Set_Condition_Variable(std::condition_variable &cv);
  void Clear_Condition_Variable();
  std::condition_variable_any*& Get_CondVar();
  std::mutex& Get_Mutex();
 private:
  std::atomic<bool> flag_;
  std::condition_variable *thread_cond_;
  std::condition_variable_any* thread_cond_any_;
  std::mutex set_clear_mutex_;
};

class InterruptibleThread {
 public:
  template<typename FunctionType, typename ...Args>
  explicit InterruptibleThread(FunctionType &&f, Args &&...args);
  void Join();
  void Detach();
  bool Joinable() const;
  void Interrupt();
  void Interruption_Point();
  void Interruptible_Wait(std::condition_variable& cv, std::unique_lock<std::mutex>& lk);
  template<typename Predicate>
  void Interruptible_Wait(std::condition_variable &cv, std::unique_lock<std::mutex> &lk, Predicate pred);
  template <typename Lockable>
  void Wait(std::condition_variable_any& cv, Lockable& lk);
  template <typename Lockable>
  void Interruptible_Wait(std::condition_variable_any& cv, Lockable& lk){
      Wait(cv, lk);
  }
 private:
  struct Clear_Cv_On_Destruct{
    ~Clear_Cv_On_Destruct(){
        this_thread_interrupt_flag_.Clear_Condition_Variable();
    }
  };
  std::thread internal_thread_;
  InterruptFlag *flag_;
  static thread_local InterruptFlag this_thread_interrupt_flag_;
};

struct StlThreadEnviroment {
  struct Task {
    std::function<void()> f;
  };
  class EnvThread {
   public:
    explicit EnvThread(std::function<void()> f) : th_(std::move(f)) {}
    ~EnvThread() { th_.join(); }
    void OnCancel() {}
   private:
    std::thread th_;
  };
  EnvThread *Createthread(std::function<void()> f) { return new EnvThread(std::move(f)); }
  Task CreateTask(std::function<void()> f) { return Task{std::move(f)}; }
  void ExecuteTask(const Task &t) { t.f(); }
};

}

#endif //TOOLS_ENV_H
