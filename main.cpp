#define EIGEN_USE_THREADS
//#define EIGEN_USE_MKL
//#define EIGEN_USE_GPU
//#define EIGEN_GPUCC


#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "unsupported/Eigen/CXX11/Tensor"
#include "unsupported/Eigen/CXX11/ThreadPool"

#include "libs/stl_thread_pool.h"
//struct BatchMatMul {
//  template <typename Input>
//  Eigen::array<Eigen::DenseIndex, 3> dimensions(const Input &input1,
//                                                const Input &input2) const {
//      Eigen::array<Eigen::DenseIndex, 3> result;
//      result[0] = input1.dimension(0);
//      result[1] = input1.dimension(1);
//      result[2] = input2.dimension(2);
//      return result;
//  }
//  template<typename Input, typename Output, typename Device>
//  void eval(const Input &input1, const Input &input2,
//            Output &output, const Device &device) const {
//      typedef typename Input::DimensionPair DimPair;
//      Eigen::array<DimPair, 1> dims;
//      dims[0] = DimPair(0, 1);
//      for (int i = 0; i < output.dimension(0); ++i) {
//          output.template chip<0>(i).device(device) = (input1.template chip<0>(i)).template contract(input2.template chip<0>(i), dims);
//      }
//  }
//};
//
//struct UserReducer {
//  static const bool PackerAccess = false;
//  explicit UserReducer(float offset): offset_(offset) {}
//  void reduce(const float val, float* accum) {*accum += val * val;}
//  float initialize() const { return 0; }
//  float finalize(const float accum) const { return 1.0f / (accum + offset_); }
// private:
//  const float offset_;
//};

template<typename T>
class Base {
 public:
  using DerivedType = T;
  void Call() {
      std::cout << "Hello World!\n" << std::endl;
  }
  void DoCall() {
      T *t = static_cast<T *>(this);
      t->Call();
  }
 protected:
  T &Derived() {
      return *static_cast<T *>(this);
  }
};

template<typename T>
class Derive : public Base<Derive<T>> {
 public:
  void Call() override {
      std::cout << "Derived!\n" << std::endl;
  }
};



void TTest(int a) {
    std::cout << a << std::endl;
}

void DoLargeCompute(int init) {
    int res = init;
    for (int i = 0; i < 10000; ++i) {
        res += 1;
    }
    printf("%d\n", res);
}



int main() {
  sss::ThreadPool pool(4);
  std::vector<std::future<void>> fut;
  fut.reserve(100);
  for (int i = 0; i < 1000; ++i) {
    fut.push_back(pool.Submit(DoLargeCompute, 0));
    if (i % 400 == 0)
      pool.StartNewThread();
  }
  pool.CancelThread();
  std::cout << pool.NumThreads() << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "here" << std::endl;
  std::cout << pool.NumJobs() << std::endl;
  return 0;
}
