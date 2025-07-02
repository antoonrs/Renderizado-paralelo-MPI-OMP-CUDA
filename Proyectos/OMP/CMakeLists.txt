# Declarar ejecutable
add_executable(omp_version
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

# Enlazar el ejecutable con las librería de OpenMP
target_link_libraries(omp_version PRIVATE OpenMP::OpenMP_CXX)