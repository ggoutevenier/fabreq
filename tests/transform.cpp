#include "fabreq/fabreq.h"
#include <mutex>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <gtest/gtest.h>
//#define DEBUG
std::mutex cout_mutex;

class Input {
    int v;
public:
    Input(int v):v(v) {};
    Input() = default;
    Input(const Input &i) = default;
    Input &operator=(const Input &a) = default;
    void setValue(int a){v=a;}
    int getValue() const {return v;}
};

class Output {
    int v;
public:
    void setValue(int a){v=a;}
    int getValue() const {return v;}
    friend bool operator<(const Output &a,const Output &b) {return a.v<b.v;}
};

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

/*  simple UDF source for demo*/
template<class T>
class SourceFunctor {
    T m_data;
    typename T::const_iterator m_it;
public:
    SourceFunctor(T &&seed):m_data(std::move(seed)) {
        reset();
    }

    SourceFunctor(SourceFunctor &&sf):m_data(std::move(sf.m_seed)), m_it(sf.m_it) {}
    ~SourceFunctor(){}
    fabreq::SourceStatus operator()(typename T::value_type &v) {
        if(m_it == m_data.end()) 
           return fabreq::SourceStatus::no_more_data;

        v=*m_it;
        ++m_it; 
        return fabreq::SourceStatus::more_data;
    }
    void reset() {m_it=m_data.cbegin();}
};

/*  simple UDF sink for demo*/
template<class T>
class SinkFunctor {
    T *m_ptr;
public:
    SinkFunctor(T &r):m_ptr(&r) {}
    SinkFunctor(SinkFunctor &&) = default;
    ~SinkFunctor(){}
    template<class A,class B>
    void operator()(const A &u, const B &v) {
        m_ptr->push_back(v);
    }

    void operator()(const typename T::value_type &v) {
        m_ptr->push_back(v);
    }
    const T &getResults() const {return *m_ptr;}
};

class RefData {
    int m_rate;
public:
    int getRate() const {return m_rate;};
};

/*  simple UDF transform for demo*/
class MultipleFuntor {
    const RefData &m_rate;
public:
    MultipleFuntor(const RefData &r):m_rate(r){}
    MultipleFuntor(const MultipleFuntor &) = default;

    void operator() (const Input &a, Output &b) const {
        if(a.getValue()==1)
            throw std::exception();
        
        apply(a,b);
#ifdef DEBUG
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Thread: " << std::this_thread::get_id() 
                      << " in = " << a.getValue()
                      << " out = " <<b.getValue() <<std::endl;
        }
#endif
    }
    
    void apply(const Input &a, Output &b) const noexcept {
        std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%100));
        b.setValue(a.getValue()*2);//m_rate.getRate());
    }
};

template<class Source, class Trans, class Sink, class Error>
void process_serial(
    Source &source, 
    const Trans &transform, 
    Sink &sink,
    Error &error
) {
    Input input;
    Output output;

    while(source(input)==fabreq::SourceStatus::more_data) {
        try {
            transform(input,output);
            sink(output);
        } catch(...) {
            error(input);
        }
    }
}

template<class Source, class Trans, class Sink, class Error>
void process_parallel(
    Source &source, 
    const Trans &transform, 
    Sink &sink,
    Error &error
) {
    fabreq::Context cx;
    
    auto &in_buffer = fabreq::source<Input>(
                cx, 
                "source",
                source,
                20 //buffer size
            );
    
    auto [error_buffer, out_buffer] = 
            fabreq::transform<Output>(
                cx,
                "transform",
                in_buffer,
                transform,
                4 // parallel degree
            );

    fabreq::sink_no_error(
        cx,
        "sink",
        out_buffer,
        sink
    );

    fabreq::sink_no_error(
        cx,
        "sink-err",
        error_buffer,
        error
    );

    cx.run(4);  // run with 4 threads
}


bool test_transform() {
    RefData refData;

// source instance    
    SourceFunctor source(generate_data(100));

// transform instance
    MultipleFuntor transform(refData);

    struct Returns {
        std::vector<Output> outputs;
        std::vector<Input> errors;
        int output_total;
        int error_total;
        Returns():output_total(0),error_total(0) {}
    } serial_results, parallel_results;

// serial run   
    {
        SinkFunctor sink_output(serial_results.outputs);
        SinkFunctor sink_error(serial_results.errors);
        process_serial(source, transform, sink_output, sink_error);

        for(auto &v : serial_results.outputs) serial_results.output_total+=v.getValue();
        for(auto &v : serial_results.errors) serial_results.error_total+=v.getValue();
    }

    source.reset();
    {
        SinkFunctor sink_output(parallel_results.outputs);
        SinkFunctor sink_error(parallel_results.errors);
        process_parallel(source, transform, sink_output, sink_error);

        for(auto &v : parallel_results.outputs) parallel_results.output_total+=v.getValue();
        for(auto &v : parallel_results.errors) parallel_results.error_total+=v.getValue();
    }
#ifdef DEBUG
    std::cout << "Serial totals " << serial_results.output_total << ", "
                                << serial_results.error_total << std::endl;

    std::cout << "Parallel totals " << parallel_results.output_total << ", "
                                << parallel_results.error_total << std::endl;
#endif

    return  parallel_results.output_total == serial_results.output_total &&
            parallel_results.error_total == serial_results.error_total;
}

TEST(Transform, TEST1) {
    EXPECT_TRUE(test_transform());
}


