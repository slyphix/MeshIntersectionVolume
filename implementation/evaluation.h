#ifndef MI_EVALUATION_H
#define MI_EVALUATION_H

#include "globals.h"

namespace eval {

struct intersection_count {
    int before_segment = 0;
    int on_segment = 0;
    MI_SHARED static intersection_count zero() { return {}; }
};

struct localized_intersection_count {
    int on_segment = 0;
    bool is_start_inside = false;
    myfloat closest_point = std::numeric_limits<myfloat>::infinity();
    MI_SHARED static localized_intersection_count zero() { return {}; }
};

MI_SHARED bool solve_intersection(const triangle &t, const line &l, myfloat &scalar);
MI_SHARED myvec face_same_direction(const myvec &reference, const myvec &target);
MI_SHARED myfloat evaluate_term(const myvec &p, const myvec &t, const myvec &u, const myvec &n);
MI_SHARED myfloat intersect_line_triangle(const ntriangle &t, const triangle_side &ts, intersection_count &ic);
MI_SHARED myfloat local_intersect_line_triangle(const ntriangle &t, const triangle_side &ts, localized_intersection_count &ic);
MI_SHARED myfloat evaluate_line_intersection(const triangle_side &ts, bool start_inside, bool end_inside);
MI_SHARED myfloat evaluate_line_intersection(const triangle_side &ts, const intersection_count &ic);
}

#include "impl/evaluation.inl"

#endif
