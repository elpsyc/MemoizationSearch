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
template<typename... T>
struct Hasher {
    static size_t hash_value(const std::tuple<T...>& t) noexcept {return hash_impl(t, std::index_sequence_for<T...>{});}
private:
    template<typename Tuple, size_t... I>static size_t hash_impl(const Tuple& t, std::index_sequence<I...>) noexcept {
        size_t seed = 0;
        using expander = int[];
        (void)expander {
            0, ((seed ^= std::hash<typename std::tuple_element<I, Tuple>::type>{}(std::get<I>(t)) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), 0)...
        };
        return seed;
    }
};
namespace std {
    template<typename... T>struct hash<std::tuple<T...>> {
        size_t operator()(const std::tuple<T...>& t) const noexcept {
            return Hasher<T...>::hash_value(t);
        }
    };
}
namespace nonstd {
    template<size_t... Indices>struct index_sequence {};
    template<size_t N, size_t... Indices>struct make_index_sequence : make_index_sequence<N - 1, N - 1, Indices...> {};
    template<size_t... Indices>struct make_index_sequence<0, Indices...> : index_sequence<Indices...> {};
    template<typename F, typename Tuple, size_t... Indices>
    auto apply_impl(F&& f, Tuple&& tuple, index_sequence<Indices...>) -> decltype(auto) {
        return f(std::get<Indices>(std::forward<Tuple>(tuple))...);
    }
    template<typename F, typename Tuple>
    auto apply(F&& f, Tuple&& tuple) -> decltype(auto) {
        return apply_impl(
            std::forward<F>(f),
            std::forward<Tuple>(tuple),
            make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>{}
        );
    }
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
        mutable std::mutex m_mutex;
        inline R operator()(Args&&... args) const{
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            auto now = std::chrono::steady_clock::now();
            auto it = m_expiry.find(argsTuple);
            if (it != m_expiry.end() && it->second > now) return m_cache.at(argsTuple);
            it = m_expiry.find(argsTuple);
            if (it != m_expiry.end() && it->second > now) return m_cache.at(argsTuple);
            auto result = apply(m_func, argsTuple);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache[argsTuple] = result;
            m_expiry[argsTuple] = now + std::chrono::milliseconds(m_cacheTime);
            return result;
        }
        inline void clearArgsCache() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache.clear(), m_expiry.clear(); 
        }
    };
    template<typename R> struct CachedFunction<R> : public CachedFunctionBase {
        mutable std::function<R()> m_func;
        mutable R m_cachedResult;
        mutable std::chrono::steady_clock::time_point m_expiry;
        explicit CachedFunction(const std::function<R()>& func, unsigned long cacheTime = g_CacheNormalTTL) : CachedFunctionBase(cacheTime), m_func(std::move(func)) {}
        inline R operator()() const noexcept {
            auto now = std::chrono::steady_clock::now();
            if (m_expiry > now) return m_cachedResult;
            m_cachedResult = m_func();
            m_expiry = now + std::chrono::milliseconds(m_cacheTime);
            return m_cachedResult;
        }
        inline void clearCache() noexcept { m_expiry = std::chrono::steady_clock::now(); }
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
        static std::mutex m_mutex;
        static std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> m_cache;
        template <typename R, typename... Args>
        static CachedFunction<R, Args...>& GetCachedFunction(void* funcPtr, const std::function<R(Args...)>& func, unsigned long cacheTime = g_CacheNormalTTL) {
            std::unique_lock<std::mutex> lock(m_mutex);
            auto& funcMap = m_cache[std::type_index(typeid(CachedFunction<R, Args...>))];
            auto insertResult = funcMap.try_emplace(funcPtr, std::make_shared<CachedFunction<R, Args...>>(func, cacheTime));
            return *std::static_pointer_cast<CachedFunction<R, Args...>>(insertResult.first->second);
        }
        void ClearCache() {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache.clear(); 
        }
    };
    std::mutex CachedFunctionFactory::m_mutex;
    decltype(CachedFunctionFactory::m_cache) CachedFunctionFactory::m_cache;
    template<typename F, size_t... Is>inline auto& makecached_impl(F&& f, unsigned long time, std::index_sequence<Is...>) noexcept {
        std::function<typename function_traits<std::decay_t<F>>::return_type(typename std::tuple_element<Is, typename function_traits<std::decay_t<F>>::args_tuple_type>::type...)> func(std::forward<F>(f));
        return CachedFunctionFactory::GetCachedFunction(&f, func, time);
    }
    template<typename F>inline auto& makecached(F&& f, unsigned long time = g_CacheNormalTTL) noexcept {
        return makecached_impl(f, time, std::make_index_sequence<std::tuple_size<typename function_traits<std::decay_t<F>>::args_tuple_type>::value>{});
    }
}
#endif // !MEMOIZATIONSEARCH
/*
* 编辑时间:2024.03.17
* 自定义std::hash特化：代码首先为std::tuple类型特化了std::hash结构，以便可以将tuple作为键用在std::unordered_map中。这是通过计算元组中每个元素的哈希值并将它们组合成一个单一的哈希值来实现的。

CachedFunctionBase结构体：这是一个基础结构体，定义了缓存有效时间m_cacheTime。它提供了设置缓存时间的方法，但禁止了拷贝和移动构造函数，确保其实例不会被不当复制或移动。

CachedFunction模板类：这是模板化的主要功能实现类。对于具有不同参数列表的函数，它使用std::function来存储函数指针，并使用两个std::unordered_map来缓存函数的结果和结果的到期时间。它重载了operator()，使得当调用缓存化函数时，会先检查缓存是否有效，如果有效就返回缓存结果，否则计算新结果并更新缓存。

无参数的特化：对于无参数函数的特殊处理，提供了一个特化版本的CachedFunction，它简化了存储和获取缓存结果的逻辑。

function_traits模板结构体：用于提取函数的返回类型和参数类型，支持普通函数指针、std::function以及成员函数指针。这对于将函数封装为std::function时，能够正确处理参数类型非常关键。

CachedFunctionFactory类：提供了一个静态方法GetCachedFunction，用于根据函数指针和缓存时间创建或获取一个CachedFunction实例。同时，它管理一个全局的缓存实例映射，用于存储和重用CachedFunction实例。还提供了一个ClearCache方法，用于清除所有缓存。

makecached和makecached_impl函数：这是一组便利函数，用于简化CachedFunction实例的工厂创建。它利用function_traits和完美转发来自动推导函数的参数和返回类型，并创建相应的CachedFunction实例 每种函数的实例只会创建一次。
*/