#ifndef __SILENT_STREAM__
#include <hls_stream.h>
template<typename T>
class silent_stream{
private:
    hls::stream<T> internal_stream;

public:
    void write(const T& val) { 
        internal_stream.write(val); 
    }
    T read() { 
        return internal_stream.read(); 
    }
    bool empty() const { 
        return internal_stream.empty(); 
    }
    ~silent_stream() = default;
};

#endif