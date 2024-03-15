﻿
#include <iostream>
#include"MemoizationSearch.h"
//斐波那契数列的缓存搜索
DWORD64 Fibonacci(int n) {
	if (n <= 1) return n;
	return nonstd::makecached(Fibonacci)(n - 1) + nonstd::makecached(Fibonacci)(n - 2);
}
int add(int a,int b) {
	std::cout << "add" << std::endl;
	return a+b;
}
int minus(int a, int b) {
	std::cout << "minus" << std::endl;
	return a - b;
}
int cachedadd(int a, int b) {
	auto& cached = nonstd::makecached(add);
	return cached(a,b);
}
int cachedadd_(int a, int b) {
	auto& cached = nonstd::makecached(add);
	return cached(a, b);
}
int cachedminus(int a, int b) {
	auto& cached = nonstd::makecached([&](int a,int b)->int{
		std::cout << "minus lambda" << std::endl;
		return a-b;
	});
	return cached(a, b);
}
int main(){
	std::cout << Fibonacci(256) << std::endl;
	std::cout << cachedadd(5,3) << std::endl;
	std::cout << cachedadd_(5,3) << std::endl;//add函数的缓存版本只被创建了一次
	std::cout << cachedminus(5,3) << std::endl;
	std::cout << cachedminus(5,3) << std::endl;
	return 0;
}
