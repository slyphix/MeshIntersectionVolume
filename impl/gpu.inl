#include "../evaluation.h"

#include "../glm/glm.hpp"

#include <algorithm>
#include <vector>

#include <thrust/device_vector.h>
#include <thrust/fill.h>

namespace mesh {
namespace impl {

    // atomicAdd wrapper
    template <typename value_t>
    __device__ __forceinline__
    value_t cuda_atomic_add(value_t *address, value_t value) {
        return atomicAdd(address, value);
    }

#if __CUDA_ARCH__ < 600
    // for devices with compute capability < 6.x (see CUDA Programming Guide)
    template <>
    __device__ __forceinline__
    double cuda_atomic_add(double* address, double val) {
        unsigned long long int* address_as_ull = (unsigned long long int*) address;
        unsigned long long int old = *address_as_ull, assumed;

        do {
            assumed = old;
            old = atomicCAS(address_as_ull, assumed, __double_as_longlong(val + __longlong_as_double(assumed)));
        } while (assumed != old);

        return __longlong_as_double(old);
    }
#endif

    using ic = eval::intersection_count;

    __device__ __forceinline__
    void fuse_count(ic &global, const ic &local) {
        if (local.on_segment != 0)
            cuda_atomic_add(&global.on_segment, local.on_segment);
        if (local.before_segment != 0)
            cuda_atomic_add(&global.before_segment, local.before_segment);
    }

    template <typename dividend_t, typename divisor_t>
    __forceinline__
    dividend_t safe_division(dividend_t x, divisor_t y) {
        return (x + y - 1) / y;
    }

    __global__
    void intersect_kernel(const ntriangle *line_buffer, std::size_t line_count, const ntriangle *triangle_buffer, std::size_t triange_count, myfloat *accum, ic *ic) {

        std::size_t line_id = blockIdx.x * blockDim.x + threadIdx.x;
        std::size_t triangle_id = blockIdx.y * blockDim.y + threadIdx.y;

        if (line_id >= line_count)
            return;

        const triangle_side side = extract_side(line_buffer[line_id / 3], line_id % 3);

        // grid stride loop
        std::size_t grid_stride_y = gridDim.y * blockDim.y;
        for (std::size_t triangle_index = triangle_id; triangle_index < triange_count; triangle_index += grid_stride_y) {

            eval::intersection_count local_ic = eval::intersection_count::zero();

            myfloat result = eval::intersect_line_triangle(triangle_buffer[triangle_index], side, local_ic);

            fuse_count(ic[line_id], local_ic);

            if (result != 0)
                cuda_atomic_add(accum, result);
        }
    }

    __global__
#ifdef MI_SPARSE_EVAL
    void evaluate_kernel(const ntriangle *line_buffer, std::size_t line_count, myfloat *accum, const ic *ic) {
#else
    void evaluate_kernel(const ntriangle *line_buffer, std::size_t line_count, myfloat *point_eval_buffer, const ic *ic) {
#endif

        const std::size_t thread_id = blockIdx.x * blockDim.x + threadIdx.x;
        const std::size_t line_id = thread_id;

        myfloat result = 0;

        if (line_id < line_count) {
            const std::size_t line_index = line_id / 3;
            const triangle_side side = extract_side(line_buffer[line_index], line_id % 3);

            result = eval::evaluate_line_intersection(side, ic[line_id]);
        }

        // see https://devblogs.nvidia.com/using-cuda-warp-level-primitives/
        constexpr unsigned full_mask = 0xffffffff;
        for (int offset = 16; offset > 0; offset /= 2)
            result += __shfl_down_sync(full_mask, result, offset);

        // too lazy for ptx instructions
        const std::size_t lane_id = threadIdx.x & 0x1f;
        const std::size_t warp_id = thread_id >> 5;

#ifdef MI_SPARSE_EVAL
        // sparse reduction
        if (lane_id == 0 && result != 0)
            cuda_atomic_add(accum, result);
#else
        // dense reduction setup
        if (lane_id == 0)
            point_eval_buffer[warp_id] = result;
#endif
    }

    void asymetric_intersect(
            const ntriangle *line_buffer,
            std::size_t line_count,
            const ntriangle *triangle_buffer,
            std::size_t triange_count,
            myfloat *accum,
            myfloat *point_eval_buffer,
            ic *ic) {

        {
            std::size_t thread_limit_y = 65535;

            dim3 threads(32, 16);
            dim3 blocks(
                    (unsigned) safe_division(line_count, threads.x),
                    (unsigned) std::min(thread_limit_y, safe_division(triange_count, threads.y))
            );
            intersect_kernel<<<blocks, threads>>>(line_buffer, line_count, triangle_buffer, triange_count, accum, ic);
        }

        {
            dim3 threads(512);
            dim3 blocks((unsigned) safe_division(line_count, threads.x));
#ifdef MI_SPARSE_EVAL
            evaluate_kernel<<<blocks, threads>>>(line_buffer, line_count, accum, ic);
#else
            evaluate_kernel<<<blocks, threads>>>(line_buffer, line_count, point_eval_buffer, ic);
#endif
        }
    }

    myfloat intersection_volume(const std::vector<ntriangle> &first_mesh, const std::vector<ntriangle> &second_mesh) {

        myfloat accum = 0;

        std::size_t first_mesh_triangles = first_mesh.size();
        std::size_t second_mesh_triangles = second_mesh.size();

        std::size_t triangles_max = std::max(first_mesh_triangles, second_mesh_triangles);

        // memory setup
        thrust::device_vector<ntriangle> first_mesh_d(first_mesh);
        thrust::device_vector<ntriangle> second_mesh_d(second_mesh);
        thrust::device_vector<myfloat> accum_d(1, 0);
#ifdef MI_SPARSE_EVAL
        thrust::device_vector<myfloat> point_eval_d;
#else
        thrust::device_vector<myfloat> point_eval_d(safe_division(triangles_max * 3, 32));
#endif
        thrust::device_vector<ic> intersections_d(triangles_max * 3, eval::intersection_count::zero());

        asymetric_intersect(first_mesh_d.data().get(), first_mesh_triangles * 3,
                            second_mesh_d.data().get(), second_mesh_triangles,
                            accum_d.data().get(),
                            point_eval_d.data().get(),
                            intersections_d.data().get());

#ifndef MI_SPARSE_EVAL
        // reduce point evaluation results
        std::size_t first_len = safe_division(first_mesh_triangles * 3, 32);
        accum += thrust::reduce(point_eval_d.data(), point_eval_d.data() + first_len);
#endif

        // reset intersection count
        thrust::fill(intersections_d.begin(), intersections_d.end(), eval::intersection_count::zero());

        // do the same with both meshes swapped
        asymetric_intersect(second_mesh_d.data().get(), second_mesh_triangles * 3,
                            first_mesh_d.data().get(), first_mesh_triangles,
                            accum_d.data().get(),
                            point_eval_d.data().get(),
                            intersections_d.data().get());

#ifndef MI_SPARSE_EVAL
        std::size_t second_len = safe_division(second_mesh_triangles * 3, 32);
        accum += thrust::reduce(point_eval_d.data(), point_eval_d.data() + second_len);
#endif

        // implicit memory transfer
        return (accum + accum_d[0]) / 6;
    }
}
}
