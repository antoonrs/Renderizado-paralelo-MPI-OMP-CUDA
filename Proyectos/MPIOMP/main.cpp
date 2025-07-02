//==================================================================================================
// Written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is distributed
// without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication along
// with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==================================================================================================

#include <float.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <vector>

#include <mpi.h>
#include <omp.h>

#include "Camera.h"
#include "Object.h"
#include "Scene.h"
#include "Sphere.h"
#include "Diffuse.h"
#include "Metallic.h"
#include "Crystalline.h"

#include "random.h"
#include "utils.h"

struct Patch {
	int px, py, pw, ph;
};

Scene loadObjectsFromFile(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;

	Scene list;

	if (file.is_open()) {
		while (std::getline(file, line)) {
			std::stringstream ss(line);
			std::string token;
			std::vector<std::string> tokens;

			while (ss >> token) {
				tokens.push_back(token);
			}

			if (tokens.empty()) continue;

			// Esperamos al menos la palabra clave "Object"
			if (tokens[0] == "Object" && tokens.size() >= 12) { // M�nimo para Sphere y un material con 1 float
				// Parsear la esfera
				if (tokens[1] == "Sphere" && tokens[2] == "(" && tokens[7] == ")") {
					try {
						float sx = std::stof(tokens[3].substr(tokens[3].find('(') + 1, tokens[3].find(',') - tokens[3].find('(') - 1));
						float sy = std::stof(tokens[4].substr(0, tokens[4].find(',')));
						float sz = std::stof(tokens[5].substr(0, tokens[5].find(',')));
						float sr = std::stof(tokens[6]);

						// Parsear el material del �ltimo objeto creado

						if (tokens[8] == "Crystalline" && tokens[9] == "(" && tokens[11].back() == ')') {
							float ma = std::stof(tokens[10]);
							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Crystalline(ma)
							));
							//std::cout << "Crystaline" << sx << " " << sy << " " << sz << " " << sr << " " << ma << "\n";
						}
						else if (tokens[8] == "Metallic" && tokens.size() == 15 && tokens[9] == "(" && tokens[14] == ")") {
							float ma = std::stof(tokens[10].substr(tokens[10].find('(') + 1, tokens[10].find(',') - tokens[10].find('(') - 1));
							float mb = std::stof(tokens[11].substr(0, tokens[11].find(',')));
							float mc = std::stof(tokens[12].substr(0, tokens[12].find(',')));
							float mf = std::stof(tokens[13].substr(0, tokens[13].length() - 1));
							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Metallic(Vec3(ma, mb, mc), mf)
							));
							//std::cout << "Metallic" << sx << " " << sy << " " << sz << " " << sr << " " << ma << " " << mb << " " << mc << " " << mf << "\n";
						}
						else if (tokens[8] == "Diffuse" && tokens.size() == 14 && tokens[9] == "(" && tokens[13].back() == ')') {
							float ma = std::stof(tokens[10].substr(tokens[10].find('(') + 1, tokens[10].find(',') - tokens[10].find('(') - 1));
							float mb = std::stof(tokens[11].substr(0, tokens[11].find(',')));
							float mc = std::stof(tokens[12].substr(0, tokens[12].find(',')));
							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Diffuse(Vec3(ma, mb, mc))
							));
							//std::cout << "Diffuse" << sx << " " << sy << " " << sz << " " << sr << " " << ma << " " << mb << " " << mc << "\n";
						}
						else {
							std::cerr << "Error: Material desconocido o formato incorrecto en la linea: " << line << std::endl;
						}
					}
					catch (const std::invalid_argument& e) {
						std::cerr << "Error: Conversi�n inv�lida en la linea: " << line << " - " << e.what() << std::endl;
					}
					catch (const std::out_of_range& e) {
						std::cerr << "Error: Valor fuera de rango en la linea: " << line << " - " << e.what() << std::endl;
					}
				}
				else {
					std::cerr << "Error: Formato de esfera incorrecto en la linea: " << line << std::endl;
				}
			}
			else {
				std::cerr << "Error: Formato de objeto incorrecto en la linea: " << line << std::endl;
			}
		}
		file.close();
	}
	else {
		std::cerr << "Error: No se pudo abrir el archivo: " << filename << std::endl;
	}
	return list;
}

