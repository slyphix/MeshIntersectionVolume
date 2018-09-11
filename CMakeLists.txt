cmake_minimum_required(VERSION 3.3)

# set(VISUALIZE ON)
# set(CUDA_SUPPORT ON)
# set(OMP_SUPPORT ON)
# set(BENCHMARKS ON)
# set(SPARSE_EVALUATION ON)
# set(SINGLE_PRECISION ON)

if(CUDA_SUPPORT)
    set(GPULANG CUDA)
endif()

project(MeshIntersection LANGUAGES CXX ${GPULANG})

set(CMAKE_CXX_STANDARD 14)

if(VISUALIZE)
    message(STATUS "Visualization enabled")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMI_VISUALIZE")
    add_subdirectory(visualize)
endif()

set(SOURCE_FILES
        debugutils.hpp
        launcher.cpp
        mesh.cpp
        mesh.h
        intersect.h
#        localized.cpp
#        localized.h
        globals.h
        evaluation.h
        impl/cpu.inl
        impl/gpu.inl
        impl/evaluation.inl)

set(HYBRID_SOURCE_FILES intersect.cpp)

if(OMP_SUPPORT)
    message(STATUS "OpenMP support enabled")

    find_package(OpenMP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

if(BENCHMARKS)
    message(STATUS "Benchmarks enabled")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMI_BENCHMARK")
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -DMI_BENCHMARK")
endif()

if(SINGLE_PRECISION)
    message(STATUS "Single precision enabled")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMI_SINGLE_PRECISION")
    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -DMI_SINGLE_PRECISION")
endif()

if(CUDA_SUPPORT)
    message(STATUS "CUDA support enabled")
    find_package(CUDA REQUIRED)

    if(SPARSE_EVALUATION)
        message(STATUS "Sparse evaluation enabled")

        set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -DMI_SPARSE_EVAL")
    endif()

    set_source_files_properties( ${HYBRID_SOURCE_FILES} PROPERTIES CUDA_SOURCE_PROPERTY_FORMAT OBJ )

    set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -O3 -Xcudafe -w")
    cuda_add_executable(isv ${SOURCE_FILES} ${HYBRID_SOURCE_FILES})
else()
    add_executable(isv ${SOURCE_FILES} ${HYBRID_SOURCE_FILES})
endif()

if(VISUALIZE)
    target_link_libraries(isv visualize)
endif()
