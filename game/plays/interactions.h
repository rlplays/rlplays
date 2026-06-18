#pragma once
#include <actions.h>
#include <base_types.h>
#include <context.h>
#include <cstdint>
#include <memory>

namespace RLPlays
{
//! @brief Used by ABlock as TInteractionResult ABlock::Interact(...TInteraction )
struct TEditorData;
struct TBlock;

enum class TBlockType : int8_t;

enum class TInteractionType : uint8_t
{
  None      = 0x0,
  Hit       = 0x1,  //!< Block was hit by a player or another block.
  Push      = 0x2,  //!< Block was pushed by a player or another block.
  Damage    = 0x4,  //!< Block was damaged by a player or another block.
  Activate  = 0x8,  //!< Block was activated by a player.
  PickedUp  = 0x10, //!< Block was picked up by a player.
  Squashed  = 0x20, //!< Block was squashed by another block.
  DamagedBy = 0x40, //!< Other Block was damaged by our block.
};

//! @brief Running traits for the blocks (not stored but provided for by the subclass).
enum class TBlockTraits : uint32_t
{
  None         = 0x00000,
  Solid        = 0x00001,
  Pushable     = 0x00002,
  Moveable     = 0x00004,
  Activatable  = 0x00008,
  Collectible  = 0x00010,
  PickableItem = 0x00020,
  Spawnable    = 0x00040,
  // 0x80 defined below as PlayerBlock.
  GoalBlock   = 0x00100 | Collectible,
  ItemPicked  = 0x00200,
  CauseDamage = 0x00400,
  TakesDamage = 0x00800,
  // The main player receiving realtime active inputs (from AI or human).
  MainPlayerBlock = 0x00080 | Pushable | Moveable | TakesDamage,
  // The other player (controlled by AI or other player or self-replay actions).
  OtherPlayerBlock = 0x01000 | Pushable | Moveable | TakesDamage,
  // Only cosmetic, not used by the player to make decisions.
  Cosmetic    = 0x02000,
  EnemyGroup1 = 0x04000,
  EnemyGroup2 = 0x08000,

  LastUseableBlockTrait = EnemyGroup2
};

static const TBlockTraits PlayerBlocks = AddEnumValue(TBlockTraits::MainPlayerBlock, TBlockTraits::OtherPlayerBlock);
static const TBlockTraits EnemyGroups = AddEnumValue(TBlockTraits::EnemyGroup1, TBlockTraits::EnemyGroup2);
constexpr int TBlockTraitsCount = 16;

//! @brief Contains the interaction input from the source of the interactor.
//! NOTE: For now, just use a flat struct. If this grows big, use a tagged/discriminated union.
struct TInteraction
{
  // In params:
  TInteractionType Interact = TInteractionType::None; //!< Interaction type (set from the source).
  //! @brief For sub-blocks, indicates the offset used from the main-block.
  Vector2 Offset = VEC_ZERO;

  //! @brief Facing direction of the source block.
  Vector2 Dir = {1, 0};

  int Damage = 0;
};

//! @brief Optional result of the interaction from the target block communicated to the source block.
//! Importantly, the code should be structured source doesn't know about the specific target type (and vice versa).
//! This helps elevate all block types to the same level (as opposed to an oracle treating human players above all other
//! blocks).
//! NOTE: For now, just use a flat struct. If this grows big, use a tagged/discriminated union.
struct TInteractionResult
{
  // Out params:
  bool wasHandled = false;
  bool hasStopped = false;
};
}
