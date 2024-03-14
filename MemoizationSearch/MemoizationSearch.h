#pragma once
#include <algorithm>
#include <concurrent_unordered_map.h>
#include <shared_mutex>
#include <vector>
#include<future>
#include <functional>
#include <Windows.h>
#include <typeindex>
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
		CacheItem(const _Tx&& _value, const timepoint& _endtime) :m_value(std::move(_value)), m_endtime(_endtime) {}
		~CacheItem() { m_value.~_Tx(); }
		inline bool IsValid(timepoint now)noexcept { return now < m_endtime; }
	};
	template<typename _Tx, typename _Ty>
	class SimpleBasicCache {
	public:
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
				iterator ret = m_Cache.end();
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
						for (auto it = m_Cache.begin(); it != m_Cache.end();) {
							it = (!it->second.IsValid(now)) ? m_Cache.unsafe_erase(it) : ++it;
						}
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
			iterator ret = m_Cache.end();
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
		inline std::pair<iterator, bool>& operator[](const _Tx& _key) {
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
	class CachedFunction {
	private:
		std::function<R(Args...)> func_;
		mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, R> cache_;
		mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, std::chrono::steady_clock::time_point> expiry_;
		DWORD cacheTime_ = CacheNormalTTL;
		CachedFunction(const CachedFunction&) = delete;
		CachedFunction& operator=(const CachedFunction&) = delete;
		CachedFunction(CachedFunction&&) = delete;
		CachedFunction& operator=(CachedFunction&&) = delete;
		template<typename... T>
		R callFunctionAndCache(std::tuple<T...> argsTuple) const {
			auto now = std::chrono::steady_clock::now();
			if (auto it = expiry_.find(argsTuple); it != expiry_.end() && it->second > now) {
				return cache_[argsTuple];
			}
			R result = std::apply(func_, argsTuple);
			cache_[argsTuple] = result;
			expiry_[argsTuple] = now + std::chrono::milliseconds(cacheTime_);
			return result;
		}
	public:
		CachedFunction(std::function<R(Args...)> func, DWORD cacheTime = CacheNormalTTL) : func_(std::move(func)), cacheTime_(cacheTime) {}
		R operator()(Args... args) const {
			auto argsTuple = std::make_tuple(args...);
			return callFunctionAndCache(argsTuple);
		}
		R operator()() const {
			if constexpr (sizeof...(Args) == 0) {
				std::tuple<> argsTuple;
				return callFunctionAndCache(argsTuple);
			}else {
				static_assert(sizeof...(Args) == 0, "This function can only be called with no arguments.");
			}
		}
		void setCacheTime(DWORD cacheTime) {
			cacheTime_ = cacheTime;
		}
		void clearCache() {
			cache_.clear();
			expiry_.clear();
		}
	};
	class CachedFunctionFactory {
	private:
		static std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> cache_;
	public:
		template <typename R, typename... Args>
		static CachedFunction<R, Args...>& GetCachedFunction(void* funcPtr, std::function<R(Args...)> func, DWORD cacheTime = CacheNormalTTL) {
			std::type_index key = std::type_index(typeid(CachedFunction<R, Args...>));
			void* ptrKey = funcPtr; // 使用函数指针地址或唯一标识作为键
			auto& funcMap = cache_[key];
			if (funcMap.find(ptrKey) == funcMap.end()) {
				auto cachedFunc = std::make_shared<CachedFunction<R, Args...>>(func, cacheTime);
				funcMap[ptrKey] = cachedFunc;
			}
			return *std::static_pointer_cast<CachedFunction<R, Args...>>(funcMap[ptrKey]);
		}
		static void ClearCache() {
			cache_.clear();
		}
	};
	std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> CachedFunctionFactory::cache_;
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