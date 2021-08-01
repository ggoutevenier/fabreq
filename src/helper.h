#ifndef _helper_h
#define _helper_h
#include <type_traits>
#include <utility>

namespace fabreq {

template <class Tuple, class F, std::size_t... I>
F for_each_impl(Tuple&& t, F&& f, std::index_sequence<I...>)
{
    return (void)std::initializer_list<int>{(std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))),0)...}, f;
}
        
template <class F, class Tuple>
constexpr decltype(auto) for_each(Tuple&& t, F&& f)
{
    return for_each_impl(std::forward<Tuple>(t),std::forward<F>(f),
                std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

template<class F, std::size_t... I>
F repeat_impl(F&& f ,  std::index_sequence<I...>) {
    return (void)std::initializer_list<int>{(std::forward<F>(f)(I),0)...}, f;
}

template<std::size_t N, class F>
constexpr decltype(auto) repeat(F&& f){
    return repeat_impl(std::forward<F>(f), std::make_index_sequence<N>{});
}
}
#endif