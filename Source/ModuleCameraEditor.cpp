#include "Globals.h"
#include "Application.h"
#include "ModuleCameraEditor.h"
#include "Mouse.h"
#include "ImGuizmo.h"
#include <algorithm>
#include <math.h>

ModuleCameraEditor::ModuleCameraEditor() {
    setLookAt(Vector3::Zero, Vector3::Up);
    setFOV(DirectX::XMConvertToRadians(60.0f));


}

// Helper

static Vector3 safeNormalize(const Vector3& v)
{
    float ls = v.LengthSquared();
    if (ls < THRESHOLD) return Vector3::Zero;
    Vector3 out = v / sqrtf(ls);
    return out;
}

//

void ModuleCameraEditor::update()
{
    auto& io = ImGui::GetIO();

    const bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    const bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

    if (ImGuizmo::IsUsing() || (ImGuizmo::IsOver() && lmbDown && !rmbDown))
    {
        rmbMode = RMBMode::None;
        wasRmbDown = false;
        wasOrbitDown = false;
        return;
    }

    const bool allowSceneInput = (sceneHovered || sceneFocused);

    if (!rmbDown && (io.WantCaptureKeyboard || io.WantCaptureMouse) && !allowSceneInput)
    {
        rmbMode = RMBMode::None;
        wasRmbDown = false;
        wasOrbitDown = false;
        return;
    }

    float dt = app->getElapsedMilis() * 0.001f;

    if (sceneHovered)
    {
        float wheel = app->consumeMouseWheel();
        if (wheel != 0.0f)
            mouseWheelZoom(wheel);
    }

    POINT cur{};
    GetCursorPos(&cur);

    //Orbit
    if (altDown && lmbDown)
    {
        if (!isFocused) {
            wasOrbitDown = false;
            return;
        }

        if (!wasOrbitDown)
        {
            wasOrbitDown = true;
            lastMouse = cur;

            orbitDistance = std::max(0.1f, (camera.position - pivot).Length());

            Vector3 dir = (pivot - camera.position);
            dir.Normalize();

            orbitPitch = asinf(dir.y);
            orbitYaw = atan2f(-dir.x, -dir.z);
            return;
        }

        const float dx = float(cur.x - lastMouse.x);
        const float dy = float(cur.y - lastMouse.y);
        lastMouse = cur;

        orbitDrag(dx, dy);
        return;
    }
    else
    {
        wasOrbitDown = false;
    }

    if (!rmbDown) {
        rmbMode = RMBMode::None;
        wasRmbDown = false;
        return;
    }

    if (!wasRmbDown)
    {
        wasRmbDown = true;
        lastMouse = cur;

        rmbMode = altDown ? RMBMode::Zoom : RMBMode::Rotate;
        return;
    }

    const float dx = float(cur.x - lastMouse.x);
    const float dy = float(cur.y - lastMouse.y);
    lastMouse = cur;

    if (rmbMode == RMBMode::Rotate) {
        applyMouseLook(dx, dy);
        updateWASD(dt);

        isFocused = false;
    }
    else if (rmbMode == RMBMode::Zoom) {
        altRightClickZoom(dy); 
    }
}

void ModuleCameraEditor::updateWASD(float dt) {
    float speed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? moveSpeed * fastMultiplier : moveSpeed;

    Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
    Vector3 right = Vector3::Transform(Vector3::Right, camera.orientation);
    Vector3 up = Vector3::Up;

    Vector3 move = Vector3::Zero;
    if (GetAsyncKeyState('W') & 0x8000) move += forward;
    if (GetAsyncKeyState('S') & 0x8000) move -= forward;
    if (GetAsyncKeyState('D') & 0x8000) move += right;
    if (GetAsyncKeyState('A') & 0x8000) move -= right;
    if (GetAsyncKeyState('E') & 0x8000) move += up;
    if (GetAsyncKeyState('Q') & 0x8000) move -= up;

    move = safeNormalize(move);
    if (move != Vector3::Zero) {
        move.Normalize();
        camera.position += move * speed * dt;
        viewDirty = true;

        isFocused = false;
    }
}

void ModuleCameraEditor::applyMouseLook(float dx, float dy)
{
    float deltaYaw = -dx * lookSensitivity;
    float deltaPitch = -dy * lookSensitivity;

    yaw += deltaYaw;
    pitch += deltaPitch;
    pitch = std::clamp(pitch, -pitchLimit, pitchLimit);

    camera.orientation = Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
    camera.orientation.Normalize();

    viewDirty = true;
}


void ModuleCameraEditor::altRightClickZoom(float dy)
{
    Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
    forward.Normalize();

    float speed = zoomSensitivity;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= fastMultiplier;

    camera.position += forward * (dy * speed);
    viewDirty = true;
}

void ModuleCameraEditor::mouseWheelZoom(float wheel)
{
    float speed = zoomSensitivity;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) speed *= fastMultiplier;

    Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
    forward.Normalize();

    constexpr float wheelMultiplier = 12.0f;

    camera.position += forward * (wheel * speed * wheelMultiplier);
    viewDirty = true;
}

