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
		inline bool IsValid(const timepoint& now)noexcept { return now < m_endtime; }//��Ϊ����bool�ǲ����в����� �������е�ʱ���ٴ����������˷�
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
			for (auto& elem : v)seed ^= hasher(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);//0x9e3779b9 �ǻƽ�ָ�����16������ʽ
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
			auto ptrKey = funcPtr; // ʹ�ú���ָ���ַ��Ψһ��ʶ��Ϊ��
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
/*ע�Ȿ��ֻ���в����ĺ�����Ч
* ּ�ڼ����ظ�����Ŀ������ر��Ƕ�����Щ����ɱ��ϸߵĺ�������ʹ��ģ��͸߼�C++���ԣ�������������������ָ�롢�����Ƶ���lambda���ʽ�������ǶԴ�����Ҫ���ֵ���һ���ͣ�

1. CacheItem ��
���ڴ洢����������ݽṹ��
����һ��ֵ m_value ��һ������ʱ�� m_endtime��
�ṩһ�� IsValid ��������黺�����Ƿ�����Ч���ڡ�
2. SimpleBasicCache ��
һ���򵥵Ļ�������ʵ�֣�ʹ�ò������� concurrent_unordered_map��
֧���첽��ӻ�����Ĺ��ܡ�
�ṩ�����Ļ������������ӡ����Һ���������
3. CachedFunction ��
����һ�����Ա�����ĺ�����
�ڲ�ʹ�� std::function ���洢����ɵ��ö���
����Ľ���͹���ʱ��洢������ std::unordered_map �У����Ǻ���������Ԫ�顣
֧�ֶԺ����ĵ��ã����ڻ����в���֮ǰ�Ľ��������ҵ���Ч�Ļ��棬�򷵻ػ���Ľ������������½������ӵ������С�
4. CachedFunctionFactory ��
���ڹ���ʹ��� CachedFunction ʵ���Ĺ����ࡣ
ʹ�� concurrent_map ���洢���е� CachedFunction ʵ������֧�ֲ������ʡ�
��̬���� GetCachedFunction ���ڻ�ȡ�򴴽�һ�� CachedFunction ʵ���������Ӧ��ʵ���Ѵ��ڣ���ֱ�ӷ��أ���������ڣ��򴴽�һ����ʵ�������ء�
5. makecached ����ģ��
�ṩ��һ�ּ��ķ�ʽ�������ͻ�ȡ CachedFunction ʵ����
����ֱ�Ӻ���ָ��� std::function��ֱ��ͨ������ָ��� std::function ʵ���ĵ�ַ��ΪΨһ��ʶ�������һ򴴽�����ĺ���ʵ����
����lambda���ʽ�������ɵ��ö������Ƚ����װΪ std::function��Ȼ��ʹ�ð�װ�����ĵ�ַ��ΪΨһ��ʶ�����������ַ����ᵼ��ÿ�ε��� makecached ������һ���µ� std::function ʵ����������Ҫ�����Ա���Ǳ�ڵ��������⡣
�ܽ�
�������ϵͳ�����ּ���ṩһ�������ͨ�õķ�ʽ�����溯�����õĽ�����ر��Ƕ�����Щ����ɱ��ϸߵĺ�������ͨ��ʹ�ø߼�C++���ԺͲ�����̼�����ʹ�û���ϵͳ�ܹ��ڶ��̻߳����а�ȫ���С�Ȼ����ʹ�����ϵͳʱ��Ҫע���������������ں����ܿ������ر����ڴ���lambda���ʽ�������޷�ֱ�ӻ�ȡ��ַ�Ŀɵ��ö���ʱ
* 
*/