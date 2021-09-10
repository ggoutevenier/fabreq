#include "fabreq/fabreq.h"
#include <mutex>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <gtest/gtest.h>

bool test_merge() {
    fabreq::Context cx;
    std::array<int,32> seed;
    std::generate_n(seed.begin(),seed.size(),[](){static Input i; return ++i.v;});
    
    auto &in1_buffer = fabreq::source<decltype(seed)::value_type>(
                cx, 
                "source", 
                [seed](auto &v) {
                    static std::atomic<int> idx=-1;
                    idx+=2;
                    if(idx>seed.size()) 
                        return fabreq::SourceStatus::no_more_data;

                    v=seed.at(idx-1); 
                    return fabreq::SourceStatus::more_data;
                },
                1,1
            );
 
    auto &in2_buffer = fabreq::source<decltype(seed)::value_type>(
                cx, 
                "source", 
                [seed](auto &v) {
                    static std::atomic<int> idx=0;
                    idx+=2;
                    if(idx>seed.size()) 
                        return fabreq::SourceStatus::no_more_data;

                    v=seed.at(idx-1); 
                    return fabreq::SourceStatus::more_data;
                },
                1,1
            );
    
    auto [out_buffer] = 
            fabreq::merge<int>(
                cx,
                "merge",
                in1_buffer,
                in2_buffer,
                2
            );

    std::vector<int> results;
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

    cx.run(20);

    uint32_t u,v;
    u = v = 0;
    
    std::for_each(seed.begin(),seed.end(),[&u](auto &a) {u+=a.v;});
    std::for_each(results.begin(),results.end(),[&v](auto &a) {v+=a.v;});
    return u==v;
}

TEST(Merge, TEST1) {
    EXPECT_TRUE(test_merge());
}


