#pragma once

class GameObject;
enum ComponentType
{
    TRANSFORM,
    LIGHT
};

class Component {
public:
    virtual ~Component() = default;

    void setGameObject(GameObject* go) { m_gameObject = go; }
    GameObject* getGameObject() const { return m_gameObject; }

    const short getID() { return m_uuid; }
    const ComponentType getType() { return m_type; }

    //virtual void drawUi();

protected:
    ComponentType m_type;
    GameObject* m_gameObject;

private:
    short m_uuid;
};