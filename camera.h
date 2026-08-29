#pragma once

#include "backend.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>

class ArcballCamera
{
public:
	glm::vec3 target = glm::vec3(0.0f);
	float distance = 3.0f;

	glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

	float rotateSpeed = 0.005f;
	float panSpeed = 0.005f;
	float zoomSpeed = 0.2f;

	float fovY = 45.0f;
	float sceneSize = 3.0f;
	bool orthoProjection = false;

	glm::mat4 getViewMatrix() const {
		glm::vec3 pos = getPosition();
		glm::vec3 up = getUpVector();

		return glm::lookAt(pos, target, up);
	}

	glm::mat4 getProjectionMatrix(float aspect) const {
		float nearPlane = std::max(0.0f, sceneSize * 0.01f);
		float farPlane = std::max(100.0f, sceneSize * 10.0f);

		glm::mat4 proj;
		if (orthoProjection)
		{
			float halfH = distance * 0.5f;
			float halfW = halfH * aspect;
			proj = glm::ortho(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
		}
		else
		{
			proj = glm::perspective(glm::radians(fovY), aspect, nearPlane, farPlane);
		}
		if (BackendInstance->getBackendType() == BackendType::Vulkan)
		{
			proj[1][1] *= -1;
		}
		return proj;
	}

	glm::vec3 getPosition() const {
		glm::vec3 offset = orientation * glm::vec3(0.0f, 0.0f, distance);
		return target + offset;
	}

	glm::vec3 getUpVector() const {
		return orientation * glm::vec3(0.0f, 1.0f, 0.0f);
	}

	void rotate(float dx, float dy)
	{
		// yaw
		glm::quat yawQuat = glm::angleAxis(-dx * rotateSpeed, glm::vec3(0.0f, 1.0f, 0.0f));

		// pitch
		glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f);
		glm::quat pitchQuat = glm::angleAxis(dy * rotateSpeed, right);

		orientation = glm::normalize(pitchQuat * yawQuat * orientation);
	}

	void pan(float dx, float dy)
	{
		glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 up = orientation * glm::vec3(0.0f, 1.0f, 0.0f);

		target += right * (-dx * panSpeed * distance * 0.2f)
			+ up * (dy * panSpeed * distance * 0.2f);
	}

	void zoom(float factor)
	{
		float speed = std::max(0.02f, distance * 0.1f);
		distance = std::max(0.01f, distance - factor * speed);
	}

	void setYawPitch(float yawRad, float pitchRad) {
		glm::quat yawQ = glm::angleAxis(yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::quat pitchQ = glm::angleAxis(pitchRad, glm::vec3(1.0f, 0.0f, 0.0f));
		orientation = glm::normalize(yawQ * pitchQ);
	}
};