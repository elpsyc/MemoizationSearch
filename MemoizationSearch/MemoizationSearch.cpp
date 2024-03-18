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
    std::cout << noparam() << std::endl;
    
    return 0;
}
/*
#include "MemoizationSearch.h": 这行代码包含了MemoizationSearch.h头文件，我们假设这个头文件中定义了nonstd::makecached模板函数，用于创建函数的缓存版本。

#include <iostream>: 包含了标准输入输出流库，使得程序可以使用std::cout来输出信息到控制台。

定义了一个名为foo的函数，接受一个整型参数a，并返回这个参数。这个函数非常简单，仅用于演示。

定义了一个名为foo1的函数，它不接受任何参数，输出"foo1"到控制台，并返回整数36。这个函数用于展示无参数函数的缓存。

在main函数中：

使用nonstd::makecached创建了一个名为cachedFoo的缓存版本的函数。这个缓存的函数是基于一个匿名函数（lambda表达式），它接受一个整型参数，输出"foo"到控制台，并返回该参数。
使用nonstd::makecached创建了另一个名为noparam的缓存版本的foo1函数。
std::cout << cachedFoo(35) << std::endl;: 调用cachedFoo函数并传递参数35，然后将返回的结果输出到控制台。由于cachedFoo是第一次被调用，所以它会输出"foo"并返回35。

接下来的多个std::cout << noparam() << std::endl;行：重复调用noparam函数，并将返回的结果输出到控制台。由于noparam是foo1函数的缓存版本，理论上第一次调用它时会输出"foo1"并返回36。但随后的调用，如果nonstd::makecached实现了正确的缓存机制，应该不会再次输出"foo1"，只会直接返回缓存的结果36。
*/
