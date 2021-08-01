
#ifndef _buffer_proxy_h
#define _buffer_proxy_h
#include <list>
#include <mutex>
#include <algorithm>

namespace fabreq {
template<class _Buffer>
class BufferProxy {
    using item_type = typename _Buffer::item_type; 
    using element_type = typename item_type::Trans::element_type; 

    struct ProxyElement {
        item_type m_item;
        std::mutex m_mutex;
        int m_use_count_source, m_use_count_trans;
    
        ProxyElement(item_type &item,int use_count) {
            m_item.swap(item);
            m_use_count_source = m_use_count_trans = use_count;
        }

        ProxyElement(ProxyElement &&p) {
            m_item.swap(p.m_item);
            m_use_count_source = p.m_use_count_source;
            m_use_count_trans = p.m_use_count_trans;
        }
        ~ProxyElement() {}

        item_type get(_Buffer &term) {
            item_type item;
            
            item.setSource(
                typename item_type::Source(
                        this->m_item.getSource().get(),
                        [&term, this](auto a) {
                            std::lock_guard<std::mutex> lock(this->m_mutex);
                            this->m_use_count_source--;
                            if(this->m_use_count_source==0 && this->m_use_count_trans==0) {
                                term.put(this->m_item);
                            }
                        })
            );

            item.setTrans(
                typename item_type::Trans(
                        this->m_item.getTrans().get(),
                        [&term, this](auto a) {
                            std::lock_guard<std::mutex> lock(this->m_mutex);
                            this->m_use_count_trans--;
                            if(this->m_use_count_source==0 && this->m_use_count_trans==0) {
                                term.put(this->m_item);
                            }
                        })
            );

            return item;
        }        
    };

    std::list<std::pair<element_type *, ProxyElement>> m_proxies;
    _Buffer &m_term;
    std::mutex m_mutex;
public:
    BufferProxy(_Buffer &term):m_term(term) {}

    ProxyElement &put(item_type &t, int use_count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto ptr = t.getTrans().get();
        auto it = std::find_if(m_proxies.begin(), m_proxies.end(), [&ptr](auto &a){return a.first==ptr;});
        
        if(it==m_proxies.end()) {
            m_proxies.emplace_back(ptr, ProxyElement(t,use_count));
            it = std::find_if(m_proxies.begin(), m_proxies.end(), [&ptr](auto &a){return a.first==ptr;});
        }
        else {
            it->second.m_item.swap(t);
            it->second.m_use_count_source = it->second.m_use_count_trans = use_count;
        }
        
        return it->second;
    }

    item_type get(ProxyElement &proxyElement) {
        return proxyElement.get(this->m_term);
    }

    auto isDone() {
        if(m_proxies.empty()) 
            return true;
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto it = m_proxies.begin();it!=m_proxies.end();) {
            auto it2=it;
            it++;
            if(it2->second.m_use_count_source==0 && it2->second.m_use_count_trans==0) 
            m_proxies.erase(it2);
        }
        if (m_proxies.empty()) {
            m_term.done();
            return true;
        }
        return false;
    }
};
}
#endif