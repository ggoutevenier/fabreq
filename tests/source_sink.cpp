#include "fabreq/fabreq.h"
#include <mutex>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <gtest/gtest.h>

bool test_source_sink() {
    fabreq::Context cx;
    std::array<int,32> seed;
    std::generate_n(seed.begin(),seed.size(),[](){static int i=1; return i++;});
    
    auto &source_buffer = fabreq::source<decltype(seed)::value_type>(
                cx, 
                "source-a", 
                [seed](auto &v) {
                    static std::atomic<int> idx=0;
                    idx++;
                    if(idx>seed.size()) 
                        return fabreq::SourceStatus::no_more_data;

                    v=seed.at(idx-1); 
                    return fabreq::SourceStatus::more_data;
                },
                10
            );
    
    std::vector<int> results;
    auto &error_buffer = 
                    fabreq::sink(
                        cx,
                        "sink-a",
                        source_buffer,
                        [&results](const auto &u, const auto &v) {
                            static std::mutex mutex;
                            std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10));            
                            {
                                if(u==1) {
                                    throw std::exception();
                            }
                            std::lock_guard<std::mutex> lock(mutex);
                            results.push_back(u);
                        }
                    });

    std::vector<int> errors;
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
        });

    cx.run(2);

    uint32_t u,v,e;
    u=0;
    v=0;
    e=0;
    
    std::for_each(seed.begin(),seed.end(),[&u](auto &a) {u+=a;});
    std::for_each(results.begin(),results.end(),[&v](auto &a) {v+=a;});
    std::for_each(errors.begin(),errors.end(),[&e](auto &a) {e+=a;});
    return u==v+e && e==1;
}



TEST(SourceSink, TEST1) {
    EXPECT_TRUE(test_source_sink());
}


