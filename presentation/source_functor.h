#ifndef __source_funtor_h
#define __source_funtor_h

namespace presentation {
/*  simple UDF source for demo*/
template<class T>
class SourceFunctor {
    T m_data;
    typename T::const_iterator m_it;
public:
    SourceFunctor(T &&seed):m_data(std::move(seed)) {
        reset();
    }

//    SourceFunctor(SourceFunctor &&sf):m_data(std::move(sf.m_seed)), m_it(sf.m_it) {}
    ~SourceFunctor(){}
    fabreq::SourceStatus operator()(typename T::value_type &v) {
        if(m_it == m_data.end()) 
           return fabreq::SourceStatus::no_more_data;

        v=*m_it;
        ++m_it; 
        return fabreq::SourceStatus::more_data;
    }
    void reset() {m_it=m_data.cbegin();}
};
}
#endif