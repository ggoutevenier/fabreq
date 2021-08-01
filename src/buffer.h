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
        virtual void clear() = 0;
        bool isDone() {return m_done;}
        void done() {m_done=true;}
    };

    template<class S,class T=void>    
    class Buffer : public BufferBase {
    public:
        using item_type=pvalue_type<S,T>;
    private:
        NonBlockingQueue<item_type> m_left,m_right;

        template <typename> struct is_tuple: std::false_type {};
        template <typename ...P> struct is_tuple<std::tuple<P...>>: std::true_type {};

        template <typename P>
        static void reset(std::unique_ptr<P> &p) {p.reset();}

        template <typename P,typename D>
        static void reset(std::unique_ptr<P,D> &p) {p.reset();}

        template <typename P>
        static void reset(P &p) {}

        template<class P>
        static void deleter_(NonBlockingQueue<item_type> &q, P *p) {
            if constexpr (is_tuple<P>::value) 
                for_each(*p,[](auto &a) {reset(a);});
            q.push_front(item_type(p, [&q](auto a) {deleter_(q,a);}));
        }

    public:
        Buffer(std::string name):BufferBase(name) {}
        ~Buffer(){}

        void clear() override {
            item_type item;
            get(item);
            while(item.getSource().get()) {
                get(item);
            };
        }

        void put(item_type &item) {
            if(isDone())
                item.reset();
            else
                m_right.push_back(std::move(item));
        }

        void put(item_type &&item) {
            if(!isDone())
                m_right.push_back(std::move(item));
        }

        void putBack(item_type &item) {
            m_right.push_front(std::move(item));
        }

        item_type getFree(std::atomic<int> &size) {
            item_type p;
            m_left.pop(p);
            if(!p.empty())
                return p;
            
            if(size>0 && size.fetch_sub(1)>0) {
               return item_type([this](auto p) {deleter_(this->m_left,p);});
            }

            return item_type();
        }

        item_type getFree() {
            item_type p;
            m_left.pop(p);
            if(!p.empty())
                return p;
            
            return item_type([this](auto p) {deleter_(this->m_left,p);});
        }

        void get(item_type &item) {
            m_right.pop(item);
        }

    };
}
#endif