#pragma once

#include <random>
#include <glm/gtc/matrix_transform.hpp>

#include "scene.h"

using namespace Scene;

void placeRandomSpheres() {
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	std::uniform_int_distribution<> distr(0, 1000); // define the range

	for (int i = 0; i < 64; i++) {
		float radius = distr(gen) / 1000.0F;
		float x = distr(gen) / 1000.0F - 0.5F;
		float y = distr(gen) / 1000.0F - 0.5F;
		float z = distr(gen) / 1000.0F - 0.5F;
		float length = x * x + y * y + z * z;
		x = x / length * 5.0F;
		y = y / length * 5.0F;
		z = z / length * 5.0F;

		bool collision = false;
		for (int j = 0; j < i; j++) {
			float dist = glm::distance(glm::vec3(x, y, z), glm::vec3(objects[j].position[0], objects[j].position[1], objects[j].position[2]));
			if (dist < radius + objects[j].scale[0]) {
				collision = true;
				break;
			}
		}

		if (collision) {
			i--;
		}
		else {
			objects.push_back(Object(1, { x, y, z }, { radius, radius, radius }, Material({ distr(gen) / 1000.0F, distr(gen) / 1000.0F, distr(gen) / 1000.0F }, { distr(gen) / 1000.0F, distr(gen) / 1000.0F, distr(gen) / 1000.0F }, { 0, 0, 0 }, 0.0f, distr(gen) / 1000.0F, 0.0F, 0.0F)));
		}
	}

	objects.push_back(Object(1, { 0.0F, 1.0F, 0.0F }, { 1.0F, 1.0F, 1.0F }, Material({ 1.0F, 1.0F, 1.0F })));

	lights.push_back(PointLight({ 0.0F, 5.0F, 0.0F }, 0.5F, { 1.0F, 1.0F, 1.0F }, 1.0F, 100.0F));

	planeVisible = false;
}

void placeMirrorSpheres() {
	for (int i = -4; i <= 3; i++) {
		for (int j = -4; j <= 3; j++) {
			objects.push_back(Object(1, { (float)i, 1.0F, (float)j }, { 0.5F, 0.5F, 0.5F }, Material({ 0.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F }, { 0.0F, 0.0F, 0.0F }, 0.0F, 0.2F, 0.0F, 0.0F)));
		}
	}

	planeMaterial = Material({ 1.0F, 1.0F, 1.0F }, { 0.75F, 0.75F, 0.75F }, { 0.0F, 0.0F, 0.0F }, 0.0F, 0.0F, 0.0F, 0.0F);

	lights.push_back(PointLight({ 0.0F, 5.0F, 0.0F }, 0.5F, { 1.0F, 1.0F, 1.0F }, 1.0F, 100.0F));
}

void placeBasicScene() {
	objects.push_back(Object(1, { 0.0F, 0.5F, 0.0F }, { 0.5F, 0.5F, 0.5F }, Material({ 1.0F, 1.0F, 1.0F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 0.0F }, 0.0F, 1.0F, 0.0F, 0.0F)));

	planeMaterial = Material({ 1.0F, 1.0F, 1.0F }, { 0.75F, 0.75F, 0.75F }, { 0.0F, 0.0F, 0.0F }, 0.0F, 0.0F, 0.0F, 0.0F);

	lights.push_back(PointLight({ 0.0F, 5.0F, 0.0F }, 0.5F, { 1.0F, 1.0F, 1.0F }, 1.0F, 100.0F));
}