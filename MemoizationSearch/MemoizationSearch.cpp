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
		inline iterator EraseCache(const _Tx& value) {
			std::unique_lock<mutextype> lock(m_mutex);
			auto iter = m_Cache.find(value);
			iterator ret = m_Cache.end();
			if (iter != m_Cache.end()) ret = m_Cache.unsafe_erase(iter);
			return ret;
		}
		inline std::pair<iterator, bool> FindCache(const _Tx& _key) {
			if (m_Cache.empty()) return { iterator(),false };
			auto iter = m_Cache.find(_key);
			return { iter, iter != m_Cache.end() && iter->second.IsValid(std::chrono::system_clock::now()) };
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
		using KeyType = std::tuple<std::decay_t<Args>...>;
		using PairType = std::pair<R, std::tuple<std::decay_t<Args>...>>;
		SimpleCache<KeyType, PairType> m_cache;
		DWORD m_Cachevalidtime;
	public:
		CachedFunction() = default;
		CachedFunction(CachedFunction&& other) noexcept : m_func(std::move(other.m_func)), m_cache(std::move(other.m_cache)), m_Cachevalidtime(other.m_Cachevalidtime) {}
		explicit CachedFunction(const std::function<R(Args...)>& func, DWORD validTime = CacheNormalTTL) : m_func(func), m_Cachevalidtime(validTime) {}
		inline constexpr R operator()(Args&&... args) noexcept {
			auto key = std::make_tuple(std::forward<Args>(args)...);
			auto [it, ishit] = m_cache.FindCache(key);
			if (ishit) {
				auto [result, cached_args] = it->second.m_value;
				return result;
			}
			else {
				R result = m_func(std::forward<Args>(args)...);
				m_cache.AddCache(std::move(key), std::make_pair(result, std::make_tuple(std::forward<Args>(args)...)), m_Cachevalidtime);
				return result;
			}
		}
	};
	template <typename R, typename... Args>
	inline constexpr CachedFunction<R, Args...> makecached(R(*func)(Args...), DWORD time = CacheNormalTTL)noexcept {
		return CachedFunction<R, Args...>(std::move(func), time);
	}
}
DWORD64 fib(DWORD64 n) {
	static auto _fib = nonstd::makecached(fib);
	if (n == 0) return 0;
	if (n == 1) return 1;
	return _fib(n - 1) + _fib(n - 2);
}
int main(){
	
	std::cout << fib(1000) << std::endl;
	return 0;
}
