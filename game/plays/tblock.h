#pragma once

#include <base_block.h>
#include <base_types.h>
#include <game_blocks.h>
#include <nlohmann/json.hpp>
#include <player_block.h>
#include <activator_blocks.h>
#include <progress_block.h>
#include <weapon_block.h>
#include <enemy_blocks.h>
#include <hover_enemy_block.h>
#include <shooter_enemy_block.h>
#include <punch_enemy_block.h>
#include <dropper_enemy_block.h>
#include <serialize.h>

namespace RLPlays
{
enum class TBlockType : int8_t
{
  None   = 0,
  Player = 1,

  // Bricks
  Brick    = 3,
  Moveable = 4,

  // Meta blocks
  Goal     = 5,
  Progress = 6,

  SwitchActivator = 7,


  // Weapon with bullet
  Weapon = 8,
  Bullet = 9,


  Reward = 10,

  // Enemies
  MovingEnemy = 11,

  // Background blocks
  Background        = 12,
  JumpingEnemy      = 13,
  PlatformActivator = 14,
  ActivatedMoveable = 15,
  HoverEnemy = 16,
  ShooterEnemy = 17,
  // Melee punch enemy
  Punch         = 18,
  PunchingEnemy = 19,
  // Vertical drop enemy
  DropperEnemy = 20,
  // NOTE: Make sure this TBlockType and the SerializerDerived macro (below) are in sync!
  LastBlock = 20,
};

// Count of block types sans None.
constexpr int TBlockTypeCount = static_cast<int>(TBlockType::LastBlock) - 1;

struct TBlock
{
  TBlockType BlockType = TBlockType::None;

  std::shared_ptr<ABlock> Block;

  //! @brief Helper to get the specific block-type from the given TBlock instance.
  template <class T_BlockType>
  std::shared_ptr<T_BlockType> GetABlockWithType() const
  {
    return std::static_pointer_cast<T_BlockType>(Block);
  }

  // Increase version when you introduce new block types to ensure recorded actions tie properly.
  // Make sure TBlockType (above) and the SerializerDerived macro below are in sync!
  SerializerDerived(TBlock, ABlock, BlockType, Block, Player, Brick, Moveable, Goal, Progress, SwitchActivator,
    Weapon, Bullet, Reward, MovingEnemy, Background, JumpingEnemy, PlatformActivator, ActivatedMoveable,
    HoverEnemy, ShooterEnemy, Punch, PunchingEnemy, DropperEnemy)
  // Generated code contains:
  // struct TReflection_Block {
  //   TBlockType TypeEnum;
  //   std::string TypeName;
  //   std::shared_ptr<ABlock> BlockInstance;
  // }
};
} // namespace RLPlays
