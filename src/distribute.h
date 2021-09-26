#ifndef _distribute_h
#define _distribute_h
#include "context.h"
#include "tuple_helper.h"

#include <functional>
#include <tuple>
#include <vector>
#include <sstream>

namespace fabreq {
 
    template<class _BufferIn, class _BufferTupleOut>
    class Distribute {
    public:
        Distribute(
            _BufferIn &in, 
            _BufferTupleOut &outs
        ):m_in(in) {
            m_outs.reserve(Size);
            for_each(outs, [this](auto &out) {
                this->m_outs.push_back(out);
            });        
        }
        ~Distribute(){
            for(auto out : m_outs) out.second.get().done();
        }

        static auto create(_BufferIn &in, _BufferTupleOut &outs) {
            return std::make_shared<Distribute<_BufferIn, _BufferTupleOut>>(in, outs);
        }

        bool operator()() {
            typename _BufferIn::item_type inV;

            m_in.get(inV);
            while(!inV.empty()) {            
                int i=0;
                while(!m_outs.at(i).first(*(inV.getTrans()))) i++;
                m_outs.at(i).second.get().put(inV);
                m_in.get(inV);
            }
            if(m_in.isDone()) {
                return true;
            }
            return false;
        }

    private:
        _BufferIn &m_in;
        constexpr static auto Size = std::tuple_size_v<std::decay_t<_BufferTupleOut>>;
        using _Func = typename std::function<bool(const typename _BufferIn::item_type::Trans::element_type &)>;
        std::vector<std::pair<_Func,std::reference_wrapper<_BufferIn>>> m_outs;
    };

    template<class B, class... Fs>
    auto distribute(
        Context &context,
        std::string name,
        B &in,
        Fs... funcs
    ) {
        int n=0;
        auto out_tuple = transform_tuple(
            std::make_tuple(funcs...,[](const auto &){return true;}),
            [&context,name,&n](auto &f) { 
                std::stringstream ss;
                ss << name << "-" << n++;
                return std::make_pair(f,std::ref(context.buffer<B>(ss.str())));
            }
        );

        auto distibute_ptr = Distribute<B,decltype(out_tuple)>::create(in, out_tuple);
        auto task = context.addTask(
                        name,
                        [distibute_ptr]() {return (*distibute_ptr)();},
                        1
                );

        for_each(out_tuple, [&task](auto &out) {out.second.addSource(task);}); 
        in.addSink(task);

        return transform_tuple(out_tuple,[](auto &out_tuple) {return std::ref(out_tuple.second);});
    }

}
#endif
