
#include <iostream>
#include"MemoizationSearch.h"
//斐波那契数列的缓存搜索
DWORD64 Fibonacci(int n) {
	if (n <= 1) return n;
	//利用nonstd::makecached函数生成一个缓存搜索的函数
	static auto fib = nonstd::makecached(Fibonacci);
	return fib(n - 1) + fib(n - 2);
}
int add(int a, int b) {
	return a + b;
}
int cachedadd(int a, int b) {
	static auto cached = nonstd::makecached(add);
	return cached(a, b);
}
int main(){
	std::cout << Fibonacci(60)<<std::endl;
	std::cout << cachedadd(3,5) << std::endl;
	std::cout << cachedadd(3,5) << std::endl;
	return 0;
}
