#include <functional>
#include <unordered_map>
#include <chrono>
#include <tuple>
#include <type_traits>
#include <memory>
#include <mutex>
#include <typeindex>
typedef unsigned long  _DWORD;
#define INLINE inline
#define NOEXCEPT noexcept
namespace std {
    template<typename... T>
    struct hash<tuple<T...>> {
        INLINE size_t operator()(const tuple<T...>& t) const NOEXCEPT {return hash_value(t, index_sequence_for<T...>{});}
    private:
        template<typename Tuple, size_t... I>
        INLINE static size_t hash_value(const Tuple& t, index_sequence<I...>) NOEXCEPT {
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
        CachedFunctionBase(const CachedFunctionBase&) = delete;
        CachedFunctionBase& operator=(const CachedFunctionBase&) = delete;
        CachedFunctionBase(CachedFunctionBase&&) = delete;
        CachedFunctionBase& operator=(CachedFunctionBase&&) = delete;
        explicit CachedFunctionBase(_DWORD cacheTime = CacheNormalTTL) : cacheTime_(cacheTime) {}
        INLINE void setCacheTime(_DWORD cacheTime)NOEXCEPT { cacheTime_ = cacheTime; }
        INLINE virtual void clearCache() = 0;
    };
    template<typename R, typename... Args>
    class CachedFunction : public CachedFunctionBase {
        mutable std::function<R(Args...)> func_;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, R> cache_;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, std::chrono::steady_clock::time_point> expiry_;
    public:
        explicit CachedFunction(std::function<R(Args...)> func, _DWORD cacheTime = CacheNormalTTL)
            : CachedFunctionBase(cacheTime), func_(std::move(func)) {}
        INLINE R operator()(Args... args) const NOEXCEPT {
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            auto now = std::chrono::steady_clock::now();
            auto it = expiry_.find(argsTuple);
            if (it != expiry_.end() && it->second > now) return cache_.at(argsTuple);
            static std::mutex mtx;
            std::unique_lock<std::mutex> lock(mtx);
            it = expiry_.find(argsTuple);
            if (it != expiry_.end() && it->second > now) return cache_.at(argsTuple);
            auto result = std::apply(func_, argsTuple);
            cache_[argsTuple] = result;
            expiry_[argsTuple] = now + std::chrono::milliseconds(cacheTime_);
            return result;
        }
        INLINE void clearCache() override{
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
        INLINE R operator()() const NOEXCEPT {
            auto now = std::chrono::steady_clock::now();
            if (expiry_ > now) return cachedResult_;
            cachedResult_ = func_();
            expiry_ = now + std::chrono::milliseconds(cacheTime_);
            return cachedResult_;
        }
        INLINE void clearCache() override{}
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
    class CachedFunctionFactory {
        static std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> cache_;
    public:
        template <typename R, typename... Args>
        static CachedFunction<R, Args...>& GetCachedFunction(void* funcPtr, const std::function<R(Args...)>& func, _DWORD cacheTime = CacheNormalTTL) {
            auto key = std::type_index(typeid(CachedFunction<R, Args...>));
            auto ptrKey = funcPtr; // 使用函数指针地址或唯一标识作为键
            auto& funcMap = cache_[key];
            if (funcMap.find(ptrKey) == funcMap.end()) {
                auto cachedFunc = std::make_shared<CachedFunction<R, Args...>>(func, cacheTime);
                funcMap[ptrKey] = cachedFunc;
            }
            return *std::static_pointer_cast<CachedFunction<R, Args...>>(funcMap[ptrKey]);
        }
        static void ClearCache() { cache_.clear(); }
    };
    std::unordered_map<std::type_index, std::unordered_map<void*, std::shared_ptr<void>>> nonstd::CachedFunctionFactory::cache_;
    template<typename F, size_t... Is>
    inline auto& makecached_impl(F f, _DWORD time, std::index_sequence<Is...>) noexcept {
        using traits = function_traits<std::decay_t<F>>;
        std::function<typename traits::return_type(typename std::tuple_element<Is, typename traits::args_tuple_type>::type...)> func(std::forward<F>(f));
        auto funcPtr = reinterpret_cast<void*>(+f); // 使用+操作符获取函数指针
        return CachedFunctionFactory::GetCachedFunction(funcPtr, func, time);
    }
    template<typename F>
    inline auto& makecached(F f, _DWORD time = CacheNormalTTL) noexcept {
        using traits = function_traits<std::decay_t<F>>;
        return makecached_impl(f, time, std::make_index_sequence<std::tuple_size<typename traits::args_tuple_type>::value>{});
    }
}
#undef INLINE
#undef NOEXCEPT
