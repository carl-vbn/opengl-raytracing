#pragma once

#include <glm/gtc/matrix_transform.hpp>

namespace Animation {
	extern bool currentlyRenderingAnimation;
	extern int currentFrame;
	extern int currentPass;
	extern int totalFrameCount;

	extern glm::vec3 positionA, positionB;
	extern glm::vec2 orientationA, orientationB;

	extern float cameraSpeed;

	extern int framePasses; // How many times a frame should be rendered before the combined result is saved to disk.
	extern int frameRate;

	void setStartPosition(glm::vec3 cameraPos, float cameraYaw, float cameraPitch);
	void setEndPosition(glm::vec3 cameraPos, float cameraYaw, float cameraPitch);
	
	void recalculateTotalFrameCount();
	glm::vec3 calculateCurrentCameraPosition();
	glm::vec2 calculateCurrentCameraOrientation();
}