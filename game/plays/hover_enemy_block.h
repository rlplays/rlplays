#pragma once
#include <serialize.h>


#include <actions.h>
#include <base_block.h>
#include <block_utils.h>
#include <context.h>
#include <game_types.h>
#include <iomanip>
#include <sstream>
#include <timer.h>
#include "game_progress.h"
#include "ghost_player.h"

using enum RLPlays::TBlockTraits;

namespace RLPlays
{

enum class THoverEnemyState : uint8_t
{
  Hover,    // Idling / patrolling in the air
  Seek,     // Detected a player below, gently following
  Hit,      // Attacking the player (within half a block)
  Flee,     // Move away from the player after hitting, then return to Seek
  Damaged,  // Just took damage, brief stun
  Dead      // Health depleted
};

//! @brief A hovering enemy that floats in the air avoiding solid objects.
//! If it senses a player below, it gently follows; if close enough, it attacks.
//! Takes 2 hits to kill.
struct THoverEnemyBlock final : ABlock
{
  // One sprite per visual state.
  TSpriteSheet HoverSprite;
  TSpriteSheet SeekSprite;
  TSpriteSheet HitSprite;
  TSpriteSheet DamagedSprite;
  TSpriteSheet DeadSprite;

  //! @brief Horizontal patrol velocity.
  TMoveSimple HoverVel = {{2, 0}, {0, -1, 1, 10000, 10000}};

  //! @brief How fast it follows the player (seek mode).
  float SeekSpeed = 4.5f;

  //! @brief How many cells (in each direction) to scan for a player below.
  Vector2 SearchAreaCells = {3, 4};

  //! @brief How far (in cells) the enemy is allowed to stray from its origin before returning.
  float MaxDriftCells = 5.0f;

  int Damage = 1;
  int Health = 1;
  int SquashDamage = 1;
  bool AllowSquash = true;

  Vector2 FacingDir = {1, 0};

  SerializerWithBase(THoverEnemyBlock, ABlock, HoverSprite, SeekSprite, HitSprite, DamagedSprite, DeadSprite,
      HoverVel, SeekSpeed, SearchAreaCells, MaxDriftCells, Damage, Health, AllowSquash, SquashDamage, FacingDir)

  // --- Runtime (non-serialized) state ---
  THoverEnemyState state_ = THoverEnemyState::Hover;
  bool patrolForward_ = true;
  Vector2 origin_ = INVALID_POS;
  Vector2 cellSize_ = {};
  Vector2 searchArea_ = {};
  TNearestBlocks nearest_ = {};
  TCountdownTimer damagedTimer_ = {NanosFromMillis(400)};
  TCountdownTimer hitCooldown_ = {NanosFromMillis(600)};
  TCountdownTimer fleeTimer_ = {NanosFromMillis(700)};
  Vector2 fleeDir_ = {};
  TScenes killedAnim_;

  // Gentle vertical bob accumulators
  float bobPhase_ = 0.0f;

  void Init(TContextPtr context) override
  {
    Traits = AddEnumValue(CauseDamage, TakesDamage, Moveable, Pushable, EnemyGroup1);
    state_ = THoverEnemyState::Hover;
    patrolForward_ = true;
    origin_ = RectCenter(Box);
    cellSize_ = context->GetCellSize();
    searchArea_ = {cellSize_.x * SearchAreaCells.x, cellSize_.y * SearchAreaCells.y};
    context->UpdateGameProgress()->MaxNumEnemies++;
  }

  void LoadContent(TContextPtr context) override
  {
    context->LoadSpriteSheet(HoverSprite);
    context->LoadSpriteSheet(SeekSprite);
    context->LoadSpriteSheet(HitSprite);
    context->LoadSpriteSheet(DamagedSprite);
    context->LoadSpriteSheet(DeadSprite);
  }

