#include <functional>
#include <unordered_map>
#include <chrono>
#include <tuple>
#include <type_traits>
#include <memory>
#include <mutex>
#include <typeindex>
#ifndef MEMOIZATIONSEARCH
#define MEMOIZATIONSEARCH
namespace std {
    template<typename... T>struct hash<tuple<T...>> {
        inline size_t operator()(const tuple<T...>& t) const noexcept { return hash_value(t, index_sequence_for<T...>{}); }
        template<typename Tuple, size_t... I>inline static size_t hash_value(const Tuple& t, index_sequence<I...>) noexcept {
            size_t seed = 0;
            (..., (seed ^= hash<typename tuple_element<I, Tuple>::type>{}(get<I>(t)) + 0x9e3779b9 + (seed << 6) + (seed >> 2)));
            return seed;
        }
    };
}
namespace nonstd {
    constexpr unsigned long g_CacheNormalTTL = 200;
    struct CachedFunctionBase {
        unsigned long m_cacheTime;
        CachedFunctionBase(const CachedFunctionBase&) = delete;
        CachedFunctionBase& operator=(const CachedFunctionBase&) = delete;
        CachedFunctionBase(CachedFunctionBase&&) = delete;
        CachedFunctionBase& operator=(CachedFunctionBase&&) = delete;
        explicit CachedFunctionBase(unsigned long cacheTime = g_CacheNormalTTL) : m_cacheTime(cacheTime) {}
        inline void setCacheTime(unsigned long cacheTime)noexcept { m_cacheTime = cacheTime; }
    };
    template<typename R, typename... Args>struct CachedFunction : public CachedFunctionBase {
        mutable std::function<R(Args...)> m_func;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, R> m_cache;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, std::chrono::steady_clock::time_point> m_expiry;
        explicit CachedFunction(const std::function<R(Args...)>& func, unsigned long cacheTime = g_CacheNormalTTL) : CachedFunctionBase(cacheTime), m_func(std::move(func)) {}
        inline R operator()(Args&&... args) const noexcept {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            auto now = std::chrono::steady_clock::now();
            auto it = m_expiry.find(argsTuple);
            if (it != m_expiry.end() && it->second > now) return m_cache.at(argsTuple);
            static std::mutex mtx;
            std::unique_lock<std::mutex> lock(mtx);
            it = m_expiry.find(argsTuple);
            if (it != m_expiry.end() && it->second > now) return m_cache.at(argsTuple);
            auto result = std::apply(m_func, argsTuple);
            m_cache[argsTuple] = result;
            m_expiry[argsTuple] = now + std::chrono::milliseconds(m_cacheTime);
            return result;
        }
        inline void clearArgsCache() { m_cache.clear(), m_expiry.clear(); }
    };
    template<typename R> struct CachedFunction<R> : public CachedFunctionBase {
        mutable std::function<R()> m_func;
        mutable R m_cachedResult;
        mutable std::chrono::steady_clock::time_point m_expiry;
        explicit CachedFunction(const std::function<R()>& func, unsigned long cacheTime = g_CacheNormalTTL) : CachedFunctionBase(cacheTime), m_func(std::move(func)) {}
        inline R operator()() const noexcept {
            auto now = std::chrono::steady_clock::now();
            if (m_expiry > now) return m_cachedResult;
            static std::mutex mtx;
            std::unique_lock<std::mutex> lock(mtx);
            m_cachedResult = m_func();
            m_expiry = now + std::chrono::milliseconds(m_cacheTime);
            return m_cachedResult;
        }
    };
    template <typename F>struct function_traits : function_traits<decltype(&F::operator())> {};
    template <typename R, typename... Args>struct function_traits<R(*)(Args...)> {
        using return_type = R;
        using args_tuple_type = std::tuple<Args...>;
    };
    template <typename R, typename... Args>struct function_traits<std::function<R(Args...)>> {
        using return_type = R;
        using args_tuple_type = std::tuple<Args...>;
    };
    template <typename ClassType, typename R, typename... Args>struct function_traits<R(ClassType::*)(Args...) const> {
        using return_type = R;
        using args_tuple_type = std::tuple<Args...>;
    };
    struct CachedFunctionFactory {
        static std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> m_cache;
        template <typename R, typename... Args> static CachedFunction<R, Args...>& GetCachedFunction(void* funcPtr, const std::function<R(Args...)>& func, unsigned long cacheTime = g_CacheNormalTTL) {
            auto& funcMap = m_cache[std::type_index(typeid(CachedFunction<R, Args...>))];
            if (funcMap.find(funcPtr) == funcMap.end())funcMap[funcPtr] = std::make_shared<CachedFunction<R, Args...>>(func, cacheTime);
            return *std::static_pointer_cast<CachedFunction<R, Args...>>(funcMap[funcPtr]);
        }
        static void ClearCache() { m_cache.clear(); }
    };
    std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> nonstd::CachedFunctionFactory::m_cache;
    template<typename F, size_t... Is>inline auto& makecached_impl(F f, unsigned long time, std::index_sequence<Is...>) noexcept {
        using traits = function_traits<std::decay_t<F>>;
        std::function<typename traits::return_type(typename std::tuple_element<Is, typename traits::args_tuple_type>::type...)> func(std::forward<F>(f));
        return CachedFunctionFactory::GetCachedFunction(reinterpret_cast<void*>(+f), func, time);
    }
    template<typename F>inline auto& makecached(F&& f, unsigned long time = g_CacheNormalTTL) noexcept {
        return makecached_impl(f, time, std::make_index_sequence<std::tuple_size<typename function_traits<std::decay_t<F>>::args_tuple_type>::value>{});
    }
}
#endif // !MEMOIZATIONSEARCH