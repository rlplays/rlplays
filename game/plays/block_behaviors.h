#pragma once
#include "actions.h"
#include "block_utils.h"
#include "interactions.h"
#include <cfloat>

#include "raymath.h"
#include "scenes.h"

using namespace RLPlays;

namespace RLPlays
{
enum class TJumpState : uint8_t
{
  None      = 0,
  StartJump = 1,
  Jumping   = 2,
  Falling   = 3,
  Landed    = 4
};

using enum TBlockTraits;

// ***NOTE***
// In general, I don't like templates or fancy stuff, but here it helps avoid a whole lot of
// repeated code across different blocks. And helps us write simple code like HandleWalk etc
// while following some basic rules like having a FacingDir/Walk in the block etc.


//! @brief Handle Walking. Expects block to contain a TMoveSimple Walk and a FacingDir.
template <class T_Block>
inline void HandleWalk(T_Block* t, const RLPlays::TPlayerAction action, Vector2& moveVec, Vector2& FacingDir,
  TMoveSimple& Walk)
{
  if (HasEnumValue(action, TPlayerAction::WalkLeft))
  {
    moveVec = AddVector(moveVec, Walk.Move(-1));
    FacingDir.x = -1;
  }
  else if (HasEnumValue(action, TPlayerAction::WalkRight))
  {
    moveVec = AddVector(moveVec, Walk.Move(1));
    FacingDir.x = 1;
  }
  else if (HasEnumValue(action, TPlayerAction::None)) { moveVec = AddVector(moveVec, Walk.ResetTo(0)); }
}

template <class T_Block>
inline bool AvoidCollision(T_Block* t, TContextPtr context, Vector2& moveVec)
{
  auto& grid = context->Grid();
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  TGrid::AddDebugMoveVec(t->Box, moveVec);
#endif

  MoveRectBy(t->Box, moveVec);
  bool collided = false;
  grid->FindNeighbors(t, [&](TGridBlockInfo& neighbors) -> bool
  {
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
    TGrid::AddDebugMoveVec(t->Box, neighbors.CollisionBackoff);
#endif
    neighbors.ApplyCollisionBackoff(t);
    collided = true;
    return true;
  }, {
    .allowedTraits = Solid, .checkCollision = true, .moveDelta = moveVec
  });
  // NOTE: moveDelta requires moveVec to back off when there is a collision.
  // So only clear moveVec AFTER figuring out the collision moves from the grid.
  moveVec = {0, 0};
  return collided;
}

//! @brief Handle Jumping. Expects block to contain:
//! Requires: TMoveSimple Jump, TMoveSimple Gravity, TJumpState JumpState and WasStandingOnPlatform.
template <class T_Block>
inline void HandleJump_GravityFall(T_Block* t, TContextPtr context, const bool isJumping, Vector2& moveVec,
  TCountdownTimer& JumpTimer, TJumpState& JumpState, TMoveSimple& Jump,
  TMoveSimple& Gravity, bool& WasStandingOnPlatform, bool mustWaitForJumpAfterLanding)
{
  // I hate both the template magic and the constexpr crap here, but this is better than macros
  // and keeps the callers simple.
  auto& grid = context->Grid();

  AvoidCollision(t, context, moveVec);
  JumpTimer.TickTimerPerFrame(context);
  if (isJumping && JumpState == TJumpState::None && WasStandingOnPlatform)
  {
    JumpState = TJumpState::StartJump;
  }

  if (JumpState == TJumpState::StartJump)
  {
    JumpTimer.Start();
    JumpState = TJumpState::Jumping;
  }
  if (JumpState == TJumpState::Jumping)
  {
    moveVec = AddVector(moveVec, Jump.Move(1));
    if (!JumpTimer.IsRunning() || !isJumping)
    {
      JumpState = TJumpState::Falling;
    }
  }
  if (JumpState == TJumpState::None || JumpState == TJumpState::Falling)
  {
    moveVec = AddVector(moveVec, Jump.ResetTo(0));
  }
  AvoidCollision(t, context, moveVec);

  // Handle Gravity.
  if (!WasStandingOnPlatform)
  {
    if (JumpState == TJumpState::Falling || JumpState == TJumpState::None || JumpState ==
      TJumpState::Landed)
    {
      moveVec = AddVector(moveVec, Gravity.Move(1));

      AvoidCollision(t, context, moveVec);
    }
  }
  else
  {
    Gravity.ResetTo(0);
  }
  WasStandingOnPlatform = false;

#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  TGrid::AddDebugMoveVec(t->Box, moveVec);
#endif
  AvoidCollision(t, context, moveVec);

  // Check for platforms below us.
  grid->FindNeighbors(t, [&](TGridBlockInfo& neighbors) -> bool
  {
    if (IsRectOnPlatform(t->Box, neighbors.Neighbor->Box))
    {
      WasStandingOnPlatform = true;
      return false;
    }
    return true;
  }, {.expandRect = {0, 2}, .allowedTraits = Solid});

  if (JumpState == TJumpState::Falling && WasStandingOnPlatform)
  {
    JumpState = TJumpState::Landed;
    JumpTimer.Stop();
  }
  // Allow jump right after landing.
  if (JumpState == TJumpState::Landed)
  {
    if (!isJumping || !mustWaitForJumpAfterLanding)
    {
      JumpState = TJumpState::None;
    }
  }
}

//! @brief Handles enemy movement and prevents falling off a cliff;
//! {@returns true} if it collided.
template <class T_Block>
bool MoveOnSolidGround_AvoidCliff(T_Block* t, TContextPtr context, Vector2& moveDelta)
{
  auto& grid = context->Grid();
  bool shouldReverse = false;
  auto checkGroundBox = t->Box;

  if (!FloatIsZero(moveDelta.x))
  {
    auto move = std::abs(moveDelta.x);
    if (moveDelta.x > 0) { checkGroundBox.x += checkGroundBox.width - move; }
    else { checkGroundBox.x -= move; }
    checkGroundBox.width = move;
    checkGroundBox.y += 2;
    bool foundGround = false;
    grid->FindNeighbors(t, [&](TGridBlockInfo& neighbors) -> bool
    {
      if (!IsEmptyRect(GetCollisionRec(checkGroundBox, neighbors.Neighbor->Box)))
      {
        foundGround = true;
        return false;
      }
      return true;
    }, {.overrideRect = &checkGroundBox, .allowedTraits = Solid});

    if (!foundGround)
    {
      // If we are not on a platform, then we should reverse and set move delta = 0.
      moveDelta = {0, 0};
      shouldReverse = true;
    }
  }
  if (AvoidCollision(t, context, moveDelta)) { shouldReverse = true; }

  return shouldReverse;
}

//! @brief Handles enemy 'attack' & squashing if needed.
//! Requires: Damage, SquashDamage, AllowSquash.
//! {@returns true} if the enemy hit an obstacle.
template <class T_Block>
bool HandleEnemyAttack(T_Block* t, TContextPtr context, const bool AllowSquash, const bool OnlyAttackByJumping,
  const int SquashDamage, int& Damage, int& AttackCount)
{
  auto& grid = context->Grid();
  bool didCollide = false;
  auto checkGroundBox = t->Box;
  AttackCount = 0;
  grid->FindNeighbors(t, [&](TGridBlockInfo& neighbors) -> bool
  {
    const auto& collider = neighbors.Neighbor;
    // TODO: Move this to the player block not here. Each block has the ability to squash other blocks.
    if (AllowSquash && collider->IsA(Solid))
    {
      // If the other block is on top of us, squash us (only if it's solid, otherwise, let the other block
      // Interact with us naturally instead of us acting on their behalf).
      if (Bottom(collider->Box) <= t->Box.y + (float(t->Box.height) * 0.3f))
      {
        t->Interact(context, {TInteractionType::Squashed, {}, {}, SquashDamage}, *collider, nullptr);
        neighbors.CollisionBackoff = VEC_ZERO;
        return false;
      }
    }
    // We only attack 'the other group' (i.e. player/enemy distinction). Also within the same enemy group
    // we don't attack each other.
    if (collider->IsA(TakesDamage) && (collider->MaskTraits(EnemyGroups) != t->MaskTraits(EnemyGroups)))
    {
      bool attack = true;
      if (OnlyAttackByJumping)
      {
        attack = false;
        // Only attack if we are jumping/falling and are above the other block.
        if (Bottom(t->Box) <= collider->Box.y + (float(collider->Box.height) * 0.4f))
        {
          attack = true;
        }
      }
      else
      {
        // Otherwise, check if we might get squashed by the opponent, don't attack in that case.
        if (Bottom(collider->Box) <= t->Box.y + (float(t->Box.height) * 0.5f))
        {
          attack = false;
        }
      }
      if (attack)
      {
        AttackCount++;
        TInteraction interact = {TInteractionType::Damage};
        interact.Damage = Damage;
        collider->Interact(context, interact, *t, nullptr);
        didCollide = true;
        return true;
      }
    }
    if (collider->IsA(Solid))
    {
      didCollide = true;
      neighbors.ApplyCollisionBackoff(t);
      return true;
    }
    return true;
  }, {
    .allowedTraits = AddEnumValue(Solid, TakesDamage), .checkCollision = true
  });

  return didCollide;
}

//! @brief Handles an enemy's damage from another block.
//! Requires Health, SquashDamage (and uses RunState).
template <class T_Block>
void HandleEnemyDamage(T_Block* t, TContextPtr context, const TInteraction& interact,
  TInteractionResult* optInteractResult)
{
  if (interact.Interact == TInteractionType::Squashed) { t->Health -= t->SquashDamage; }
  if (interact.Interact == TInteractionType::Damage) { t->Health -= interact.Damage; }
  // TODO(perumaal): Have a timer to avoid multiple hits in quick succession?
  if (t->Health <= 0)
  {
    // t->RunState = AddEnumValue(t->RunState, TRunState::Invisible);
    t->Traits = AddEnumValue(t->Traits, TBlockTraits::Cosmetic);
    if (optInteractResult != nullptr) { optInteractResult->wasHandled = true; }
  }
}

//! @brief Handles being moved / pushed by another block.
template <class T_Block>
void HandlePush(T_Block* t, TContextPtr context, const TInteraction& interact,
  TInteractionResult* optInteractResult)
{
  if (interact.Interact == TInteractionType::Push)
  {
    context->MoveBlockBy(t, interact.Offset);
  }
}

// Check if there is a moveable block on top of our block before we moved.
template <class T_Block>
void MoveBlockOnTopOfUs(T_Block* t, TContextPtr context, Vector2& moveDelta)
{
  auto box = OffsetRect(t->Box, {0, -1});
  context->Grid()->FindNeighbors(t, [&](const TGridBlockInfo& neighbors) -> bool
  {
    const auto collider = neighbors.Neighbor;
    if (IsRectOnPlatform(collider->Box, t->Box))
    {
      // Move "that" block with "us".
      // context->MoveBlockBy(collider, moveDelta);
      collider->Interact(context, {.Interact = TInteractionType::Push, .Offset = moveDelta}, *t, nullptr);
    }
    return true;
  }, {.overrideRect = &box, .allowedTraits = Moveable});
}

template <class T_Block>
bool CheckCollision_Backoff(T_Block* t, TContextPtr context, Vector2& moveDelta)
{
  bool didCollide = false;
  context->Grid()->FindNeighbors(t, [&](TGridBlockInfo& neighbors) -> bool
  {
    neighbors.Neighbor->Interact(context, {TInteractionType::Hit}, *t, nullptr);
    // Don't move our actual box just yet as we need to figure out other things and move other blocks too.
    neighbors.ApplyCollisionBackoff(t);
    didCollide = true;
    return true;
  }, {.allowedTraits = Solid, .checkCollision = true, .moveDelta = moveDelta});
  return didCollide;
}

struct TNearestBlocks
{
  Rectangle SearchRect = {};
  Vector2 NearestPos = INVALID_POS;
  ABlock* NearestBlock = nullptr;
  float NearestDistSq = -1;
};

template <class T_Block>
TNearestBlocks CheckForBlocks(T_Block* t, TContextPtr context, const TBlockTraits traits, const Vector2& searchExpand)
{
  TNearestBlocks result;
  result.NearestDistSq = FLT_MAX;
  auto blockCenter = RectCenter(t->Box);
  result.SearchRect = ExpandRect(t->Box, searchExpand);
  context->Grid()->FindNeighbors(t, [&](const TGridBlockInfo& neighbors) -> bool
  {
    const auto neighbor = neighbors.Neighbor;
    auto d = Vector2DistanceSqr(RectCenter(neighbor->Box), blockCenter);
    if (d < result.NearestDistSq)
    {
      result.NearestDistSq = d;
      result.NearestPos = RectCenter(neighbor->Box);
      result.NearestBlock = neighbor;
    }
    return true;
  }, {.overrideRect = &result.SearchRect, .allowedTraits = traits});
  return result;
}

// Written with help from Claude 4.5.
inline void DrawExplosionParticles(const Rectangle& box, const float animProgress, float particleSize)
{
  // Get center of the box
  const Vector2 center = RectCenter(box);

  // Number of particles to draw
  constexpr int numParticles = 30;

  // Create explosion particles radiating from center
  for (int i = 0; i < numParticles; i++)
  {
    // Calculate angle for this particle (evenly distributed in circle)
    const float angle = rand() % 360; // 360.0f / numParticles) * i;

    // Distance from center increases with animation progress
    const float distance = animProgress * (box.width + box.height) * 0.5f;

    // Calculate particle position
    const Vector2 particlePos = {
      center.x + cos(angle * DEG2RAD) * distance,
      center.y + sin(angle * DEG2RAD) * distance
    };

    // Randomize particle colors (fire/explosion colors)
    Color particleColor;
    switch (i % 4)
    {
    case 0: particleColor = ORANGE;
      break;
    case 1: particleColor = RED;
      break;
    case 2: particleColor = YELLOW;
      break;
    default: particleColor = {255, 100, 0, 255};
      break; // Bright orange
    }

    // Fade out as animation progresses
    const float alpha = (1.0f - animProgress) * 0.9f;
    particleColor = Fade(particleColor, alpha);

    // Particle size decreases with animation progress
    particleSize = (1.0f - animProgress * 0.5f) * particleSize;

    // Draw particle as circle
    DrawCircleV(particlePos, particleSize, particleColor);
  }

  // Add flash effect at the start
  if (animProgress < 0.3f)
  {
    const float flashAlpha = (0.3f - animProgress) / 0.3f;
    DrawCircleV(center, box.width * 0.7f, Fade(WHITE, flashAlpha * 0.7f));
  }
}

template <class T_Block>
inline bool RunExplodeBlockAnim(T_Block* t, TContextPtr context, TScenes& scenes, Rectangle& finalBox, float& finalFade,
  const int animTimeMs = 500_ms, const float explosionScale = 4.0f, const float particleSize = 8.0f)
{
  finalFade = 0;
  if (scenes.BeginScenes(context))
  {
    if (scenes.RunScene(animTimeMs))
    {
      const auto s = (float)scenes.ReversePercentTime();
      finalBox = ScaleRect(finalBox, s + 0.25f);
      finalFade = s;
      DrawExplosionParticles(ScaleRect(t->Box, explosionScale), 1 - s, particleSize);
    }
    scenes.EndScenes();
    return true;
  }
  return false;
}
} // namespace RLPlays
