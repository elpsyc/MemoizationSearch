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
    auto cachedFoo1 = nonstd::makecached(foo1);
    while (true)
    {
        auto result1 = cachedFoo(35);
        std::cout << "Result1:" << result1 << std::endl;
        getchar();
    }
    
    return 0;
}
