//
// Created by 孙嘉禾 on 2020/1/22.
//

#include <atomic>
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
  int a[10];
  std::atomic<int*> b(a);
  auto* c = b + 5;
  *c = 55;
  std::cout << a[5] << std::endl;
}

