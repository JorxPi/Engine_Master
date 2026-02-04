#pragma once
#include <vector>
#include "Component.h"
#include <SimpleMath.h>

using namespace DirectX::SimpleMath;

class GameObject;

class Transform final : public Component {
public:
	Transform();
	const Vector3* getPosition() const { return &m_position; }
	const Quaternion* getRotation() const { return &m_rotation; }
	const Vector3* getEulerAngles() const { return &m_eulerAngles; }
	const Vector3* getScale() const { return &m_scale; }
	const Transform* getParent() { return m_parent; }
	const Transform* getChild(int index) { return m_children[index]; }
	const Matrix* getTransformation();
	Vector3 getForward() const;
	Vector3 getUp() const;
	Vector3 getRight() const;


	const Transform* findChild(const char* name);

	const void setPosition(Vector3* newPosition) { m_position = *newPosition; dirty = true; }
	const void setRotation(Quaternion* newRotation);
	const void setRotation(Vector3* newRotation);
	const void setScale(Vector3* newScale) { m_scale = *newScale;  dirty = true; }

	const void setParent(Transform* parent) { m_parent = parent; }
	const void addChild(Transform* child) { m_children.push_back(child); }
	const void removeChild(Transform* child);

	const void translate(Vector3* position);
	const void rotate(Vector3* eulerAngles);
	const void rotate(Quaternion* rotation);
	const void scalate(Vector3* scale);

	const Vector3 convertQuaternionToEulerAngles(Quaternion* rotation);

private:
	Vector3 m_position = Vector3::Zero;
	Quaternion m_rotation = Quaternion::Identity;
	Vector3 m_eulerAngles = Vector3::Zero;
	Vector3 m_scale = Vector3::One;
	Matrix m_transformation;
	bool dirty = true;

	Transform* m_parent = nullptr;
	Transform* root = nullptr;
	std::vector<Transform*> m_children;

	const void calculateMatrix();
};
