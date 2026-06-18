#pragma once
#include <serialize.h>

#include <base_block.h>
#include <context.h>

#include "block_behaviors.h"
#include "block_utils.h"

namespace RLPlays
{
using enum TBlockTraits;

struct TBrickBlock : ABlock
{
  TTexture Tex;
  SerializerWithBase(TBrickBlock, ABlock, Tex)
  void Init(TContextPtr context) override { Traits = Solid; }
  void LoadContent(TContextPtr context) override { context->LoadTexture(Tex); }
  void Draw(TContextPtr context) override { context->DrawTexture(Tex, Box, WHITE); }
};

//! @brief Moves from one place to another and changes direction if it hits anything solid; repeats.
struct TMoveableBlock : ABlock
{
  TTexture Tex;
  //! @brief Velocity/max distance to move (and return).
  TMoveSimple MoveVel = {{5, 0}};

  SerializerWithBase(TMoveableBlock, ABlock, Tex, MoveVel)

  void Init(TContextPtr context) override
  {
    Traits = Solid;
    forward_ = true;
  }

  void LoadContent(TContextPtr context) override { context->LoadTexture(Tex); }

  void Update(TContextPtr context) override
  {
    auto moveDelta = MoveVel.Move(forward_ ? 1 : -1);
    MoveRectBy(Box, moveDelta);
    if (CheckCollision_Backoff(this, context, moveDelta)) { forward_ = !forward_; }
    MoveBlockOnTopOfUs(this, context, moveDelta);
  }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    if (interact.Interact == TInteractionType::Hit) { forward_ = !forward_; }
  }

  void Draw(TContextPtr context) override { context->DrawTexture(Tex, Box, WHITE); }

#if RLPLAYS_EDITOR
  void EditorEnsureMirror(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
    const TBlock& thatBlock) override
  {
    // If the block is mirrored, we need to ensure that the move direction is also mirrored.
    MoveVel.Velocity.x = -MoveVel.Velocity.x;
  }
#endif

private:
  bool forward_ = true;
};

//! @brief Moves from one place to another and changes direction if it hits anything solid; repeats.
struct TActivatedMoveableBlock : ABlock
{
  TTexture Tex;
  //! @brief Velocity/max distance to move (and return).
  TMoveSimple MoveVel = {{5, 0}};

  //! @brief If true, the block moves once activated, moves/returns, and stops.
  //! Otherwise, will keep moving until deactivated.
  bool ActivateToMoveOnce = false;

  SerializerWithBase(TActivatedMoveableBlock, ABlock, Tex, MoveVel, ActivateToMoveOnce)

  bool shouldMove = true;

  void Init(TContextPtr context) override
  {
    Traits = Solid;
    forward_ = true;
    shouldMove = false;
    origBox_ = Box;
  }

  void LoadContent(TContextPtr context) override { context->LoadTexture(Tex); }

  void Update(TContextPtr context) override
  {
    if (!shouldMove) { return; }
    auto moveDelta = MoveVel.Move(forward_ ? 1 : -1);
    MoveRectBy(Box, moveDelta);
    if (CheckCollision_Backoff(this, context, moveDelta)) { forward_ = !forward_; }
    MoveBlockOnTopOfUs(this, context, moveDelta);
    if (ActivateToMoveOnce && shouldMove && !forward_)
    {
      if (Vector2Length(DiffRectPos(Box, origBox_)) <= Vector2Length(MoveVel.Velocity))
      {
        shouldMove = false;
        Box = origBox_;
        forward_ = true;
      }
    }
  }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    if (interact.Interact == TInteractionType::Hit) { forward_ = !forward_; }
    if (interact.Interact == TInteractionType::Activate && !shouldMove) { shouldMove = true; }

    if (optInteractResult != nullptr) { optInteractResult->hasStopped = !shouldMove; }
  }

  void Draw(TContextPtr context) override { context->DrawTexture(Tex, Box, WHITE); }

#if RLPLAYS_EDITOR
  void EditorEnsureMirror(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
    const TBlock& thatBlock) override
  {
    // If the block is mirrored, we need to ensure that the move direction is also mirrored.
    MoveVel.Velocity.x = -MoveVel.Velocity.x;
  }
#endif

private:
  Rectangle origBox_ = {};
  bool forward_ = true;
};


//! @brief Final goal block that completes the round.
struct TGoalBlock : ABlock
{
  TTexture Tex;
  SerializerWithBase(TGoalBlock, ABlock, Tex)
  void Init(TContextPtr context) override { Traits = GoalBlock; }
  void LoadContent(TContextPtr context) override { context->LoadTexture(Tex); }
  void Draw(TContextPtr context) override { context->DrawTexture(Tex, Box, WHITE); }
};

//! @brief Reward block that gives rewards to the player when they get it.
struct TRewardBlock : ABlock
{
  TTexture Tex;
  int NumRewards = 1;
  SerializerWithBase(TRewardBlock, ABlock, Tex)

  void Init(TContextPtr context) override
  {
    Traits = AddEnumValue(Collectible, PickableItem);
    context->UpdateGameProgress()->MaxNumRewards += NumRewards;
  }

  void LoadContent(TContextPtr context) override { context->LoadTexture(Tex); }
  void Draw(TContextPtr context) override { if (!IsA(ItemPicked)) { context->DrawTexture(Tex, Box, WHITE); } }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    if (interact.Interact == TInteractionType::PickedUp)
    {
      Traits = AddEnumValue(Traits, Cosmetic, ItemPicked);
      RunState = AddEnumValue(RunState, TRunState::Invisible, TRunState::Removed);
    }
  }

  int GetCollectibleReward() override { return NumRewards; }
};

struct TBackgroundBlock : ABlock
{
  TTexture Tex;
  SerializerWithBase(TBackgroundBlock, ABlock, Tex)

  void Init(TContextPtr context) override
  {
    Traits = Cosmetic;
    Depth = TLayerDepth::Background;
  }

  void LoadContent(TContextPtr context) override { context->LoadTexture(Tex); }

  // TODO(perumaal): Make this viewport coords fixed to the camera instead of using world coordinates.
  //       This way the camera can move around but the bg stays put.
  void Draw(TContextPtr context) override { context->DrawTexture(Tex, Box, WHITE); }

  bool IsCandidateForMirroring(TContextPtr context) const override
  {
    return Box.width < context->GetCamera().Viewport.width / 2;
  }
};
} // namespace RLPlays