Scene randomScene() {
	int n = 500;
	Scene list;
	list.add(new Object(
		new Sphere(Vec3(0, -1000, 0), 1000),
		new Diffuse(Vec3(0.5, 0.5, 0.5))
	));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float choose_mat = Mirandom();
			Vec3 center(a + 0.9f * Mirandom(), 0.2f, b + 0.9f * Mirandom());
			if ((center - Vec3(4, 0.2f, 0)).length() > 0.9f) {
				if (choose_mat < 0.8f) {  // diffuse
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Diffuse(Vec3(Mirandom() * Mirandom(),
							Mirandom() * Mirandom(),
							Mirandom() * Mirandom()))
					));
				}
				else if (choose_mat < 0.95f) { // metal
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Metallic(Vec3(0.5f * (1 + Mirandom()),
							0.5f * (1 + Mirandom()),
							0.5f * (1 + Mirandom())),
							0.5f * Mirandom())
					));
				}
				else {  // glass
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Crystalline(1.5f)
					));
				}
			}
		}
	}

	list.add(new Object(
		new Sphere(Vec3(0, 1, 0), 1.0),
		new Crystalline(1.5f)
	));
	list.add(new Object(
		new Sphere(Vec3(-4, 1, 0), 1.0f),
		new Diffuse(Vec3(0.4f, 0.2f, 0.1f))
	));
	list.add(new Object(
		new Sphere(Vec3(4, 1, 0), 1.0f),
		new Metallic(Vec3(0.7f, 0.6f, 0.5f), 0.0f)
	));

	return list;
}

void rayTracingCPULocalCoord(unsigned char* img, int w, int h, int ns = 10, int px = 0, int py = 0, int pw = -1, int ph = -1) {
	if (pw == -1) pw = w;
	if (ph == -1) ph = h;
	int patch_w = pw - px;
	// Scene world = randomScene();
	Scene world = loadObjectsFromFile("../../../../MPI/Scene1.txt");
	world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
	world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));

	Vec3 lookfrom(13, 2, 3);
	Vec3 lookat(0, 0, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.1f;

	Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, float(w) / float(h), aperture, dist_to_focus);

	for (int j = 0; j < (ph - py); j++) {
		for (int i = 0; i < (pw - px); i++) {

			Vec3 col(0, 0, 0);
			for (int s = 0; s < ns; s++) {
				float u = float(i + px + Mirandom()) / float(w);
				float v = float(j + py + Mirandom()) / float(h);
				Ray r = cam.get_ray(u, v);
				col += world.getSceneColor(r);
			}
			col /= float(ns);
			col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

			img[(j * patch_w + i) * 3 + 2] = char(255.99 * col[0]);
			img[(j * patch_w + i) * 3 + 1] = char(255.99 * col[1]);
			img[(j * patch_w + i) * 3 + 0] = char(255.99 * col[2]);
		}
	}
}

void rayTracingCPU(unsigned char* img, int w, int h, int ns = 10, int px = 0, int py = 0, int pw = -1, int ph = -1) {
	if (pw == -1) pw = w;
	if (ph == -1) ph = h;
	int patch_w = pw - px;
	// Scene world = randomScene();
	Scene world = loadObjectsFromFile("../../../../MPI/Scene1.txt");
	world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
	world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));

	Vec3 lookfrom(13, 2, 3);
	Vec3 lookat(0, 0, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.1f;

	//std::cout << "RT de " << px << " a " << pw << std::endl;

	Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, float(w) / float(h), aperture, dist_to_focus);

	for (int j = py; j < ph; j++) {
		for (int i = px; i < pw; i++) {

			Vec3 col(0, 0, 0);
			for (int s = 0; s < ns; s++) {
				float u = float(i + Mirandom()) / float(w);
				float v = float(j + Mirandom()) / float(h);
				Ray r = cam.get_ray(u, v);
				col += world.getSceneColor(r);
			}
			col /= float(ns);
			col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

			img[(j * w + i) * 3 + 2] = char(255.99 * col[0]);
			img[(j * w + i) * 3 + 1] = char(255.99 * col[1]);
			img[(j * w + i) * 3 + 0] = char(255.99 * col[2]);
		}
	}
}

Patch divideByRows(int w, int h, int np, int rank) {
	int rows_per_proc = h / np;
	int extra_rows = h % np;

	int start_row = rank * rows_per_proc + std::min(rank, extra_rows);
	int num_rows = rows_per_proc + (rank < extra_rows ? 1 : 0);

	return { 0, start_row, w, start_row + num_rows };
}

Patch divideByCols(int w, int h, int np, int rank) {
	int cols_per_proc = w / np;
	int extra_cols = w % np;

	int start_col = rank * cols_per_proc + std::min(rank, extra_cols);
	int num_cols = cols_per_proc + (rank < extra_cols ? 1 : 0);

	return { start_col, 0, start_col + num_cols, h };
}

