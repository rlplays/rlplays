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

#include "block_behaviors.h"
#include "scenes.h"

#include "ghost_player.h"

#if RLPLAYS_EDITOR && DEBUG
#define SHOW_GHOST 1
#else
#undef SHOW_GHOST
#endif

namespace RLPlays
{
struct TPlayerBlock final : ABlock
{
  TSpriteSheet PlayerSprite;
  TSpriteSheet PriorSprite;
  TSpriteSheet AISprite;
  TMoveSimple Walk = {{10, 0}};
  TMoveSimple Jump = {{0, -25}, {0, 0, 1}};
  TCountdownTimer JumpTimer = {NanosFromSeconds(.21f)};
  TJumpState JumpState = TJumpState::None;
  TMoveSimple Gravity = {{0, 30}, {0, 0, 1}};
  Vector2 FacingDir = {1, 0};
  int MaxHearts = 1;

  //! @brief Item(s) used by the player such as weapons, bombs etc.
  std::shared_ptr<TBlock> Item1;

  int PlayerId = PlayerId1;
  int HeartsLeft = 0;
  int Damage = 1;
  int SquashDamage = 1;
  int NumEnemies = 0;

  bool WasStandingOnPlatform = false;

  SerializerWithBase(TPlayerBlock, ABlock, PlayerSprite, PriorSprite, Walk, Jump, Gravity, JumpTimer, PlayerId,
      FacingDir, MaxHearts, Item1);

  void InteractItem_(TContextPtr context, const TInteractionType interactType)
  {
    if (Item1 == nullptr)
    {
      return;
    }
    GetABlock(Item1)->Interact(context, {interactType, RectTopLeft(Box), FacingDir}, *this, nullptr);
  }

  void Init(TContextPtr context) override
  {
    const auto* progress = context->GetGameProgress();
    if (progress->GetActivePlayerId() == PlayerId)
    {
      Traits = MainPlayerBlock;
    }
    else
    {
      Traits = OtherPlayerBlock;
    }
    Depth = TLayerDepth::Foreground;
    HeartsLeft = MaxHearts;
    WasStandingOnPlatform = false;

    if (progress->ShouldShowPlayer(PlayerId))
    {
      RunState = RemoveEnumValue(RunState, TRunState::Invisible);
    }
    else
    {
      RunState = AddEnumValue(RunState, TRunState::Invisible);
    }

    if (Item1 != nullptr)
    {
      AddSubSerializerBlock(context, Item1);
      InteractItem_(context, TInteractionType::PickedUp);
    }
#if SHOW_GHOST
    ghost_.Init(context, Traits);
#endif
  }

  // RL Training (and player's intuition) depends on consistent gravity/jump behavior. So convert/enforce these here.
  void Convert(std::shared_ptr<TContext> context) override
  {
    Gravity = {{0, 30}, {0, 0, 1}};
    Jump = {{0, -25}, {0, 0, 1}};
    Walk = {{10, 0}};
    JumpTimer = {NanosFromSeconds(.21f)};
  }

  void LoadContent(TContextPtr context) override
  {
    context->LoadSpriteSheet(GetSprite_(context));
#if SHOW_GHOST
    ghost_.LoadContent(context);
#endif
  }

  void HandleActions(TContextPtr context, const TPlayerActions& actions) override
  {
#if DEBUG
    auto* progress = context->GetGameProgress();
    if (progress->GameType == TGameType::SinglePlayer)
    {
      if (progress->GetInactivePlayerId() == PlayerId)
      {
        TLOG(LOG_ERROR, "-- Invalid action for inactive player in single-player mode.");
        return;
      }
    }
#endif
    actions_ = actions;
  }

