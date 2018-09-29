#ifndef MI_MESHLOADER_H
#define MI_MESHLOADER_H

#include "globals.h"

#include <string>
#include <vector>

#include "glm/glm.hpp"

namespace mesh {

void transform(const mymat4 &transformation, std::vector<triangle> &mesh);
void generate_normals(std::vector<ntriangle> &out, const std::vector<triangle> &in);
std::vector<ntriangle> generate_normals(const std::vector<triangle> &in);

myfloat classic_volume(const std::vector<triangle> &mesh);
myfloat volume(const std::vector<triangle> &mesh);
myfloat volume(const std::vector<ntriangle> &mesh);

std::vector<triangle> make_tetrahedron(const mymat4 &transformation = mymat4(1));
std::vector<triangle> make_axis_aligned_unit_cube(const mymat4 &transformation = mymat4(1));

bool load_mesh(const std::string &path, std::vector<triangle> &target);
std::vector<triangle> load_mesh(const std::string &path);


template <typename float_t> struct default_perturbation {};

template <> struct default_perturbation<float> {
    static constexpr float eps() { return 1e-5f; }
};
template <> struct default_perturbation<double> {
    static constexpr double eps() { return 1e-10; }
};

constexpr int sane_hash_cutoff = 5;

void unify_vertices(const std::vector<triangle> &input, std::vector<myvec> &vertices, std::vector<std::size_t> &indices, int hash_cutoff = sane_hash_cutoff);
void unify_vertices(const std::vector<ntriangle> &input, std::vector<myvec> &vertices, std::vector<std::size_t> &indices, int hash_cutoff = sane_hash_cutoff);
void perturb_vertices(std::vector<triangle> &mesh, myfloat eps = default_perturbation<myfloat>::eps());

void find_opposing_indices(std::vector<std::size_t> &opposing_index, const std::vector<std::size_t> &indices);


template <typename iterator>
void axis_aligned_bounding_box(iterator first, iterator last, myvec &min, myvec &max) {
    min = myvec(std::numeric_limits<myfloat>::infinity());
    max = myvec(-std::numeric_limits<myfloat>::infinity());

    for (; first != last; ++first) {
        min = glm::min(min, *first);
        max = glm::max(max, *first);
    }
}

}
#endif
