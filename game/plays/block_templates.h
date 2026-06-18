#pragma once
#include <tblock.h>
#include <world_editor.h>

#ifndef RLPLAYS_EDITOR
static_assert(false, "block_templates.h MUST ONLY BE USED IN THE EDITOR BUILD.");
#endif
namespace RLPlays
{
constexpr int cellSize = 96;

inline std::shared_ptr<ABlock> CreateWeapon(TTemplateBlocks& templateBlocks)
{
  // Weapon that shoots a bullet.
  auto block = std::make_shared<TWeaponBlock>();
  block->Box = {0, 0, cellSize / 8, cellSize / 8};
  block->Tex = TTexture("sprites/weapons/tile_0050.png");
  auto bulletBlock = std::make_shared<TBulletBlock>();
  bulletBlock->Box = {0, 0, cellSize / 8, cellSize / 8};
  bulletBlock->Tex = TTexture("sprites/weapons/tile_0044.png");
  block->BulletBlock = templateBlocks.CreateBlock<TBulletBlock>(bulletBlock, TBlockType::Bullet);
  return block;
}

inline std::shared_ptr<ABlock> CreatePunch(TTemplateBlocks& templateBlocks)
{
  // Punch hitbox sub-block for TPunchingEnemyBlock.
  auto punchBlock = std::make_shared<TPunchBlock>();
  punchBlock->Box = {0, 0, cellSize / 2, cellSize / 2};
  punchBlock->Tex = TTexture("sprites/weapons/tile_0050.png");
  punchBlock->Damage = 1;
  punchBlock->Duration = {NanosFromMillis(200)};
  return punchBlock;
}

inline void AddBlockFromTemplates(const std::shared_ptr<TWorld>& world, TTemplateBlocks& templateBlocks,
    const std::string& name, const TBlockId blockId)
{
  auto block = world->GetBlockWithIdForEditor(blockId);
  if (block != nullptr) { templateBlocks.AddBlockFromTemplateFile(name, *block, block->BlockType); }
}

// Templates we can easily add in the editor with preconfigured blocks.
inline void CreateTemplates(TContextPtr context, TTemplateBlocks& templateBlocks, TWorldFiles& worldFiles)
{
  templateBlocks = {};
  {
    // A simple player block.
    auto block = templateBlocks.AddBlock<TPlayerBlock>("Player", TBlockType::Player);
    block->Box = {0, 0, cellSize - 12, cellSize - 12}; // Some leeway to prevent locking up between blocks. D'oh.
    block->PlayerSprite = TSpriteSheet("sprites/player3-sheet.png", 4, {cellSize, cellSize});
    block->PriorSprite = TSpriteSheet("sprites/player-sheet.png", 4, {cellSize, cellSize});
    block->PlayerId = 1;
    block->Jump = {{0, -20}, {0, 0, 1, 0.1, 0.1}};
    block->JumpTimer = {NanosFromMillis(250)};
    block->Walk = {{9, 0}, {0, -1, 1, 0.5, 0.3}};
  }
  {
    // A player block with a weapon.
    auto block = templateBlocks.AddBlock<TPlayerBlock>("Player with weapon", TBlockType::Player);
    block->Box = {0, 0, cellSize - 12, cellSize - 12}; // Some leeway to prevent locking up between blocks. D'oh.
    block->PlayerSprite = TSpriteSheet("sprites/player3-sheet.png", 4, {cellSize, cellSize});
    block->PriorSprite = TSpriteSheet("sprites/player-sheet.png", 4, {cellSize, cellSize});
    block->PlayerId = 1;
    block->Jump = {{0, -20}, {0, 0, 1, 0.1, 0.1}};
    block->JumpTimer = {NanosFromMillis(250)};
    block->Walk = {{9, 0}, {0, -1, 1, 0.5, 0.3}};
    block->Item1 = templateBlocks.CreateBlock<TWeaponBlock>(CreateWeapon(templateBlocks), TBlockType::Weapon);
  }

  {
    // A simple brick block.
    auto block = templateBlocks.AddBlock<TBrickBlock>("Brick", TBlockType::Brick);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/tile_0047.png", {cellSize, cellSize});
  }

  {
    // Moveable block that moves in one direction and returns in the other when it hits another solid block.
    auto block = templateBlocks.AddBlock<TMoveableBlock>("Moving platform", TBlockType::Moveable);
    block->Box = {0, 0, cellSize, cellSize};
    block->MoveVel = {{5, 0}, {0, -1, 1, 10000, 10000}};
    block->Tex = TTexture("sprites/tile_0100.png", {cellSize, cellSize});
  }

  {
    // Final round-progress block.
    auto block = templateBlocks.AddBlock<TGoalBlock>("Final goal", TBlockType::Goal);
    block->Box = {0, 0, cellSize / 2, cellSize / 2};
    block->Tex = TTexture("sprites/tile_0067.png", {cellSize / 2, cellSize / 2});
  }

  {
    // Show progress block.
    auto block = templateBlocks.AddBlock<TProgressBlock>("Progress box", TBlockType::Progress);
    block->Box = {0, 0, cellSize, cellSize};
  }

  {
    // Switch Activator that activates a Moving block
    auto block = templateBlocks.AddBlock<TSwitchActivatorBlock>(
        "Switch activator with moving platform", TBlockType::SwitchActivator);
    block->Box = {0, 0, cellSize, cellSize};
    block->InactiveTex = TTexture("sprites/tile_0064.png", {cellSize, cellSize});
    block->ActiveTex = TTexture("sprites/tile_0066.png", {cellSize, cellSize});

    const auto activatorBlock =
        templateBlocks.AssignNewBlockTo<TActivatedMoveableBlock>(TBlockType::ActivatedMoveable, block->ActivatorBlock);
    activatorBlock->Box = {cellSize * 10, cellSize * 5, cellSize, cellSize};
    activatorBlock->MoveVel = {{5, 0}, {0, -1, 1, 10000, 10000}};
    activatorBlock->Tex = TTexture("sprites/tile_0100.png", {cellSize, cellSize});
  }

  {
    templateBlocks.AddBlock<TWeaponBlock>("Weapon with bullet", CreateWeapon(templateBlocks), TBlockType::Weapon);
  }

  {
    // Final round-progress block.
    auto block = templateBlocks.AddBlock<TRewardBlock>("Reward", TBlockType::Reward);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/reward1.png", {cellSize, cellSize});
  }

  {
    // Enemy that moves forward and back.
    auto block = templateBlocks.AddBlock<TMovingEnemyBlock>("Moving enemy (left/right)", TBlockType::MovingEnemy);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/enemies/tile_0013.png", {cellSize, cellSize});
    block->MoveVel = {{2, 0}, {0, -1, 1, 10000, 10000}};
  }

  {
    // Enemy that jumps / moves forward and back.
    auto block = templateBlocks.AddBlock<TJumpingEnemyBlock>("Jumping enemy", TBlockType::JumpingEnemy);
    block->Box = {0, 0, cellSize, cellSize};
    block->Sprite = TSpriteSheet("sprites/enemies/enemy1-sheet.png", 4, {cellSize / 4, cellSize / 4});
    block->MoveVel = {{2, 0}, {0, -1, 1, 10000, 10000}};
    block->Jump = {{0, -20}, {0, 0, 1, 0.1, 0.1}};
    block->JumpTimer = {NanosFromMillis(250)};
  }

  {
    // Background block tiled across the entire viewport.
    auto block = templateBlocks.AddBlock<TBackgroundBlock>("Background (Tiled Viewport)", TBlockType::Background);
    block->Box = context->GetCamera().Viewport;
    block->Tex = TTexture("sprites/bg/pattern_0008.png", {256, 256});
  }
  {
    // Background block - in the center.
    auto block = templateBlocks.AddBlock<TBackgroundBlock>("Background (Small)", TBlockType::Background);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/pattern_tiles/tile_0000.png", {cellSize, cellSize});
  }
  {
    // Hovering enemy that floats and follows the player.
    auto block = templateBlocks.AddBlock<THoverEnemyBlock>("Hover enemy", TBlockType::HoverEnemy);
    block->Box = {0, 0, cellSize, cellSize};
    block->HoverSprite = TSpriteSheet("sprites/enemies/tile_0024.png", 1, {cellSize / 4, cellSize / 4});
    block->SeekSprite = TSpriteSheet("sprites/enemies/tile_0025.png", 1, {cellSize / 4, cellSize / 4});
    block->HitSprite = TSpriteSheet("sprites/enemies/tile_0026.png", 1, {cellSize / 4, cellSize / 4});
    block->DamagedSprite = TSpriteSheet("sprites/enemies/tile_0024.png", 1, {cellSize / 4, cellSize / 4});
    block->DeadSprite = TSpriteSheet("sprites/enemies/tile_0024.png", 1, {cellSize / 4, cellSize / 4});
    block->HoverVel = {{2, 0}, {0, -1, 1, 10000, 10000}};
    block->Health = 1;
  }

  {
    // Static turret enemy that shoots a horizontal bullet at any moving character in range.
    auto block = templateBlocks.AddBlock<TShooterEnemyBlock>("Shooter enemy", TBlockType::ShooterEnemy);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/enemies/tile_0013.png", {cellSize, cellSize});
    block->TriggerRangeCells = 5.0f;
    block->Health = 1;
    auto bulletBlock = std::make_shared<TBulletBlock>();
    bulletBlock->Box = {0, 0, cellSize / 8, cellSize / 8};
    bulletBlock->Tex = TTexture("sprites/weapons/tile_0044.png");
    block->BulletBlock = templateBlocks.CreateBlock<TBulletBlock>(bulletBlock, TBlockType::Bullet);
  }

  {
    // Platform Activator that activates a Moving block when a solid object/player stands on top
    auto block = templateBlocks.AddBlock<TPlatformActivatorBlock>(
        "Platform activator with moving platform", TBlockType::PlatformActivator);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/tile_0140.png", {cellSize, cellSize});
    block->PlatformTex = TTexture("sprites/tile_0108.png", {cellSize, cellSize});
    const auto activatorBlock =
        templateBlocks.AssignNewBlockTo<TActivatedMoveableBlock>(TBlockType::ActivatedMoveable, block->ActivatorBlock);
    activatorBlock->Box = {cellSize * 10, cellSize * 5, cellSize, cellSize};
    activatorBlock->MoveVel = {{0, 5}, {0, -1, 1, 10000, 1000}};
    activatorBlock->ActivateToMoveOnce = true;
    activatorBlock->Tex = TTexture("sprites/tile_0100.png", {cellSize, cellSize});
  }

  {
    // Static melee enemy that punches any player/enemy that comes within one block horizontally.
    auto block = templateBlocks.AddBlock<TPunchingEnemyBlock>("Punching enemy", TBlockType::PunchingEnemy);
    block->Box = {0, 0, cellSize, cellSize};
    block->Tex = TTexture("sprites/enemies/tile_0013.png", {cellSize, cellSize});
    block->TriggerRangeCells = 1.0f;
    block->RecoilTime = {NanosFromMillis(800)};
    block->Health = 1;
    block->PunchBlock = templateBlocks.CreateBlock<TPunchBlock>(CreatePunch(templateBlocks), TBlockType::Punch);
  }

  {
    // Ceiling block that drops a bullet onto any moving character detected directly below.
    auto block = templateBlocks.AddBlock<TDropperEnemyBlock>("Dropper enemy", TBlockType::DropperEnemy);
    block->Box = {0, 0, cellSize, cellSize / 2};
    block->Tex = TTexture("sprites/tile_0047.png", {cellSize, cellSize / 2});
    block->TriggerRangeCells = 5.0f;
    block->Health = 1;
    auto bulletBlock = std::make_shared<TBulletBlock>();
    bulletBlock->Box = {0, 0, cellSize / 8, cellSize / 8};
    bulletBlock->Tex = TTexture("sprites/weapons/tile_0044.png");
    bulletBlock->MoveBullet = {{0, 20}};
    block->BulletBlock = templateBlocks.CreateBlock<TBulletBlock>(bulletBlock, TBlockType::Bullet);
  }

  auto files = worldFiles.GetFiles("templates/");
  for (const auto& file : files)
  {
    TGameLoadInfo loadInfo = {.Filename = file};
    auto game = LoadGame(worldFiles, loadInfo);
    if (game.Game != nullptr)
    {
      auto& world = game.Game->World;
      AddBlockFromTemplates(world, templateBlocks, world->WorldInfo.Desc, world->WorldInfo.MainBlockId);
      AddBlockFromTemplates(world, templateBlocks, world->WorldInfo.Desc2, world->WorldInfo.MainBlockId2);
      AddBlockFromTemplates(world, templateBlocks, world->WorldInfo.Desc3, world->WorldInfo.MainBlockId2);
    }
  }
}
} // namespace RLPlays