  void Update(TContextPtr context) override
  {
    if (Health <= 0)
    {
      state_ = THoverEnemyState::Dead;
      return;
    }

    damagedTimer_.TickTimerPerFrame(context);
    hitCooldown_.TickTimerPerFrame(context);
    fleeTimer_.TickTimerPerFrame(context);

    // If we just took damage, stay in Damaged state until the timer runs out.
    if (state_ == THoverEnemyState::Damaged)
    {
      if (damagedTimer_.IsRunning())
      {
        return; // Don't move while stunned.
      }
      state_ = THoverEnemyState::Hover;
    }

    // If fleeing, move away from the player until the flee timer expires, then seek again.
    if (state_ == THoverEnemyState::Flee)
    {
      if (fleeTimer_.IsRunning())
      {
        MoveRectBy(Box, Vector2Scale(fleeDir_, SeekSpeed));
        auto& grid = context->Grid();
        grid->FindNeighbors(this,
            [&](TGridBlockInfo& neighbors) -> bool
            {
              neighbors.ApplyCollisionBackoff(this);
              return true;
            },
            {.allowedTraits = Solid, .checkCollision = true, .moveDelta = Vector2Scale(fleeDir_, SeekSpeed)});
        return;
      }
      state_ = THoverEnemyState::Seek;
    }

    // Look for a player block below/around us.
    nearest_ = CheckForBlocks(this, context, AddEnumValue(MainPlayerBlock, OtherPlayerBlock), searchArea_);

    bool playerBelow = false;
    if (nearest_.NearestBlock != nullptr)
    {
      // Only consider the player if they are roughly below us (player center.y > our center.y).
      auto myCenter = RectCenter(Box);
      if (nearest_.NearestPos.y > myCenter.y)
      {
        playerBelow = true;
      }
    }

    Vector2 moveDelta = {0, 0};
    auto myCenter = RectCenter(Box);

    if (playerBelow)
    {
      // Check distance from origin - if too far, go back to hovering.
      float driftFromOrigin = Vector2Distance(myCenter, origin_);
      if (driftFromOrigin > MaxDriftCells * cellSize_.x)
      {
        // Too far from home, return to hover near origin.
        state_ = THoverEnemyState::Hover;
        auto dirToOrigin = Vector2Subtract(origin_, myCenter);
        if (!IsZeroVec(dirToOrigin))
        {
          dirToOrigin = Vector2Normalize(dirToOrigin);
          moveDelta = Vector2Scale(dirToOrigin, SeekSpeed);
        }
      }
      else
      {
        // Gently move towards the player.
        auto dirToPlayer = Vector2Subtract(nearest_.NearestPos, myCenter);
        float distToPlayer = Vector2Length(dirToPlayer);

        // Half a block distance for attacking.
        float halfBlock = cellSize_.x * 0.05f;
        if (distToPlayer <= halfBlock && !hitCooldown_.IsRunning())
        {
          state_ = THoverEnemyState::Hit;
          // Call Interact on the player block directly.
          TInteraction interact = {TInteractionType::Damage};
          interact.Damage = Damage;
          nearest_.NearestBlock->Interact(context, interact, *this, nullptr);
          hitCooldown_.Start();
          // Flee away from the player.
          fleeDir_ = (distToPlayer > 0.0f)
                         ? Vector2Negate(Vector2Normalize(dirToPlayer))
                         : Vector2{-FacingDir.x, -0.5f};
          fleeTimer_.Start();
          state_ = THoverEnemyState::Flee;
          return; // Don't apply movement toward the player on the hit frame.
        }
        else
        {
          state_ = THoverEnemyState::Seek;
        }

        FacingDir.x = (dirToPlayer.x >= 0) ? 1.0f : -1.0f;
        if (!IsZeroVec(dirToPlayer))
        {
          dirToPlayer = Vector2Normalize(dirToPlayer);
          moveDelta = Vector2Scale(dirToPlayer, SeekSpeed);
        }
      }
    }
    else
    {
      state_ = THoverEnemyState::Hover;
      // Simple horizontal patrol with a gentle vertical bob.
      moveDelta = HoverVel.Move(patrolForward_ ? 1 : -1);

      // Gentle vertical bobbing.
      bobPhase_ += 0.05f;
      moveDelta.y += sinf(bobPhase_) * 0.5f;
    }

    // Avoid solid objects.
    MoveRectBy(Box, moveDelta);
    bool collided = false;
    auto& grid = context->Grid();
    grid->FindNeighbors(this,
        [&](TGridBlockInfo& neighbors) -> bool
        {
          neighbors.ApplyCollisionBackoff(this);
          collided = true;
          return true;
        },
        {.allowedTraits = Solid, .checkCollision = true, .moveDelta = moveDelta});

    if (collided && state_ == THoverEnemyState::Hover)
    {
      patrolForward_ = !patrolForward_;
    }

    // Handle attacking (fallback via collision for cases not caught by distance check).
    if (state_ == THoverEnemyState::Hit)
    {
      int attacked = 0;
      HandleEnemyAttack(this, context, AllowSquash, /*only attack by jumping*/ false, SquashDamage, Damage, attacked);
      if (attacked > 0)
      {
        hitCooldown_.Start();
        auto myPos = RectCenter(Box);
        fleeDir_ = (nearest_.NearestBlock != nullptr)
                       ? Vector2Normalize(Vector2Subtract(myPos, nearest_.NearestPos))
                       : Vector2{-FacingDir.x, -0.5f};
        fleeTimer_.Start();
        state_ = THoverEnemyState::Flee;
      }
    }
  }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
      TInteractionResult* optInteractResult) override
  {
    HandlePush(this, context, interact, optInteractResult);

    if (interact.Interact == TInteractionType::Squashed || interact.Interact == TInteractionType::Damage)
    {
      HandleEnemyDamage(this, context, interact, optInteractResult);
      if (Health > 0)
      {
        state_ = THoverEnemyState::Damaged;
        damagedTimer_.Start();
      }
      else
      {
        state_ = THoverEnemyState::Dead;
      }
    }
  }

