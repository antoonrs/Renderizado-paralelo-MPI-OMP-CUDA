# Declarar ejecutable
add_executable(mpi_omp_version
    main.cpp
	Camera.h
	CollisionData.h
	Crystalline.cpp
	Crystalline.h
	Diffuse.h
	Material.h
	Metallic.cpp
	Metallic.h
	Object.h
	random.cpp
	random.h
	Ray.h
	Scene.cpp
	Scene.h
	Shape.h
	Sphere.cpp
	Sphere.h
	utils.cpp
	utils.h
	Vec3.h
)

# Enlazar el ejecutable con las librería de MPI
target_link_libraries(mpi_omp_version PRIVATE MPI::MPI_CXX OpenMP::OpenMP_CXX)
