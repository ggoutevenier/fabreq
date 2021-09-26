#ifndef _merge_h
#define _merge_h
#include "context.h"
#include "tuple_helper.h"
#include <memory>
#include <vector>
#include <functional>

namespace fabreq {
    template<class _BufferInTuple,class _BufferOut>
    class Merge {
    public:
        Merge(_BufferInTuple& ins, _BufferOut &out):m_ins(ins),m_out(out) {}
        ~Merge() {
            m_out.done();
        }
        static auto create(_BufferInTuple &ins, _BufferOut &out) {
            return std::make_shared<Merge<_BufferInTuple,_BufferOut>>(ins, out);
        }

        bool operator()() {
            bool done=true;
  
            for_each(m_ins, [&done,this](auto &in) {
                done=done&&in.isDone();
                typename std::remove_reference_t<decltype(in)>::item_type inV;
                in.get(inV);
                while(!inV.empty()) {
                    this->m_out.put(inV);
                    in.get(inV);
                }
            });
            return done;
        }
    private:
        _BufferInTuple m_ins;
        _BufferOut &m_out;
    };

    template<class In, class... Ins>
    decltype(auto) merge(
        Context &context,
        std::string name,
        In& in,
        Ins&... ins
    ) {       
        auto in_tuple = std::tie(in,ins...);
        auto &out = context.buffer<In>(name);

        auto merge_ptr = Merge<decltype(in_tuple),decltype(out)>::create(in_tuple,out);
        auto task = context.addTask(name,
                            [merge_ptr](){return (*merge_ptr)();},
                            1
                        ); 
        for_each(in_tuple, [&task](auto &in) {in.addSink(task);});         
        out.addSource(task);

        return out;
    }
}
#endif
