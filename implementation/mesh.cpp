
#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <limits>
#include <random>
#include <unordered_map>

#include "evaluation.h"
#include "mesh.h"

#include "glm/glm.hpp"

namespace mesh {

bool load_binary_stl_file(std::ifstream &instream, std::vector<triangle> &t) {
    instream.seekg(80, std::ios_base::beg); // skip ascii header
    int trinum = 0;
    instream.read((char *) &trinum, 4); // number of vertices
    for (int k = 0; k < trinum; ++k) {
        float tmp;
        for (int i = 0; i < 3; ++i) // surface normal
            instream.read((char *) &tmp, 4);

        std::array<myvec, 3> buf {};
        for (int i = 0; i < 3; ++i) {
            float_t v[3];
            instream.read((char *) v, 4 * 3);
            buf[i] = {(myfloat) v[0], (myfloat) v[1], (myfloat) v[2]};
        }
        t.emplace_back(buf[0], buf[1], buf[2]);
        instream.read((char *) &tmp, 2);
    }
    return true;
}

bool load_ascii_stl_file(std::ifstream &instream, std::vector<triangle> &t) {
    // ignore first line "solid xxx"
    instream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string sentinel, ignore;
    double x, y, z;
    while (instream >> sentinel && sentinel == "facet") {
        instream >> ignore >> x >> y >> z; // normal
        instream >> ignore >> ignore; // "outer loop"

        std::array<myvec, 3> buf {};
        for (int i = 0; i < 3; ++i) {
            instream >> ignore >> x >> y >> z;
            buf[i] = {(myfloat) x, (myfloat) y, (myfloat) z};
        }
        t.emplace_back(buf[0], buf[1], buf[2]);
        instream >> ignore >> ignore; // "endloop", "endfacet"
    }
    return true;
}


bool load_stl_file(const std::string &filename, std::vector<triangle> &target) {
    std::ifstream instream(filename, std::ios::binary);
    if (!instream) return false;

    char first[5 + 1] = "";
    instream.read(first, 5);
    instream.seekg(0);
    if (std::string(first) != "solid") {
        return load_binary_stl_file(instream, target);
    } else {
        return load_ascii_stl_file(instream, target);
    }
}


bool load_mesh(const std::string &path, std::vector<triangle> &target) {
    std::string::size_type index = path.find_last_of('.');
    if (index == std::string::npos)
        return false;
    std::string suffix = path.substr(index + 1);
    if (suffix == "stl") {
        return load_stl_file(path, target);
    }
    return false;
}

std::vector<triangle> load_mesh(const std::string &path) {
    std::vector<triangle> buffer;
    if (!load_mesh(path, buffer))
        throw std::runtime_error("Invalid file.");
    return buffer;
}

void generate_normals(std::vector<ntriangle> &out, const std::vector<triangle> &in) {
    for (const auto &t : in) {
        myvec a = t.a, b = t.b, c = t.c;
        myvec n = glm::normalize(glm::cross(b - a, c - a));

        out.emplace_back(a, b, c, n);
    }
}

std::vector<ntriangle> generate_normals(const std::vector<triangle> &in) {
    std::vector<ntriangle> out;
    generate_normals(out, in);
    return out;
}


inline std::size_t combined_hash(std::size_t seed) { return seed; }

template <typename T, typename ...Rest>
inline std::size_t combined_hash(std::size_t seed, const T& e, Rest ...rest) {
    std::hash<T> hasher;
    seed ^= hasher(e) + 0x9e3779b9 + (seed<<6u) + (seed>>2u);
    return combined_hash(seed, rest...);
}

template <typename V>
struct vector_hash {
    std::size_t operator()(const V& v) const noexcept {
        return combined_hash(0, v.x, v.y, v.z);
    }
};

template <typename P>
struct pair_hash {
    std::size_t operator()(const P& p) const noexcept {
        return combined_hash(0, p.first, p.second);
    }
};


template <typename triangle_t>
void unify_impl(const std::vector<triangle_t> &input, std::vector<myvec> &vertices, std::vector<std::size_t> &indices, int hash_cutoff) {

    myfloat pre_cutoff_factor = std::pow(myfloat(10), hash_cutoff);
    myfloat post_cutoff_factor = 1 / pre_cutoff_factor;
    // assume every vertex will be shared by at least 3 triangles
    auto bucket_count = input.size() / 3;

    std::unordered_map<const myvec, std::size_t, vector_hash<const myvec>> vertices_to_index(bucket_count);

    for (const triangle &triangle : input) {
        for (const auto &vertex : triangle) {
            myvec cutoff = post_cutoff_factor * glm::round(vertex * pre_cutoff_factor);
            auto lookup = vertices_to_index.find(cutoff);

            if (lookup == vertices_to_index.end()) {
                std::size_t next_index = vertices.size();

                // insert original vertex
                vertices.push_back(vertex);
                indices.push_back(next_index);
                vertices_to_index[cutoff] = next_index;
            } else {
                indices.push_back(lookup->second);
            }
        }
    }
}
void unify_vertices(const std::vector<triangle> &input, std::vector<myvec> &vertices, std::vector<std::size_t> &indices, int hash_cutoff) {
    unify_impl(input, vertices, indices, hash_cutoff);
}
void unify_vertices(const std::vector<ntriangle> &input, std::vector<myvec> &vertices, std::vector<std::size_t> &indices, int hash_cutoff) {
    unify_impl(input, vertices, indices, hash_cutoff);
}


// see https://en.wikipedia.org/wiki/Single-precision_floating-point_format
float oom(float in) {
    if (in == 0) return 1;
    std::uint32_t binary = *reinterpret_cast<std::uint32_t *>(&in);
    std::uint32_t exponent = binary & 0x7F800000u;
    return *reinterpret_cast<float *>(&exponent);
}
// see https://en.wikipedia.org/wiki/Double-precision_floating-point_format
double oom(double in) {
    if (in == 0) return 1;
    std::uint64_t binary = *reinterpret_cast<std::uint64_t *>(&in);
    std::uint64_t exponent = binary & 0x7FF0000000000000ull;
    return *reinterpret_cast<double *>(&exponent);
}

void perturb_vertices(std::vector<triangle> &mesh, myfloat eps) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<myfloat> dis(-eps, eps);

