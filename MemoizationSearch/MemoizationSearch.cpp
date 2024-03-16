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
    auto cachedFoo = nonstd::makecached(foo);
    auto cachedFoo1 = nonstd::makecached(foo1);
    auto result = cachedFoo(35);
    while (true)
    {
        auto result1 = cachedFoo1();
        std::cout << "Result1:" << result1 << std::endl;
        getchar();
    }
    
    std::cout << "Result: " << result << std::endl;
    
    return 0;
}
