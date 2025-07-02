cmake_minimum_required(VERSION "3.20")
project(plantilla_cap LANGUAGES "CXX" "CUDA")

# Opciones CUDA
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES OFF)
endif()
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
add_compile_options("$<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>>:-G>")
include_directories(SYSTEM "$<$<COMPILE_LANGUAGE:CUDA>:${CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES}>")

# Dependencias comunes (si todos los proyectos las usan)
find_package(OpenMP REQUIRED COMPONENTS CXX)
find_package(MPI REQUIRED COMPONENTS CXX)
find_package(CUDAToolkit REQUIRED)

# Incluir subdirectorios de proyectos
add_subdirectory(CUDA)
add_subdirectory(MPI)
add_subdirectory(MPIOMP)
add_subdirectory(OMP)
