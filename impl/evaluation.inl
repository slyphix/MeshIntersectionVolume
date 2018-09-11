
#include "../glm/glm.hpp"

#include <iostream>
#include <vector>

#ifdef MI_VISUALIZE
extern std::vector<float> lines;
#endif


namespace eval {

constexpr myfloat det_epsilon = myfloat(1e-5);

// solve linear equation for line-triangle instersection
MI_SHARED
bool solve_intersection(const triangle &t, const line &l, myfloat &scalar) {
    // solve x * (b - a) + y * (c - a) + z * (start - end) = start - a
    mymat lineq;
    lineq[0] = t.b - t.a;
    lineq[1] = t.c - t.a;
    lineq[2] = l.start - l.end;

    // line is parallel to surface
    if (std::abs(glm::determinant(lineq)) < det_epsilon)
        return false;

    myvec target = l.start - t.a;
    myvec solution = glm::inverse(lineq) * target;

    // line does not intersect triangle
    if (solution[0] < 0 || solution[1] < 0 || solution[0] + solution[1] > 1)
        return false;

    scalar = solution[2];
    return true;
}

MI_SHARED
myvec face_same_direction(const myvec &reference, const myvec &target) {
    return glm::dot(reference, target) < 0 ? -target : target;
}

/*
MI_SHARED
myvec inside_direction(const myvec &start, const myvec &end, const myvec &third_vertex) {
    // find vector perpendicular to (end - start) vector facing third vertex (inside triangle)
    myvec line_direction = glm::normalize(end - start);
    myvec projection = glm::dot(line_direction, third_vertex - start) * line_direction + start;
    return third_vertex - projection;
}
*/

MI_SHARED
myfloat evaluate_term(const myvec &p, const myvec &t, const myvec &u, const myvec &n) {

#if defined(MI_DEBUG) && !defined(MI_CUDA_ENABLED)
    #pragma omp critical (IO)
    std::cout << "      generating term for position " << p << " with tangent " << t << " and binormal " << u
              << " and normal " << n << ": " << (glm::dot(p, t) * glm::dot(p, u) * glm::dot(p, n)) << std::endl;
#endif

#if defined(MI_VISUALIZE) && !defined(MI_CUDA_ENABLED)
    #pragma omp critical (TermBuffer)
    lines.insert(lines.end(), { (float) p.x, (float) p.y, (float) p.z, (float) t.x, (float) t.y, (float) t.z,
                                (float) u.x, (float) u.y, (float) u.z, (float) n.x, (float) n.y, (float) n.z });
#endif

    return glm::dot(p, t) * glm::dot(p, u) * glm::dot(p, n);
}

MI_SHARED
myfloat generate_intersection_terms(const myvec &intersection_point, const myvec &line_direction, const myvec &line_normal, const myvec &triangle_normal) {
    myfloat sum = 0;

    myvec inside_direction = glm::normalize(glm::cross(line_normal, line_direction));

    // generate term tangential to line
    {
        myvec tangent = face_same_direction(-triangle_normal, glm::normalize(line_direction));
        myvec binormal = inside_direction;

        sum += evaluate_term(intersection_point, tangent, binormal, line_normal);
    }

    // generate terms along face intersection
    {
        myvec tangent = face_same_direction(inside_direction, glm::normalize(glm::cross(line_normal, triangle_normal)));
        // coplanar with line
        {
            myvec binormal = face_same_direction(-triangle_normal, glm::normalize(glm::cross(line_normal, tangent)));
            sum += evaluate_term(intersection_point, tangent, binormal, line_normal);
        }
        // coplanar with triangle
        {
            myvec binormal = face_same_direction(-line_normal, glm::normalize(glm::cross(triangle_normal, tangent)));
            sum += evaluate_term(intersection_point, tangent, binormal, triangle_normal);
        }
    }

    return sum;
}


// generate terms for intersection points
MI_SHARED
myfloat intersect_line_triangle(const ntriangle &t, const triangle_side &ts, intersection_count &ic) {
    myfloat scalar;
    if (!solve_intersection(t, ts, scalar))
        return 0;

    if (scalar > 1)
        return 0;

    if (scalar < 0) {
        // std::cout << "found intersection at " << scalar << ":  " <<  (1 - scalar) * start + scalar * end << std::endl;
        // std::cout << "    triangle " << triangle[0] << " " << triangle[1] << " " << triangle[2] << std::endl;
        ic.before_segment += 1;
        return 0;
    }

    ic.on_segment += 1;

    // intersection point
    myvec isp = (1 - scalar) * ts.start + scalar * ts.end;

#if defined(MI_DEBUG) && !defined(MI_CUDA_ENABLED)
    #pragma omp critical (IO)
    std::cout << "found intersection point at " << isp << std::endl;
#endif

    return generate_intersection_terms(isp, ts.end - ts.start, ts.n, t.n);
}

// generate terms for points inside the other volume
MI_SHARED
myfloat evaluate_line_intersection(const triangle_side &ts, bool start_inside, bool end_inside) {
    myfloat accum = 0;

    // std::cout << "found " << ic.on_segment << " intersections on segment and " << ic.before_segment << " before." << std::endl;

    // generate term for start point
    if (start_inside) {
#if defined(MI_DEBUG) && !defined(MI_CUDA_ENABLED)
        #pragma omp critical (IO)
        std::cout << "start point " << start << " is inside the other polyhedron" << std::endl;
#endif
        myvec tangent = glm::normalize(ts.end - ts.start);
        myvec binormal = glm::cross(ts.n, tangent);

        accum += evaluate_term(ts.start, tangent, binormal, ts.n);
    }

    // generate term for end point
    if (end_inside) {
#if defined(MI_DEBUG) && !defined(MI_CUDA_ENABLED)
        #pragma omp critical (IO)
        std::cout << "end point " << end << " is inside the other polyhedron" << std::endl;
#endif
        myvec tangent = glm::normalize(ts.start - ts.end);
        myvec binormal = - glm::cross(ts.n, tangent);

        accum += evaluate_term(ts.end, tangent, binormal, ts.n);
    }

    return accum;
}

MI_SHARED
myfloat evaluate_line_intersection(const triangle_side &ts, const intersection_count &ic) {
    return evaluate_line_intersection(ts, ic.before_segment % 2 == 1, (ic.before_segment + ic.on_segment) % 2 == 1);
}

MI_SHARED
myfloat local_intersect_line_triangle(const ntriangle &t, const triangle_side &ts, localized_intersection_count &lic) {
    myfloat scalar;
    if (!solve_intersection(t, ts, scalar))
        return 0;

    if (scalar > 1 || scalar < 0)
        return 0;

    // intersection point
    myvec isp = (1 - scalar) * ts.start + scalar * ts.end;

    lic.on_segment += 1;
    if (scalar < lic.closest_point) {
        lic.closest_point = scalar;
        lic.is_start_inside = glm::dot(ts.start - isp, t.n) > 0;
    }

    return generate_intersection_terms(isp, ts.end - ts.start, ts.n, t.n);
}

}
