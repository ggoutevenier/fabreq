#include "fabreq/fabreq.h"
#include <mutex>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <gtest/gtest.h>

bool test_distribute() {
    fabreq::Context cx;
    std::array<int,32> seed;
    std::generate_n(seed.begin(),seed.size(),[](){static int i(0); return ++i;});
    
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
                10,1
            );
    auto [a_buffer,default_buffer] =
        fabreq::distribute(
            cx,
            "distribute",
            source_buffer,
            [](const auto &a) {return a%2==0;}
        );

    std::vector<int> results[2];
            fabreq::sink_no_error(
                        cx,
                        "sink-a",
                        a_buffer,
                        [&results](const auto &u, const auto &v) {
                            static std::mutex mutex;
                            std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10));            
                            std::lock_guard<std::mutex> lock(mutex);
                            results[0].push_back(u);
                        }
                    ,1);

            fabreq::sink_no_error(
                        cx,
                        "sink-default",
                        default_buffer,
                        [&results](const auto &u, const auto &v) {
                            static std::mutex mutex;
                            std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%10));            
                            std::lock_guard<std::mutex> lock(mutex);
                            results[1].push_back(u);
                        }
                    ,1);

    cx.run(20);

    uint32_t u,v[2];
    u=v[0],v[1]=0;
    
    std::for_each(seed.begin(),seed.end(),[&u](auto &a) {u+=a;});
    std::for_each(results[0].begin(),results[0].end(),[&v](auto &a) {v[0]+=a;});
    std::for_each(results[1].begin(),results[1].end(),[&v](auto &a) {v[1]+=a;});
    return u==v[0]+v[1] && v[0]!=0 && v[1]!=0;
}

TEST(Distribute, TEST1) {
    EXPECT_TRUE(test_distribute());
}


