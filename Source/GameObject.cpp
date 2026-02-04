#include "Globals.h"
#include "GameObject.h"
#include "LightComponent.h"

GameObject::GameObject(short newUuid) : m_uuid(newUuid)
{
    m_transform = new Transform();
    m_transform->setGameObject(this);
}

GameObject::~GameObject()
{
}

bool GameObject::AddComponent(Component* newComponent)
{
    if (newComponent->getType() == TRANSFORM)
    {
        return false;
    }

    newComponent->setGameObject(this);
    m_components.push_back(newComponent);
    return true;
}

bool GameObject::RemoveComponent(Component* componentToRemove)
{
    const size_t before = m_components.size();

    m_components.erase(
        std::remove_if(m_components.begin(), m_components.end(),
            [componentToRemove](Component* component)
            {
                if (component == componentToRemove)
                {
                    delete component;
                    return true;
                }
                return false;
            }),
        m_components.end()
    );

    return m_components.size() != before;
}

//

Component* GameObject::FindComponent(ComponentType type)
{
    for (Component* c : m_components) {
        if (c && c->getType() == type) {
            return c;
        }
    }
    return nullptr;
}

const Component* GameObject::FindComponent(ComponentType type) const
{
    for (Component* c : m_components) {
        if (c && c->getType() == type) {
            return c;
        }
    }
    return nullptr;
}

LightComponent* GameObject::GetLightComponent()
{
    return static_cast<LightComponent*>(FindComponent(ComponentType::LIGHT));
}

const LightComponent* GameObject::GetLightComponent() const
{
    return static_cast<const LightComponent*>(FindComponent(ComponentType::LIGHT));
}