  void Update(TContextPtr context) override
  {
    auto progress = context->GetGameProgress();
    if (progress->GetPlayerState(PlayerId) != TPlayerState::Alive || !progress->ShouldShowPlayer(PlayerId))
    {
      Traits = AddEnumValue(Traits, TBlockTraits::Cosmetic);
      return;
    }
    const auto action = actions_.Action;
    Vector2 moveVec = {0, 0};

    // If other blocks 'push' us, first get out of the way before doing our own movement.
    HandleWalk(this, action, moveVec, FacingDir, Walk);
    HandleJump_GravityFall(this, context, HasEnumValue(action, TPlayerAction::Jump), moveVec, JumpTimer, JumpState,
        Jump, Gravity, WasStandingOnPlatform, /* mustWaitForJumpAfterLanding */ true);
    int attacked = 0;

    HandleEnemyAttack(this, context, /* allow squash */ true, /* only attack by jumping */ true, 1, Damage, attacked);
    if (attacked > 0)
    {
      context->UpdateGameProgress()->UpdatePlayerProgress(PlayerId)->NumEnemies += attacked;
    }
    RequestRunState(TRunState::RequirePostUpdate);
  }


  void CheckRewards_(ABlock* block, std::shared_ptr<TContext> context)
  {
    if (!HasEnumValue(block->Traits, Collectible))
    {
      return;
    }
    if (HasEnumValue(block->Traits, ItemPicked))
    {
      return;
    }
    context->UpdateGameProgress()->UpdatePlayerProgress(PlayerId)->NumRewards += block->GetCollectibleReward();
    // Iteract with reward correctly here...
    block->Interact(context, {TInteractionType::PickedUp}, *this, nullptr);
  }

  //! @brief This is a business logic method.
  //! {@note Checks for player behavior such as reaching goal, activating blocks, collecting rewards etc.}
  //! {returns} true if the player was moved due to collisions with other solid objects.
  bool CheckPlayerBehavior(TContextPtr context)
  {
    const auto& grid = context->Grid();
    auto wasMoved = false;
    grid->FindNeighbors(this,
        [&](TGridBlockInfo& neighbors) -> bool
        {
          auto neighbor = neighbors.Neighbor;
          if (neighbor->IsA(GoalBlock))
          {
            reachedGoal_ = true;
          }
          CheckRewards_(neighbor, context);
          if (neighbor->IsA(Activatable) && HasEnumValue(actions_.Action, TPlayerAction::Activate))
          {
            neighbor->Interact(context, {TInteractionType::Activate}, *this, nullptr);
          }
          if (!neighbor->IsA(Solid))
          {
            return true;
          }
          const auto moveAfter = neighbors.CheckCollision(Box, VEC_ZERO, neighbor);
          if (!IsEmptyVec(moveAfter))
          {
            if (neighbor->IsA(GoalBlock))
            {
              reachedGoal_ = true;
            }
            CheckRewards_(neighbor, context);
            MoveRectBy(Box, moveAfter);
            wasMoved = true;
          }
          return true;
        },
        {.allowedTraits = AddEnumValue(GoalBlock, Activatable, Solid)});
    return wasMoved;
  }

  // Because we may be 'pushed' by other blocks, we need to account for collisions.
  void PostUpdate(TContextPtr context) override
  {
    auto progress = context->GetGameProgress();
    if (progress->GetPlayerState(PlayerId) != TPlayerState::Alive || !progress->ShouldShowPlayer(PlayerId))
    {
      return;
    }

    auto& grid = context->Grid();

    // Check for 'squished to death' (if we were moved by another block).
    if (CheckPlayerBehavior(context))
    {
      grid->FindNeighbors(this,
          [&](TGridBlockInfo& _) -> bool
          {
            squished_ = true;
            return false;
          },
          {.allowedTraits = Solid, .checkCollision = true});
    }

    // Check if the player went out of the screen.
    if (!DoesRectContainRect(context->GetCamera().Viewport, Box))
    {
      fellTooHigh_ = true;
    }
    if (squished_ || fellTooHigh_)
    {
      context->UpdateGameProgress()->UpdatePlayerProgress(PlayerId)->PlayerState = TPlayerState::Dead;
    }
    if (reachedGoal_)
    {
      context->UpdateGameProgress()->UpdatePlayerProgress(PlayerId)->HasReachedGoal = true;
    }

    if (Item1 != nullptr)
    {
      InteractItem_(
          context, HasEnumValue(actions_.Action, TPlayerAction::Use) ? TInteractionType::Hit : TInteractionType::None);
    }


    // This must be the last thing we do: reset action for the next frame in case we don't receive subsequent
    // inputs.
    actions_.Action = TPlayerAction::None;
  }

