#ifndef _clone_h
#define _clone_h
#include "context.h"
#include "tuple_helper.h"
#include "pvalue.h"
#include "buffer_proxy.h"

#include <sstream>

namespace fabreq {
    template<class _BufferIn, class _BufferTupleOut>
    class Clone {
        using item_type = typename _BufferIn::item_type;
    public:
        Clone(_BufferIn &in, _BufferTupleOut outs, _BufferIn &term):m_in(in),m_outs(outs),m_proxy(term) { }    
        static auto create(_BufferIn &in, _BufferTupleOut outs, _BufferIn &term) {
            return std::make_shared<Clone<_BufferIn, _BufferTupleOut>>(in, outs, term);
        }

        bool operator()() {
            item_type inV;
            m_in.get(inV);
            while(!inV.empty()) {
                constexpr auto size = std::tuple_size_v<std::decay_t<_BufferTupleOut>>;
                auto &item = m_proxy.put(inV, size);

                for_each(
                    m_outs, 
                    [this, &item](auto &out) {
                        out.put(m_proxy.get(item));
                    }
                );
                m_in.get(inV);
            }
            if(m_in.isDone()) {
                return m_proxy.isDone();
            }
            return false;
        }
    
        void done() {
            for_each(m_outs, [](auto &out) {out.done();});
        }
    private:
        _BufferIn &m_in;
        _BufferTupleOut m_outs;
        BufferProxy<_BufferIn> m_proxy;
        std::mutex m_mutex;
    };

    template<size_t _CloneCount, class _Buffer>
    auto clone(
        Context &context,
        std::string name,
        _Buffer &in
    ) {
        auto outs = make_tuple<_CloneCount>(
            [&context,&name](auto i) -> _Buffer&{
                std::stringstream ss;
                ss << name << "-" << i;
                _Buffer &o = context.buffer<_Buffer>(ss.str());
                return std::ref(o); 
            }
        );

        auto &term = context.buffer<_Buffer>(name+"-term");
        auto clone_ptr = Clone<
                            std::remove_reference_t<decltype(in)>, 
                            decltype(outs)
                        >::create(in, outs, term);

        auto task = context.addTask(
                            name,                         
                            [clone_ptr](){return clone_ptr->operator()();},
                            [clone_ptr](){clone_ptr->done();
                        }
                        ,1); 

        for_each(outs,[&task](auto &out) {out.addSource(task);});
        term.addSource(task);
        in.addSink(task);

        return std::tuple_cat(std::tie(term),outs);
    }
}
#endif