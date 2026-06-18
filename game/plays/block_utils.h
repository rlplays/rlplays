#pragma once
#include <serialize.h>

#include <actions.h>
#include <base_block.h>
#include <context.h>

namespace RLPlays
{
/** @brief
A simple animable struct to tween values/vectors from one to another per frame (@60fps).
Uses double-precision to (hopefully) guarantee idempotency.
*/
struct TStrength
{
  double Val = 0.0f;
  double Min = 0.0f;
  double Max = 1.0f;

  double FwdDiffPerFrame = 0.1f;
  double BckDiffPerFrame = 0.1f;
  Serializer(TStrength, Val, Min, Max, FwdDiffPerFrame, BckDiffPerFrame)

  // This works based on frames because we never change the FPS - it's set in the world at the top-level.
  // For faster FPS, we would simply run UpdateFrame at a higher frequency - that's it.
  // It keeps the logic simple and idempotent.
  double ApplyDiff(const double diffMultiplier)
  {
    Val += (diffMultiplier >= 0 ? (diffMultiplier * FwdDiffPerFrame) : (diffMultiplier * BckDiffPerFrame));
    return FinalVal();
  }

  double ResetTo(const double val)
  {
    if (std::abs(val - Val) <= std::min(FwdDiffPerFrame, BckDiffPerFrame)) { return (Val = val); }
    if (val < Val) { Val -= BckDiffPerFrame; }
    else { Val += FwdDiffPerFrame; }
    return FinalVal();
  }

  double FinalVal()
  {
    if (Val < Min) { Val = Min; }
    else if (Val > Max) { Val = Max; }
    return Val;
  }

  bool IsMax() const { return (Val >= Max); }
  bool IsMin() const { return (Val <= Min); }
};

/**
 * @brief A simple struct to move a block in a particular direction.
 * To initialize: TMoveSimple{Velocity: {x, y}, VelStrength{Val, Min, Max, Fwd, Bck}}
 */
struct TMoveSimple
{
  Vector2 Velocity = {}; // Distance to move per frame (@60fps). In World coords.
  TStrength VelocityStrength = {0, -1, 1};
  Serializer(TMoveSimple, Velocity, VelocityStrength)

  inline Vector2 Move(double multiplier)
  {
    VelocityStrength.ApplyDiff(multiplier);
    return FinalVelocity();
  }

  Vector2 FinalVelocity()
  {
    auto val = VelocityStrength.FinalVal();
    return {TO_FLT(Velocity.x * val), TO_FLT(Velocity.y * val)};
  }

  Vector2 ResetTo(const double val)
  {
    VelocityStrength.ResetTo(val);
    return FinalVelocity();
  }

  [[nodiscard]] bool IsZero() { return IsZeroVec(FinalVelocity()); }
};


struct TBlockUtils
{
  //! @brief Returns the rectangle for randomizing the player start position. Does not actually change the box position.
  static Rectangle RandomizePlayerStartPos(TContextPtr context);

  //! @brief Sets the player's starting position (use in conjunction with RandomizePlayerStartPos or other means).
  static void SetPlayerStartPos(TContextPtr context, const Rectangle& box);
};

//! @brief Produces a string with the current (local) time with the provided formatting
//!        {@param fmt} string.
inline std::string GetTimeStr(const char* fmt = "%Y_%m_%d_%H_%M")
{
  auto currTime = std::time(nullptr);
  struct tm timeinfo;
#ifdef _WIN32
  localtime_s(&timeinfo, &currTime);
#else
  localtime_r(&currTime, &timeinfo);
#endif
  char timeBuffer[32];
  std::strftime(timeBuffer, sizeof(timeBuffer), fmt, &timeinfo);
  return timeBuffer;
}
} // namespace RLPlays
