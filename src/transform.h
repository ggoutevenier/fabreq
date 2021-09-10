#ifndef _transform_h
#define _transform_h
#include "context.h"
#include <functional>
#include <memory>

namespace fabreq {
//    enum class TransformStatus {OK=0,ERROR=1};
    template<class In, class Out, class F>
    class Transform {
    public:     
        Transform(In &in, In &err, Out &out, F f): in(in),err(err), out(out),f(f){}
    
        static auto create(In &in, In &err, Out &out, F f) {
            return std::make_shared<Transform<In, Out, F>>(in, err, out, f);
        }

    bool operator()() {    
        typename In::item_type inV;
        typename Out::item_type outV;
        in.get(inV);
        while(inV.getTrans().get()) {
            outV = out.getFree();
            if(outV.empty()) {
                in.putBack(inV);
                return false;
            }
            try {
                f(*inV.getTrans().get(),*outV.getTrans().get());
                typename In::item_type::Source &a = inV.getSource();
                typename In::item_type::Source &b = outV.getSource();
                a.swap(b);
                out.put(outV);
            } catch(...) {
                err.put(inV);
            }
            in.get(inV);
        }
        return in.isDone();
    }
    void done() {
        out.done();
        err.done();
    }
private:
    In &in, &err;
    Out &out;
    F f;
};

template<class B, class In,class F>
auto transform(
    Context &context,
    std::string name,
    In &in,
    F func,
    int max_tasks=1
) {
    using Out = Buffer<typename In::item_type::Source::element_type,B>;
    auto &out = context.buffer<Out>(name);
    auto &err = context.buffer<In>(name);
    auto transform_ptr = std::make_shared<Transform<In, Out, decltype(func)>>(in, err, out, func);

    auto task = context.addTask(
                        name,
                        [transform_ptr](){return transform_ptr->operator()();},
                        [transform_ptr](){transform_ptr->done();},
                        max_tasks
                    );
    in.addSink(task);
    out.addSource(task);
    err.addSource(task);
    return std::tie(err,out);
}

}
#endif