void ModuleCameraEditor::orbitDrag(float dx, float dy)
{
    orbitYaw += -dx * lookSensitivity;
    orbitPitch += -dy * lookSensitivity;
    orbitPitch = std::clamp(orbitPitch, -pitchLimit, pitchLimit);

    Vector3 forward;
    forward.x = -sinf(orbitYaw) * cosf(orbitPitch);
    forward.y = sinf(orbitPitch);
    forward.z = -cosf(orbitYaw) * cosf(orbitPitch);
    forward = safeNormalize(forward);

    camera.position = pivot - forward * orbitDistance;

    setOrientationFromForwardNoRoll(forward);
}



void ModuleCameraEditor::focusOnGeometry(const Vector3& pivotW)
{
    pivot = pivotW;
    isFocused = true;

    Vector3 camToPivot = pivot - camera.position;
    float dist = camToPivot.Length();

    if (dist < 0.001f)
    {
        Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
        forward = safeNormalize(forward);
        if (forward == Vector3::Zero)
            forward = Vector3::Forward;

        camToPivot = forward;
        dist = 10.0f;
    }
    else
    {
        camToPivot /= dist;
    }

    orbitDistance = std::max(2.0f, dist);

    camera.position = pivot - camToPivot * orbitDistance;

    Vector3 forward = pivot - camera.position;
    setOrientationFromForwardNoRoll(forward);

    Vector3 dir = safeNormalize(pivot - camera.position);
    orbitPitch = asinf(dir.y);
    orbitPitch = std::clamp(orbitPitch, -pitchLimit, pitchLimit);
    orbitYaw = atan2f(-dir.x, -dir.z);

    wasRmbDown = false;
    wasOrbitDown = false;
}

void ModuleCameraEditor::requestResize(uint32_t w, uint32_t h) {
    if (h == 0) h = 1;

    camera.aspect = float(w) / float(h);
    projDirty = true;
}

void ModuleCameraEditor::setFOV(float verticalFov)
{
    camera.fovY = std::clamp(verticalFov,
        DirectX::XMConvertToRadians(1.0f),
        DirectX::XMConvertToRadians(179.0f));

    projDirty = true;
}

void ModuleCameraEditor::setAspectRatio(float asp)
{
    camera.aspect = std::max(asp, 0.0001f);
    projDirty = true;
}

void ModuleCameraEditor::setPlaneDistances(float nearP, float farP)
{
    camera.nearPlane = std::max(nearP, 0.0001f);
    camera.farPlane = std::max(farP, camera.nearPlane + 0.0001f);
    projDirty = true;
}

void ModuleCameraEditor::setPosition(const Vector3& p)
{
    camera.position = p;
    viewDirty = true;
}

void ModuleCameraEditor::setOrientation(const Quaternion& q)
{
    camera.orientation = q;
    camera.orientation.Normalize();
    viewDirty = true;
}

void ModuleCameraEditor::setLookAt(const Vector3& target, const Vector3& worldUp)
{
    Vector3 direction = safeNormalize(target - camera.position);
    if (direction == Vector3::Zero) return;

    direction.Normalize();

    yaw = atan2f(-direction.x, -direction.z);
    pitch = asinf(direction.y);
    pitch = std::clamp(pitch, -pitchLimit, pitchLimit);

    camera.orientation = Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
    camera.orientation.Normalize();
    viewDirty = true;
}

Camera& ModuleCameraEditor::editCamera() {
    viewDirty = true;
    projDirty = true; 
    return camera;
}

const Camera& ModuleCameraEditor::readCamera() const {
    return camera;
}

const Matrix& ModuleCameraEditor::getViewMatrix()
{
    if (viewDirty) buildViewMatrix();
    return view;
}

const Matrix& ModuleCameraEditor::getProjectionMatrix()
{
    if (projDirty) buildProjectionMatrix();
    return proj;
}

void ModuleCameraEditor::buildViewMatrix() {
    Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
    Vector3 up = Vector3::Transform(Vector3::Up, camera.orientation);

    view = Matrix::CreateLookAt(camera.position, camera.position + forward, up);
    viewDirty = false;
}

void ModuleCameraEditor::buildProjectionMatrix() {
    proj = Matrix::CreatePerspectiveFieldOfView(camera.fovY, camera.aspect, camera.nearPlane, camera.farPlane);
    projDirty = false;
}

void ModuleCameraEditor::setOrientationFromForwardNoRoll(const Vector3& forward)
{
    Vector3 f = safeNormalize(forward);
    if (f == Vector3::Zero) return;

    yaw = atan2f(-f.x, -f.z);
    pitch = asinf(f.y);
    pitch = std::clamp(pitch, -pitchLimit, pitchLimit);

    camera.orientation = Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
    camera.orientation.Normalize();
    viewDirty = true;
}


