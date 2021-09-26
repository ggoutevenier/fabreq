#ifndef thread_pool_h
#define thread_pool_h

#include <thread>
#include <deque>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <cassert>

namespace fabreq {
class ThreadPool {
    int min_max(int n,int low, int high) {
        return std::min(std::max(n,low),high);
    }

public:
    using task_type = std::function<bool()>;//TODO noexcpt
    using callback_type = std::function<void()>;
    class Task {
        std::string name;   
//        callback_type dtor_callback;
    public:
//        Task(std::string n,task_type t,callback_type c):name(n), task(t),dtor_callback(c){}
        Task(std::string n,task_type t):name(n), task(t){}
        ~Task() {
//            dtor_callback();
        }
        task_type task;
    };

    std::weak_ptr<Task> addTask(std::string name, task_type task,int n=1) {
        n = min_max(n,1,256);
        auto t = std::make_shared<Task>(name, task);

        for(int i =0;i<n;i++) {
            m_tasks.push_back(t);
        }

        return t;
    }
/*    std::weak_ptr<Task> addTask(std::string name, task_type task, callback_type completion, int n=1) {
        n = min_max(n,1,256);
        auto t = std::make_shared<Task>(name, task, completion);

        for(int i =0;i<n;i++) {
            m_tasks.push_back(t);
        }

        return t;
    }*/

    void async_run(int n=std::thread::hardware_concurrency()) {
        n = min_max(n,0,256);
        for(int i=0;i<n;i++)
            m_threads.emplace_back([this](){this->process();});//std::bind(&ThreadPool::process,this));
    }

    void wait() {
        for(auto &thread : m_threads)        
            thread.join();
    }

    void run(int n = std::thread::hardware_concurrency()) {
        async_run(n-1);
        process();
        wait();
    }
private:
    std::vector<std::thread> m_threads;
    using ptask_type = std::shared_ptr<Task>;
    std::deque<ptask_type> m_tasks;
    std::mutex m_mutex;
    void process() {
        ptask_type ptask;
        bool isDone=false;
        std::unique_lock<std::mutex> lock(m_mutex);
        for(;;) {
            if(m_tasks.empty()) {
                lock.unlock();
                break;
            }
            ptask.swap(m_tasks.front());
            assert(m_tasks.front().get()==0);
            m_tasks.pop_front();
            lock.unlock();
            isDone = ptask.get()->task();
            lock.lock();
            if(!isDone) {
                m_tasks.push_back(std::move(ptask));
                assert(ptask.get()==0);
            }
            else {
                ptask = nullptr;
                assert(ptask.get()==0);
            }
        }
    }
};
}


#endif