Patch divideByBlocks(int width, int height, int num_processes, int rank) {
	// numero de filas
	int rows = static_cast<int>(std::floor(std::sqrt(num_processes)));
	if (num_processes >= 2) rows = std::max(rows, 2);
	rows = std::min(rows, num_processes);

	// numero de columnas
	int cols_base = (num_processes + rows - 1) / rows; // ceil(np/rows)

	// distribucion de procesos (columnas) por cada fila:
	std::vector<int> cols_in_row(rows);
	for (int r = 0; r < rows; ++r) {
		if (r < rows - 1)
			cols_in_row[r] = cols_base;
		else
			cols_in_row[r] = num_processes - cols_base * (rows - 1);
	}

	// para cada rank
	int row = rank / cols_base;
	int col = rank % cols_base;  // siempre col < cols_in_row[row] porque rank < np

	int baseH = height / rows;
	int remH = height % rows;
	int py = row * baseH + std::min(row, remH);
	int h_block = baseH + (row < remH ? 1 : 0);
	int ph = py + h_block;

	int thisRowCols = cols_in_row[row];
	int baseW = width / thisRowCols;
	int remW = width % thisRowCols;
	int px = col * baseW + std::min(col, remW);
	int w_block = baseW + (col < remW ? 1 : 0);
	int pw = px + w_block;

	return { px, py, pw, ph };
}

Patch subdivideByRows(int w, int h, int nt, int tid, int px, int py) {
	int rows_per_thread = h / nt;
	int extra_rows = h % nt;

	int start_row = tid * rows_per_thread + std::min(tid, extra_rows) + py;
	int num_rows = rows_per_thread + (tid < extra_rows ? 1 : 0);

	return { px, start_row, px+w, start_row + num_rows };
}

Patch subdivideByCols(int w, int h, int nt, int tid, int px, int py) {
	int cols_per_proc = w / nt;
	int extra_cols = w % nt;

	int start_col = tid * cols_per_proc + std::min(tid, extra_cols) + px;
	int num_cols = cols_per_proc + (tid < extra_cols ? 1 : 0);

	return { start_col, py, start_col + num_cols, py+h };
}

Patch subdivideByBlocks(int w, int h, int nt, int tid, int offsetX, int offsetY) {
	// numero de filas
	int rows = static_cast<int>(std::floor(std::sqrt(nt)));
	if (nt >= 2) rows = std::max(rows, 2);
	rows = std::min(rows, nt);

	// numero de columnas
	int cols_base = (nt + rows - 1) / rows; // ceil(np/rows)

	// distribucion de procesos (columnas) por cada fila:
	std::vector<int> cols_in_row(rows);
	for (int r = 0; r < rows; ++r) {
		if (r < rows - 1)
			cols_in_row[r] = cols_base;
		else
			cols_in_row[r] = nt - cols_base * (rows - 1);
	}

	// para cada rank
	int row = tid / cols_base;
	int col = tid % cols_base;  // siempre col < cols_in_row[row] porque rank < np

	int baseH = h / rows;
	int remH = h % rows;
	int py = row * baseH + std::min(row, remH);
	int h_block = baseH + (row < remH ? 1 : 0);
	int ph = py + h_block;

	int thisRowCols = cols_in_row[row];
	int baseW = w / thisRowCols;
	int remW = w % thisRowCols;
	int px = col * baseW + std::min(col, remW);
	int w_block = baseW + (col < remW ? 1 : 0);
	int pw = px + w_block;

	return { px + offsetX, py + offsetY, pw + offsetX, ph + offsetY };
}

