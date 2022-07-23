#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include "gui.h"
#include "scene.h"
#include "animation.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "procedural_scenes.h"
#define CDS_FULLSCREEN 4

const GLfloat vertices[] = {
	-1.0F, -1.0F, 0.0F,
	1.0F, -1.0F, 0.0F,
	1.0F, 1.0F, 0.0F,
	1.0F, 1.0F, 0.0F,
	-1.0F, 1.0F, 0.0F,
	-1.0F, -1.0F, 0.0F
};

const GLfloat UVs[] = {
	0.0F, 0.0F,
	1.0F, 0.0F,
	1.0F, 1.0F,
	1.0F, 1.0F,
	0.0F, 1.0F,
	0.0F, 0.0F,
};

int screenWidth = 1920, screenHeight = 1080;
GLuint shaderProgram;
GLuint screenTexture;
bool mouseAbsorbed = false;
bool refreshRequired = false;

glm::mat4 rotationMatrix(1);
glm::vec3 forwardVector(0, 0, -1);

GLuint directOutPassUniformLocation, accumulatedPassesUniformLocation, timeUniformLocation, camPosUniformLocation, rotationMatrixUniformLocation, aspectRatioUniformLocation, debugKeyUniformLocation;

GLuint fbo;

void renderAnimation(GLFWwindow* window, glm::vec3 posA, float yawA, float pitchA, glm::vec3 posB, float yawB, float pitchB, int frames, int framePasses, int* renderedFrames=nullptr);

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;

	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);

	refreshRequired = true;
}

void mousebuttonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && !mouseAbsorbed && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		if (glfwGetKey(window, GLFW_KEY_E)) {
			Scene::mousePlace(mouseX, mouseY, screenWidth, screenHeight, Scene::cameraPosition, rotationMatrix);
		}
		else {
			Scene::selectHovered(mouseX, mouseY, screenWidth, screenHeight, Scene::cameraPosition, rotationMatrix);
		}
	}
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			if (mouseAbsorbed) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				mouseAbsorbed = false;
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwSetCursorPos(window, screenWidth / 2.0, screenHeight / 2.0);
				mouseAbsorbed = true;

				Scene::selectedObjectIndex = -1;
				glUniform1i(glGetUniformLocation(shaderProgram, "u_selectedSphereIndex"), -1);
			}
		}
		else if (key == GLFW_KEY_R) {
			GUI::animationRenderWindowVisible = !GUI::animationRenderWindowVisible;
		}
	}
}

// Loads the shader source from disk, replaces the constants and returns a new OpenGL Program
GLuint createShaderProgram(const char* vertex_file_path, const char* fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("Unable to open %s.\n", vertex_file_path);
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

// Uses createShaderProgram to create a program with the correct constants depending on the Scene and reassigns everything that needs to be. If a program already exists, it is deleted.
void recompileShader() {
	if (shaderProgram) glDeleteProgram(shaderProgram);
	shaderProgram = createShaderProgram("shaders\\vertex.glsl", "shaders\\fragment.glsl");
	glUseProgram(shaderProgram);
	Scene::bind(shaderProgram);

	directOutPassUniformLocation = glGetUniformLocation(shaderProgram, "u_directOutputPass");
	accumulatedPassesUniformLocation = glGetUniformLocation(shaderProgram, "u_accumulatedPasses");
	timeUniformLocation = glGetUniformLocation(shaderProgram, "u_time");
	camPosUniformLocation = glGetUniformLocation(shaderProgram, "u_cameraPosition");
	rotationMatrixUniformLocation = glGetUniformLocation(shaderProgram, "u_rotationMatrix");
	aspectRatioUniformLocation = glGetUniformLocation(shaderProgram, "u_aspectRatio");
	debugKeyUniformLocation = glGetUniformLocation(shaderProgram, "u_debugKeyPressed");

	glUniform1i(glGetUniformLocation(shaderProgram, "u_screenTexture"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram, "u_skyboxTexture"), 1);
}

float* load_image_data(char const* filename, int* x, int* y, int* channels_in_file, int desired_channels) {
	return stbi_loadf(filename, x, y, channels_in_file, desired_channels);
}

void free_image_data(void* imageData) {
	stbi_image_free(imageData);
}

bool handleMovementInput(GLFWwindow* window, double deltaTime, glm::vec3& cameraPosition, float& cameraYaw, float& cameraPitch, glm::mat4* rotationMatrix) {
	bool moved = false;
	
	double mouseX;
	double mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
	glfwSetCursorPos(window, screenWidth / 2.0, screenHeight / 2.0);

	float xOffset = (float)(mouseX - screenWidth/2.0);
	float yOffset = (float)(mouseY - screenHeight/2.0);

	if (xOffset != 0.0F || yOffset != 0.0F) moved = true;

	cameraYaw += xOffset * 0.002F;
	cameraPitch += yOffset * 0.002F;

	if (cameraPitch > 1.5707F)
		cameraPitch = 1.5707F;
	if (cameraPitch < -1.5707F)
		cameraPitch = -1.5707F;

	*rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), cameraPitch, glm::vec3(1, 0, 0)), cameraYaw, glm::vec3(0, 1, 0));
	
	glm::vec3 forward = glm::vec3(glm::vec4(0, 0, -1, 0) * (*rotationMatrix));
	
	glm::vec3 up(0, 1, 0);
	glm::vec3 right = glm::cross(forward, up);

	glm::vec3 movementDirection(0);
	float multiplyer = 1;

	if (glfwGetKey(window, GLFW_KEY_W)) {
		movementDirection += forward;
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		movementDirection -= forward;
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		movementDirection += right;
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		movementDirection -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE)) {
		movementDirection += up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
		movementDirection -= up;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) {
		multiplyer = 5;
	}


	if (glm::length(movementDirection) > 0.0f) {
		cameraPosition += glm::normalize(movementDirection) * (float)deltaTime * (float)multiplyer;
		moved = true;
	}

	return moved;
}

