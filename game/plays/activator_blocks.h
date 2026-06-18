#pragma once
#include <serialize.h>

#include <actions.h>
#include <base_block.h>
#include <context.h>

namespace RLPlays
{
struct TBlock;

// These blocks can be activated when the player presses the Use button:
// - Switch activator moves a platform when a switch is activated.
// - Platform activator may drop a block from above etc.
// These were all vibe coded for fun, so can be extended in interesting ways.

struct TSwitchActivatorBlock final : ABlock
{
  TTexture InactiveTex;
  TTexture ActiveTex;
  std::shared_ptr<TBlock> ActivatorBlock;
  SerializerWithBase(TSwitchActivatorBlock, ABlock, InactiveTex, ActiveTex, ActivatorBlock)
  bool IsActive = false;

  void Init(TContextPtr context) override
  {
    if (ActivatorBlock == nullptr) return;
    AddSubSerializerBlock(context, ActivatorBlock);
    IsActive = false;
    Traits = TBlockTraits::Activatable;
  }

  void LoadContent(TContextPtr context) override
  {
    context->LoadTexture(InactiveTex);
    context->LoadTexture(ActiveTex);
  }


  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    if (ActivatorBlock == nullptr) return;
    if (interact.Interact == TInteractionType::Activate)
    {
      GetABlock(ActivatorBlock)->Interact(context, {TInteractionType::Activate}, *this, nullptr);
      IsActive = !IsActive;
    }
  }

  // Mirror the ActivatorBlock too, requires access to the other block's activator block which is not available at the header-time.
#if RLPLAYS_EDITOR
  void EditorEnsureMirror(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
    const TBlock& originalBlock) override;
#endif

  void Draw(TContextPtr context) override
  {
    if (ActivatorBlock == nullptr) return;
    context->DrawTexture(IsActive ? ActiveTex : InactiveTex, Box, WHITE);
  }
};

//! @brief When activated, activates a sub-block and keeps it activated until its done.
struct TPlatformActivatorBlock final : ABlock
{
  TTexture PlatformTex;
  TTexture Tex;
  float PlatformHeight = 48;
  float PlatformActivatedHeight = 32;
  std::shared_ptr<TBlock> ActivatorBlock;

  SerializerWithBase(TPlatformActivatorBlock, ABlock, Tex, PlatformTex, PlatformHeight, PlatformActivatedHeight,
    ActivatorBlock)

  enum class TPlatformState { Inactive, Active_Moving } PlatformState = TPlatformState::Inactive;

  void Init(TContextPtr context) override
  {
    PlatformState = TPlatformState::Inactive;
    Traits = TBlockTraits::Solid;
    if (ActivatorBlock == nullptr) return;
    AddSubSerializerBlock(context, ActivatorBlock);
    interactionResult_ = {};
  }

  void LoadContent(TContextPtr context) override
  {
    context->LoadTexture(Tex);
    context->LoadTexture(PlatformTex);
  }

  void Update(TContextPtr context) override
  {
    if (ActivatorBlock == nullptr) return;

    Rectangle platformBox = {Box.x, Box.y - PlatformHeight, Box.width, PlatformHeight};
    bool foundBlockOnTop = false;
    context->Grid()->FindNeighbors(this, [&](TGridBlockInfo& neighbors) -> bool
    {
      foundBlockOnTop = true;
      return true;
    }, {
      .overrideRect = &platformBox,
      .allowedTraits = AddEnumValue(TBlockTraits::Solid, TBlockTraits::Pushable, TBlockTraits::Moveable),
      .checkCollision = true,
    });
    if (foundBlockOnTop)
    {
      if (PlatformState == TPlatformState::Inactive)
      {
        PlatformState = TPlatformState::Active_Moving;
        GetABlock(ActivatorBlock)->Interact(context, {TInteractionType::Activate}, *this, &interactionResult_);
      }
    }
    if (PlatformState == TPlatformState::Active_Moving)
    {
      GetABlock(ActivatorBlock)->Interact(context, {TInteractionType::None}, *this, &interactionResult_);

      if (interactionResult_.hasStopped) { PlatformState = TPlatformState::Inactive; }
    }
  }


#if DEBUG
  void DrawDebug(TContextPtr context) override
  {
    auto box = Box;
    DrawRectangleRec(box, Fade(RED, 0.25f));
    box.y -= PlatformHeight;
    DrawRectangleRec(box, Fade(YELLOW, 0.55f));
  }
#endif

  void Draw(TContextPtr context) override
  {
    auto box = Box;
    if (PlatformState == TPlatformState::Inactive) { box.y -= PlatformHeight; }
    else
    {
      box.y -= PlatformActivatedHeight;
    }
    context->DrawTexture(PlatformTex, box, WHITE);
    context->DrawTexture(Tex, Box, WHITE);
  }

private:
  TInteractionResult interactionResult_;
};
}
