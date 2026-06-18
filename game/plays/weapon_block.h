#pragma once
#include <serialize.h>

#include <base_block.h>
#include <context.h>

namespace RLPlays
{
enum TWeaponState : uint8_t
{
  Idle      = 0,
  Shooting  = 1,
  HitObject = 2,
  Done      = 3
};

struct TBulletBlock final : ABlock
{
  TTexture Tex;
  Vector2 Offset;
  TCountdownTimer MaxTimePerBullet = {NanosFromMillis(5000)};
  TMoveSimple MoveBullet = {{20, 0}};
  int Damage = 1;

  SerializerWithBase(TBulletBlock, ABlock, Tex, Offset, MaxTimePerBullet, Damage, MoveBullet)

  void Init(TContextPtr context) override
  {
    Traits = AddEnumValue(CauseDamage, Spawnable);
    RunState = AddEnumValue(RunState, TRunState::Invisible);

    state_ = Idle;
  }

  void LoadContent(TContextPtr context) override
  {
    context->LoadTexture(Tex);
  }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    RunState = RemoveEnumValue(RunState, TRunState::Invisible);
    facing_ = interact.Dir;
    state_ = Shooting;
    float x = (Offset.x * FloatSign(facing_.x));
    if (facing_.x < 0) { x -= Box.width; }
    context->MoveBlockBy(this, {x, Offset.y});
    MaxTimePerBullet.Start();
  }

  void Update(TContextPtr context) override
  {
    if (state_ == Idle) return;

    if (state_ == Shooting)
    {
      const Vector2 moveVec = MoveBullet.Move(facing_.x >= 0 ? 1 : -1);
      context->MoveBlockBy(this, moveVec);
      MaxTimePerBullet.TickTimerPerFrame(context);
      if (!MaxTimePerBullet.IsRunning()) { state_ = Done; }
      auto& grid = context->Grid();
      grid->FindNeighbors(this, [&](TGridBlockInfo& colliders) -> bool
      {
        auto collider = colliders.Neighbor;
        const auto moveAfter = colliders.CheckCollision(Box, moveVec, collider);
        if (!IsZeroVec(moveAfter))
        {
          if (HasEnumValue(collider->Traits, Solid)) { state_ = Done; }
          if (HasEnumValue(collider->Traits, TakesDamage))
          {
            collider->Interact(context, {TInteractionType::Damage, VEC_ZERO, facing_, Damage}, *this, nullptr);
            state_ = Done;
          }
        }
        return true;
      }, {.allowedTraits = AddEnumValue(Solid, TakesDamage)});
    }
    if (state_ == Done)
    {
      context->RemoveSpawnedBlock(*this);
      state_ = Idle;
    }
  }

  void Draw(TContextPtr context) override
  {
    context->DrawTexture(Tex, facing_.x > 0 ? Box : RectXInvert(Box), WHITE);
  }

private:
  Vector2 facing_ = {1, 0};
  TWeaponState state_ = Idle;
};

struct TWeaponBlock : ABlock
{
  TTexture Tex;
  Vector2 Offset;
  TCountdownTimer TimeBetweenFiring = {NanosFromMillis(250)};
  std::shared_ptr<TBlock> BulletBlock;

  SerializerWithBase(TWeaponBlock, ABlock, Tex, BulletBlock, Offset)

  void Init(TContextPtr context) override
  {
    if (BulletBlock == nullptr) return;

    Traits = PickableItem;
    facing_ = {1, 0};
    tempBox_ = Box;
    context->InitBlockIfNeeded(context, GetABlock(BulletBlock));
  }

  void LoadContent(TContextPtr context) override
  {
    if (BulletBlock == nullptr) return;
    context->LoadTexture(Tex);
    GetABlock(BulletBlock)->LoadContent(context);
  }

  void Draw(TContextPtr context) override
  {
    context->DrawTexture(Tex, facing_.x > 0 ? tempBox_ : RectXInvert(tempBox_), WHITE);
  }

  void Shoot_(TContextPtr context, const ABlock& ownerBlock)
  {
    if (state_ == Idle)
    {
      auto bullet = context->SpawnBlock(context, BulletBlock, spawnPoint_);
      TInteractionResult result = {};
      TInteraction shoot = {TInteractionType::Hit, VEC_ZERO, facing_};
      GetABlock(bullet)->Interact(context, shoot, *this, &result);
      TimeBetweenFiring.Start();
      state_ = Shooting;
    }
  }

  void Interact(TContextPtr context, const TInteraction& interact, const ABlock& that,
    TInteractionResult* optInteractResult) override
  {
    if (BulletBlock == nullptr) return;
    if (HasEnumValue(interact.Interact, TInteractionType::PickedUp))
    {
      Traits = AddEnumValue(Traits, ItemPicked);
    }
    facing_ = interact.Dir;
    tempBox_ = Box;
    auto totalOffset = AddVector(interact.Offset, Offset);
    if (facing_.x < 0) { totalOffset.x -= (tempBox_.width); }
    OffsetRectBy(tempBox_, totalOffset);
    spawnPoint_ = RectTopLeft(tempBox_);
    spawnPoint_.y += (tempBox_.height / 2.0f);
    if (facing_.x >= 0) { spawnPoint_.x += tempBox_.width; }

    RunState = CopyEnumValue(RunState, that.GetRunState(), TRunState::Invisible);

    if (HasEnumValue(interact.Interact, TInteractionType::Hit))
    {
      Shoot_(context, that);
    }
  }

  // Must handle main block going away.

  void Update(TContextPtr context) override
  {
    if (state_ == Shooting)
    {
      TimeBetweenFiring.TickTimerPerFrame(context);
      if (!TimeBetweenFiring.IsRunning()) { state_ = Done; }
    }
    if (state_ == Done)
    {
      TimeBetweenFiring.Stop();
      state_ = Idle;
    }
  }

private:
  Rectangle tempBox_ = {};
  Vector2 facing_ = {1, 0};
  Vector2 spawnPoint_ = {0, 0};
  TWeaponState state_ = Idle;
};
}
