
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
int cachedfunc() {
	std::cout << "cachedfunc called!" << std::endl;
	return 66;
}
auto& cached = nonstd::makecached(cachedfunc);
int cachedminus(int a, int b) {
	while (true)
	{
		std::cout<<&cached<<std::endl;
		Sleep(20);
	}
	return 0;
}
int main(){

	std::thread t1([]() {cachedminus(5, 3); });
	std::thread t2([]() {cachedminus(5, 3); });
	t1.detach();
	t2.detach();
	getchar();
	return 0;
}
