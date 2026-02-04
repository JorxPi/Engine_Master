#include "Globals.h"
#include "Transform.h"
#include "GameObject.h"

Transform::Transform()
{
    m_type = TRANSFORM;
}

const Matrix* Transform::getTransformation()
{
    if (dirty)
    {
        calculateMatrix();
    }
    return &m_transformation;
}

Vector3 Transform::getForward() const {
    Vector3 f = Vector3::Transform(Vector3::Forward, m_rotation);
    f.Normalize();
    return f;
}

Vector3 Transform::getUp() const {
    Vector3 u = Vector3::Transform(Vector3::Up, m_rotation);
    u.Normalize();
    return u;
}

Vector3 Transform::getRight() const {
    Vector3 r = Vector3::Transform(Vector3::Right, m_rotation);
    r.Normalize();
    return r;
}

const Transform* Transform::findChild(const char* name)
{
    for (size_t i = 0; i < m_children.size(); i++)
    {
        const char* childName = m_children[i]->m_gameObject->GetName();
        if (childName && std::strcmp(childName, name) == 0)
        {
            return m_children[i];
        }
    }
    return nullptr;
}

const void Transform::setRotation(Quaternion* newRotation)
{
    m_rotation = *newRotation;

    m_eulerAngles = convertQuaternionToEulerAngles(&m_rotation);

    dirty = true;
}

const void Transform::setRotation(Vector3* newRotation)
{
    m_eulerAngles = *newRotation;

    m_rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(m_eulerAngles.y), XMConvertToRadians(m_eulerAngles.x), XMConvertToRadians(m_eulerAngles.z));

    dirty = true;
}

const void Transform::removeChild(Transform* child)
{
    m_children.erase(std::remove_if(m_children.begin(), m_children.end(), [child](Transform* c)
        {
            if (c == child)
            {
                delete c;
                return true;
            }
            return false;
        }), m_children.end());
}

const void Transform::translate(Vector3* position)
{
    m_position = m_position + *position;
    dirty = true;
}

const void Transform::rotate(Vector3* eulerAngles)
{
    m_eulerAngles = m_eulerAngles + *eulerAngles;

    m_rotation = Quaternion::CreateFromYawPitchRoll(XMConvertToRadians(m_eulerAngles.y), XMConvertToRadians(m_eulerAngles.x), XMConvertToRadians(m_eulerAngles.z));

    dirty = true;

}

const void Transform::rotate(Quaternion* rotation)
{
    m_rotation = (*rotation) * m_rotation;
    m_rotation.Normalize();

    m_eulerAngles = convertQuaternionToEulerAngles(&m_rotation);

    dirty = true;
}

const void Transform::scalate(Vector3* scale)
{
    m_scale = m_scale + *scale;
    dirty = true;
}

const Vector3 Transform::convertQuaternionToEulerAngles(Quaternion* rotation)
{
    Quaternion q = *rotation;

    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    float rollRadians = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    float pitchRadians;
    if (std::abs(sinp) >= 1.0f)
        pitchRadians = std::copysign(DirectX::XM_PIDIV2, sinp);
    else
        pitchRadians = std::asin(sinp);

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    float yawRadians = std::atan2(siny_cosp, cosy_cosp);

    float pitchDegrees = DirectX::XMConvertToDegrees(pitchRadians);
    float yawDegrees = DirectX::XMConvertToDegrees(yawRadians);
    float rollDegrees = DirectX::XMConvertToDegrees(rollRadians);

    return Vector3(pitchDegrees, yawDegrees, rollDegrees);
}

const void Transform::calculateMatrix()
{
    m_transformation = Matrix::CreateScale(m_scale) * Matrix::CreateFromQuaternion(m_rotation) * Matrix::CreateTranslation(m_position);

}