#ifndef __sink_functor_h
#define __sink_functor_h
#include "tuple_helper.h"
namespace presentation {
/*  simple UDF sink for demo*/
template<class T>
class SinkFunctor {
    T *m_ptr;
public:
    SinkFunctor(T &r):m_ptr(&r) {}
//    SinkFunctor(SinkFunctor &&) = default;
    ~SinkFunctor(){}

    template<class A,class B>
    void operator()(const A &u, const B &v) {
        if constexpr (fabreq::is_tuple<typename T::value_type>::value)
            m_ptr->push_back(std::tie(u,v));
        else
            m_ptr->push_back(v);
    }

    template<class A>
    void operator()(const A &v) {
        if constexpr (fabreq::is_tuple<A>::value && fabreq::is_tuple<typename T::value_type>::value)
            m_ptr->push_back(v);
        else if constexpr (!fabreq::is_tuple<A>::value && !fabreq::is_tuple<typename T::value_type>::value)
            m_ptr->push_back(v);
    }

    const T &getResults() const {return *m_ptr;}
};
}
#endif