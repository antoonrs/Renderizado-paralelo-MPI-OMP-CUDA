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

			if (tokens.empty()) continue; // Línea vacía

			// Esperamos al menos la palabra clave "Object"
			if (tokens[0] == "Object" && tokens.size() >= 12) { // Mínimo para Sphere y un material con 1 float
				// Parsear la esfera
				if (tokens[1] == "Sphere" && tokens[2] == "(" && tokens[7] == ")") {
					try {
						float sx = std::stof(tokens[3].substr(tokens[3].find('(') + 1, tokens[3].find(',') - tokens[3].find('(') - 1));
						float sy = std::stof(tokens[4].substr(0, tokens[4].find(',')));
						float sz = std::stof(tokens[5].substr(0, tokens[5].find(',')));
						float sr = std::stof(tokens[6]);

						// Parsear el material del último objeto creado

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
							std::cerr << "Error: Material desconocido o formato incorrecto en la línea: " << line << std::endl;
						}
					}
					catch (const std::invalid_argument& e) {
						std::cerr << "Error: Conversión inválida en la línea: " << line << " - " << e.what() << std::endl;
					}
					catch (const std::out_of_range& e) {
						std::cerr << "Error: Valor fuera de rango en la línea: " << line << " - " << e.what() << std::endl;
					}
				}
				else {
					std::cerr << "Error: Formato de esfera incorrecto en la línea: " << line << std::endl;
				}
			}
			else {
				std::cerr << "Error: Formato de objeto incorrecto en la línea: " << line << std::endl;
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

void rayTracingCPU(unsigned char* img, int w, int h, int ns = 10, int px = 0, int py = 0, int pw = -1, int ph = -1) {
	if (pw == -1) pw = w;
	if (ph == -1) ph = h;
	int patch_w = pw - px;
	// Scene world = randomScene();
	Scene world = loadObjectsFromFile("../../../../OMP/Scene1.txt");
	world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
	world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));

	Vec3 lookfrom(13, 2, 3);
	Vec3 lookat(0, 0, 0);
	float dist_to_focus = 10.0;
	float aperture = 0.1f;

	Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, float(w) / float(h), aperture, dist_to_focus);

	//std::cout << "RT de " << px << " a " << pw << " y de " << py << " a " << ph << std::endl;

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

Patch divideByRows(int w, int h, int nt, int tid) {
	int rows_per_thread = h / nt;
	int extra_rows = h % nt;

	int start_row = tid * rows_per_thread + std::min(tid, extra_rows);
	int num_rows = rows_per_thread + (tid < extra_rows ? 1 : 0);

	return { 0, start_row, w, start_row + num_rows };
}

Patch divideByCols(int w, int h, int nt, int tid) {
	int cols_per_proc = w / nt;
	int extra_cols = w % nt;

	int start_col = tid * cols_per_proc + std::min(tid, extra_cols);
	int num_cols = cols_per_proc + (tid < extra_cols ? 1 : 0);

	return { start_col, 0, start_col + num_cols, h };
}

Patch divideByBlocks(int w, int h, int nt, int tid) {
	//num filas
	int rows = static_cast<int>(std::floor(std::sqrt(nt)));
	if (nt >= 2) rows = std::max(rows, 2);
	rows = std::min(rows, nt);

	//num columnas
	int cols_base = (nt + rows - 1) / rows;

	// distribucion de procesos (columnas)
	std::vector<int> cols_in_row(rows);
	for (int r = 0; r < rows; ++r) {
		if (r < rows - 1) {
			cols_in_row[r] = cols_base;
		}
		else {
			cols_in_row[r] = nt - cols_base * (rows - 1);
		}
	}

	// para cada rank
	int row = tid / cols_base;
	int col = tid % cols_base;

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

	return { px, py, pw, ph };
}

void identifyThread(int tid, int& threadInFrame, int& frameId, int numFrames, const std::vector<int>& frameOffsets, const std::vector<int>& threadsPerFrame) {
	for (int i = 0; i < numFrames; ++i) {
		if (tid >= frameOffsets[i] && tid < frameOffsets[i] + threadsPerFrame[i]) {
			frameId = i;
			break;
		}
	}
	threadInFrame = tid - frameOffsets[frameId];
}

