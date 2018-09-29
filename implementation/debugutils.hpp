#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

#include <iostream>
#include <vector>
#include <array>

#include "glm/vec3.hpp"

inline std::ostream& operator <<(std::ostream &stream, const myvec &v) {
    stream << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
    return stream;
};

inline std::ostream& operator <<(std::ostream &stream, const triangle &v) {
    stream << "triangle( " << v.a << " , " << v.b << " , " << v.c << " )";
    return stream;
};

inline std::ostream& operator <<(std::ostream &stream, const ntriangle &v) {
    stream << "triangle( " << v.a << " , " << v.b << " , " << v.c << " , normal = " << v.n << " )";
    return stream;
};

template <typename T>
inline std::ostream& operator <<(std::ostream &stream, const std::vector<T> &v) {
    stream << "[\n";
    for (std::size_t i = 0; i < v.size(); ++i) {
        stream << "  " << v[i] << "\n";
    }
    stream << "]\n";
    return stream;
};


#endif