// Source: https://lencerf.github.io/post/2019-09-21-save-the-opengl-rendering-to-image-file/
void saveImage(GLFWwindow* w, int accumulatedPasses, const char* filepath) {
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	float* floatBuffer = new float[bufferSize];
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_FLOAT, floatBuffer);

	unsigned char* byteBuffer = new unsigned char[bufferSize];
	for (int i = 0; i < bufferSize; i++) byteBuffer[i] = (unsigned char)(std::min(floatBuffer[i] / accumulatedPasses, 1.0f) * 255);

	stbi_flip_vertically_on_write(true);
	stbi_write_png(filepath, width, height, nrChannels, byteBuffer, stride);

	delete[] floatBuffer;
	delete[] byteBuffer;
}

void renderAnimation(GLFWwindow* window, glm::vec3 posA, float yawA, float pitchA, glm::vec3 posB, float yawB, float pitchB, int frames, int framePasses, int* renderedFrames) {
	if (renderedFrames != nullptr) *renderedFrames = 0;
	
	glUniform1i(glGetUniformLocation(shaderProgram, "u_framePasses"), framePasses);
	for (int frame = 0; frame < frames; frame++) {
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) break;
		
		float t = (float)frame / frames;
		glm::vec3 position = posA + (posB - posA) * t;
		float yaw = yawA + (yawB - yawA) * t;
		float pitch = pitchA + (pitchB - pitchA) * t;

		glm::mat4 rotMatrix = glm::rotate(glm::rotate(glm::mat4(1), pitch, glm::vec3(1, 0, 0)), yaw, glm::vec3(0, 1, 0));

		glUniform3f(camPosUniformLocation, position.x, position.y, position.z);
		glUniformMatrix4fv(rotationMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(rotMatrix));
		glUniform1f(aspectRatioUniformLocation, (float)screenWidth / screenHeight);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glUniform1i(directOutPassUniformLocation, 0);
		glUniform1i(accumulatedPassesUniformLocation, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		saveImage(window, 1, std::string("anim\\").append(std::to_string(frame)).append(".png").c_str());
		if (renderedFrames != nullptr) *renderedFrames += 1;

		std::cout << "Rendered frame " << frame << "/" << frames << std::endl;
	}
	glUniform1i(glGetUniformLocation(shaderProgram, "u_framePasses"), Scene::framePasses);
}

