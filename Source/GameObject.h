#pragma once
#include "Transform.h"
#include "Component.h"
#include <vector>

class Scene;
enum Layer;
enum Tag;

class LightComponent;

class GameObject {
public:
	GameObject(short newUuid);
	~GameObject();

	const short GetID() { return m_uuid; }
	const char* GetName() { return m_name; }
	const bool GetActive() { return m_active; }
	const bool GetStatic() { return m_isStatic; }
	const Layer GetLayer() { return m_layer; }
	const Tag GetTag() { return m_tag; }
	Scene* GetScene() { return m_scene; }
	GameObject* GetParent() { return m_parent; }
	Transform* GetTransform() { return m_transform; }
	const Transform* GetTransform() const { return m_transform; }

	void SetName(char* newName) { m_name = newName; }
	void SetActive(bool newActive) { m_active = newActive; }
	void SetStatic(bool newIsStatic) { m_isStatic = newIsStatic; }
	void SetLayer(Layer newLayer) { m_layer = newLayer; }
	void SetTag(Tag newTag) { m_tag = newTag; }
	bool AddComponent(Component* newComponent);
	bool RemoveComponent(Component* componentToRemove);

	//Will delete later when scene can just create and have ownership of gameobjects
	Component* FindComponent(ComponentType type);
	const Component* FindComponent(ComponentType type) const;
	LightComponent* GetLightComponent();
	const LightComponent* GetLightComponent() const;


private:
	short m_uuid;
	char* m_name;
	bool m_active;
	bool m_isStatic;
	Layer m_layer;
	Tag m_tag;
	Scene* m_scene;
	GameObject* m_parent;
	Transform* m_transform;
	std::vector<Component*> m_components;
};