#ifndef _source_h
#define _source_h
#include "context.h"
#include <functional>
#include <memory>
#include <iostream>
#include <atomic>

namespace fabreq {
    enum struct SourceStatus {more_data=1, no_more_data=2}; 

    template<class _Buffer, class _Func>
    class Source {
    public:
        Source(_Buffer &out, _Func &&f):m_out(out),m_func(std::forward<_Func>(f)) {}
        ~Source() {
            m_out.done();
        }
        static auto create(_Buffer &out, _Func &&f) {
            return std::make_shared<Source<_Buffer, _Func>>(out, std::forward<_Func>(f));
        }

        bool operator()() noexcept {
            for(;;) {
                auto item = m_out.getFree();
                if(item.empty()) break;
                if(m_func(*item.getSource())==SourceStatus::no_more_data) {
                    item.getSource().reset();
                    return true;
                }
                m_out.put(item);
            }
            return false;
        }

    private:
        _Buffer &m_out;
        _Func m_func;
    };

    template<class S,class F>
    auto &source(
        Context &context,
        std::string name,
        F &&func,
        int max_buff=1
    ) {
        using B = Buffer<S>;

        auto &out = context.buffer<B>(name,max_buff);
        auto source_ptr = Source<B,F>::create(out,std::forward<F>(func));

        auto task = context.addTask(
                        name,
                        [source_ptr](){return (*source_ptr)();},
                        1
                    ); 

        out.addSource(task);
        return out;
    }
}
#endif