  TSpriteSheet& GetSprite_(std::shared_ptr<TContext> context)
  {
    if (context->GetGameProgress()->IsActivePlayer(PlayerId))
    {
      return PlayerSprite;
    }
    return PriorSprite;
  }

  TScenes arrowTransition_;
  TScenes winningTransition_;
  TScenes killedAnim_;

  void Draw(TContextPtr context) override
  {
    // Animate the sprite sheet if we are walking, otherwise, stop at index 0.
    auto color = WHITE;
    auto box = Box;
    const auto& progress = context->GetGameProgress();
    auto isItemVisible = progress->HasActions;
    if (context->GetGameProgress()->GetPlayerProgress(PlayerId).PlayerState != TPlayerState::Alive)
    {
      auto fade = 1.0f;
      if (!RunExplodeBlockAnim(this, context, killedAnim_, box, fade, 3000_ms, 8.0f, 20.0f))
      {
        RunState = AddEnumValue(RunState, TRunState::Removed);
        return;
      }
      color = Fade(GRAY, fade);
      isItemVisible = false;
    }
    auto angle = 0.0f;

    const std::shared_ptr<ABlock> item = Item1 != nullptr ? GetABlock(Item1) : nullptr;
    if (progress->IsActivePlayer(PlayerId) &&
      (progress->GetGameState() == TGameState::AboutToRun || progress->GetGameState() == TGameState::MenuDismissing ||
        progress->GetGameState() == TGameState::MenuDismissedBeforeRunning))
    {
      if (arrowTransition_.BeginScenes(context, /*repeating*/ true))
      {
        Vector2 v1 = {}, v2 = {}, v3 = {};
        if (arrowTransition_.RunScene(500_ms, 0_ms, 100_ms))
        {
          v1 = {Box.x, Box.y - (Box.height + Box.height * (float)arrowTransition_.PercentTime())};
          v2 = {Box.x + Box.width, Box.y - (Box.height + Box.height * (float)arrowTransition_.PercentTime())};
          v3 = {Box.x + Box.width / 2, Box.y - (Box.height * (float)arrowTransition_.PercentTime())};
        }
        if (arrowTransition_.RunScene(500_ms, 0_ms, 100_ms))
        {
          v1 = {Box.x, Box.y - (Box.height + Box.height * (float)arrowTransition_.ReversePercentTime())};
          v2 = {Box.x + Box.width, Box.y - (Box.height + Box.height * (float)arrowTransition_.ReversePercentTime())};
          v3 = {Box.x + Box.width / 2, Box.y - (Box.height * (float)arrowTransition_.ReversePercentTime())};
        }
        arrowTransition_.EndScenes();
        // Must be CCW!!
        DrawTriangle(v3, v2, v1, Fade(RED, 0.5));
      }

      if (winningTransition_.BeginScenes(context, true))
      {
        auto factor = 0.0f;
        if (winningTransition_.RunScene(1000_ms, 500_ms, 500_ms))
        {
          factor = (float)winningTransition_.PercentTime();
        }
        if (winningTransition_.RunScene(1000_ms))
        {
          factor = (float)winningTransition_.ReversePercentTime();
        }

        box = ScaleRect(box, 1.0f + 0.5f * factor);
        angle = factor * 5.0f;
        winningTransition_.EndScenes();
      }
    }
    if (item != nullptr)
    {
      item->SetVisible(isItemVisible);
    }
    context->DrawAnimSprite(GetSprite_(context), FacingDir.x >= 0 ? box : RectXInvert(box), color,
        Walk.IsZero() ? 0 : -1, angle, {0.0f, 0.0f});
  }

#if DEBUG
  void DrawDebug(TContextPtr context) override
  {
    if (HasEnumValue(RunState, TRunState::Invisible)) return;
    if (WasStandingOnPlatform)
    {
      DrawRectangleLinesEx({Box.x, Bottom(Box) - 7, Box.width, 15}, 7, BLUE);
    }
    if (JumpState == TJumpState::Jumping)
    {
      DrawRectangleLinesEx({Box.x, Box.y - 7, Box.width, 15}, 7, GREEN);
    }
    if (JumpState == TJumpState::Falling)
    {
      DrawRectangleLinesEx({Box.x, Box.y - 7, Box.width, 15}, 7, RED);
    }
    if (JumpState == TJumpState::Landed)
    {
      DrawRectangleLinesEx({Box.x, Box.y - 7, Box.width, 15}, 7, BLUE);
    }
    if (JumpState == TJumpState::StartJump)
    {
      DrawRectangleLinesEx({Box.x, Box.y - 7, Box.width, 15}, 7, WHITE);
    }


#if SHOW_GHOST
    ghost_.Draw(context, GetSprite_(context));
#endif
  }
#endif

#if RLPLAYS_EDITOR
  void EditorDraw(const std::shared_ptr<TEditorData>& editorData, TContextPtr context) override
  {
    context->LoadSpriteSheet(PlayerSprite);
    context->LoadSpriteSheet(PriorSprite);

    context->DrawAnimSprite(PlayerSprite, FacingDir.x >= 0 ? Box : RectXInvert(Box), WHITE, -1);
    context->DrawAnimSprite(PlayerSprite, FacingDir.x >= 0 ? Box : OffsetRect(Box, {Box.width, 0}), GRAY, -1);
  }

