#pragma once

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <initializer_list>
#include <algorithm>
#include <vector>

namespace Scene {
	struct Material {
		float albedo[3];
		float specular[3];
		float emission[3];
		float emissionStrength;
		float roughness;
		float specularHighlight;
		float specularExponent;
	
		Material(const std::initializer_list<float>& color);
		Material(const std::initializer_list<float>& albedo, const std::initializer_list<float>& specular, const std::initializer_list<float>& emission, float emissionStrength, float roughness, float specularHighlight, float specularExponent);
		Material();
	};

	struct Object {
		unsigned int type; // Type 0 = none (invisible), Type 1 = sphere, Type 2 = box
		float position[3];
		float scale[3]; // For spheres, only the x value will be used as the radius
		Material material;

		Object(unsigned int type, const std::initializer_list<float>& position, const std::initializer_list<float>& scale, Material material);
		Object();
	};

	struct PointLight {
		float position[3];
		float radius;
		float color[3];
		float power;
		float reach; // Only points within this distance of the light will be affected

		PointLight(const std::initializer_list<float>& position, float radius, const std::initializer_list<float>& color, float power, float reach);
		PointLight();
	};

	extern glm::vec3 cameraPosition;
	extern float cameraYaw, cameraPitch;

	extern GLuint boundShader;
	extern std::vector<Object> objects;
	extern std::vector<PointLight> lights;
	extern Material planeMaterial;
	extern int shadowResolution;
	extern int lightBounces;
	extern int framePasses;
	extern float blur;
	extern float bloomRadius;
	extern float bloomIntensity;
	extern float skyboxStrength;
	extern float skyboxGamma;
	extern float skyboxCeiling;
	extern int selectedObjectIndex;
	extern GLuint skyboxTexture;
	extern bool planeVisible;

	void bind(GLuint shaderProgram);
	void unbind();
	void selectHovered(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition, glm::mat4 rotationMatrix);
	void mousePlace(float mouseX, float mouseY, int screenWidth, int screenHeight, glm::vec3 cameraPosition, glm::mat4 rotationMatrix);
}


