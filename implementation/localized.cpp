
#include "evaluation.h"
#include "mesh.h"

#include <algorithm>
#include <deque>
#include <functional>
#include <numeric>

#ifdef MI_LOCALIZED_CONSISTENCY_CHECKS
#include <utility>
#endif

namespace mesh {

    enum class vertex_location { unknown, inside, outside };

    myfloat localized_intersect_line_all_triangles(const std::vector<ntriangle> &triangles, const triangle_side &line,
                                                   vertex_location &start_location, vertex_location &end_location) {

        myfloat accum = 0;
        eval::localized_intersection_count ic = eval::localized_intersection_count::zero();

        for (const auto &triangle : triangles) {
            accum += eval::local_intersect_line_triangle(triangle, line, ic);
        }

        // no intersections on segment
        if (ic.on_segment == 0)
            return accum;

        // classify vertices
        bool is_end_inside = ic.is_start_inside ^ (ic.on_segment % 2 == 1);
        vertex_location local_start_location = ic.is_start_inside ? vertex_location::inside : vertex_location::outside;
        vertex_location local_end_location = is_end_inside ? vertex_location::inside : vertex_location::outside;

#ifdef MI_LOCALIZED_CONSISTENCY_CHECKS
        // check consistency
        if (start_location != vertex_location::unknown && start_location != local_start_location) {
            std::cerr << "Inconsistent classification for start vertex." << std::endl;
            std::exit(2);
        }
        if (end_location != vertex_location::unknown && end_location != local_end_location) {
            std::cerr << "Inconsistent classification for end vertex." << std::endl;
            std::exit(2);
        }
#endif
        start_location = local_start_location;
        end_location = local_end_location;

        return accum;
    }

    // this is quite messy and needs a rewrite (including sensible types)
    using adjacency_list = std::vector<std::vector<std::size_t>>;
    adjacency_list adjacency(std::size_t vertex_count, const std::vector<std::size_t> &unified_indices) {
        adjacency_list al(vertex_count, std::vector<std::size_t>{});

        for (std::size_t tri = 0; tri < unified_indices.size(); tri += 3) {
            for (std::size_t edge = 0; edge < 3; ++edge) {
                std::size_t from = unified_indices[tri + edge];
                std::size_t to = unified_indices[tri + (edge + 1) % 3];

                al[from].push_back(to);
            }
        }

        return al;
    }

    bool localized_asymetric_intersect(const std::vector<ntriangle> &triangles, const std::vector<ntriangle> &lines, myfloat &volume) {
        myfloat accum = 0;

        // unify vertices
        std::vector<myvec> unified_vertices;
        std::vector<std::size_t> unified_indices;
        mesh::unify_vertices(lines, unified_vertices, unified_indices);

        std::vector<vertex_location> locations(unified_vertices.size(), vertex_location::unknown);

        for (std::size_t i = 0; i < lines.size(); ++i) {
            // write to unified vertex representation
            accum += localized_intersect_line_all_triangles(triangles, extract_side(lines[i], 0),
                    locations[unified_indices[3 * i + 0]], locations[unified_indices[3 * i + 1]]);
            accum += localized_intersect_line_all_triangles(triangles, extract_side(lines[i], 1),
                    locations[unified_indices[3 * i + 1]], locations[unified_indices[3 * i + 2]]);
            accum += localized_intersect_line_all_triangles(triangles, extract_side(lines[i], 2),
                    locations[unified_indices[3 * i + 2]], locations[unified_indices[3 * i + 0]]);
        }

        std::size_t intersections = std::count_if(locations.begin(), locations.end(), [](const vertex_location &vl){
            return vl != vertex_location::unknown;
        });

        if (!intersections) {
            return false;
        }

#ifdef MI_LOCALIZED_DEBUG
        std::cout << "local classification:";

        for (std::size_t i = 0; i < locations.size(); ++i) {
            std::string l = locations[i] == vertex_location::inside ? "inside" :
                            locations[i] == vertex_location::outside ? "outside" : "unclassified";
            std::cout << " [" << i << " " << l << "]";
        }
        std::cout << std::endl;
#endif

        // complete vertex classification by traversing adjacency graph
        adjacency_list al = adjacency(unified_vertices.size(), unified_indices);

        std::vector<bool> visited(unified_vertices.size(), false);

        std::deque<std::size_t> deque;
        for (std::size_t i = 0; i < locations.size(); ++i) {
            deque.clear();

            deque.push_back(i);
            while (!deque.empty()) {
                std::size_t vertex_index = deque.front();
                deque.pop_front();

                if (visited[vertex_index])
                    continue;

                vertex_location location = locations[vertex_index];
                if (location != vertex_location::inside)
                    continue;

                visited[vertex_index] = true;

                for (std::size_t neighbor : al[vertex_index]) {
                    if (locations[neighbor] != vertex_location::unknown)
                        continue;

                    locations[neighbor] = vertex_location::inside;
                    deque.push_back(neighbor);
                }
            }
        }

#ifdef MI_LOCALIZED_DEBUG
        std::cout << "global classification:";

        for (std::size_t i = 0; i < locations.size(); ++i) {
            std::string l = locations[i] == vertex_location::inside ? "inside" :
                            locations[i] == vertex_location::outside ? "outside" : "unclassified";
            std::cout << " [" << i << " " << l << "]";
        }
        std::cout << std::endl;
#endif

        // evaluate vertex classification
        for (std::size_t i = 0; i < lines.size(); ++i) {
            accum += eval::evaluate_line_intersection(extract_side(lines[i], 0),
                    locations[unified_indices[3 * i + 0]] == vertex_location::inside,
                    locations[unified_indices[3 * i + 1]] == vertex_location::inside);
            accum += eval::evaluate_line_intersection(extract_side(lines[i], 1),
                    locations[unified_indices[3 * i + 1]] == vertex_location::inside,
                    locations[unified_indices[3 * i + 2]] == vertex_location::inside);
            accum += eval::evaluate_line_intersection(extract_side(lines[i], 2),
                    locations[unified_indices[3 * i + 2]] == vertex_location::inside,
                    locations[unified_indices[3 * i + 0]] == vertex_location::inside);
        }
        volume += accum;
        return true;
    }

    // this could be moved to mesh.cpp
    bool is_inside(const std::vector<ntriangle> &inner, const std::vector<ntriangle> &outer) {

        triangle_side inner_side = extract_side(inner[0], 0);
        auto count_before = [&](std::size_t count, const ntriangle &tri) -> std::size_t {
            myfloat scalar;
            if (!eval::solve_intersection(tri, inner_side, scalar))
                return count;
            return scalar < 0 ? count + 1 : count;
        };

        return std::accumulate(outer.begin(), outer.end(), std::size_t(), count_before) % 2 == 1;
    }

    myfloat localized_intersection_volume(const std::vector<ntriangle> &first_mesh, const std::vector<ntriangle> &second_mesh) {
        if (first_mesh.empty() || second_mesh.empty())
            return 0;

        // check if meshes intersect
        myfloat accum = 0;
        bool does_intersect = localized_asymetric_intersect(first_mesh, second_mesh, accum)
                            | localized_asymetric_intersect(second_mesh, first_mesh, accum);

        if (does_intersect)
            return accum / 6;

        if (is_inside(first_mesh, second_mesh))
            return mesh::volume(first_mesh);

        if (is_inside(second_mesh, first_mesh))
            return mesh::volume(second_mesh);

        return 0;
    }
}