    std::vector<myvec> unified_vertices;
    std::vector<std::size_t> unified_indices;
    unify_vertices(mesh, unified_vertices, unified_indices);

    for (auto &v : unified_vertices)
        v += myvec(dis(gen) * oom(v.x), dis(gen) * oom(v.y), dis(gen) * oom(v.z));

    for (std::size_t i = 0; i < mesh.size(); ++i) {
        mesh[i].a = unified_vertices[unified_indices[3 * i]];
        mesh[i].b = unified_vertices[unified_indices[3 * i + 1]];
        mesh[i].c = unified_vertices[unified_indices[3 * i + 2]];
    }
}


void find_opposing_indices(std::vector<std::size_t> &opposing_index, const std::vector<std::size_t> &indices) {
    using edge = const std::pair<std::size_t, std::size_t>;
    using triangle_index = std::size_t;
    using index_map = std::unordered_multimap<edge, triangle_index, pair_hash<edge>>;
    index_map third_vertex;

    opposing_index.clear();
    opposing_index.resize(indices.size());

    for (std::size_t i = 0; i < indices.size(); i += 3) {
        std::size_t a = indices[i], b = indices[i + 1], c = indices[i + 2];

        third_vertex.emplace(std::minmax(a, b), i + 2);
        third_vertex.emplace(std::minmax(b, c), i + 0);
        third_vertex.emplace(std::minmax(a, c), i + 1);
    }

    auto opposing_vertex = [&third_vertex](std::size_t start, std::size_t end, std::size_t this_triangle_vertex) {
        index_map::iterator it_start, it_end;
        // find vertices adjacent to edge
        std::tie(it_start, it_end) = third_vertex.equal_range(std::minmax(start, end));
        // range should contain exactly 2 elements
        //if (third_vertex.count(std::minmax(start, end)) != 2)
        //    std::exit(1);
        triangle_index first = it_start->second;
        triangle_index second = (++it_start)->second;
        // one of those is the abc triangle, but we need the other one
        return first == this_triangle_vertex ? second : first;
    };

    for (std::size_t i = 0; i < indices.size(); i += 3) {
        opposing_index[i]     = opposing_vertex(indices[i + 1], indices[i + 2], i);
        opposing_index[i + 1] = opposing_vertex(indices[i], indices[i + 2], i + 1);
        opposing_index[i + 2] = opposing_vertex(indices[i], indices[i + 1], i + 2);
    }
}

myfloat evaluate_edge(const myvec &start, const myvec &end, const myvec &normal) {
    myvec tangent = glm::normalize(end - start);
    myvec surface = glm::cross(normal, tangent);
    return eval::evaluate_term(start, tangent, surface, normal)
            + eval::evaluate_term(end, -tangent, surface, normal);
}

myfloat classic_volume(const std::vector<triangle> &mesh) {
    myfloat sum = 0;
    for (const auto &t : mesh) {
        sum += glm::dot(t.a, glm::cross(t.b, t.c));
    }
    return sum / 6;
}

myfloat volume(const std::vector<triangle> &mesh) {
    myfloat sum = 0;
    for (const auto &t : mesh) {
        myvec a = t.a, b = t.b, c = t.c;
        myvec n = glm::normalize(glm::cross(b - a, c - a));

        sum += evaluate_edge(a, b, n) + evaluate_edge(b, c, n) + evaluate_edge(c, a, n);
    }
    return sum / 6;
}

myfloat volume(const std::vector<ntriangle> &mesh) {
    myfloat sum = 0;
    for (const auto &t : mesh) {
        myvec a = t.a, b = t.b, c = t.c, n = t.n;

        sum += evaluate_edge(a, b, n) + evaluate_edge(b, c, n) + evaluate_edge(c, a, n);
    }
    return sum / 6;
}

void transform(const mymat4 &transformation, std::vector<triangle> &mesh) {
    for (triangle &triangle : mesh) {
        for (auto &vertex : triangle) {
            vertex = myvec(transformation * myvec4(vertex, 1));
        }
    }
}

std::vector<triangle> make_tetrahedron(const mymat4 &transformation) {
    std::vector<triangle> tetrahedron = {
            { {0, 0, 0}, {0, 0, 1}, {0, 1, 0} },
            { {0, 0, 0}, {0, 1, 0}, {1, 0, 0} },
            { {0, 0, 0}, {1, 0, 0}, {0, 0, 1} },
            { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} }
    };
    transform(transformation, tetrahedron);
    return tetrahedron;
}

std::vector<triangle> make_axis_aligned_unit_cube(const mymat4 &transformation) {
    std::vector<triangle> vertices = {
            { {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 1.0, 1.0} },
            { {1.0, 1.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0} },
            { {1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0} },
            { {1.0, 1.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0} },
            { {0.0, 0.0, 0.0}, {0.0, 1.0, 1.0}, {0.0, 1.0, 0.0} },
            { {1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, 0.0} },
            { {0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}, {1.0, 0.0, 1.0} },
            { {1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0} },
            { {1.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {1.0, 0.0, 1.0} },
            { {1.0, 1.0, 1.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0} },
            { {1.0, 1.0, 1.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 1.0} },
            { {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}, {1.0, 0.0, 1.0} }
    };
    transform(transformation, vertices);
    return vertices;
}

}
