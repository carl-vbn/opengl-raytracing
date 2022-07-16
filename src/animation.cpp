#include "animation.h"

namespace Animation {
	bool currentlyRenderingAnimation = false;
	int currentFrame = 0;
	int currentPass = 0;
	int totalFrameCount;

	glm::vec3 positionA, positionB;
	glm::vec2 orientationA, orientationB;

	float cameraSpeed = 1.0f;

	int framePasses = 16;
	int frameRate = 24;

	void setStartPosition(glm::vec3 cameraPos, float cameraYaw, float cameraPitch) {
		positionA = cameraPos;
		orientationA = glm::vec2(cameraYaw, cameraPitch);
	}

	void setEndPosition(glm::vec3 cameraPos, float cameraYaw, float cameraPitch) {
		positionB = cameraPos;
		orientationB = glm::vec2(cameraYaw, cameraPitch);
	}

	void recalculateTotalFrameCount() {
		totalFrameCount = glm::distance(positionA, positionB) / cameraSpeed * frameRate;
	}

	glm::vec3 calculateCurrentCameraPosition() {
		return glm::mix(positionA, positionB, (float) currentFrame / totalFrameCount);
	}
	
	glm::vec2 calculateCurrentCameraOrientation() {
		return glm::mix(orientationA, orientationB, (float)currentFrame / totalFrameCount);
	}
}