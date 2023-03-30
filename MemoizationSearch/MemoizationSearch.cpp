#include <algorithm>
#include <concurrent_unordered_map.h>
#include <shared_mutex>
#include <vector>
#include <functional>
#include <Windows.h>
#include <iostream>
namespace nonstd {
	constexpr DWORD CacheNormalTTL = 200;
	template<class Key, class Val>
	using concurrent_map = Concurrency::concurrent_unordered_map<Key, Val>;
	template<typename _Tx>
	class CacheItem {
	public:
		using timepoint = std::chrono::time_point<std::chrono::system_clock>;
		timepoint m_endtime;
		_Tx   m_value;
		CacheItem() = default;
		CacheItem(const _Tx& _value, const timepoint& _endtime) :m_value(_value), m_endtime(_endtime) {}
		~CacheItem() { m_value.~_Tx(); }
		inline bool IsValid(timepoint now)noexcept { return now < m_endtime; }
	};
	template<typename _Tx, typename _Ty>
	class SimpleBasicCache {
	public:
		concurrent_map<_Tx, CacheItem<_Ty>> m_Cache;
		using pair_type = typename std::decay_t<decltype(m_Cache)>::value_type;
		using iterator = typename std::decay_t<decltype(m_Cache)>::iterator;
		mutable std::mutex m_mutex;
		using mutextype = decltype(m_mutex);
		SimpleBasicCache() { srand((unsigned int)time(0)); }
		~SimpleBasicCache()noexcept {
			std::unique_lock<mutextype> lock(m_mutex);
			m_Cache.clear();
		}
		inline iterator AddCache(const _Tx& _key, const _Ty& _value, DWORD _validtime = 200) {
			auto nowTime = std::chrono::system_clock::now();
			auto newValue = CacheItem<_Ty>(_value, nowTime + std::chrono::milliseconds(_validtime + rand() % 30));
			std::unique_lock<mutextype> lock(m_mutex, std::defer_lock);
			auto lb = m_Cache.find(_key);
			iterator ret = m_Cache.end();
			if (lb != m_Cache.end()) {
				lb->second = newValue;
				ret = lb;
			}
			else {
				lock.lock();
				ret = m_Cache.insert(lb, pair_type(_key, newValue));
				lock.unlock();
			}
			static auto firsttime = std::chrono::system_clock::now();
			auto now = std::chrono::system_clock::now();
			if (std::chrono::duration_cast<std::chrono::milliseconds>(now - firsttime).count() > 5000) {
				lock.lock();
				firsttime = now;
				for (auto it = m_Cache.begin(); it != m_Cache.end();) {
					it = (!it->second.IsValid(now)) ? m_Cache.unsafe_erase(it) : ++it;
				}
				lock.unlock();
			}
			return ret;
		}
		inline iterator erase(const _Tx& value) {
			std::unique_lock<mutextype> lock(m_mutex);
			auto iter = m_Cache.find(value);
			iterator ret = m_Cache.end();
			if (iter != m_Cache.end()) ret = m_Cache.unsafe_erase(iter);
			return ret;
		}
		inline std::pair<iterator, bool> find(const _Tx& _key) {
			if (m_Cache.empty()) return { iterator(),false };
			auto iter = m_Cache.find(_key);
			return { iter, iter != m_Cache.end() && iter->second.IsValid(std::chrono::system_clock::now()) };
		}
		inline std::pair<iterator, bool> operator[](const _Tx& _key) {
			return find(_key);
		}
		inline void Clear() {
			std::unique_lock<mutextype> lock(m_mutex);
			m_Cache.clear();
		}
	};
	template<class _Kty, class _Vty> using SimpleCache = SimpleBasicCache<_Kty, _Vty>;
	template<typename T> struct std::hash<std::vector<T>> {
		std::size_t operator()(const std::vector<T>& v) const {
			std::hash<T> hasher;
			std::size_t seed = 0;
			for (auto& elem : v)seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
	template<typename... Args> struct std::hash<std::tuple<Args...>> {
		std::size_t operator()(const std::tuple<Args...>& t) const { return std::hash<std::vector<std::size_t>>()(get_indices(t)); }
		template<std::size_t... Is> std::vector<std::size_t> indices(std::tuple<Args...> const& t, std::index_sequence<Is...>) const { return { std::hash<Args>()(std::get<Is>(t))... }; }
		template<std::size_t N> std::vector<std::size_t> indices(std::tuple<Args...> const& t) const { return indices(t, std::make_index_sequence<N>{}); }
		std::vector<std::size_t> get_indices(std::tuple<Args...> const& t) const { return indices<sizeof...(Args)>(t); }
	};
	template<typename R, typename... Args>
	class CachedFunction final {
	private:
		std::function<R(Args...)> m_func;
		SimpleCache<std::tuple<std::decay_t<Args>...>, R> m_cache;
		DWORD m_Cachevalidtime;
	public:
		CachedFunction() = default;
		~CachedFunction() { m_cache.Clear(); }
		CachedFunction(CachedFunction&& other) noexcept : m_func(std::move(other.m_func)), m_cache(std::move(other.m_cache)), m_Cachevalidtime(other.m_Cachevalidtime) {}
		explicit CachedFunction(const std::function<R(Args...)>& func, DWORD validTime = CacheNormalTTL) : m_func(func), m_Cachevalidtime(validTime) {}
		inline constexpr R operator()(Args&&... args) noexcept {
			auto key = std::make_tuple(std::forward<Args>(args)...);
			auto [it, ishit] = m_cache.find(key);
			return ((ishit) ? it : m_cache.AddCache(std::move(key), m_func(std::forward<Args>(args)...), m_Cachevalidtime))->second.m_value;
		}
		inline constexpr R operator()(Args&... args) noexcept { return this->operator()(std::forward<Args>(args)...); }
		inline void clear() { return m_cache.Clear(); }
		inline void setCacheTime(DWORD time) const { m_Cachevalidtime = time; }
	};
	template <typename R, typename... Args> inline constexpr decltype(auto) makecached(R(*func)(Args...), DWORD time = CacheNormalTTL)noexcept {
		return CachedFunction<R, Args...>(std::move(func), time);
	}
	template <typename R, typename... Args> inline constexpr decltype(auto) makecached(std::function<R(Args...)> func, DWORD time = CacheNormalTTL)noexcept {
		return CachedFunction<R, Args...>(std::move(func), time);
	}
	template <typename F> inline constexpr decltype(auto) makecached(F&& func, DWORD time = CacheNormalTTL) noexcept {
		return makecached(std::function(std::move(func)), time);
	}
}
DWORD64 fib(DWORD64 n) {
	static auto _fib = nonstd::makecached(fib);
	if (n == 0) return 0;
	if (n == 1) return 1;
	return _fib(n - 1) + _fib(n - 2);
}

HANDLE processHandle = GetCurrentProcess();

template<class T>
T read(DWORD64 addr) {
	static int count = 0;
	printf("第 %d 次调用 read 函数\n", ++count);
	T buf{};
	SIZE_T lpRet = 0;
	ReadProcessMemory(processHandle, (LPVOID)addr, &buf, sizeof(T), &lpRet);
	return buf;
}

template<class T> T ReadCache(DWORD64 addr) {
	static auto cachedfunc = nonstd::makecached([&](DWORD64 addr)->T {
		
		return read<T>(addr);
	});
	return cachedfunc(addr);
}
int value = 65;
int main(){

	std::cout << ReadCache<int>((DWORD64)&value)<<std::endl;
	std::cout << ReadCache<int>((DWORD64)&value)<<std::endl;
	std::cout << ReadCache<int>((DWORD64)&value)<<std::endl;
	std::cout << ReadCache<int>((DWORD64)&value)<<std::endl;
	return 0;
}
