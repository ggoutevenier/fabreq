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
        Source(_Buffer &out, _Func f, int size):m_out(out),m_func(f),m_size(size) {}
        static auto create(_Buffer &out, _Func f, int size ) {
            return std::make_shared<Source<_Buffer, _Func>>(out, f, size);
        }

        bool operator()() noexcept {
            for(;;) {
                auto item = m_out.getFree(m_size);
                if(item.empty()) break;
                if(m_func(*item.getSource())==SourceStatus::no_more_data) 
                    return true;
                m_out.put(item);
            }
            return false;
        }

        void done() {m_out.done();};
    private:
        _Buffer &m_out;
        _Func m_func;
        std::atomic<int> m_size;
    };

    template<class S,class F>
    auto &source(
        Context &context,
        std::string name,
        F func,
        //SourceStatus(*func)(S &), 
        int max_buff=1,
        int max_tasks=1
    ) {
        using B = Buffer<S>;
 //       using F = decltype(func);

        auto &out = context.buffer<B>(name);
        auto source_ptr = Source<B,F>::create(out,func,max_buff);

        auto task = context.addTask(
                        name,
                        [source_ptr](){return (*source_ptr)();},
                        [source_ptr](){source_ptr->done();},
                        max_tasks
                    ); 
        out.addSource(task);
        return out;
    }
}
#endif