int main(int argc, char** argv) {
	// totalThreads, numFrames, w, h, ns, strategy (cols|rows|blocks)
	int totalThreads = std::atoi(argv[1]); // 8
	int numFrames = std::atoi(argv[2]); // 4;
	int w = std::atoi(argv[3]); // 1024;
	int h = std::atoi(argv[4]); // 1024;
	int ns = std::atoi(argv[5]); // 10;
	std::string strategy = argv[6];

	omp_set_num_threads(totalThreads);

	double time_start, time_end;
	time_start = omp_get_wtime();
	
	//std::cout << "Iniciando rayTracing en CPU con " << totalThreads << " hilos OMP" << std::endl;

	if (numFrames > totalThreads) {
		std::cerr << "CUIDADO: solo hay " << totalThreads << " procesos, ajustando numero de fotogramas de " << numFrames << " a " << totalThreads << std::endl;
		numFrames = totalThreads;
	}
	
	int bufferSize = sizeof(unsigned char) * w * h * 3;
	std::vector<unsigned char*> frameBuffers(numFrames, nullptr);

	for (int i = 0; i < numFrames; ++i) {
		frameBuffers[i] = (unsigned char*)calloc(bufferSize, 1);
	}

	std::vector<int> frameOffsets(numFrames);
	std::vector<int> threadsPerFrame(numFrames);
	int baseThreadPerFrame = totalThreads / numFrames;
	int extra = totalThreads % numFrames;
	int offset = 0;
	for (int i = 0; i < numFrames; ++i) {
		threadsPerFrame[i] = baseThreadPerFrame + (i < extra ? 1 : 0);
		frameOffsets[i] = offset;
		offset += threadsPerFrame[i];
	}


	std::vector<double> frameTimes(numFrames);

	#pragma omp parallel
	{
		double startLocal, endLocal;
		int tid = omp_get_thread_num();
		/*
		int threadsPerFrame = totalThreads / numFrames + (tid % numFrames < totalThreads % numFrames ? 1 : 0);
		int frameId = tid / threadsPerFrame; // id del frame sobre el que trabaja el hilo
		int threadInFrame = tid % threadsPerFrame; // id del thread dentro del grupo que trabaja en este hilo
		*/
		int frameId = -1;
		int threadInFrame = -1;
		identifyThread(tid, threadInFrame, frameId, numFrames, frameOffsets, threadsPerFrame);


		if (threadInFrame == 0){
			startLocal = omp_get_wtime();
		}

		unsigned char* data = frameBuffers[frameId];

		Patch myPatch;
		if (strategy == "cols") myPatch = divideByCols(w, h, threadsPerFrame[frameId], threadInFrame);
		else if (strategy == "rows") myPatch = divideByRows(w, h, threadsPerFrame[frameId], threadInFrame);
		else myPatch = divideByBlocks(w, h, threadsPerFrame[frameId], threadInFrame);

		rayTracingCPU(data, w, h, ns, myPatch.px, myPatch.py, myPatch.pw, myPatch.ph);

		#pragma omp barrier
		if (threadInFrame == 0) {
			std::string filename = "../../../../OMP/Imagenes/imgCPUImg" + std::to_string(frameId + 1) + ".bmp";
			writeBMP(filename.c_str(), data, w, h);

			endLocal = omp_get_wtime();
				
			#pragma omp critical
			{
				//std::cout << "Fotograma " << frameId + 1 << " guardado por hilo " << tid << std::endl;
				//std::cout << "Tiempo local " << (endLocal - startLocal) << std::endl;
				frameTimes[frameId] = (endLocal - startLocal);
			}
		}
		
		
	}

	time_end = omp_get_wtime();
	//std::cout << "Imagenes creadas en " << (time_end - time_start) << std::endl;
	
	// para el CSV
	std::cout << numFrames << ","
		<< w << ","
		<< h << ","
		<< ns << ","
		<< totalThreads << ","
		<< (time_end - time_start);
	for (double t : frameTimes) {
		std::cout << "," << t;
	}
	std::cout << std::endl;

	for (int i = 0; i < numFrames; ++i) {
		free(frameBuffers[i]);
	}

	//getchar();
	return (0);
}
