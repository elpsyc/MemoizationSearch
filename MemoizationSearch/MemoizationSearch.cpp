#include "MemoizationSearch.h"
#include <iostream>
#include <Windows.h>
#include <stddef.h>
int foo1() {
    std::cout << "foo1" << std::endl;
    return 36;
}
DWORD64 Fibonacci(int n);
auto& fib = nonstd::makecached(Fibonacci);//一个函数只会生成一个实例
DWORD64 Fibonacci(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}
HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
template<typename T>
T read(LPVOID addr) {
    std::cout << "readapi called" << std::endl;
    T t{};
	ReadProcessMemory(hProcess, (LPCVOID)addr, &t, sizeof(T), NULL);
	return t;
}
template<typename T>
auto& readcache = nonstd::makecached(read<T>);
template<typename T>
T ReadCache(LPVOID addr) {
    T t = readcache<T>((LPVOID)addr);
	return t;
}

int main() {
    //有参数的lamda的缓存版本
    auto &cachedlambda = nonstd::makecached([](int a) {
        std::cout << "foo" << std::endl;
        return a;
    });
    //无参数的函数的缓存版本
    auto& noparam = nonstd::makecached(foo1);
    auto& noparam2 = nonstd::makecached(foo1);
    auto& noparamlambda = nonstd::makecached([]() {
        std::cout << "noparamlambda" << std::endl;
        return 37;
    });
    //有参数的lamda的缓存版本
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    Sleep(1000);
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    cachedlambda.SetCacheTime(300);
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    //一个函数只会生成一个实例
    std::cout << &noparam << std::endl;//无参数的情况
    std::cout << &noparam2 << std::endl;//地址相同实例只有一个
    //无参数函数的缓存版本
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    noparam.ClearCache();
    std::cout << noparam() << std::endl;
    //无参数lambda的缓存版本
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    noparamlambda.ClearCache();//清除缓存
    std::cout << noparamlambda() << std::endl;
    //斐波那契数列的缓存版本
    std::cout << Fibonacci(256) << std::endl;
    //In cross process reading, each level of offset does not need to be read and thrown into the cache every time.For example, if you are reading a 5th level offset, the first 4 levels of offset can be thrown into the cache, and the last level of offset can be read every time.This can reduce the number of reads.The default cache expiration time is 200ms, which can be set by yourself.Generally, 200ms is not noticeable to the human eye, and the normal reaction time is 250ms
    return 0;
}
/*
include "MemoizationSearch.h": This line includes the MemoizationSearch.h header file, which defines the nonstd::makecached template function to create cached versions of functions.
 #include <iostream>: Includes the standard input-output stream library, which allows the program to use std::cout to output information to the console.
 Defines a function named foo that takes an integer parameter a and returns that parameter. This function is very simple and is only for demonstration purposes.
 Defines a function named foo1 that takes no arguments and prints "foo1" to the console and returns the integer 36. This function is used to demonstrate caching of functions that take no arguments.
 In the main function:
 A cached version of the function named cachedFoo is created using nonstd::makecached. The cached function is based on an anonymous function (lambda expression) that takes an integer parameter and prints "foo" to the console and returns that parameter.
 Another cached version of the foo1 function named noparam is created using nonstd::makecached. std::cout << cachedFoo(35) << std::endl; : Calls the cachedFoo function and passes it the argument 35, then prints the returned result to the console.Since cachedFoo is called the first time, it prints "foo" and returns 35.
 The following multiple std::cout << noparam() << std::endl; lines: Repeatedly call the noparam function and print the returned result to the console.Since noparam is a cached version of the foo1 function, the first call to it should in theory print "foo1" and return 36.But subsequent calls, if nonstd::makecached implements the correct caching mechanism, should not print "foo1" again, but simply return the cached result 36.
*/