  TSpriteSheet& GetCurrentSprite_()
  {
    switch (state_)
    {
    case THoverEnemyState::Seek: return SeekSprite;
    case THoverEnemyState::Hit: return HitSprite;
    case THoverEnemyState::Flee: return HitSprite; // Reuse hit sprite while fleeing.
    case THoverEnemyState::Damaged: return DamagedSprite;
    case THoverEnemyState::Dead: return DeadSprite;
    default: return HoverSprite;
    }
  }

  void Draw(TContextPtr context) override
  {
    auto box = Box;
    auto fade = 1.0f;

    if (IsA(TBlockTraits::Cosmetic))
    {
      if (!RunExplodeBlockAnim(this, context, killedAnim_, box, fade))
      {
        RunState = AddEnumValue(RunState, TRunState::Removed);
        return;
      }
    }

    auto color = Fade(WHITE, fade);
    if (state_ == THoverEnemyState::Damaged)
    {
      // Flash red when damaged.
      color = Fade(RED, fade);
    }

    auto& sprite = GetCurrentSprite_();
    context->DrawAnimSprite(sprite, FacingDir.x >= 0 ? box : RectXInvert(box), color, -1, 0, {0.0f, 0.0f});
  }

#if DEBUG
  void DrawDebug(TContextPtr context) override
  {
    // Draw the search area.
    auto searchRect = ExpandRect(Box, searchArea_);
    DrawRectangleRec(searchRect, Fade(PURPLE, 0.15f));
    // Draw origin marker.
    DrawCircleV(origin_, 4.0f, Fade(YELLOW, 0.5f));
    if (nearest_.NearestBlock != nullptr)
    {
      DrawLineV(nearest_.NearestPos, RectCenter(Box), Fade(RED, 0.75f));
    }
  }
#endif

#if RLPLAYS_EDITOR
  void EditorEnsureMirror(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
      const TBlock& thatBlock) override
  {
    HoverVel.Velocity.x = -HoverVel.Velocity.x;
  }
#endif
};
} // namespace RLPlays
