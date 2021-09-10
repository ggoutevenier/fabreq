#include "fabreq/fabreq.h"

#include <mutex>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <gtest/gtest.h>
#include <future>

template<class T, size_t N>
decltype(auto) make_source(fabreq::Context &cx, const std::array<T, N> &seed) {
    return fabreq::source<T>(
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
}

template<class T>
void sink_no_error(fabreq::Context &cx, std::string name, fabreq::Buffer<T> &buffer,std::vector<T> &results) { 
    fabreq::sink_no_error(cx, name, buffer, 
                        [&results](const auto &u, const auto &v) {
                            static std::mutex mutex;
                            std::this_thread::sleep_for(std::chrono::milliseconds(std::rand()%100));            
                            std::lock_guard<std::mutex> lock(mutex);
                            results.push_back(u);
                        }
                    ,1);
}

bool test_clone() {
    fabreq::Context cx;
    std::array<int,32> seed;
    std::generate_n(seed.begin(),seed.size(),[](){static int i(0); return ++i;});
    
    auto &source_buffer = make_source(cx,seed);
 
    auto [term_buffer, a_buffer, b_buffer] =
        fabreq::clone<2>(
            cx,
            "clone",
            source_buffer
        );

    std::vector<int> results[2];
    sink_no_error(cx, "sink-a", a_buffer, results[0]);
    sink_no_error(cx, "sink-b", b_buffer, results[1]);

    cx.run(20);

    uint32_t u,v[2];
    u=v[0]=v[1]=0;
    
    std::for_each(seed.begin(),seed.end(),[&u](auto &a) {u+=a;});
    std::for_each(results[0].begin(),results[0].end(),[&v](auto &a) {v[0]+=a;});
    std::for_each(results[1].begin(),results[1].end(),[&v](auto &a) {v[1]+=a;});
    return u==v[0] && u==v[1];
}

bool test_clone_term() {
    fabreq::Context cx;
    std::array<int,1> seed;
    std::generate_n(seed.begin(),seed.size(),[](){return 1;});
    
    auto &source_buffer = make_source(cx,seed);
    auto [term_buffer, a_buffer] = fabreq::clone<1>(cx, "clone", source_buffer);
    
    std::promise<bool> promise_1;  auto future_1 = promise_1.get_future();    
    std::promise<bool> promise_2;  auto future_2 = promise_2.get_future();    
    
    fabreq::sink_no_error(cx, "sink-a", a_buffer, 
                        [&future_1](const auto &u, const auto &v) {
                            future_1.wait_for(std::chrono::milliseconds(100));
                            LOG4CXX_DEBUG(logger, "test/clone sink-a ");
                        }
                    ,100);
    
    int result=0;
    fabreq::sink_no_error(cx, "sink-term", term_buffer, 
                        [&result, &promise_2](const auto &u, const auto &v) {
                            result=9;
                            LOG4CXX_DEBUG(logger, "test/clone sink-term should run after test/clone sink-a");
                            promise_2.set_value(true);

                        }
                    ,1);

    bool rtn;
    std::thread t([&rtn, &result, &promise_1, &future_2]() {
        rtn = result==0;
        LOG4CXX_DEBUG(logger, "test/clone begin");
        promise_1.set_value(false);
        future_2.wait_for(std::chrono::milliseconds(300));
        rtn &= result==9;
        LOG4CXX_DEBUG(logger, "test/clone end");
    });
    t.detach();
    cx.run(4);

    return rtn;

}

TEST(Clone, term_collapse) {
    EXPECT_TRUE(test_clone_term());
}

TEST(Clone, TEST1) {
    EXPECT_TRUE(test_clone());
}
