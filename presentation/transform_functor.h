#ifndef __transform_h
#define __transform_h
#include "data_element.h"
#include <exception>
#include <thread>
extern std::mutex cout_mutex;

namespace presentation {
/*  simple UDF transform for demo*/
class MultipleFunctor1 {
    const RefData &m_rate;
public:
    MultipleFunctor1(const RefData &r):m_rate(r){}
//    MultipleFunctor1(const MultipleFunctor1 &) = default;

    void operator() (const Input &a, Output1 &b) const {
        if(a.getValue()==1)
            throw std::exception();
        
        apply(a,b);
#ifdef DEBUG
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Trans1 Thread: " << std::this_thread::get_id() 
                      << " in = " << a.getValue()
                      << " out = " <<b.getValue() <<std::endl;
        }
#endif
    }
    
    void apply(const Input &a, Output1 &b) const noexcept {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%100));
        b.setValue(a.getValue()*m_rate.getRate());
    }
};

class MultipleFunctor2 {
    const RefData &m_rate;
public:
    MultipleFunctor2(const RefData &r):m_rate(r){}
    MultipleFunctor2(const MultipleFunctor2 &) = default;

    void operator() (const Output1 &a, Output2 &b) const {
        apply(a,b);
#ifdef DEBUG
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Trans2 Thread: " << std::this_thread::get_id() 
                      << " in = " << a.getValue()
                      << " out = " <<b.getValue() <<std::endl;
        }
#endif
    }
    
    void apply(const Output1 &a, Output2 &b) const noexcept {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%100));
        b.setValue(a.getValue()*m_rate.getRate());
    }
};

}
#endif