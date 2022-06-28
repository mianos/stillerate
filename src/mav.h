#pragma once

template <class T>
class CMAverage {
    size_t ma_size;
    T *buffer;
    size_t ma_used = 0;
public:
    CMAverage(size_t p_ma_size) : ma_size(p_ma_size) {
        buffer = new T[ma_size];
        for (int ii = 0; ii < ma_size; ii++) {
            buffer[ii] = T{0};
        }
    }

    ~CMAverage() {
        delete[] buffer;
    }
    T avg(T in) {
        for (int ii = ma_size - 1; ii > 0; ii--) {
            buffer[ii] = buffer[ii - 1];
        }
        buffer[0] = in;
        ma_used = ma_used < ma_size ? ma_used + 1 : ma_size;
        T sum = 0;
        for (int ii = 0; ii < ma_used; ii++) {
            sum += buffer[ii];
        }
        return sum / ma_used;
    }
};
