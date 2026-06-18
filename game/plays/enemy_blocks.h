#pragma once
#include <serialize.h>

#include <base_block.h>
#include <context.h>
#include <block_behaviors.h>

using enum RLPlays::TBlockTraits;

namespace RLPlays
{
//! @brief Moves from one place to another and changes direction if it hits anything solid; repeats.
struct TMovingEnemyBlock final : ABlock
{
  TTexture Tex;
  TSpriteSheet Sprite;

  //! @brief Velocity/max distance to move (and return).
  TMoveSimple MoveVel = {{5, 0}};
  int Damage = 1;
  int Health = 1;
  int SquashDamage = 1;
  bool AllowSquash = true;
  SerializerWithBase(TMovingEnemyBlock, ABlock, Tex, MoveVel, Damage, Health, AllowSquash, SquashDamage, Sprite)

  bool Forward = true;
  TScenes killedAnim_;

  void Init(TContextPtr context) override
  {
    Traits = AddEnumValue(CauseDamage, TakesDamage, Moveable, Pushable, EnemyGroup1);
    Forward = true;
    context->UpdateGameProgress()->MaxNumEnemies++;
  }

  void LoadContent(TContextPtr context) override
  {
    if (Sprite.Empty()) { context->LoadTexture(Tex); }
    else { context->LoadSpriteSheet(Sprite); }
  }

  void Update(TContextPtr context) override
  {
    if (Health <= 0) { return; }
    // IDEA(perumaal): This could come from an RL policy too - and the rest of the logic would remain as is
    //                 to ensure physics/world constrain it to valid moves. Similar in spirit to the player block.
    auto moveDelta = MoveVel.Move(Forward ? 1 : -1);

    auto didCollide = MoveOnSolidGround_AvoidCliff(this, context, moveDelta);
    int attacked = 0;

    didCollide |= HandleEnemyAttack(this, context, {AllowSquash}, /*only attack by jumping*/ false, {SquashDamage},
      Damage, attacked);
    if (didCollide) { Forward = !Forward; }
  }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    HandlePush(this, context, interact, optInteractResult);
    HandleEnemyDamage(this, context, interact, optInteractResult);
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
    if (Sprite.Empty())
    {
      context->DrawTexture(Tex, MoveVel.FinalVelocity().x >= 0 ? box : RectXInvert(box), Fade(WHITE, fade));
    }
    else
    {
      context->DrawAnimSprite(Sprite, MoveVel.FinalVelocity().x >= 0 ? box : RectXInvert(box), Fade(WHITE, fade),
        -1, 0, {0.0f, 0.0f});
    }
  }

#if RLPLAYS_EDITOR
  void EditorEnsureMirror(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
    const TBlock& thatBlock) override
  {
    // If the block is mirrored, we need to ensure that the move direction is also mirrored.
    MoveVel.Velocity.x = -MoveVel.Velocity.x;
  }
#endif
};

struct TJumpingEnemyBlock final : ABlock
{
  TSpriteSheet Sprite;

  //! @brief Velocity/max distance to move (and return).
  TMoveSimple MoveVel = {{5, 0}};
  TMoveSimple Jump = {{0, -25}, {0, 0, 1}};
  TMoveSimple Gravity = {{0, 19}, {0, 0, 1}};
  TCountdownTimer JumpTimer = {NanosFromSeconds(.081f)};
  Vector2 FacingDir = {1, 0};
  float ChargeSpeedup = 2.1f;
  int Health = 1;
  int SquashDamage = 1;
  bool AllowSquash = true;
  int Damage = 1;
  Vector2 SearchAreaCells = {3, 1};
  Rectangle focus_ = {};
  TNearestBlocks nearest_ = {};
  Vector2 cellSize_, searchArea_;
  TJumpState JumpState = TJumpState::None;
  bool WasStandingOnPlatform = false;
  TScenes killedAnim_;
  int JumpStaggerMs = 300_ms;
  int JumpStaggerDelayMs = 500_ms;

  enum class TJumpEnemyState { Enemy_Checking_Left, Enemy_Checking_Right, Enemy_Attack } state_ =
      TJumpEnemyState::Enemy_Checking_Left;

  SerializerWithBase(TJumpingEnemyBlock, ABlock, Sprite, MoveVel, Jump, Gravity, JumpTimer,
    FacingDir, Damage, Health, AllowSquash, SquashDamage, SearchAreaCells, ChargeSpeedup,
    JumpStaggerMs, JumpStaggerDelayMs)
  TScenes jumpStagger_;

