#ifndef _transform_h
#define _transform_h
#include "context.h"
#include <functional>
#include <memory>

namespace fabreq {
    template<class In, class Out, class F>
class Transform {
public:     
        Transform(In &in, In &err, Out &out, F &&f): in(in),err(err), out(out),f(std::forward<F>(f)){}
        ~Transform(){}
        static auto create(In &in, In &err, Out &out, F &&f) {
            return std::make_shared<Transform<In, Out, F>>(in, err, out, std::forward<F>(f));
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
                auto &a = inV.getSource();
                auto &b = outV.getSource();
                a.swap(b);
                out.put(outV);
            } catch(...) {
                err.put(inV);
            }

            inV.getTrans().reset();
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
    F &&func,
    int max_tasks=1
) {
    using Out = Buffer<typename In::item_type::Source::element_type,B>;
    auto &out = context.buffer<Out>(name+"-out");
    auto &err = context.buffer<In>(name+"-err");
    auto transform_ptr = std::make_shared<Transform<In, Out, decltype(func)>>(in, err, out, std::forward<F>(func));

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