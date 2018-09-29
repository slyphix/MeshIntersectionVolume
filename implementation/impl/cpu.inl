
#include "../evaluation.h"
#include "../globals.h"
#include "../intersect.h"

#include <algorithm>
#include <numeric>

#include "../glm/glm.hpp"

namespace mesh {
namespace impl {

    myfloat intersect_line_all_triangles(const std::vector<ntriangle> &triangles, const triangle_side &line) {
        myfloat accum = 0;
        eval::intersection_count ic = eval::intersection_count::zero();

        for (const auto &triangle : triangles) {
            accum += eval::intersect_line_triangle(triangle, line, ic);
        }

        accum += eval::evaluate_line_intersection(line, ic);
        return accum;
    }

    myfloat asymetric_intersect(const std::vector<ntriangle> &triangles, const std::vector<ntriangle> &lines) {
        myfloat accum = 0;

        // proof-of-concept openmp support (requires signed variables / msvc does not support collapse)
        #pragma omp parallel for reduction(+:accum)
        for (std::int64_t i = 0; std::size_t(i) < lines.size() * 3; ++i) {
            const std::int64_t tri = i / 3;
            const std::int64_t line = i % 3;
            accum += intersect_line_all_triangles(triangles, extract_side(lines[tri], (std::size_t) line));
        }
        return accum;
    }

    myfloat intersection_volume(const std::vector<ntriangle> &first_mesh, const std::vector<ntriangle> &second_mesh) {
        return (asymetric_intersect(first_mesh, second_mesh) + asymetric_intersect(second_mesh, first_mesh)) / 6;
    }
}
}
