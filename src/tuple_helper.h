#ifndef _tuple_helper_h
#define _tuple_helper_h
#include <tuple>

namespace fabreq {
    template<typename T>
    class ShowType;
    
    template<typename Func, size_t... Is>
    auto make_tuple_(Func&& func ,  std::index_sequence<Is...>) {
        return std::tie(func(Is)...); // std::make_tuple decays types
    }

    template<size_t N, typename Func>
    auto make_tuple(Func&& func ){
        using indices = std::make_index_sequence<N>;
	    return make_tuple_(std::forward<Func>(func), indices{});
    }

    template<typename Tuple, typename Func, size_t... Is>
    void for_each_(Tuple&& tuple, Func&& func, std::index_sequence<Is...>)
    {
        auto l = { (func(std::get<Is>(tuple)), 0)... };
    }

    template<typename Tuple, typename Func>
    void for_each(Tuple && tuple, Func&& func)
    {
        constexpr auto size = std::tuple_size_v<std::decay_t<Tuple>>; //connot have referenes
        using indices = std::make_index_sequence<size>;
        for_each_(std::forward<Tuple>(tuple), std::forward<Func>(func), indices{});
    }

    template<typename Tuple, typename Func, size_t... Is>
    auto transform_tuple_(Tuple&& tuple, Func&& func, std::index_sequence<Is...>)
    {
        return std::make_tuple(func(std::get<Is>(tuple))...);
    }

    template<typename Tuple, typename Func>
    auto transform_tuple(Tuple&& tuple, Func&& func)
    {
        constexpr auto size = std::tuple_size_v<std::decay_t<Tuple>>; //connot have referenes
        using indices = std::make_index_sequence<size>;
        return transform_tuple_(std::forward<Tuple>(tuple), std::forward<Func>(func), indices{});
    }

    template <typename> struct is_tuple: std::false_type {};
    template <typename ...P> struct is_tuple<std::tuple<P...>>: std::true_type {};

}
#endif