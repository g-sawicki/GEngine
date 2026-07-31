#pragma once

namespace GEngine {

struct PerspectiveDesc {
    float FovDegrees;
    float AspectRatio;
    float NearZ;
    float FarZ;
};

class Camera {
  public:
    Camera(const PerspectiveDesc& desc, const DirectX::XMFLOAT3& position = {});

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    void SetPosition(const DirectX::XMFLOAT3& position);
    void SetRotation(float pitch, float yaw);

    /// Move in local space: forward, right, up.
    void Translate(float forwardDelta, float rightDelta, float upDelta);

    void Rotate(float pitchDelta, float yawDelta);

    void SetAspectRatio(float aspectRatio);
    void SetFov(float fovDegrees);
    void SetNearZ(float nearZ);
    void SetFarZ(float farZ);

    [[nodiscard]] DirectX::XMMATRIX GetViewMatrix() const;
    [[nodiscard]] DirectX::XMMATRIX GetProjectionMatrix() const;
    [[nodiscard]] DirectX::XMMATRIX GetViewProjectionMatrix() const;
    [[nodiscard]] const DirectX::XMFLOAT3& GetPosition() const noexcept { return m_Position; }
    [[nodiscard]] float GetPitch() const noexcept { return m_Pitch; }
    [[nodiscard]] float GetYaw() const noexcept { return m_Yaw; }
    [[nodiscard]] float GetFov() const noexcept { return m_Fov; }
    [[nodiscard]] float GetAspectRatio() const noexcept { return m_AspectRatio; }
    [[nodiscard]] float GetNearZ() const noexcept { return m_NearZ; }
    [[nodiscard]] float GetFarZ() const noexcept { return m_FarZ; }
    [[nodiscard]] DirectX::XMFLOAT3 GetForward() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetRight() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetUp() const noexcept;

  private:
    void ComputeLocalAxes(DirectX::XMVECTOR& outForward, DirectX::XMVECTOR& outRight, DirectX::XMVECTOR& outUp) const;
    void UpdateViewMatrix() const;
    void UpdateProjectionMatrix() const;

    mutable DirectX::XMFLOAT4X4 m_ViewMatrix{};
    mutable DirectX::XMFLOAT4X4 m_ProjectionMatrix{};

    DirectX::XMFLOAT3 m_Position{};
    float m_Pitch{};
    float m_Yaw{};

    float m_Fov{};
    float m_AspectRatio{};
    float m_NearZ{};
    float m_FarZ{};

    mutable bool m_ViewDirty{true};
    mutable bool m_ProjectionDirty{true};
};

} // namespace GEngine
