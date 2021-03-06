#ifndef _sink_h
#define _sink_h
#include "context.h"
#include <functional>
#include <memory>

namespace fabreq {
    template<class _Buffer, class _Func>
    class Sink {
    public:
        Sink(_Buffer &in, _Buffer &err,_Func &&func) : m_in(in),m_err(err), m_func(std::forward<_Func>(func)){}
        Sink() {
            m_err.done();
        }
        static auto create(_Buffer &in, _Buffer &err, _Func f) {
            return std::make_shared<Sink<_Buffer, _Func>>(in, err, f);
        }

        bool operator ()() {        
            typename _Buffer::item_type v;
            m_in.get(v);
            while(!v.empty()) {
                try {
                    m_func(*v.getSource(), *v.getTrans());
                } catch(...) {
                    m_err.put(v);
                }
                v.getTrans().reset();
                v.getSource().reset();
                m_in.get(v);
            }
            return m_in.isDone();
        }
    private:
        _Buffer &m_in, &m_err;
        _Func m_func;
    };

    template<class _Buffer,class _Func>
    decltype(auto) sink(
        Context &context,
        std::string name,
        _Buffer &in,
        _Func &&func
    ) {
        auto &err = context.buffer<_Buffer>(name+"-errors");

        auto sink_ptr = Sink<std::remove_reference_t<
                                        decltype(in)>,
                                        decltype(func)
                                >::create(in, err, std::forward<_Func>(func));        

        auto task = context.addTask(
                    name,
                    [sink_ptr](){return (*sink_ptr)();},
                    1
                );
                
        in.addSink(task);
        err.addSource(task);

        return err;
    }

    template<class _Buffer,class _Func>
    void sink_no_error(
        Context &context,
        std::string name,
        _Buffer &in,
        _Func &&func
    ) {
        auto &err = sink(context,name,in,std::forward<_Func>(func));
        err.done();
    }
}
#endif