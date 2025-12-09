#pragma once

#include "Module.h"
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

struct Camera
{
	DirectX::SimpleMath::Vector3 position = { 0.0f, 10.0f, 10.0f };
	Quaternion orientation = Quaternion::Identity;

	float aspect = 16.0f / 9.0f;

	float fovX = DirectX::XM_PIDIV4;
	float fovY = DirectX::XM_PIDIV4;

	float nearPlane = 0.1;
	float farPlane = 1000;
};

class ModuleCameraEditor : public Module
{
public:
	ModuleCameraEditor();
	void update() override;
	void updateWASD(float dt);
	void rightClickDrag(float dx, float dy);
	void altRightClickZoom(float dy);
	void mouseWheelZoom(float wheel);
	void orbitDrag(float dx, float dy);
	void focusOnGeometry();

	void requestResize(uint32_t w, uint32_t h);

	void setFOV(float horizontalFov);
	void setAspectRatio(float asp);
	void setPlaneDistances(float nearP, float farP);
	void setPosition(const Vector3& p);
	void setOrientation(const Quaternion& q);
	void setLookAt(const Vector3& target, const Vector3& worldUp = Vector3::Up);

	Camera& editCamera();
	const Camera& readCamera() const;

	const Matrix& getViewMatrix();
	const Matrix& getProjectionMatrix();



private:
	float calculateVerticalFovFromHorizontal(float fovX, float aspect);
	void buildViewMatrix();
	void buildProjectionMatrix();
	
	Camera camera;

	bool viewDirty = true;
	bool projDirty = true;

	DirectX::SimpleMath::Matrix view;
	DirectX::SimpleMath::Matrix proj;

	// -- Editor --
	enum class RMBMode { None, Rotate, Zoom };
	RMBMode rmbMode = RMBMode::None;
	bool wasRmbDown = false;

	float moveSpeed = 5.0f;
	float fastMultiplier = 4.0f;

	bool  isLooking = false;
	POINT lastMouse = { 0,0 };

	float yaw = 0.0f;
	float pitch = 0.0f; 

	float lookSensitivity = 0.0020f;  
	float pitchLimit = DirectX::XMConvertToRadians(89.0f);

	float zoomSensitivity = 0.05f;

	// Orbit pivot in the middle for the assignment, when object have their own pivot I will change this
	Vector3 pivot = Vector3::Zero;
	bool isFocused = false;
	float orbitDistance = 10.0f;
	bool wasOrbitDown = false;
};
