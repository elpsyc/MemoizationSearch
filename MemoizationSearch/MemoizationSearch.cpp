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
    auto &cachedlambda = nonstd::makecached([](int a) {
        std::cout << "foo" << std::endl;
        return a;
    });
    auto& noparam = nonstd::makecached(foo1);
    auto& noparam2 = nonstd::makecached(foo1);
    auto& noparamlambda = nonstd::makecached([]() {
        std::cout << "noparamlambda" << std::endl;
        return 37;
        });
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    std::cout << &noparam << std::endl;//有参数的情况
    std::cout << &noparam2 << std::endl;//地址相同实例只有一个
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    std::cout << Fibonacci(256) << std::endl;
    
    return 0;
}
