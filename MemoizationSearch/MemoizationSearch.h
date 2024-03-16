#include <functional>
#include <unordered_map>
#include <chrono>
#include <tuple>
#include <type_traits>
#include <memory>
typedef unsigned long       _DWORD;
namespace std {
    template<typename... T>
    struct hash<tuple<T...>> {
        size_t operator()(const tuple<T...>& t) const noexcept {
            return hash_value(t, index_sequence_for<T...>{});
        }
    private:
        template<typename Tuple, size_t... I>
        static size_t hash_value(const Tuple& t, index_sequence<I...>) noexcept {
            size_t seed = 0;
            (..., (seed ^= hash<typename tuple_element<I, Tuple>::type>{}(get<I>(t)) + 0x9e3779b9 + (seed << 6) + (seed >> 2)));
            return seed;
        }
    };
}
namespace nonstd {
    constexpr _DWORD CacheNormalTTL = 200;
    class CachedFunctionBase {
    protected:
        _DWORD cacheTime_;
    public:
        explicit CachedFunctionBase(_DWORD cacheTime = CacheNormalTTL) : cacheTime_(cacheTime) {}
        void setCacheTime(_DWORD cacheTime) { cacheTime_ = cacheTime; }
        virtual void clearCache() = 0;
    };
    template<typename R, typename... Args>
    class CachedFunction : public CachedFunctionBase {
        mutable std::function<R(Args...)> func_;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, R> cache_;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, std::chrono::steady_clock::time_point> expiry_;
    public:
        explicit CachedFunction(std::function<R(Args...)> func, _DWORD cacheTime = CacheNormalTTL)
            : CachedFunctionBase(cacheTime), func_(std::move(func)) {}
        R operator()(Args... args) const {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            auto now = std::chrono::steady_clock::now();
            auto it = expiry_.find(argsTuple);
            if (it != expiry_.end() && it->second > now) {
                return cache_.at(argsTuple);
            }
            else {
                expiry_.erase(it);
                return this->operator()(args...);
            }
            R result = std::apply(func_, argsTuple);
            cache_[argsTuple] = result;
            expiry_[argsTuple] = now + std::chrono::milliseconds(cacheTime_);
            return result;
        }
        void clearCache() override {
            cache_.clear();
            expiry_.clear();
        }
    };
    template<typename R>
    class CachedFunction<R> : public CachedFunctionBase {
        mutable std::function<R()> func_;
        mutable R cachedResult_;
        mutable std::chrono::steady_clock::time_point expiry_;
    public:
        explicit CachedFunction(std::function<R()> func, _DWORD cacheTime = CacheNormalTTL)
            : CachedFunctionBase(cacheTime), func_(std::move(func)) {}
        R operator()() const {
            auto now = std::chrono::steady_clock::now();
            if (expiry_ > now) {
                return cachedResult_;
            }
            cachedResult_ = func_();
            expiry_ = now + std::chrono::milliseconds(cacheTime_);
            return cachedResult_;
        }
        void clearCache() override {}
    };
    template <typename F>
    struct function_traits : function_traits<decltype(&F::operator())> {};
    template <typename R, typename... Args>
    struct function_traits<R(*)(Args...)> {
        using return_type = R;
        using args_tuple_type = std::tuple<Args...>;
    };
    template <typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>> {
        using return_type = R;
        using args_tuple_type = std::tuple<Args...>;
    };
    template <typename ClassType, typename R, typename... Args>
    struct function_traits<R(ClassType::*)(Args...) const> {
        using return_type = R;
        using args_tuple_type = std::tuple<Args...>;
    };
    template<typename F, size_t... Is>
    auto makecached_impl(F&& f, _DWORD time, std::index_sequence<Is...>) {
        using traits = function_traits<std::decay_t<F>>;
        return CachedFunction<typename traits::return_type, std::tuple_element_t<Is, typename traits::args_tuple_type>...>(
            std::function<typename traits::return_type(std::tuple_element_t<Is, typename traits::args_tuple_type>...)>(std::forward<F>(f)), time);
    }
    template<typename F>
    auto makecached(F&& f, _DWORD time = CacheNormalTTL) {
        using traits = function_traits<std::decay_t<F>>;
        return makecached_impl(std::forward<F>(f), time, std::make_index_sequence<std::tuple_size<typename traits::args_tuple_type>::value>{});
    }
}
/*
* 这段代码定义了一个C++中的缓存系统，用于记忆函数调用的结果，这可能有助于提高对于重复使用相同参数调用的昂贵操作的性能。它包括自定义std::tuple的哈希，通过CachedFunction类实现的缓存机制，以及用于函数类型推导的实用程序。

std::tuple的自定义哈希
通过为std::tuple计算所有元素的组合哈希值，来扩展std::hash以支持std::tuple。这是因为缓存使用的std::unordered_map需要可哈希的键，而默认情况下std::tuple没有哈希函数。
CachedFunctionBase类
一个抽象基类，为缓存函数定义了通用属性和行为，如管理缓存生命周期（cacheTime_）和清除缓存的虚拟方法（clearCache()）。
CachedFunction<R, Args...>模板类
一个从CachedFunctionBase继承的模板类，为具有参数(Args...)和返回类型(R)的函数实现缓存机制。
它存储一个代表要缓存的可调用对象的std::function，一个从参数到结果的缓存映射，以及用于跟踪缓存条目何时应该失效的过期映射。
operator()方法检查给定参数的缓存是否存在且未过期。如果是，它返回缓存的结果。否则，它计算结果，更新缓存，并返回新的结果。
CachedFunction<R>的专门化
CachedFunction模板的一个专门化版本，用于没有参数的函数。因为没有涉及参数，所以通过直接存储单个结果及其过期时间来简化了缓存。
function_traits模板
一组模板，用于推导函数的返回类型和参数类型。这用于根据给定函数自动推断实例化CachedFunction对象所需的类型。
makecached和makecached_impl函数
这些实用函数简化了CachedFunction对象的创建。makecached推导函数的签名，并将创建转发给makecached_impl，后者使用正确的类型实例化一个CachedFunction。
使用std::function和std::unordered_map进行运行时多态和缓存。
它旨在缓存函数调用的结果，这对于计算昂贵且预计会用相同的参数集重复调用这些函数的情况特别有用。
缓存机制的实现灵活，可以应用于任意数量参数和不同返回类型的函数。
如同为std::hash<std::tuple<T...>>所做的那样，扩展std命名空间通常不推荐，可能导致未定义行为。这里出于演示目的这么做。
对于函数调用非常昂贵（例如，在计算时间上）且预期将这些函数与相同的参数集合重复调用多次的场景，这个缓存系统可能特别有用
 */