int main() {
	std::cout << "Loading skybox" << std::endl;
	int sbWidth, sbHeight, sbChannels;
	float* skyboxData = stbi_loadf("skyboxes\\kiara_9_dusk_2k.hdr", &sbWidth, &sbHeight, &sbChannels, 0);

	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW!" << std::endl;
		return -1;
	}

	GLFWwindow* programWindow = glfwCreateWindow(screenWidth, screenHeight, "OpenGL Raytracing", NULL, NULL);

	if (!programWindow) {
		std::cout << "Failed to create a window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwSetFramebufferSizeCallback(programWindow, framebufferSizeCallback);
	glfwSetMouseButtonCallback(programWindow, mousebuttonCallback);
	glfwSetKeyCallback(programWindow, keyCallback);

	glfwMakeContextCurrent(programWindow);

	if (glewInit() != GLEW_OK) {
		std::cout << "Failed to initialize GLEW!" << std::endl;
		glfwTerminate();
		return -1;
	}

	placeBasicScene();
	recompileShader();

	GUI::init(programWindow);

	glGenTextures(1, &Scene::skyboxTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, Scene::skyboxTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, sbWidth, sbHeight, 0, GL_RGB, GL_FLOAT, skyboxData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(skyboxData);

	GLuint vertexArray;
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	
	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);

	GLuint uvBuffer;
	glGenBuffers(1, &uvBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);


	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(UVs), UVs, GL_STATIC_DRAW);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(vertexArray);

	glGenTextures(1, &screenTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR: Framebuffer is not complete!" << std::endl;
		return -1;
	}

	glUniform1i(glGetUniformLocation(shaderProgram, "u_screenTexture"), 0);
	glUniform1i(glGetUniformLocation(shaderProgram, "u_skyboxTexture"), 1);

	glViewport(0, 0, screenWidth, screenHeight);
	glDisable(GL_DEPTH_TEST);

	double deltaTime = 0.0f;
	int freezeCounter = 0;
	int accumulatedPasses = 0;
	while (!glfwWindowShouldClose(programWindow) && !GUI::shouldQuit) {
		double preTime = glfwGetTime();
		glfwPollEvents();

		if (Animation::currentlyRenderingAnimation) {
			if (Animation::currentFrame == -1) Animation::currentFrame = 0; // Setting currentFrame to -1 ensures we don't start writing frames before this code has been called.

			Scene::cameraPosition = Animation::calculateCurrentCameraPosition();
			glm::vec2 cameraOrientation = Animation::calculateCurrentCameraOrientation();
			rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), cameraOrientation.y, glm::vec3(1, 0, 0)), cameraOrientation.x, glm::vec3(0, 1, 0));
		
			if (Animation::currentPass == 0) refreshRequired = true;
		}
		else {
			if (mouseAbsorbed) {
				if (handleMovementInput(programWindow, deltaTime, Scene::cameraPosition, Scene::cameraYaw, Scene::cameraPitch, &rotationMatrix)) {
					refreshRequired = true;
				}
			}
			else {
				rotationMatrix = glm::rotate(glm::rotate(glm::mat4(1), Scene::cameraPitch, glm::vec3(1, 0, 0)), Scene::cameraYaw, glm::vec3(0, 1, 0));
			}


			if (glfwGetKey(programWindow, GLFW_KEY_ESCAPE) && glfwGetKey(programWindow, GLFW_KEY_LEFT_SHIFT)) break;
			glUniform1i(debugKeyUniformLocation, glfwGetKey(programWindow, GLFW_KEY_F));
		}

		if (refreshRequired) {
			accumulatedPasses = 0;
			refreshRequired = false;
			glUniform1i(accumulatedPassesUniformLocation, accumulatedPasses); // If the shader receives a value of 0 for accumulatedPasses, it will discard the buffer and just output what it rendered on that frame.
		}


		glUniform1f(timeUniformLocation, (float)preTime);
		glUniform3f(camPosUniformLocation, Scene::cameraPosition.x, Scene::cameraPosition.y, Scene::cameraPosition.z);
		glUniformMatrix4fv(rotationMatrixUniformLocation, 1, GL_FALSE, glm::value_ptr(rotationMatrix));
		glUniform1f(aspectRatioUniformLocation, (float)screenWidth / screenHeight);

		// Step 1: render to FBO
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glUniform1i(directOutPassUniformLocation, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		accumulatedPasses += 1;

		// Step 2: render to screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUniform1i(directOutPassUniformLocation, 1);
		glUniform1i(accumulatedPassesUniformLocation, accumulatedPasses);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		if (!mouseAbsorbed && !Animation::currentlyRenderingAnimation) GUI::render();

		glfwSwapBuffers(programWindow);

		deltaTime = glfwGetTime() - preTime;
		if (deltaTime > 1.0F) {
			freezeCounter += 1;
			if (freezeCounter >= 2) {
				std::cout << "Freeze detected. Shutting down..." << std::endl;
				break;
			}
		}
		else {
			freezeCounter = 0;
		}

		// Because Animation::currentlyRenderingAnimation is set by the GUI, it will be true down here before it is caught above. This is why GUI will initially set the currentFrame to -1 so this code knows it must not do anything.
		if (Animation::currentlyRenderingAnimation && Animation::currentFrame >= 0) {
			if (Animation::currentPass >= Animation::framePasses - 1) {
				saveImage(programWindow, 1, std::string("render_output\\").append(std::to_string(Animation::currentFrame)).append(".png").c_str());
			
				Animation::currentFrame++;
				if (Animation::currentFrame >= Animation::totalFrameCount) {
					Animation::currentlyRenderingAnimation = false;
				}

				Animation::currentPass = 0;
			}
			else {
				Animation::currentPass += 1;
			}

			if (glfwGetKey(programWindow, GLFW_KEY_ESCAPE)) Animation::currentlyRenderingAnimation = false;
		}
	}

	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &uvBuffer);
	glDeleteVertexArrays(1, &vertexArray);
	glDeleteProgram(shaderProgram);
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &screenTexture);


	GUI::cleanup();

	glfwDestroyWindow(programWindow);
	glfwTerminate();

	return 0;
}