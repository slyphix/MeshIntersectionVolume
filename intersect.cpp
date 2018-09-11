
#include "globals.h"
#include "mesh.h"

#include <vector>

#ifdef MI_CUDA_ENABLED
#   include "impl/gpu.inl"
#else
#   include "impl/cpu.inl"
#endif

namespace mesh {

    myfloat intersection_volume(const std::vector<triangle> &first_mesh, const std::vector<triangle> &second_mesh) {
        // this could be computed on the gpu
        std::vector<ntriangle> first_mesh_normals = mesh::generate_normals(first_mesh);
        std::vector<ntriangle> second_mesh_normals = mesh::generate_normals(second_mesh);
        return impl::intersection_volume(first_mesh_normals, second_mesh_normals);
    }

    myfloat intersection_volume(const std::vector<ntriangle> &first_mesh, const std::vector<ntriangle> &second_mesh) {
        return impl::intersection_volume(first_mesh, second_mesh);
    }
}
