#include "MemoizationSearch.h"
#include <iostream>
#include <Windows.h>
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
int data = 100;
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
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    cachedlambda.clearArgsCache();//清除缓存
    std::cout << cachedlambda(35) << std::endl;//有参数的情况
    //一个函数只会生成一个实例
    std::cout << &noparam << std::endl;//无参数的情况
    std::cout << &noparam2 << std::endl;//地址相同实例只有一个
    //无参数函数的缓存版本
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    std::cout << noparam() << std::endl;
    //无参数lambda的缓存版本
    std::cout << noparamlambda() << std::endl;
    std::cout << noparamlambda() << std::endl;
    noparamlambda.clearCache();//清除缓存
    std::cout << noparamlambda() << std::endl;
    //斐波那契数列的缓存版本
    std::cout << Fibonacci(256) << std::endl;
    //读取内存的缓存版本
    std::cout << "data:" << ReadCache<int>((LPVOID)&data) << std::endl;
    std::cout << "cached data:" << ReadCache<int>((LPVOID)&data) << std::endl;
    //在跨进程读取当中每一级偏移不用每次都去读取丢到缓存中就好了,比如说你读的是5级偏移,那么前4级偏移都可以丢到缓存中 最后一级偏移每次都去读取,这样就可以减少读取次数 默认缓存过期时间是200ms 这个时间可以自己设置 一般来说200ms人眼是感觉不到的,正常人的反应时间是250ms
    return 0;
}
/*
#include "MemoizationSearch.h": 这行代码包含了MemoizationSearch.h头文件，这个头文件中定义了nonstd::makecached模板函数，用于创建函数的缓存版本。

#include <iostream>: 包含了标准输入输出流库，使得程序可以使用std::cout来输出信息到控制台。

定义了一个名为foo的函数，接受一个整型参数a，并返回这个参数。这个函数非常简单，仅用于演示。

定义了一个名为foo1的函数，它不接受任何参数，输出"foo1"到控制台，并返回整数36。这个函数用于展示无参数函数的缓存。

在main函数中：

使用nonstd::makecached创建了一个名为cachedFoo的缓存版本的函数。这个缓存的函数是基于一个匿名函数（lambda表达式），它接受一个整型参数，输出"foo"到控制台，并返回该参数。
使用nonstd::makecached创建了另一个名为noparam的缓存版本的foo1函数。
std::cout << cachedFoo(35) << std::endl;: 调用cachedFoo函数并传递参数35，然后将返回的结果输出到控制台。由于cachedFoo是第一次被调用，所以它会输出"foo"并返回35。

接下来的多个std::cout << noparam() << std::endl;行：重复调用noparam函数，并将返回的结果输出到控制台。由于noparam是foo1函数的缓存版本，理论上第一次调用它时会输出"foo1"并返回36。但随后的调用，如果nonstd::makecached实现了正确的缓存机制，应该不会再次输出"foo1"，只会直接返回缓存的结果36。
*/
