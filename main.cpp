#include <iostream>
#include "libs/stl_thread_pool.h"

void Test(){
    std::cout << "Hello World" << std::endl;
}

int main() {
    using namespace sss;
    ThreadPool pool(4);
    for (int i = 0; i < 100; i++)
        pool.Submit(std::function<void()>(Test));
//    auto res = pool.Submit(Test);
    std::this_thread::sleep_for(std::chrono::seconds(2));
//    if (res.valid()){
//        std::cout << "Job Finish !" << std::endl;
//    }
    return 0;
}