  void Init(TContextPtr context) override
  {
    Traits = AddEnumValue(CauseDamage, TakesDamage, Moveable, Pushable, EnemyGroup1);
    state_ = TJumpEnemyState::Enemy_Checking_Left;
    cellSize_ = context->GetCellSize();
    searchArea_ = {cellSize_.x * 3, cellSize_.y * 1};
    focus_ = ExpandRect(Box, searchArea_);
    context->UpdateGameProgress()->MaxNumEnemies++;
  }

  void LoadContent(TContextPtr context) override { context->LoadSpriteSheet(Sprite); }

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
    context->DrawAnimSprite(Sprite, FacingDir.x <= 0 ? box : RectXInvert(box), Fade(WHITE, fade),
      -1, 0, {0.0f, 0.0f});
  }


  void Update(TContextPtr context) override
  {
    if (Health <= 0) { return; }
    // IDEA(perumaal): This could come from an RL policy too - and the rest of the logic would remain as is
    //                 to ensure physics/world constrain it to valid moves. Similar in spirit to the player block.
    nearest_ = CheckForBlocks(this, context, AddEnumValue(MainPlayerBlock, OtherPlayerBlock), searchArea_);
    if (Box.x <= focus_.x) { state_ = TJumpEnemyState::Enemy_Checking_Right; }
    else if (RectRight(Box) >= RectRight(focus_)) { state_ = TJumpEnemyState::Enemy_Checking_Left; }
    Vector2 moveDelta = {0, 0};
    bool jumping = false;
    if (nearest_.NearestBlock != nullptr)
    {
      state_ = TJumpEnemyState::Enemy_Attack;
      FacingDir.x = ((nearest_.NearestPos.x > RectCenter(Box).x) ? 1 : -1) * std::abs(FacingDir.x);
      moveDelta = Vector2Scale(MoveVel.Move((FacingDir.x >= 0 ? 1 : -1)), ChargeSpeedup);
      jumping = true;
    }
    else
    {
      if (state_ == TJumpEnemyState::Enemy_Checking_Left) { FacingDir.x = -std::abs(FacingDir.x); }
      if (state_ == TJumpEnemyState::Enemy_Checking_Right) { FacingDir.x = std::abs(FacingDir.x); }
      moveDelta = MoveVel.Move((FacingDir.x >= 0 ? 1 : -1));
      if (RectBottom(focus_) < RectBottom(Box) - cellSize_.y) { jumping = true; }
    }
    if (jumping)
    {
      if (jumpStagger_.BeginScenes(context, true))
      {
        if (jumpStagger_.RunScene(JumpStaggerMs)) { jumping = true; }
        if (jumpStagger_.RunScene(JumpStaggerDelayMs)) { jumping = false; }
        jumpStagger_.EndScenes();
      }
    }
    HandleJump_GravityFall(this, context, jumping, moveDelta, JumpTimer, JumpState, Jump,
      Gravity, WasStandingOnPlatform, /*mustWaitForJumpAfterLanding*/ false);
    auto didCollide = MoveOnSolidGround_AvoidCliff(this, context, moveDelta);
    int attacked = 0;
    didCollide |= HandleEnemyAttack(this, context, AllowSquash, /*only attack by jumping*/ false, SquashDamage,
      Damage, attacked);
    if (didCollide)
    {
      if (state_ == TJumpEnemyState::Enemy_Checking_Left) { state_ = TJumpEnemyState::Enemy_Checking_Right; }
      else if (state_ == TJumpEnemyState::Enemy_Checking_Right) { state_ = TJumpEnemyState::Enemy_Checking_Left; }
    }
  }

#if DEBUG
  void DrawDebug(TContextPtr context) override
  {
    DrawRectangleRec(focus_, Fade(YELLOW, 0.25f));
    if (nearest_.NearestBlock != nullptr)
    {
      DrawRectangleRec(nearest_.SearchRect, Fade(YELLOW, 0.5f));
      DrawLineV(nearest_.NearestPos, RectCenter(Box), Fade(BLUE, 0.75f));
    }
  }
#endif

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    HandlePush(this, context, interact, optInteractResult);
    HandleEnemyDamage(this, context, interact, optInteractResult);
  }
};
}