  void EditorEnsureMirror(
      const std::shared_ptr<TEditorData>& editorData, TContextPtr context, const TBlock& thatBlock) override
  {
    PlayerId = PlayerId2;
    FacingDir = GetABlockFor<TPlayerBlock>(thatBlock)->FacingDir;
    FacingDir.x = -FacingDir.x; // Mirror the facing direction.
  }
#endif

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
      TInteractionResult* optInteractResult) override
  {
    auto* gameProgress = context->UpdateGameProgress();
    auto* playerProgress = gameProgress->UpdatePlayerProgress(PlayerId);
    if (playerProgress->PlayerState != TPlayerState::Alive)
    {
      return;
    }
    if (interact.Interact == TInteractionType::DamagedBy)
    {
      playerProgress->NumEnemies++;
    }

    HandlePush(this, context, interact, optInteractResult);
    if (interact.Interact == TInteractionType::Damage)
    {
      // TODO(perumaal): Make sure we don't take more damage immediately (Mario Blink for ~1000ms).
      HeartsLeft -= interact.Damage;
      HeartsLeft = std::max(0, HeartsLeft);
      if (HeartsLeft == 0)
      {
        context->UpdateGameProgress()->SetPlayerState(PlayerId, TPlayerState::Dead);
      }
    }
  }

  [[nodiscard]] bool DoesHandleActions(const bool replay) const override
  {
    if (replay)
    {
      return PlayerId == PlayerId2;
    }
    return PlayerId == PlayerId1;
  }

private:
  TPlayerActions actions_;
  bool squished_ = false;
  bool fellTooHigh_ = false;
  bool reachedGoal_ = false;
  // TPlayerState playerState_ = TPlayerState::Alive;

#if SHOW_GHOST
  TGhostPlayerBlocks ghost_;
#endif
};
} // namespace RLPlays
