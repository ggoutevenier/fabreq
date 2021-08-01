#include "fabreq/fabreq.h"
#include <mutex>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <gtest/gtest.h>
struct Input {
    int v;
    Input(int v):v(v) {};
    Input() = default;
    Input(const Input &i) = default;
    Input &operator=(const Input &a) = default;
};

struct Output {
    int v;
/*    Output(int i):v(i) {};    
    Output() = default;
    Output(const Output &i) = default;
    Output &operator=(const Output &a) = default;*/
};

bool test_transform() {
    fabreq::Context cx;
    std::array<Input,32> seed;
    std::generate_n(seed.begin(),seed.size(),[](){static Input i; return ++i.v;});
    
    auto &in_buffer = fabreq::source<decltype(seed)::value_type>(
                cx, 
                "source", 
                [seed](auto &v) {
                    static std::atomic<int> idx=0;
                    idx++;
                    if(idx>seed.size()) 
                        return fabreq::SourceStatus::no_more_data;

                    v=seed.at(idx-1); 
                    return fabreq::SourceStatus::more_data;
                },
                10,1
            );
    
    auto [error_buffer, out_buffer] = 
            fabreq::transform<Output>(
                cx,
                "transform",
                in_buffer,
                [](const auto &a,auto &b) {
                    if(a.v==1)
                    throw std::exception();
                    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10));
                    b.v=a.v*2;
                }
            );

    std::vector<Output> results;
            fabreq::sink_no_error(
                        cx,
                        "sink",
                        out_buffer,
                        [&results](const auto &u, const auto &v) {
                            static std::mutex mutex;
                            std::lock_guard<std::mutex> lock(mutex);
                            results.push_back(v);
                        },
                        4
                    );

    std::vector<Input> errors;
    fabreq::sink_no_error(
        cx,
        "sink-err",
        error_buffer,
        [&errors](const auto &u, const auto &v) {
            static std::mutex mutex;
            std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10));            
            {
                std::lock_guard<std::mutex> lock(mutex);
                errors.push_back(u);
            }
        },1);

    cx.run(20);

    uint32_t u,v,e;
    u=0;
    v=0;
    e=0;
    
    std::for_each(seed.begin(),seed.end(),[&u](auto &a) {u+=a.v;});
    std::for_each(results.begin(),results.end(),[&v](auto &a) {v+=a.v;});
    std::for_each(errors.begin(),errors.end(),[&e](auto &a) {e+=a.v;});
    return (u-e)*2==v && e==1;
}

TEST(Transform, TEST1) {
//    EXPECT_TRUE(test_transform());
}


