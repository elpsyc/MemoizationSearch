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
    static inline  size_t hash_value(const std::tuple<T...>& t) noexcept {return hash_impl(t, std::index_sequence_for<T...>{});}
private:
    template<typename Tuple, std::size_t... I>static inline std::size_t hash_impl(const Tuple& t,const std::index_sequence<I...>&) noexcept {
        std::size_t seed = 0;
        using expander = int[];
        (void)expander {0, ((seed ^= std::hash<typename std::tuple_element<I, Tuple>::type>{}(std::get<I>(t)) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), 0)...};
        return seed;
    }
};
template<typename... T>struct std::hash<std::tuple<T...>> { std::size_t operator()(const std::tuple<T...>& t) const noexcept {return Hasher<T...>::hash_value(t);}};
namespace nonstd {
    template<std::size_t... Indices>struct index_sequence {};
    template<std::size_t N, std::size_t... Indices>struct make_index_sequence : make_index_sequence<N - 1, N - 1, Indices...> {};
    template<std::size_t... Indices>struct make_index_sequence<0, Indices...> : index_sequence<Indices...> {};
    template<typename F, typename Tuple, std::size_t... Indices>decltype(auto) inline apply_impl(F&& f, Tuple&& tuple, index_sequence<Indices...>) {return f(std::get<Indices>(std::forward<Tuple>(tuple))...);}
    template<typename F, typename Tuple> inline decltype(auto) apply(F&& f, Tuple&& tuple)noexcept {return apply_impl(std::forward<F>(f),std::forward<Tuple>(tuple),nonstd::make_index_sequence<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>{});}
    constexpr unsigned long g_CacheNormalTTL = 200;
    struct CachedFunctionBase {
        unsigned long m_cacheTime;
        CachedFunctionBase(const CachedFunctionBase&) = delete;
        CachedFunctionBase& operator=(const CachedFunctionBase&) = delete;
        CachedFunctionBase(CachedFunctionBase&&) = delete;
        CachedFunctionBase& operator=(CachedFunctionBase&&) = delete;
        explicit CachedFunctionBase(unsigned long cacheTime = g_CacheNormalTTL) : m_cacheTime(cacheTime) {}
        inline void setCacheTime(unsigned long cacheTime) noexcept { m_cacheTime = cacheTime; }
    };
    template<typename R, typename... Args>struct CachedFunction : public CachedFunctionBase {
        mutable std::function<R(Args...)> m_func;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, R> m_cache;
        mutable std::unordered_map<std::tuple<std::decay_t<Args>...>, std::chrono::steady_clock::time_point> m_expiry;
        explicit CachedFunction(const std::function<R(Args...)>& func, unsigned long cacheTime = g_CacheNormalTTL) : CachedFunctionBase(cacheTime), m_func(std::move(func)) {}
        mutable std::mutex m_mutex;
        inline R operator()(Args&... args) const  noexcept {return this->operator()(args...);}
        inline R operator()(Args&&... args) const noexcept{
            auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
            auto now = std::chrono::steady_clock::now();
            auto it = m_expiry.find(argsTuple);
            if (it != m_expiry.end() && it->second >= now) return m_cache.at(argsTuple);
            auto result = nonstd::apply(m_func, argsTuple);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache[argsTuple] = result;
            m_expiry[argsTuple] = now + std::chrono::milliseconds(m_cacheTime);
            return result;
        }
        inline void clearArgsCache() noexcept{
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
            if (m_expiry >= now) return m_cachedResult;
            m_cachedResult = m_func();
            m_expiry = now + std::chrono::milliseconds(m_cacheTime);
            return m_cachedResult;
        }
        inline void clearCache() noexcept { m_expiry = std::chrono::steady_clock::now()- std::chrono::milliseconds(m_cacheTime); }
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
        static inline CachedFunction<R, Args...>& GetCachedFunction(void* funcPtr, const std::function<R(Args...)>& func, unsigned long cacheTime = g_CacheNormalTTL)noexcept {
            auto& funcMap = m_cache[std::type_index(typeid(CachedFunction<R, Args...>))];
            std::unique_lock<std::mutex> lock(m_mutex);//��ѯ������
            auto insertResult = funcMap.try_emplace(funcPtr, std::make_shared<CachedFunction<R, Args...>>(func, cacheTime));
            return *std::static_pointer_cast<CachedFunction<R, Args...>>(insertResult.first->second);
        }
        static inline void ClearCache() noexcept {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cache.clear(); 
        }
    };
    std::mutex CachedFunctionFactory::m_mutex;
    decltype(CachedFunctionFactory::m_cache) CachedFunctionFactory::m_cache;
    template<typename F, std::size_t... Is>inline auto& makecached_impl(F&& f, unsigned long time, std::index_sequence<Is...>) noexcept {
        std::function<typename function_traits<std::decay_t<F>>::return_type(typename std::tuple_element<Is, typename function_traits<std::decay_t<F>>::args_tuple_type>::type...)> func(std::forward<F>(f));
        return CachedFunctionFactory::GetCachedFunction(&f, func, time);
    }
    template<typename F>inline auto& makecached(F&& f, unsigned long time = g_CacheNormalTTL) noexcept {return makecached_impl(f, time, std::make_index_sequence<std::tuple_size<typename function_traits<std::decay_t<F>>::args_tuple_type>::value>{});}
}
#endif // !MEMOIZATIONSEARCH
/*
* �༭ʱ��:2024.03.17
* �Զ���std::hash�ػ�����������Ϊstd::tuple�����ػ���std::hash�ṹ���Ա���Խ�tuple��Ϊ������std::unordered_map�С�����ͨ������Ԫ����ÿ��Ԫ�صĹ�ϣֵ����������ϳ�һ����һ�Ĺ�ϣֵ��ʵ�ֵġ�

CachedFunctionBase�ṹ�壺����һ�������ṹ�壬�����˻�����Чʱ��m_cacheTime�����ṩ�����û���ʱ��ķ���������ֹ�˿������ƶ����캯����ȷ����ʵ�����ᱻ�������ƻ��ƶ���

CachedFunctionģ���ࣺ����ģ�廯����Ҫ����ʵ���ࡣ���ھ��в�ͬ�����б�ĺ�������ʹ��std::function���洢����ָ�룬��ʹ������std::unordered_map�����溯���Ľ���ͽ���ĵ���ʱ�䡣��������operator()��ʹ�õ����û��滯����ʱ�����ȼ�黺���Ƿ���Ч�������Ч�ͷ��ػ���������������½�������»��档

�޲������ػ��������޲������������⴦���ṩ��һ���ػ��汾��CachedFunction�������˴洢�ͻ�ȡ���������߼���

function_traitsģ��ṹ�壺������ȡ�����ķ������ͺͲ������ͣ�֧����ͨ����ָ�롢std::function�Լ���Ա����ָ�롣����ڽ�������װΪstd::functionʱ���ܹ���ȷ����������ͷǳ��ؼ���

CachedFunctionFactory�ࣺ�ṩ��һ����̬����GetCachedFunction�����ڸ��ݺ���ָ��ͻ���ʱ�䴴�����ȡһ��CachedFunctionʵ����ͬʱ��������һ��ȫ�ֵĻ���ʵ��ӳ�䣬���ڴ洢������CachedFunctionʵ�������ṩ��һ��ClearCache����������������л��档

makecached��makecached_impl����������һ��������������ڼ�CachedFunctionʵ���Ĺ���������������function_traits������ת�����Զ��Ƶ������Ĳ����ͷ������ͣ���������Ӧ��CachedFunctionʵ�� ÿ�ֺ�����ʵ��ֻ�ᴴ��һ�Ρ�
*/