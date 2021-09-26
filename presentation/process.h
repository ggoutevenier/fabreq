#ifndef __process_h
#define __process_h
#include "fabreq/fabreq.h"
#include "data_element.h"

namespace presentation {

template<class Source, class Trans1, class Trans2, class Sink, class Error>
void process_serial2(
    Source &source, 
    const Trans1 &trans1, 
    const Trans2 &trans2, 
    Sink &&sink,
    Error &&error
) {
    Input input;
    Output1 output1;
    Output2 output2;

    while(source(input)==fabreq::SourceStatus::more_data) {
        try {
            trans1(input,output1);
            trans2(output1,output2);
            sink(input,output2);
        } catch(...) {
            error(input);
        }
    }
}

template<class Source, class Trans1, class Trans2, class Sink, class Error>
void process_parallel2(
    Source &source, 
    const Trans1 &trans1, 
    const Trans2 &trans2, 
    Sink &&sink,
    Error &&error
) {
    fabreq::Context cx;
    
    auto &in_buffer = fabreq::source<Input>(cx, "source", source, 20 /*buffer size*/);
    
    auto [error_buffer1, out_buffer1] = 
            fabreq::transform<Output1>(cx, "transform1", in_buffer, trans1,  4/* parallel degree */);

    auto [error_buffer2, out_buffer2] = 
            fabreq::transform<Output2>(cx, "transform2", out_buffer1, trans2, 4);

    fabreq::sink_no_error(cx, "sink", out_buffer2, sink);
    fabreq::sink_no_error(cx, "sink-err", error_buffer1, error);

    cx.run(4);  // run on n threads
}
}
#endif