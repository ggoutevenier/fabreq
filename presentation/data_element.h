#ifndef __data_element_h
#define __data_element_h
namespace presentation {
class Input {
    int v;
public:
    Input(int v):v(v) {};
    Input() = default;
    Input(const Input &i) = default;
    Input &operator=(const Input &a) = default;
    void setValue(int a){v=a;}
    int getValue() const {return v;}
};

class Output1{
    int v;
public:
    void setValue(int a){v=a;}
    int getValue() const {return v;}
    friend bool operator<(const Output1 &a,const Output1 &b) {return a.v<b.v;}
};

class Output2 {
    int v;
public:
    void setValue(int a){v=a;}
    int getValue() const {return v;}
    friend bool operator<(const Output2 &a,const Output2 &b) {return a.v<b.v;}
};

class RefData {
    int m_rate;
public:
    RefData(int r): m_rate(r){}
    int getRate() const {return m_rate;};
};

}
#endif