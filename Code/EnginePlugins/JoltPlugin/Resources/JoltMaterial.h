#pragma once

#include <Core/Physics/SurfaceResource.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>

class WJoltMaterial : public JPH::PhysicsMaterial
{
public:
  WJoltMaterial();
  ~WJoltMaterial();

  WSurfaceResource* m_pSurface = nullptr;

  float m_fRestitution = 0.0f;
  float m_fFriction = 0.2f;
  WColorGammaUB m_DebugColor = WColor::White;
};
