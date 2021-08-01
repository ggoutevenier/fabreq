#ifndef _context_h
#define _context_h
#include "tuple_helper.h"
#include "thread_pool.h"
#include "buffer.h"

#include <list>
#include <iostream>

namespace fabreq {
    class Context {
    public:   
        template<class _Buffer>
        decltype(auto) buffer(std::string name) {
            using B = typename std::remove_reference_t<_Buffer>;
            m_buffers.push_back(std::make_unique<B>(name));
            return static_cast<B&>(*m_buffers.back());
        }

        template<class _Task, class _Callback>
        auto addTask(std::string name, _Task task, _Callback callback, int max_tasks) {
            return m_threads.addTask(name, task, callback, max_tasks);
        }

        void run(int n) {
            for(auto &b : m_buffers) {
                if(b->hasSource() && !b->hasSink() && !b->isDone()) {
                    b->done();
                    std::cerr << "NO SINK ON BUFFER " << b->getName() <<std::endl;;
                }
            }
            m_threads.run(n);
        }

    private:
        ThreadPool m_threads;
        std::list<std::unique_ptr<BufferBase>> m_buffers;
    };
}
#endif