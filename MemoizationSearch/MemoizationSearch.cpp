#include "MemoizationSearch.h"
#include <iostream>
int foo(int a) {
    return a;
}
int foo1() {
    std::cout << "foo1" << std::endl;
    return 36;
}
int main() {
    auto &cachedFoo = nonstd::makecached([](int a) {
        std::cout << "foo" << std::endl;
        return a;
    });

    std::cout << cachedFoo(35) << std::endl;
    
    return 0;
}
