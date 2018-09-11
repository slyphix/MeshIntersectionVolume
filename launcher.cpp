
#include "globals.h"
#include "intersect.h"
#include "mesh.h"

#include "debugutils.hpp"

#ifdef MI_VISUALIZE
#include "visualize.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>


void center_pair_around_origin(std::vector<triangle> &fst, std::vector<triangle> &snd) {

    auto reduce = [](const myvec &acc, const triangle &t){
        return acc + t.a + t.b + t.c;
    };

    myvec sum = std::accumulate(fst.begin(), fst.end(), myvec(), reduce)
            + std::accumulate(snd.begin(), snd.end(), myvec(), reduce);

    std::size_t count = fst.size() + snd.size();
    myvec avg = sum / static_cast<myfloat>(count);

    auto shift = [&](triangle &t){
        t.a -= avg;
        t.b -= avg;
        t.c -= avg;
    };

    std::for_each(fst.begin(), fst.end(), shift);
    std::for_each(snd.begin(), snd.end(), shift);
}

// test data

//mymat4 flip = glm::rotate(pi / 2, myvec(0, 1, 0)) * glm::rotate(pi, myvec(1, 0, 0));
//mymat4 scale = glm::scale(myvec(10));
//mymat4 translate = glm::translate(myvec(-5));
//std::vector<triangle> first_mesh = mesh::make_tetrahedron(translate * scale);
//std::vector<triangle> second_mesh = mesh::make_tetrahedron(scale * flip);

//std::vector<triangle> first_mesh {{ {-3, 0, -5}, {-3, 0, 2}, {5, 0, 0} }};
//std::vector<triangle> second_mesh {{ {-4, -3, 0}, {4, -3, 0}, {0, 3, 0} }};

//std::vector<triangle> first_mesh { { {-0.5, 0, 0}, {0.5, 0, 0}, {0, 0.2, 1} } };
//std::vector<triangle> second_mesh { { {0, -0.5, 0}, {0, 0.5, 0}, {0, 0, 1} },
//                                    { {0, 0.5, 0}, {0, -0.5, 0}, {0.3, 0, -0.5} } };

#ifdef MI_VISUALIZE
std::vector<float> lines;
#endif

int main(int argc, char **argv) {

    std::cout << std::setprecision(std::numeric_limits<myfloat>::digits10 + 1);

#ifdef MI_BENCHMARK

    int runs = 10;
    std::string filename("sphere10.stl");
    myfloat translation(0.5);

    if (argc > 1) {
        filename = argv[1];
    }
    if (argc > 2) {
        translation = std::stof(argv[2]);
    }
    if (argc > 3) {
        runs = std::stoi(argv[3]);
    }

    std::vector<triangle> original_mesh = mesh::load_mesh(filename);
    std::cout << "Loaded " << filename << " (" << original_mesh.size() << " triangles)" << std::endl;

    myfloat volume_sum = 0;
    double time_sum = 0;

    for (int i = 0; i < runs; ++i) {
        std::vector<triangle> mesh = original_mesh;
        std::vector<triangle> mesh_copy = original_mesh;
        mesh::transform(glm::translate(myvec(translation, 0, 0)), mesh_copy);

        center_pair_around_origin(mesh, mesh_copy);
        mesh::perturb_vertices(mesh);

        std::vector<ntriangle> mesh_normals = mesh::generate_normals(mesh);
        std::vector<ntriangle> mesh_copy_normals = mesh::generate_normals(mesh_copy);

        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        myfloat volume = mesh::intersection_volume(mesh_normals, mesh_copy_normals);
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();

        std::chrono::duration<double> delta = end - start;
        std::cout << volume << " | " << delta.count() << " s\n";

        volume_sum += volume;
        time_sum += delta.count();
    }
    std::cout << std::endl;
    std::cout << "Average over " << runs << " runs:" << std::endl;
    std::cout << "Intersection volume: " << (volume_sum / runs) << std::endl;
    std::cout << "Elapsed time: " << (time_sum / runs) << " s" << std::endl;

#else

    std::vector<triangle> first_mesh;
    std::vector<triangle> second_mesh;

    // command line interface
    if (argc <= 1 || argc >= 4) {
        std::cerr << "Invalid number of arguments supplied.";
        return 1;
    } else if (argc == 2) {
        first_mesh = mesh::load_mesh(argv[1]);
        myfloat volume = mesh::volume(first_mesh);
        std::cout << "Mesh volume: " << volume << std::endl;
    } else {
        first_mesh = mesh::load_mesh(argv[1]);
        second_mesh = mesh::load_mesh(argv[2]);

        // compute intersection
        center_pair_around_origin(first_mesh, second_mesh);
        mesh::perturb_vertices(first_mesh);

        std::vector<ntriangle> first_mesh_normals = mesh::generate_normals(first_mesh);
        std::vector<ntriangle> second_mesh_normals = mesh::generate_normals(second_mesh);

        std::cout << "Preparation complete. Triangles: "
                  << first_mesh.size() << " vs " << second_mesh.size() << "." << std::endl;

        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        myfloat volume = mesh::intersection_volume(first_mesh_normals, second_mesh_normals);
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();

        std::cout << "Intersection volume: " << volume << std::endl;
        std::chrono::duration<double> delta = end - start;
        std::cout << delta.count() << " seconds elapsed." << std::endl;
    }

#endif

#ifdef MI_VISUALIZE
    visualize::init(argc, argv);

    std::vector<float> mesh_buffer;
    auto insert = [&](const std::vector<triangle> &v){
        auto p = reinterpret_cast<const myfloat *>(v.data());
        for (int i = 0; i < v.size() * 9; ++i) {
            mesh_buffer.push_back((float) p[i]);
        }
    };
    insert(first_mesh);
    insert(second_mesh);

    int len_meshes[] { (int) first_mesh.size(), (int) second_mesh.size() };
    //float colors[] { 1, 1, 1,
    //                 0.5f, 0.5f, 0.7f };
    float colors[] { 0, 0, 0,
                     0.2f, 0.2f, 0.4f };

    int term_count = (int) lines.size() / 12;

    std::cout << "Visualizing " << term_count << " terms." << std::endl;

    return visualize::launch(term_count, lines.data(), 2, len_meshes, mesh_buffer.data(), colors);
#endif
}
