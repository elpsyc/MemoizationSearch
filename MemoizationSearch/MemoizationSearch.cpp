
#include <iostream>
#include"MemoizationSearch.h"
//斐波那契数列的缓存搜索
DWORD64 Fibonacci(int n) {
	if (n <= 1) return n;
	//利用nonstd::makecached函数生成一个缓存搜索的函数
	auto& fib = nonstd::makecached(Fibonacci);
	return fib(n - 1) + fib(n - 2);
}
int add(int a,int b) {
	std::cout << "add" << std::endl;
	return a+b;
}
int cachedadd(int a, int b) {
	auto& cached = nonstd::makecached(add);
	return cached(a,b);
}
int main(){
	std::cout << Fibonacci(60) << std::endl;
	return 0;
}
