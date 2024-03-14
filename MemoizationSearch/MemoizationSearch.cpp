
#include <iostream>
#include"MemoizationSearch.h"
//斐波那契数列的缓存搜索
__int64 Fibonacci(int n) {
	if (n <= 1) return n;
	//利用nonstd::makecached函数生成一个缓存搜索的函数
	static auto fib = nonstd::makecached(Fibonacci);
	return fib(n - 1) + fib(n - 2);
}
int main(){
	std::cout << Fibonacci(10);
	return 0;
}
