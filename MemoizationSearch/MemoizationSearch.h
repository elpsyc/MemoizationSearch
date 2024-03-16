#include <functional>
#include <unordered_map>
#include <chrono>
#include <tuple>
#include <type_traits>
#include <memory>
#include <mutex>
typedef unsigned long       _DWORD;
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
            R result = std::apply(func_, argsTuple);
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
    template<typename F, size_t... Is>
    INLINE auto makecached_impl(F&& f, _DWORD time, std::index_sequence<Is...>) NOEXCEPT {
        using traits = function_traits<std::decay_t<F>>;
        return CachedFunction<typename traits::return_type, std::tuple_element_t<Is, typename traits::args_tuple_type>...>(
            std::function<typename traits::return_type(std::tuple_element_t<Is, typename traits::args_tuple_type>...)>(std::forward<F>(f)), time);
    }
    template<typename F>
    INLINE auto makecached(F&& f, _DWORD time = CacheNormalTTL) NOEXCEPT {
        using traits = function_traits<std::decay_t<F>>;
        return makecached_impl(std::forward<F>(f), time, std::make_index_sequence<std::tuple_size<typename traits::args_tuple_type>::value>{});
    }
}
#undef INLINE
#undef NOEXCEPT
/*
* ��δ��붨����һ��C++�еĻ���ϵͳ�����ڼ��亯�����õĽ�����������������߶����ظ�ʹ����ͬ�������õİ�����������ܡ��������Զ���std::tuple�Ĺ�ϣ��ͨ��CachedFunction��ʵ�ֵĻ�����ƣ��Լ����ں��������Ƶ���ʵ�ó���

std::tuple���Զ����ϣ
ͨ��Ϊstd::tuple��������Ԫ�ص���Ϲ�ϣֵ������չstd::hash��֧��std::tuple��������Ϊ����ʹ�õ�std::unordered_map��Ҫ�ɹ�ϣ�ļ�����Ĭ�������std::tupleû�й�ϣ������
CachedFunctionBase��
һ��������࣬Ϊ���溯��������ͨ�����Ժ���Ϊ����������������ڣ�cacheTime_���������������ⷽ����clearCache()����
CachedFunction<R, Args...>ģ����
һ����CachedFunctionBase�̳е�ģ���࣬Ϊ���в���(Args...)�ͷ�������(R)�ĺ���ʵ�ֻ�����ơ�
���洢һ������Ҫ����Ŀɵ��ö����std::function��һ���Ӳ���������Ļ���ӳ�䣬�Լ����ڸ��ٻ�����Ŀ��ʱӦ��ʧЧ�Ĺ���ӳ�䡣
operator()���������������Ļ����Ƿ������δ���ڡ�����ǣ������ػ���Ľ���������������������»��棬�������µĽ����
CachedFunction<R>��ר�Ż�
CachedFunctionģ���һ��ר�Ż��汾������û�в����ĺ�������Ϊû���漰����������ͨ��ֱ�Ӵ洢��������������ʱ�������˻��档
function_traitsģ��
һ��ģ�壬�����Ƶ������ķ������ͺͲ������͡������ڸ��ݸ��������Զ��ƶ�ʵ����CachedFunction������������͡�
makecached��makecached_impl����
��Щʵ�ú�������CachedFunction����Ĵ�����makecached�Ƶ�������ǩ������������ת����makecached_impl������ʹ����ȷ������ʵ����һ��CachedFunction��
ʹ��std::function��std::unordered_map��������ʱ��̬�ͻ��档
��ּ�ڻ��溯�����õĽ��������ڼ��㰺����Ԥ�ƻ�����ͬ�Ĳ������ظ�������Щ����������ر����á�
������Ƶ�ʵ��������Ӧ�����������������Ͳ�ͬ�������͵ĺ�����
��ͬΪstd::hash<std::tuple<T...>>��������������չstd�����ռ�ͨ�����Ƽ������ܵ���δ������Ϊ�����������ʾĿ����ô����
���ں������÷ǳ��������磬�ڼ���ʱ���ϣ���Ԥ�ڽ���Щ��������ͬ�Ĳ��������ظ����ö�εĳ������������ϵͳ�����ر�����
 */