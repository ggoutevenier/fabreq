#ifndef _join_h
#define _join_h
#include "context.h"
#include "tuple_helper.h"
#include <memory>
#include <map>
#include <functional>
#include <mutex>
#include <tuple>
namespace fabreq {

template<class Term, class Ins,class Out>
class Join {
    using T = typename std::remove_reference_t<Out>::item_type;
    using Tuples = typename T::Trans::element_type;
//    ShowType<TuplePtrs> dummy;
    std::mutex m_mutex;
    std::map<const void *, Tuples> m_outs;

    template<class B>
    void process(B &in) {
        using In = typename B::item_type;
        In inV;
        in.get(inV);
        while(!inV.empty()) {
            void *idx = inV.getSource().get();            
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                inV.getTrans().swap(std::get<typename In::Trans>(m_outs[idx]));
            }
            in.get(inV);
        }
    }

    void processTerm(Term &in, bool &done) {
        typename Term::item_type inV;
        in.get(inV);
        while(!inV.empty()) {
            auto outV = out.getFree();
            void *idx = inV.getSource().get();
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                outV.getTrans()->swap(m_outs.at(idx));
            }

            outV.getSource().swap(inV.getSource());
            out.put(outV);
            in.get(inV);
        }
        done&=in.isDone();            
    }
public:
    Join(
        Term& term,
        Ins ins,
        Out& out
    ):term(term),ins(ins),out(out) {
    }
    bool operator()() {
        bool done=true;

        for_each(
            ins, 
            [this](auto &in) {this->process(in);}
        );
        processTerm(term,done);
        return done;
    }
    void done() {out.done();}
private:
    Term& term;
    Ins ins;
    Out& out;
};

    template<class Term, class... Ins>
    auto &join(
        Context &context,
        std::string name,
        Term& term,
        Ins&... ins        
    ) {
        auto iTuples = std::tuple<Ins&...>(ins...);
        using S = typename Term::item_type::Source::element_type;

        auto t =
                transform_tuple(
                    iTuples,
                    [](auto &in) {
                        using item_type = typename std::remove_reference_t<decltype(in)>::item_type::Trans;
                        return item_type();
                    }
                );
        

        auto &out = context.buffer<Buffer<S, decltype(t)>>(name);
        auto f = std::make_shared<Join<
                    std::remove_reference_t<decltype(term)>,
                    decltype(iTuples),
                    decltype(out)>
                >(term,iTuples,out);

        auto p = context.addTask(
            name, 
            [f](){return f->operator()();},
            [f](){f->done();},
            1
        ); 

        for_each(iTuples, [&p](auto &in) {in.addSink(p);});
        term.addSink(p);
        out.addSource(p);
  
        return out;
    };
}
#endif