int main(int argc, char** argv) {
	////////// threadsPorProceso, nFotogramas, width, height, ns, strategy (cols|rows|blocks), subStrategy (cols|rows|blocks)
	int threadsPorProceso = std::atoi(argv[1]);
	int nFotogramas = std::atoi(argv[2]);
	int w = std::atoi(argv[3]);
	int h = std::atoi(argv[4]);
	int ns = std::atoi(argv[5]);
	std::string strategy = argv[6];
	std::string subStrategy = argv[7];

	MPI_Init(&argc, &argv);
	int worldRank, worldNP;
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
	MPI_Comm_size(MPI_COMM_WORLD, &worldNP);

	// si hay menos procesos que fotogramas, ajustamos para renderizar solo tantas fotos como procesos haya
	if (worldNP < nFotogramas) {
		if (worldRank == 0) {
			//std::cerr << "CUIDADO: solo hay " << worldNP << " procesos, ajustando numero de fotogramas de " << nFotogramas << " a " << worldNP << std::endl;
		}
		nFotogramas = worldNP;
	}

	// repartir datos entre los fotogramas
	std::vector<int> procsPerFrame(nFotogramas), frameStart(nFotogramas);
	int base = worldNP / nFotogramas;
	int rem = worldNP % nFotogramas;
	for (int f = 0; f < nFotogramas; ++f) {
		procsPerFrame[f] = base + (f < rem ? 1 : 0);
		frameStart[f] = (f == 0 ? 0 : frameStart[f - 1] + procsPerFrame[f - 1]);
	}

	// a que fotograma pertenece
	int frameIdx = 0;
	for (int f = 0; f < nFotogramas; ++f) {
		if (worldRank >= frameStart[f] &&
			worldRank < frameStart[f] + procsPerFrame[f]) {
			frameIdx = f;
			break;
		}
	}

	// comunicador local por cada fotograma
	MPI_Comm frameComm;
	MPI_Comm_split(MPI_COMM_WORLD, frameIdx, worldRank, &frameComm);
	int rank, np;
	MPI_Comm_rank(frameComm, &rank);
	MPI_Comm_size(frameComm, &np);

	if (rank == 0) {
		/*std::cout << "Fotograma " << (frameIdx + 1) << "/" << nFotogramas
			<< " -> imagen " << w << "x" << h
			<< " con " << ns << " spp, strategy=" << strategy << ", subStrategy=" << subStrategy
			<< std::endl;*/
	}

	// preparar los patches
	std::vector<Patch> patches(np);
	if (rank == 0) {
		for (int i = 0; i < np; ++i) {
			if (strategy == "cols")   patches[i] = divideByCols(w, h, np, i);
			else if (strategy == "rows")   patches[i] = divideByRows(w, h, np, i);
			else                            patches[i] = divideByBlocks(w, h, np, i);
		}
	}
	MPI_Bcast(patches.data(), sizeof(Patch) * np, MPI_BYTE, 0, frameComm);

	Patch my = patches[rank];
	
	/*std::cout << "Proceso " << worldRank
		<< " de la imagen " << (frameIdx + 1)
		<< " maneja columnas [" << my.px << "," << my.pw << "] "
		<< "y filas [" << my.py << "," << my.ph << "]\n";
		*/
	// raytracing y medición temporal
	unsigned char* local_data = (unsigned char*)calloc(w * h * 3, 1);
	double init_time = 0.0, end_time = 0.0;
	if (rank == 0) init_time = omp_get_wtime();

	omp_set_num_threads(threadsPorProceso);

	#pragma omp parallel
	{
		int tid = omp_get_thread_num();
		int nt = omp_get_num_threads();
		
		Patch subpatch;
		if (subStrategy == "cols") {
			subpatch = subdivideByCols(my.pw - my.px, my.ph - my.py, nt, tid, my.px, my.py);
		}
		else if (subStrategy == "rows") {
			subpatch = subdivideByRows(my.pw - my.px, my.ph - my.py, nt, tid, my.px, my.py);
		}
		else {
			subpatch = subdivideByBlocks(my.pw - my.px, my.ph - my.py, nt, tid, my.px, my.py);
		}

		/*
		#pragma omp critical
		{
			std::cout << "Proceso " << worldRank << " thread " << tid << ": " << "maneja columnas[" << subpatch.px << ", " << subpatch.pw << "] "
				<< "y filas [" << subpatch.py << "," << subpatch.ph << "]\n";
		}
		*/

		rayTracingCPU(local_data, w, h, ns, subpatch.px, subpatch.py, subpatch.pw, subpatch.ph);
	}

	unsigned char* global_data = nullptr;
	if (rank == 0) global_data = (unsigned char*)calloc(w * h * 3, 1);
	MPI_Reduce(local_data, global_data, w * h * 3, MPI_UNSIGNED_CHAR, MPI_SUM, 0, frameComm);

	double frameTime = 0.0;
	if (rank == 0) {
		end_time = omp_get_wtime();
		frameTime = end_time - init_time;
	}

	// crear la foto
	if (rank == 0) {
		char filename[256];
		std::sprintf(filename, "../../../../MPIOMP/Imagenes/imgCPUImg%d.bmp", frameIdx + 1);
		writeBMP(filename, global_data, w, h);
		//std::cout << "Imagen creada en " << frameTime << " s" << std::endl;
		free(global_data);
	}
	free(local_data);

	MPI_Comm_free(&frameComm);

	// enviar tiempos de cada fotograma (solo si no es el proceso 0 global)
	if (rank == 0 && worldRank != 0) {
		MPI_Send(&frameTime, 1, MPI_DOUBLE, 0, frameIdx, MPI_COMM_WORLD);
	}

	// el maestro recibe tiempos, calcula tiempo total y emite CSV
	if (worldRank == 0) {
		std::vector<double> times(nFotogramas);
		if (rank == 0) times[0] = frameTime;
		for (int f = 1; f < nFotogramas; ++f) {
			MPI_Recv(&times[f], 1, MPI_DOUBLE,
				MPI_ANY_SOURCE, f, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		double totalTime = *std::max_element(times.begin(), times.end());

		// para el CSV
		std::cout << nFotogramas << ","
			<< w << ","
			<< h << ","
			<< ns << ","
			<< worldNP << ","
			<< threadsPorProceso << ","
			<< subStrategy << ","
			<< totalTime;
		for (double t : times) {
			std::cout << "," << t;
		}
		std::cout << std::endl;
	}

	MPI_Finalize();
	return 0;
}