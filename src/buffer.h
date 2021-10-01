#ifndef _buffer_h
#define _buffer_h
#include "nonblocking_queue.h"
#include "thread_pool.h"
#include "pvalue.h"

#include <memory>
#include <atomic>

namespace fabreq {
    class BufferBase {
    protected:
        std::string m_name;
        bool m_done;
        using ptask_type=std::weak_ptr<typename ThreadPool::Task>;
        ptask_type m_source, m_sink;
       
    public:
        BufferBase(std::string name): m_name(name),m_done(false) {}
        virtual ~BufferBase() {};
        void addSource(ptask_type p) {
            assert(m_source.expired());
            m_source=p;
        }
        void addSink(ptask_type p) {
            assert(m_sink.expired());
            m_sink=p;
        }
        const std::string &getName() {return m_name;}
        bool hasSource() {return !m_source.expired();}
        bool hasSink() {return !m_sink.expired();}
        bool isDone() {return m_done;}
        void done() {m_done=true;}
    };

    template<class S,class T=void>    
    class Buffer : public BufferBase {
    public:
        using item_type=pvalue_type<S,T>;
    private:
        NonBlockingQueue<item_type> m_left,m_right;
        
        template<class P>
        static void deleter_(NonBlockingQueue<item_type> &q, P *p) {
            if constexpr (is_tuple<P>::value) 
                for_each(*p,[](auto &a) {reset(a);});
            q.push_front(item_type(p, [&q](auto a) {deleter_(q,a);}));
        }
        item_type alloc_item() {return item_type([this](auto p) {deleter_(this->m_left,p);});}
        bool m_alloc;
    public:
        Buffer(std::string name,size_t size=0):BufferBase(name),m_left(size),m_alloc(size==0) {
//            for(auto &v : m_left) alloc_item().swap(v);
            for_each(m_left.begin(), m_left.end(), [this](auto &v){this->alloc_item().swap(v);});
        }
        

        ~Buffer() override {}

        void put(item_type &item) {
            if(!isDone())
                m_right.push_back(std::move(item));
        }

        void putBack(item_type &item) {
            m_right.push_front(std::move(item));
        }

        item_type getFree() {
            item_type p;
            m_left.pop(p);
            
            if(!p.empty())
                return p;
            else if(m_alloc)
                return alloc_item();
            else
                return item_type();
        }

        void get(item_type &item) {
            m_right.pop(item);
        }
    };
}
#endif