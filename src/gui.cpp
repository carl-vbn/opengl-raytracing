#include "gui.h"
#include "animation.h"

#include <string>
#include <iostream>
#include <algorithm>

// The excessive use of the extern keyword here is probably not ideal. These functions should be declared in a header file but trying that resulted in linking errors whereas this works fine.
extern float* load_image_data(char const* filename, int* x, int* y, int* channels_in_file, int desired_channels);
extern void free_image_data(void* imageData);
extern bool refreshRequired;

namespace GUI {
	GLFWwindow* window;
	bool shouldQuit = false;
	bool animationRenderWindowVisible = false;

	void init(GLFWwindow* window) {
		GUI::window = window;

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 130");
	}

	void cleanup() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	std::string arrayElementName(const char* arrayName, int index, const char* keyName) {
		return std::string(arrayName).append("[").append(std::to_string(index)).append("].").append(keyName);
	}

	void shaderFloatParameter(const char* name, const char* displayName, float* floatPtr) {
		ImGui::Text(displayName);
		ImGui::SameLine();
		if (ImGui::InputFloat(std::string("##").append(name).c_str(), floatPtr)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, name), *floatPtr);
			refreshRequired = true;
		}
	}
	
	void shaderSliderParameter(const char* name, const char* displayName, float* floatPtr) {
		ImGui::Text(displayName);
		ImGui::SameLine();
		if (ImGui::SliderFloat(std::string("##").append(name).c_str(), floatPtr, 0.0f, 1.0f)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, name), *floatPtr);
			refreshRequired = true;
		}
	}

	void shaderVecParameter(const char* name, const char* displayName, float* floatPtr) {
		ImGui::Text(displayName);
		ImGui::SameLine();
		if (ImGui::InputFloat3(std::string("##").append(name).c_str(), floatPtr)) {
			if (Scene::boundShader) glUniform3f(glGetUniformLocation(Scene::boundShader, name), *(floatPtr + 0), *(floatPtr + 1), *(floatPtr + 2));
			refreshRequired = true;
		}
	}

	void shaderColorParameter(const char* name, const char* displayName, float* floatPtr) {
		ImGui::Text(displayName);
		ImGui::SameLine();
		if (ImGui::ColorPicker3(name, floatPtr)) {
			if (Scene::boundShader) glUniform3f(glGetUniformLocation(Scene::boundShader, name), *(floatPtr + 0), *(floatPtr + 1), *(floatPtr + 2));
			refreshRequired = true;
		}
	}

	void objectSettingsUI() {
		ImGui::Begin("Selected object");
		
		ImGui::PushItemWidth(-1);
		if (Scene::selectedObjectIndex != -1) {
			int i = Scene::selectedObjectIndex;
			std::string indexStr = std::to_string(i);

			ImGui::Text(std::string("Object #").append(indexStr).c_str());
			
			shaderVecParameter(arrayElementName("u_objects", i, "position").c_str(), "Position", Scene::objects[i].position);
			
			ImGui::Text("Is box");
			ImGui::SameLine();
			bool isBox = Scene::objects[i].type == 2;
			std::string typeVariableName = arrayElementName("u_objects", i, "type");
			std::string scaleVariableName = arrayElementName("u_objects", i, "scale");
			if (ImGui::Checkbox(std::string("##").append(typeVariableName).c_str(), &isBox)) {
				Scene::objects[i].type = isBox ? 2 : 1;
				if (Scene::boundShader) glUniform1ui(glGetUniformLocation(Scene::boundShader, typeVariableName.c_str()), Scene::objects[i].type);
				refreshRequired = true;

				if (isBox) {
					Scene::objects[i].scale[0] *= 2.0f;
					Scene::objects[i].scale[1] *= 2.0f;
					Scene::objects[i].scale[2] *= 2.0f;
				}
				else {
					float minDimension = std::min({ Scene::objects[i].scale[0], Scene::objects[i].scale[1], Scene::objects[i].scale[2] });
					Scene::objects[i].scale[0] = minDimension / 2.0f;
					Scene::objects[i].scale[1] = minDimension / 2.0f;
					Scene::objects[i].scale[2] = minDimension / 2.0f;
				}

				if (Scene::boundShader) glUniform3f(glGetUniformLocation(Scene::boundShader, scaleVariableName.c_str()), Scene::objects[i].scale[0], Scene::objects[i].scale[1], Scene::objects[i].scale[2]);
			}
			
			if (Scene::objects[i].type == 1) {
				ImGui::Text("Radius");
				ImGui::SameLine();
				if (ImGui::InputFloat(std::string("##").append(scaleVariableName).c_str(), &Scene::objects[i].scale[0])) {
					Scene::objects[i].scale[1] = Scene::objects[i].scale[0];
					Scene::objects[i].scale[2] = Scene::objects[i].scale[0];
					if (Scene::boundShader) glUniform3f(glGetUniformLocation(Scene::boundShader, scaleVariableName.c_str()), Scene::objects[i].scale[0], Scene::objects[i].scale[1], Scene::objects[i].scale[2]);
					refreshRequired = true;
				}
			}
			else if (Scene::objects[i].type == 2) {
				shaderVecParameter(scaleVariableName.c_str(), "Scale", Scene::objects[i].scale);
			}

			shaderColorParameter(arrayElementName("u_objects", i, "material.albedo").c_str(), "Albedo", Scene::objects[i].material.albedo);
			shaderColorParameter(arrayElementName("u_objects", i, "material.specular").c_str(), "Specular", Scene::objects[i].material.specular);
			shaderColorParameter(arrayElementName("u_objects", i, "material.emission").c_str(), "Emission", Scene::objects[i].material.emission);
			shaderFloatParameter(arrayElementName("u_objects", i, "material.emissionStrength").c_str(), "Emission Strength", &Scene::objects[i].material.emissionStrength);

			shaderSliderParameter(arrayElementName("u_objects", i, "material.roughness").c_str(), "Roughness", &Scene::objects[i].material.roughness);
			shaderSliderParameter(arrayElementName("u_objects", i, "material.specularHighlight").c_str(), "Highlight", &Scene::objects[i].material.specularHighlight);
			shaderSliderParameter(arrayElementName("u_objects", i, "material.specularExponent").c_str(), "Exponent", &Scene::objects[i].material.specularExponent);

			ImGui::NewLine();
		}
		else {
			ImGui::Text("Plane");

			ImGui::Text("Visible");
			ImGui::SameLine();
			if (ImGui::Checkbox("##plane_visible", &Scene::planeVisible)) {
				if (Scene::boundShader) glUniform1ui(glGetUniformLocation(Scene::boundShader, "u_planeVisible"), Scene::planeVisible);
				refreshRequired = true;
			}

			shaderColorParameter("u_planeMaterial.albedo", "Albedo", Scene::planeMaterial.albedo);
			shaderColorParameter("u_planeMaterial.specular", "Specular", Scene::planeMaterial.specular);
			shaderColorParameter("u_planeMaterial.emission", "Emission", Scene::planeMaterial.emission);
			shaderFloatParameter("u_planeMaterial.emissionStrength", "Emission Strength", &Scene::planeMaterial.emissionStrength);

			shaderSliderParameter("u_planeMaterial.roughness", "Roughness", &Scene::planeMaterial.roughness);
			shaderSliderParameter("u_planeMaterial.specularHighlight", "Highlight", &Scene::planeMaterial.specularHighlight);
			shaderSliderParameter("u_planeMaterial.specularExponent", "Exponent", &Scene::planeMaterial.specularExponent);
		}


		ImGui::PopItemWidth();
		ImGui::End();
	}

	void lightSettingsUI() {
		ImGui::Begin("Light settings");

		ImGui::PushItemWidth(-1);
		for (int i = 0; i < Scene::lights.size(); i++) {
			std::string indexStr = std::to_string(i);

			ImGui::Text(std::string("Light #").append(indexStr).c_str());
			ImGui::Text("Position");
			ImGui::SameLine();
			if (ImGui::InputFloat3(std::string("##light_pos_").append(indexStr).c_str(), Scene::lights[i].position)) {
				if (Scene::boundShader) glUniform3f(glGetUniformLocation(Scene::boundShader, std::string("u_lights[").append(std::to_string(i)).append("].position").c_str()), Scene::lights[i].position[0], Scene::lights[i].position[1], Scene::lights[i].position[2]);
				refreshRequired = true;
			}

			ImGui::Text("Radius");
			ImGui::SameLine();
			if (ImGui::InputFloat(std::string("##light_radius_").append(indexStr).c_str(), &Scene::lights[i].radius)) {
				if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, std::string("u_lights[").append(std::to_string(i)).append("].radius").c_str()), Scene::lights[i].radius);
				refreshRequired = true;
			}

			ImGui::Text(std::string("Light #").append(indexStr).c_str());
			ImGui::Text("Color");
			ImGui::SameLine();
			if (ImGui::ColorPicker3(std::string("##light_color_").append(indexStr).c_str(), Scene::lights[i].color)) {
				if (Scene::boundShader) glUniform3f(glGetUniformLocation(Scene::boundShader, std::string("u_lights[").append(std::to_string(i)).append("].color").c_str()), Scene::lights[i].color[0], Scene::lights[i].color[1], Scene::lights[i].color[2]);
				refreshRequired = true;
			}

			ImGui::Text("Power");
			ImGui::SameLine();
			if (ImGui::InputFloat(std::string("##light_power_").append(indexStr).c_str(), &Scene::lights[i].power)) {
				if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, std::string("u_lights[").append(std::to_string(i)).append("].power").c_str()), Scene::lights[i].power);
				refreshRequired = true;
			}

			ImGui::Text("Reach");
			ImGui::SameLine();
			if (ImGui::InputFloat(std::string("##light_reach_").append(indexStr).c_str(), &Scene::lights[i].reach)) {
				if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, std::string("u_lights[").append(std::to_string(i)).append("].reach").c_str()), Scene::lights[i].reach);
				refreshRequired = true;
			}

			if (i < 2) ImGui::NewLine();
		}

		ImGui::PopItemWidth();
		ImGui::End();
	}

	void appSettingsUI() {
		ImGui::Begin("App");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::PushItemWidth(-1);
		ImGui::Text("Shadow resolution");
		ImGui::SameLine();
		if (ImGui::InputInt("##shadowResolution", &Scene::shadowResolution)) {
			if (Scene::boundShader) glUniform1i(glGetUniformLocation(Scene::boundShader, "u_shadowResolution"), Scene::shadowResolution);
			refreshRequired = true;
		}

		ImGui::Text("Light bounces");
		ImGui::SameLine();
		if (ImGui::InputInt("##lightBounces", &Scene::lightBounces)) {
			if (Scene::boundShader) glUniform1i(glGetUniformLocation(Scene::boundShader, "u_lightBounces"), Scene::lightBounces);
			refreshRequired = true;
		}

		ImGui::Text("Passes per frame");
		ImGui::SameLine();
		if (ImGui::InputInt("##framePasses", &Scene::framePasses)) {
			if (Scene::boundShader) glUniform1i(glGetUniformLocation(Scene::boundShader, "u_framePasses"), Scene::framePasses);
			refreshRequired = true;
		}

		ImGui::Text("Blur");
		ImGui::SameLine();
		if (ImGui::InputFloat("##blur", &Scene::blur)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, "u_blur"), Scene::blur);
			refreshRequired = true;
		}

		ImGui::Text("Bloom Radius");
		ImGui::SameLine();
		if (ImGui::InputFloat("##bloomRadius", &Scene::bloomRadius)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, "u_bloomRadius"), Scene::bloomRadius);
			refreshRequired = true;
		}

		ImGui::Text("Bloom Intensity");
		ImGui::SameLine();
		if (ImGui::InputFloat("##bloomIntensity", &Scene::bloomIntensity)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, "u_bloomIntensity"), Scene::bloomIntensity);
			refreshRequired = true;
		}

		if (ImGui::Button("Quit")) {
			shouldQuit = true;
		}
		ImGui::PopItemWidth();

		ImGui::End();
	}

	void skyboxSettingsUI() {
		ImGui::Begin("Skybox");

		ImGui::PushItemWidth(-1);
		ImGui::Text("Intensity");
		ImGui::SameLine();
		if (ImGui::InputFloat("##skyboxStrength", &Scene::skyboxStrength)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, "u_skyboxStrength"), Scene::skyboxStrength);
			refreshRequired = true;
		}

		ImGui::Text("Gamma");
		ImGui::SameLine();
		if (ImGui::InputFloat("##skyboxGamma", &Scene::skyboxGamma)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, "u_skyboxGamma"), Scene::skyboxGamma);
			refreshRequired = true;
		}

		ImGui::Text("Ceiling");
		ImGui::SameLine();
		if (ImGui::InputFloat("##skyboxCeiling", &Scene::skyboxCeiling)) {
			if (Scene::boundShader) glUniform1f(glGetUniformLocation(Scene::boundShader, "u_skyboxCeiling"), Scene::skyboxCeiling);
			refreshRequired = true;
		}

		static char skyboxFilename[64];

		ImGui::Text("Filename");
		ImGui::SameLine();
		ImGui::InputText("##skyboxFileName", skyboxFilename, 64);
		
		if (ImGui::Button("Load")) {
			int sbWidth, sbHeight, sbChannels;
			float* skyboxData = load_image_data(std::string("skyboxes\\").append(skyboxFilename).c_str(), &sbWidth, &sbHeight, &sbChannels, 0);
			if (skyboxData) {
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, Scene::skyboxTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sbWidth, sbHeight, 0, GL_RGB, GL_FLOAT, skyboxData);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glActiveTexture(GL_TEXTURE0);

				free_image_data(skyboxData);

				skyboxFilename[0] = 0;

				refreshRequired = true;
			}
			else {
				std::cout << "Failed to load skyboxes\\" << skyboxFilename << std::endl;
			}
		}

		ImGui::PopItemWidth();
		ImGui::End();
	}

	void animationRenderingUI() {
		ImGui::Begin("Animation Rendering");

		if (ImGui::Button("Set start point")) {
			Animation::setStartPosition(Scene::cameraPosition, Scene::cameraYaw, Scene::cameraPitch);
		}

		if (ImGui::Button("Set end point")) {
			Animation::setEndPosition(Scene::cameraPosition, Scene::cameraYaw, Scene::cameraPitch);
		}

		ImGui::InputInt("animationFramePasses", &Animation::framePasses);
		ImGui::InputFloat("animationSpeed", &Animation::cameraSpeed);
		ImGui::InputInt("animationFrameRate", &Animation::frameRate);

		if (ImGui::Button("Render")) {
			Animation::currentFrame = -1; // Set to -1 so the main loop can finish its current iteration before the animation rendering process starts
			Animation::currentPass = 0;
			Animation::recalculateTotalFrameCount();
			Animation::currentlyRenderingAnimation = true;
		}

		ImGui::Text("Rendered %d/%d frames.", Animation::currentFrame, Animation::totalFrameCount);
		
		ImGui::End();
	}

	void cameraSettingsUI() {
		ImGui::Begin("Camera");
		ImGui::PushItemWidth(-1);
		ImGui::Text("Position");
		ImGui::SameLine();
		if (ImGui::InputFloat3("##cameraPosition", &Scene::cameraPosition.x)) {
			refreshRequired = true;
		}

		ImGui::Text("Orientation (Yaw-Pitch)");
		ImGui::SameLine();
		float cameraOrientation[2] = {Scene::cameraYaw, Scene::cameraPitch};
		if (ImGui::InputFloat2("##cameraOrientation", cameraOrientation)) {
			Scene::cameraYaw = cameraOrientation[0];
			Scene::cameraPitch = cameraOrientation[1];

			refreshRequired = true;
		}
		ImGui::PopItemWidth();
		ImGui::End();
	}

	void render() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		objectSettingsUI();
		lightSettingsUI();
		appSettingsUI();
		skyboxSettingsUI();
		cameraSettingsUI();
		if (animationRenderWindowVisible) animationRenderingUI();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}