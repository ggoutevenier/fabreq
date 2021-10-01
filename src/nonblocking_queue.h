#ifndef _nonblocking_queue_h
#define _nonblocking_queue_h
#include <deque>
#include <mutex>

namespace fabreq {
template<class U>
class NonBlockingQueue {
public:
    NonBlockingQueue() = default;
    NonBlockingQueue(size_t size):m_container(size) {}
    void push_back(U &&item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.emplace_back(std::move(item));
    }
    void push_front(U &&item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_container.emplace_front(std::move(item));
    }

    void pop(U &item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_container.empty()) {
            item.swap(m_container.front());
            m_container.pop_front();
        }
        else
            item.reset();
    }
    auto begin() {return m_container.begin();}
    auto end() {return m_container.end();}
private:
    std::deque<U> m_container;
    std::mutex m_mutex;
};
}
#endif