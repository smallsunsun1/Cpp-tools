//
// Created by 孙嘉禾 on 2019/12/25.
//

#ifndef TOOLS_WORK_QUEUE_H
#define TOOLS_WORK_QUEUE_H

#include <atomic>
#include <mutex>

template <typename Work, unsigned kSize>
class RunQueue {
 public:
  RunQueue(): front_(0), back_(0) {
      for (unsigned i = 0; i < kSize; ++i) {
          array_[i].state.store(kEmpty, std::memory_order::memory_order_relaxed);
      }
  }
  ~RunQueue() {}
  Work PushFront(Work w) {
      unsigned front = front_.load(std::memory_order::memory_order_relaxed);
      Elem* e = &array_[front & kMask];
      uint8_t s = e->state.load(std::memory_order::memory_order_relaxed);
      if (s != kEmpty || !e->state.compare_exchange_strong(s, kBusy, std::memory_order::memory_order_acquire))
          return w;
      front_.store(front + 1 + (kSize << 1), std::memory_order::memory_order_relaxed);
      e->w = std::move(w);
      e->state.store(kReady, std::memory_order::memory_order_release);
      return Work();
  }
  Work PopFront() {
      unsigned front = front_.load(std::memory_order::memory_order_relaxed);
      Elem* e = &array_[(front_ - 1) & kMask];
      uint8_t s = e->state.load(std::memory_order_relaxed);
      if (s != kReady || !e->state.compare_exchange_strong(s, kBusy, std::memory_order::memory_order_acquire))
          return Work();
      Work w = std::move(e->w);
      e->state.store(kEmpty, std::memory_order_release);
      front = ((front - 1) & kMask2) | (front & ~kMask2);
      front_.store(front, std::memory_order_relaxed);
      return w;
  }
  Work PushBack(Work w) {
      std::lock_guard<std::mutex> lock(mu_);
      unsigned back = back_.load(std::memory_order::memory_order_release);
      Elem* e = &array_[(back - 1) & kMask];
      uint8_t s = e->state.load(std::memory_order_relaxed);
      if (s != kEmpty ||
          !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
          return w;
      back = ((back - 1) & kMask2) | (back & ~kMask2);
      back_.store(back, std::memory_order_relaxed);
      e->w = std::move(w);
      e->state.store(kReady, std::memory_order_release);
      return Work();
  }
  Work PopBack() {
      if (Empty())
          return Work();
      std::lock_guard<std::mutex> lock(mu_);
      unsigned back = back_.load(std::memory_order_relaxed);
      Elem* e = &array_[back & kMask];
      uint8_t s = e->state.load(std::memory_order_relaxed);
      if (s != kReady ||
          !e->state.compare_exchange_strong(s, kBusy, std::memory_order_acquire))
          return Work();
      Work w = std::move(e->w);
      e->state.store(kEmpty, std::memory_order_release);
      back_.store(back + 1 + (kSize << 1), std::memory_order_relaxed);
      return w;
  }
  unsigned PopBackHalf(std::vector<Work>* result) {
      if (Empty()) return 0;
      std::lock_guard<std::mutex> lock(mu_);
      unsigned back = back_.load(std::memory_order_relaxed);
      unsigned size = Size();
      unsigned mid = back;
      if (size > 1) mid = back + (size - 1) / 2;
      unsigned n = 0;
      unsigned start = 0;
      for (; static_cast<int>(mid - back) >= 0; mid--) {
          Elem* e = &array_[mid & kMask];
          uint8_t s = e->state.load(std::memory_order_relaxed);
          if (n == 0) {
              if (s != kReady || !e->state.compare_exchange_strong(
                  s, kBusy, std::memory_order_acquire))
                  continue;
              start = mid;
          } else {
              // Note: no need to store temporal kBusy, we exclusively own these
              // elements.
              eigen_plain_assert(s == kReady);
          }
          result->push_back(std::move(e->w));
          e->state.store(kEmpty, std::memory_order_release);
          n++;
      }
      if (n != 0)
          back_.store(start + 1 + (kSize << 1), std::memory_order_relaxed);
      return n;
  }
  unsigned Size() const {return SizeOrNotEmpty<true>();}
  bool Empty() const {return SizeOrNotEmpty<false>() == 0;}
 private:
  static constexpr unsigned kMask = kSize - 1;
  static constexpr unsigned kMask2 = (kSize << 1) - 1;
  struct Elem {
    std::atomic<uint8_t> state;
    Work w;
  };
  enum {
    kEmpty,
    kBusy,
    kReady
  };
  std::mutex mu_;
  std::atomic<unsigned> front_;
  std::atomic<unsigned> back_;
  std::array<Elem, kSize> array_;

  inline unsigned CalculateSize(unsigned front, unsigned back) const {
      int size = (front & kMask2) - (back & kMask2);
      if (size < 0)
          size += 2 * kSize;
      if (size > static_cast<int>(kSize)) size = kSize;
      return static_cast<unsigned>(size);
  }
  template <bool NeedSizeEstimate>
  unsigned SizeOrNotEmpty() const {
      unsigned front = front_.load(std::memory_order::memory_order_acquire);
      while (true) {
        unsigned back = back_.load(std::memory_order::memory_order_acquire);
        unsigned front_new = front_.load(std::memory_order::memory_order_acquire);
        if (front != front_new) {
            front = front_new;
            std::atomic_thread_fence(std::memory_order::memory_order_acquire);
            continue;
        }
        if (NeedSizeEstimate) {
            return CalculateSize(front, back);
        } else {
            unsigned maybe_zero = ((front ^ back) & kMask2);
            static_assert((CalculateSize(front, back) == 0) == (maybe_zero == 0));
            return maybe_zero;
        }
      }
  }
  RunQueue(const RunQueue&) = delete;
  void operator=(const RunQueue&) = delete;
};

#endif //TOOLS_WORK_QUEUE_H
