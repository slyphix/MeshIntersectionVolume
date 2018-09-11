#ifndef MI_GLOBALS_H
#define MI_GLOBALS_H

#include "glm/fwd.hpp"
#include "glm/vec3.hpp"

#ifdef __CUDACC__
#   define MI_CUDA_ENABLED
#endif

#ifdef MI_CUDA_ENABLED
#   define MI_DEVICE __device__
#   define MI_HOST __host__
#else
#   define MI_DEVICE
#   define MI_HOST
#endif

#define MI_SHARED MI_DEVICE MI_HOST inline

#ifdef MI_SINGLE_PRECISION
using myfloat = float;
#else
using myfloat = double;
#endif

using myvec = glm::vec<3, myfloat, glm::highp>;
using myvec4 = glm::vec<4, myfloat, glm::highp>;
using mymat = glm::mat<3, 3, myfloat, glm::highp>;
using mymat4 = glm::mat<4, 4, myfloat, glm::highp>;

struct triangle {
    myvec a;
    myvec b;
    myvec c;
    MI_SHARED triangle(const myvec &a, const myvec &b, const myvec &c) : a(a), b(b), c(c) {}
};

inline myvec *begin(triangle &t) {
    return &t.a;
}
inline const myvec *begin(const triangle &t) {
    return &t.a;
}

inline myvec *end(triangle &t) {
    return begin(t) + 3;
}
inline const myvec *end(const triangle &t) {
    return begin(t) + 3;
}


struct ntriangle : public triangle {
    myvec n;
    MI_SHARED ntriangle(const myvec &a, const myvec &b, const myvec &c, const myvec &n) : triangle(a, b, c), n(n) {}
};

struct line {
    myvec start;
    myvec end;
    MI_SHARED line(const myvec &start, const myvec &end) : start(start), end(end) {}
};

struct triangle_side : public line {
    myvec third;
    myvec n;
    MI_SHARED triangle_side(const myvec &start, const myvec &end, const myvec &third, const myvec &n)
            : line(start, end), third(third), n(n) {}
};

MI_SHARED
triangle_side extract_side(const ntriangle &t, std::size_t index) {
    switch (index % 3) {
        case 0:
            return {t.a, t.b, t.c, t.n};
        case 1:
            return {t.b, t.c, t.a, t.n};
        default:
            return {t.c, t.a, t.b, t.n};
    }
}

constexpr double pi = 3.1415926535897932384626433;

#endif
