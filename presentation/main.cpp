#ifndef DEBUG
#define DEBUG
#endif
#include "process.h"
#include "data_element.h"
#include "source_functor.h"
#include "sink_functor.h"
#include "transform_functor.h"
#include <mutex>
#include <cstdlib>

std::mutex cout_mutex;

namespace presentation {
std::vector<Input> generate_data(int n) {
    std::vector<Input> data(n);

    std::generate_n(
            data.begin(), 
            data.size(),
            [i=Input()]() mutable {
                int v =  i.getValue();
                v++;
                i.setValue(v);
                return i;
            });
    return data;
}

void run_processes() {
    SourceFunctor source(generate_data(10));  // populates vector with values 1-10

    RefData refData(2); // greates a rate of 2
    MultipleFunctor1 transform1(refData);
    MultipleFunctor2 transform2(refData);

// structures to hold output data
    struct Results {
        std::vector<std::tuple<Input,Output2>> outputs;
        std::vector<Input> errors;
        double runtime;
        int output_total;
        int error_total;
        Results():output_total(0),error_total(0) {}
    } serial_results, parallel_results;

// serial run   
    {
        auto start = std::chrono::high_resolution_clock::now();
        process_serial2(
            source, 
            transform1, 
            transform2, 
            SinkFunctor(serial_results.outputs),
            SinkFunctor(serial_results.errors)
        );        
        auto end = std::chrono::high_resolution_clock::now();
        serial_results.runtime = std::chrono::duration<double,std::milli>(end-start).count();
    }

// parallel run
    source.reset();
    {
        auto start = std::chrono::high_resolution_clock::now();
        process_parallel2(
            source, 
            transform1, 
            transform2,  
            SinkFunctor(parallel_results.outputs), 
            SinkFunctor(parallel_results.errors)
        );
        auto end = std::chrono::high_resolution_clock::now();
        parallel_results.runtime = std::chrono::duration<double,std::milli>(end-start).count();

    }

// agg results
    for(auto &v : parallel_results.outputs) parallel_results.output_total+=std::get<1>(v).getValue();
    for(auto &v : parallel_results.errors) parallel_results.error_total+=v.getValue();
    for(auto &v : serial_results.outputs) serial_results.output_total+=std::get<1>(v).getValue();
    for(auto &v : serial_results.errors) serial_results.error_total+=v.getValue();

#ifdef DEBUG
    std::cout << "Serial totals "
                << serial_results.runtime << "," 
                << serial_results.output_total << ", "
                << serial_results.error_total << "\n";

    std::cout << "Parallel totals " 
                << parallel_results.runtime << "," 
                << parallel_results.output_total << ", "
                << parallel_results.error_total << std::endl;
#endif
}
}

int main(int argc, char **argv) {
    presentation::run_processes();
#ifdef DEBUG
    new char; //force a memory leak 
#endif
    return 0;
}

