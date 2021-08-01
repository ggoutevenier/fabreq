#ifndef _pvalue_h
#define _pvalue_h
#include <functional>
#include <memory>
namespace fabreq {

    template<class S, class T=void>
    class pvalue_type {
    public:
        using Deleter = std::function<void(T *)>;
        using Source = std::unique_ptr<S, std::function<void(S *)>>;
        using Trans = std::unique_ptr<T, Deleter>;
         
        pvalue_type():  source(nullptr,[](S*){}),
                        trans(nullptr,[](T*){}){}

        pvalue_type(pvalue_type<S,T> &&pv) {
            swap(pv);
        }

        pvalue_type(T *t, Deleter f) {
            Trans(t, f).swap(trans);
        }

        pvalue_type(Deleter f) {
            Trans(new T(), f).swap(trans);
        }

        pvalue_type<S,T> &operator=(pvalue_type<S,T> &&a) {
            swap(a);
            return *this;
        }

        pvalue_type<S,T> &operator=(pvalue_type<S,T> &a) {
            swap(a);
            return *this;
        }

        Source &getSource() {
            return source;
        }
        Trans &getTrans() {
            return trans;
        }  
        void setSource(Source &&s) {
            source.swap(s);
        }
        
        void setTrans(Trans &&t) {
            trans.swap(t);
        }

        void swap(pvalue_type<S,T> &a) {
            this->source.swap(a.getSource());
            this->trans.swap(a.getTrans());
        }

        void reset() {
            auto &s = getSource();
            if(s.get())
                s.reset();

            auto &t = getTrans();
            if(t.get())
                t.reset();
        }

        bool empty() {
            return trans.get()==0;
        }
    private:
        Source source;
        Trans trans;      
    };

    template<class S>
    class pvalue_type<S,void> {
    public:
        using Deleter = std::function<void(S *)>;

        using Source = std::unique_ptr<S, Deleter>;
        using Trans = Source;

        pvalue_type(pvalue_type<S> &&pv) {
            swap(pv);
        }

        pvalue_type(pvalue_type<S> &pv) {
            swap(pv);
        }

        pvalue_type(): source(nullptr,[](S*){}){}
        pvalue_type(S *s, Deleter f) {
            Source(s, f).swap(source);
        }

        pvalue_type(Deleter f) {
            Source(new S(), f).swap(source);
        }

        pvalue_type<S> &operator=(pvalue_type<S> &&a) {
            swap(a);
            return *this;
        }

        pvalue_type<S> &operator=(pvalue_type<S> &a) {
            swap(a);
            return *this;
        }

        Source &getSource() {
            return source;
        }

        Trans &getTrans() {
            return source;
        }  

        void setSource(Source &&s) {
            source.swap(s);
        }
        
        void setTrans(Trans &&t) {
        }

        void swap(pvalue_type<S> &a) {
            source.swap(a.getSource());
        }

        void reset() {
            auto &s = getSource();
            if(s.get())
                s.reset();
        }
        
        bool empty() {
            return source.get()==0;
        }

    private:
        Source source;
    };
}
#endif
