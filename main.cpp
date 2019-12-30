#include <iostream>
#include "libs/stl_thread_pool.h"

void Test(){
    std::cout << "Hello World" << std::endl;
}

int main() {
    using namespace sss;
    std::queue<int> queue;
    queue.push(2);
    auto rvalue = std::move(queue.front());
    std::cout << queue.front();
    std::future<void> f;
    return 0;
}
