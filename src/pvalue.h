#ifndef _pvalue_h
#define _pvalue_h
#include <functional>
#include <memory>
namespace fabreq {

    /** 
     * Source & Transation type container
     * Container has two seperate unque_ptrs
     * Source is the orinial sourced data (TODO make immutable)
     * Trans is the transactional unique_ptr that can be modified
     * The container is considered empty if there is no transactional pointer
     * Swaping swaps both source and trans
     */
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

        ~pvalue_type() {
            delete source.release();
            delete trans.release();
        }
        pvalue_type<S,T> &operator=(pvalue_type<S,T> &&a) {
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
            source.swap(a.getSource());
            if(!trans.get())
                trans.swap(a.getTrans());
        }

        void reset() {
            if(source.get())
                source.reset();

            if(trans.get())
                trans.reset();
        }

        bool empty() {
            return trans.get()==0;
        }
    private:
        Source source;
        Trans trans;      
    };

    /**
     * Source only type container
     * 
     * Holds unique_ptr to Source Type
     * All operations only apply to the source
     * setTrans is a noop function
     * getTrans returns the source unique_ptr
     */
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

        ~pvalue_type() {
            delete source.release();        
        }
        pvalue_type<S> &operator=(pvalue_type<S> &&a) {
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
            if(source.get())
                source.reset();
        }
        
        bool empty() {
            return source.get()==0;
        }

    private:
        Source source;
    };
}
#endif
    