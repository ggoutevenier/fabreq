
#ifndef _buffer_proxy_h
#define _buffer_proxy_h
#include "buffer.h"
#include <unordered_map>
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
            /*! /brief get a proxy unique_ptr for the original unique_ptr
             *
             *  /param term a Buffer to place the proxy object in when 
             *     all references are remvoed
             *  /return proxied unique_ptr
             * 
             *  Returns unique_ptr that is a proxy for the original unique_ptr 
             *  when the proxy's dtor is called the deleter function decrements the
             *  use counters when both source and term use counters reach 0 the 
             *  proxied unique_ptr is put(swapped) into the term buffer 
             * */
            item_type get(_Buffer &term) {
                item_type item;
            
                item.setSource(
                    typename item_type::Source(
                        m_item.getSource().get(),
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
                        m_item.getTrans().get(),
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

        std::unordered_map<element_type *, ProxyElement> m_proxies;
        _Buffer &m_term;
        std::mutex m_mutex;
    public:
        BufferProxy(_Buffer &term):m_term(term) {}

        /** /brief add an item to the collection of proxies
         * /param item is the an item_type to proxy
         * /param use_count is the number of proxies to allot
         * /return the ProxyElement
        */
        ProxyElement &put(item_type &item, int use_count) {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto ptr = item.getTrans().get();
            auto it = m_proxies.find(ptr);
        
            if(it==m_proxies.end()) {
                m_proxies.emplace(ptr, ProxyElement(item, use_count));
                it = m_proxies.find(ptr);
            }
            else {
                it->second.m_item.swap(item);
                it->second.m_use_count_source = it->second.m_use_count_trans = use_count;
            }
        
            return it->second;
        }

        /** /brief get a proxy unique_ptr
        * /param proxyElement is the element that we need a uniqe_ptr from
        * /return item_type 
        */
        item_type get(ProxyElement &proxyElement) {
            return proxyElement.get(this->m_term);
        }

        /** /brief is the proxy class done being used
         * 
         * removes all proxied entries that are empty and
         * once there are no longer any proxied entries it 
         * will be safe to destroy the proxy
         * 
         * /return true if there are no objects proxied 
         */ 
        bool isDone() {
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