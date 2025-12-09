#include "Globals.h"
#include "Application.h"
#include "ModuleCameraEditor.h"
#include "Mouse.h"
#include <algorithm>
#include <math.h>

ModuleCameraEditor::ModuleCameraEditor() {
    setLookAt(Vector3::Zero, Vector3::Up);
    setFOV(DirectX::XMConvertToRadians(60.0f));


}

void ModuleCameraEditor::update()
{
    auto& io = ImGui::GetIO();

    if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
        rmbMode = RMBMode::None;
        wasRmbDown = false;
        return;
    }

    float dt = app->getElapsedMilis() * 0.001f;

    if (GetAsyncKeyState('F') & 0x0001)
        focusOnGeometry();

    float wheel = app->consumeMouseWheel();
    if (wheel != 0.0f)
        mouseWheelZoom(wheel);

    POINT cur{};
    GetCursorPos(&cur);

    const bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    const bool lmbDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    const bool rmbDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

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

        wasOrbitDown = false;

        const bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        rmbMode = altDown ? RMBMode::Zoom : RMBMode::Rotate;
        return;
    }

    const float dx = float(cur.x - lastMouse.x);
    const float dy = float(cur.y - lastMouse.y);
    lastMouse = cur;

    if (rmbMode == RMBMode::Rotate) {
        rightClickDrag(dx, dy);
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

    if (move.LengthSquared() > 1e-8f) {
        move.Normalize();
        camera.position += move * speed * dt;
        viewDirty = true;

        isFocused = false;
    }
}

void ModuleCameraEditor::rightClickDrag(float dx, float dy) {
    float deltaYaw = -dx * lookSensitivity;
    float deltaPitch = -dy * lookSensitivity;

    Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
    Vector3 up = Vector3::Transform(Vector3::Up, camera.orientation);

    forward.Normalize();
    up.Normalize();

    if (fabsf(deltaYaw) > 1e-6f) {
        Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Up, deltaYaw);
        forward = Vector3::Transform(forward, qYaw);
        up = Vector3::Transform(up, qYaw);
    }

    Vector3 right = forward.Cross(up);
    if (right.LengthSquared() < 1e-8f) {
        right = Vector3::Right;
    }
    right.Normalize();
    up = right.Cross(forward);
    up.Normalize();

    if (fabsf(deltaPitch) > 1e-6f) {
        Quaternion qPitch = Quaternion::CreateFromAxisAngle(right, deltaPitch);

        Vector3 newForward = Vector3::Transform(forward, qPitch);
        Vector3 newUp = Vector3::Transform(up, qPitch);

        newForward.Normalize();
        newUp.Normalize();

        float maxY = sinf(pitchLimit);
        if (fabsf(newForward.y) <= maxY) {
            forward = newForward;
            up = newUp;

            right = forward.Cross(up);
            if (right.LengthSquared() < 1e-8f) {
                right = Vector3::Right;
            }
            right.Normalize();
            up = right.Cross(forward);
            up.Normalize();
        }
    }

    Matrix worldM = Matrix::CreateWorld(Vector3::Zero, forward, up);
    camera.orientation = Quaternion::CreateFromRotationMatrix(worldM);
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
        float deltaYaw = -dx * lookSensitivity;
        float deltaPitch = -dy * lookSensitivity;

        Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
        Vector3 up = Vector3::Transform(Vector3::Up, camera.orientation);

        forward.Normalize();
        up.Normalize();

        if (fabsf(deltaYaw) > 1e-6f) {
            Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3::Up, deltaYaw);
            forward = Vector3::Transform(forward, qYaw);
            up = Vector3::Transform(up, qYaw);
        }

        Vector3 right = forward.Cross(up);
        if (right.LengthSquared() < 1e-8f) {
            right = Vector3::Right;
        }
        right.Normalize();
        up = right.Cross(forward);
        up.Normalize();

        if (fabsf(deltaPitch) > 1e-6f) {
            Quaternion qPitch = Quaternion::CreateFromAxisAngle(right, deltaPitch);

            Vector3 newForward = Vector3::Transform(forward, qPitch);
            Vector3 newUp = Vector3::Transform(up, qPitch);

            newForward.Normalize();
            newUp.Normalize();

            float maxY = sinf(pitchLimit);
            if (fabsf(newForward.y) <= maxY) {
                forward = newForward;
                up = newUp;

                right = forward.Cross(up);
                if (right.LengthSquared() < 1e-8f) {
                    right = Vector3::Right;
                }
                right.Normalize();
                up = right.Cross(forward);
                up.Normalize();
            }
        }

        Matrix worldM = Matrix::CreateWorld(Vector3::Zero, forward, up);
        camera.orientation = Quaternion::CreateFromRotationMatrix(worldM);
        camera.orientation.Normalize();

        forward = Vector3::Transform(Vector3::Forward, camera.orientation);
        forward.Normalize();
        camera.position = pivot - forward * orbitDistance;

        viewDirty = true;
    }


void ModuleCameraEditor::focusOnGeometry()
{
    pivot = Vector3::Zero;
    isFocused = true;

    Vector3 camToPivot = pivot - camera.position;
    float dist = camToPivot.Length();

    if (dist < 0.001f)
    {
        Vector3 forward = Vector3::Transform(Vector3::Forward, camera.orientation);
        forward.Normalize();
        camToPivot = forward;
        dist = 10.0f;
    }
    else
    {
        camToPivot /= dist;
    }

    orbitDistance = std::max(2.0f, dist);

    camera.position = pivot - camToPivot * orbitDistance;

    Matrix viewM = Matrix::CreateLookAt(camera.position, pivot, Vector3::Up);
    Matrix invView = viewM.Invert();
    camera.orientation = Quaternion::CreateFromRotationMatrix(invView);
    camera.orientation.Normalize();

    wasRmbDown = false;
    wasOrbitDown = false;

    viewDirty = true;
}



void ModuleCameraEditor::requestResize(uint32_t w, uint32_t h) {
    if (h == 0) h = 1;

    camera.aspect = float(w) / float(h);
    camera.fovY = calculateVerticalFovFromHorizontal(camera.fovX, camera.aspect);
    projDirty = true;
}

void ModuleCameraEditor::setFOV(float horizontalFov)
{
    horizontalFov = std::clamp(horizontalFov, DirectX::XMConvertToRadians(1.0f), DirectX::XMConvertToRadians(179.0f));

    camera.fovX = horizontalFov;
    camera.fovY = calculateVerticalFovFromHorizontal(camera.fovX, camera.aspect);
    projDirty = true;
}

void ModuleCameraEditor::setAspectRatio(float asp)
{
    camera.aspect = std::max(asp, 0.0001f);
    camera.fovY = calculateVerticalFovFromHorizontal(camera.fovX, camera.aspect);
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
    Vector3 dir = target - camera.position;
    if (dir.LengthSquared() < 1e-10f) return;

    dir.Normalize();

    yaw = atan2f(dir.x, -dir.z);
    pitch = asinf(dir.y);
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

float ModuleCameraEditor::calculateVerticalFovFromHorizontal(float fovX, float aspect)
{
    aspect = std::max(aspect, 0.0001f);
    return 2.0f * atanf(tanf(fovX * 0.5f) / aspect);
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