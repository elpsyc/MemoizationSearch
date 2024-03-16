#include"MemoizationSearch.h"
#include <iostream>
int foo(int a) {
    return a;
}
int foo1() {
    std::cout << "foo1" << std::endl;
    return 36;
}
int main() {
    auto cachedFoo = nonstd::makecached([](int a) {
        std::cout << "foo" << std::endl;
        return a;
    });
    auto& cachedFoo1 = nonstd::makecached(foo1);
    auto& cachedFoo2 = nonstd::makecached(foo1);
    std::cout << &cachedFoo1 << std::endl;
    std::cout << &cachedFoo2 << std::endl;
    
    return 0;
}
