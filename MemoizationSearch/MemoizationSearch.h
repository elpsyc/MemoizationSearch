#pragma once
#include <algorithm>
#include <concurrent_unordered_map.h>
#include <shared_mutex>
#include <vector>
#include <future>
#include <functional>
#include <Windows.h>
#include <typeindex>
namespace nonstd {
	constexpr DWORD CacheNormalTTL = 200;
	template<class Key, class Val>
	using concurrent_map = Concurrency::concurrent_unordered_map<Key, Val>;
	template<typename _Tx>
	struct CacheItem {
		using timepoint = std::chrono::time_point<std::chrono::system_clock>;
		timepoint m_endtime;
		_Tx   m_value;
		CacheItem() = default;
		CacheItem(const _Tx& _value, const timepoint& _endtime) :m_value(_value), m_endtime(_endtime) {}
		CacheItem(const _Tx&& _value, const timepoint& _endtime) :m_value(std::move(_value)), m_endtime(_endtime) {}
		~CacheItem() { m_value.~_Tx(); }
		inline bool IsValid(const timepoint& now)noexcept { return now < m_endtime; }//因为重载bool是不能有参数的 能用已有的时间再次求是性能浪费
	};
	template<typename _Tx, typename _Ty>
	struct SimpleBasicCache {
		concurrent_map<_Tx, CacheItem<_Ty>> m_Cache;
		using pair_type = typename std::decay_t<decltype(m_Cache)>::value_type;
		using iterator = typename std::decay_t<decltype(m_Cache)>::iterator;
		mutable std::shared_mutex m_mutex;
		using mutextype = decltype(m_mutex);
		SimpleBasicCache() { srand((unsigned int)time(0)); }
		~SimpleBasicCache()noexcept {
			std::unique_lock<mutextype> lock(m_mutex);
			m_Cache.clear();
		}
		inline iterator AddAysncCache(const _Tx& _key, const _Ty& _value, DWORD _validtime = 200) {
			auto fut= std::async(std::launch::async, [&]()->iterator {
				auto nowTime = std::chrono::system_clock::now();
				auto newValue = CacheItem<_Ty>(_value, nowTime + std::chrono::milliseconds(_validtime + rand() % 30));
				std::unique_lock<mutextype> lock(m_mutex, std::defer_lock);
				auto lb = m_Cache.find(_key);
				auto ret = m_Cache.end();
				if (lb != m_Cache.end()) {
					lb->second = newValue;
					ret = lb;
				}else {
					if (lock.try_lock()) {
						ret = m_Cache.insert(lb, pair_type(_key, newValue));
						lock.unlock();
					}
				}
				static auto firsttime = std::chrono::system_clock::now();
				auto now = std::chrono::system_clock::now();
				if (std::chrono::duration_cast<std::chrono::milliseconds>(now - firsttime).count() > 5000) {
					firsttime = now;
					if (lock.try_lock()) {
						for (auto it = m_Cache.begin(); it != m_Cache.end();)it = (!it->second.IsValid(now)) ? m_Cache.unsafe_erase(it) : ++it;
						lock.unlock();
					}
				}
				return ret;
			});
			return fut;
		}
		inline iterator erase(const _Tx& value) {
			std::shared_lock lock(m_mutex);
			auto iter = m_Cache.find(value);
			auto ret = m_Cache.end();
			std::unique_lock<mutextype> ulock(m_mutex, std::defer_lock);
			if (ulock.try_lock()) {
				if (iter != m_Cache.end()) ret = m_Cache.unsafe_erase(iter);
				lock.unlock();
				return ret;
			}
			return (iterator)m_Cache.end();
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
			for (auto& elem : v)seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);//0x9e3779b9 是黄金分割数的16进制形式
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
	class CachedFunction {
		std::function<R(Args...)> func_;
		mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, R> cache_;
		mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, std::chrono::steady_clock::time_point> expiry_;
		DWORD cacheTime_ = CacheNormalTTL;
		CachedFunction(const CachedFunction&) = delete;
		CachedFunction& operator=(const CachedFunction&) = delete;
		CachedFunction(CachedFunction&&) = delete;
		CachedFunction& operator=(CachedFunction&&) = delete;
		template<typename... T>
		R callFunctionAndCache(const std::tuple<T...>& argsTuple) const {
			auto now = std::chrono::steady_clock::now();
			if (auto it = expiry_.find(argsTuple); it != expiry_.end() && it->second > now)return cache_[argsTuple];
			auto result = std::apply(func_, argsTuple);
			cache_[argsTuple] = result;
			expiry_[argsTuple] = now + std::chrono::milliseconds(cacheTime_);
			return result;
		}
	public:
		explicit CachedFunction(const std::function<R(Args...)>& func, DWORD cacheTime = CacheNormalTTL) : func_(std::move(func)), cacheTime_(cacheTime) {}
		R operator()(Args... args) const {return callFunctionAndCache(std::make_tuple(args...));}
		void setCacheTime(DWORD cacheTime) {cacheTime_ = cacheTime;}
		void clearCache() {
			cache_.clear();
			expiry_.clear();
		}
	};
	class CachedFunctionFactory {
		static concurrent_map<std::type_index, concurrent_map<void*, std::shared_ptr<void>>> cache_;
	public:
		template <typename R, typename... Args>
		static CachedFunction<R, Args...>& GetCachedFunction(void* funcPtr,const std::function<R(Args...)>& func, DWORD cacheTime = CacheNormalTTL) {
			std::type_index key = std::type_index(typeid(CachedFunction<R, Args...>));
			auto ptrKey = funcPtr; // 使用函数指针地址或唯一标识作为键
			auto& funcMap = cache_[key];
			if (funcMap.find(ptrKey) == funcMap.end()) {
				auto cachedFunc = std::make_shared<CachedFunction<R, Args...>>(func, cacheTime);
				funcMap[ptrKey] = cachedFunc;
			}
			return *std::static_pointer_cast<CachedFunction<R, Args...>>(funcMap[ptrKey]);
		}
		static void ClearCache() {cache_.clear();}
	};
	concurrent_map<std::type_index, concurrent_map<void*, std::shared_ptr<void>>> CachedFunctionFactory::cache_;
	template <typename R, typename... Args>
	inline CachedFunction<R, Args...>& makecached(R(*func)(Args...), DWORD time = nonstd::CacheNormalTTL) noexcept {
		return CachedFunctionFactory::GetCachedFunction(reinterpret_cast<void*>(func), std::function<R(Args...)>(func), time);
	}
	template <typename R, typename... Args>
	inline CachedFunction<R, Args...>& makecached(const std::function<R(Args...)>& func, DWORD time = CacheNormalTTL) noexcept {
		return CachedFunctionFactory::GetCachedFunction(reinterpret_cast<void*>(func), std::function<R(Args...)>(func), time);
	}
	template <typename F>
	inline constexpr decltype(auto) makecached(F&& func, DWORD time = CacheNormalTTL) noexcept {
		auto tempfunc=std::function(std::move(func));
		return CachedFunctionFactory::GetCachedFunction(&tempfunc, tempfunc, time);
	}
}
/*注意本类只对有参数的函数有效
* 旨在减少重复计算的开销，特别是对于那些计算成本较高的函数。它使用模板和高级C++特性，包括并发容器、智能指针、类型推导和lambda表达式。下面是对代码主要部分的逐一解释：

1. CacheItem 类
用于存储缓存项的数据结构。
包含一个值 m_value 和一个过期时间 m_endtime。
提供一个 IsValid 方法来检查缓存项是否还在有效期内。
2. SimpleBasicCache 类
一个简单的基础缓存实现，使用并发容器 concurrent_unordered_map。
支持异步添加缓存项的功能。
提供基本的缓存操作，如添加、查找和清除缓存项。
3. CachedFunction 类
代表一个可以被缓存的函数。
内部使用 std::function 来存储任意可调用对象。
缓存的结果和过期时间存储在两个 std::unordered_map 中，键是函数参数的元组。
支持对函数的调用，并在缓存中查找之前的结果，如果找到有效的缓存，则返回缓存的结果，否则计算新结果并添加到缓存中。
4. CachedFunctionFactory 类
用于管理和创建 CachedFunction 实例的工厂类。
使用 concurrent_map 来存储所有的 CachedFunction 实例，以支持并发访问。
静态方法 GetCachedFunction 用于获取或创建一个 CachedFunction 实例。如果对应的实例已存在，则直接返回；如果不存在，则创建一个新实例并返回。
5. makecached 函数模板
提供了一种简便的方式来创建和获取 CachedFunction 实例。
对于直接函数指针和 std::function，直接通过函数指针或 std::function 实例的地址作为唯一标识符来查找或创建缓存的函数实例。
对于lambda表达式和其他可调用对象，首先将其包装为 std::function，然后使用包装后对象的地址作为唯一标识符。由于这种方法会导致每次调用 makecached 都创建一个新的 std::function 实例，可能需要调整以避免潜在的性能问题。
总结
这个缓存系统的设计旨在提供一个灵活且通用的方式来缓存函数调用的结果，特别是对于那些计算成本较高的函数。它通过使用高级C++特性和并发编程技术，使得缓存系统能够在多线程环境中安全运行。然而，使用这个系统时需要注意管理缓存的生命周期和性能开销，特别是在处理lambda表达式和其他无法直接获取地址的可调用对象时
* 
*/