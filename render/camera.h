#pragma once

class Camera {
public:
	Vec3d vCameraPosition;
	Vec3d vLookDir;
	Vec3d vUp = { 0,1,0 };
	Vec3d vTarget = { 0,0,1 };

	float fYaw;
	float fPitch;

	Camera() {
		vCameraPosition = { 0.0f, 0.0f, 0.0f };
		vLookDir = { 0.0f, 0.0f, 1.0f };

		fYaw = 0.0f;
		fPitch = 0.0f;
	}

	Camera(Vec3d vCamera, Vec3d vLookDir) {
		this->vCameraPosition = vCamera;
		this->vLookDir = vLookDir;

		fYaw = 0.0f;
		fPitch = 0.0f;
	}
};