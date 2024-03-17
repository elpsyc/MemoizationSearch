#include "MemoizationSearch.h"
#include <iostream>
#include<Windows.h>
int foo(int a) {
    return a;
}
int foo1() {
    std::cout << "foo1" << std::endl;
    return 36;
}
DWORD64 Fibonacci(int n) {
    if (n <= 1) return n;
    //利用nonstd::makecached函数生成一个缓存搜索的函数
    auto& fib = nonstd::makecached(Fibonacci);
    return fib(n - 1) + fib(n - 2);
}
int main() {
    auto &cachedFoo = nonstd::makecached([](int a) {
        std::cout << "foo" << std::endl;
        return a;
    });
    auto& noparam = nonstd::makecached(foo1);
    std::cout << cachedFoo(35) << std::endl;//有参数的情况
    std::cout << noparam() << std::endl;//有参数的情况
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << Fibonacci(256) << std::endl;
    
    return 0;
}
