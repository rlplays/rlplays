#include <base_block.h>
#include <grid.h>
#include <grid_colliders.h>

using namespace RLPlays;
using enum RLPlays::TBlockTraits;

int TGridCells::IndexOfBlock(const TBlockId blockId) const
{
  for (int i = 0; i < Size; ++i)
  {
    if (((*this)[i])->BlockId == blockId)
    {
      return i;
    }
  }
  return InvalidBlockId;
}

bool TGridCells::RemoveBlock(const TBlockId blockId)
{
  for (int i = 0; i < Size; ++i)
  {
    if (((*this)[i])->BlockId == blockId)
    {
      return this->remove_at(i);
    }
  }
  return false;
}

bool TGridBlockInfo::IsCandidate(const ABlock* collider) const
{
  if (collider->HasRunState(TRunState::Invisible, TRunState::Removed)) { return false; }
  if (collider->IsA(Cosmetic)) { return false; }
  if (collider->BlockId == ourBlockId_) { return false; }
  if (CheckCollisionRecs(collider->Box, ourRect_)) { return true; }
  return false;
}


[[nodiscard]] Vector2 TGridBlockInfo::CheckCollision(Rectangle ourBox, const Vector2 movedBy,
  const ABlock* thatBlock) const
{
  float x = 0, y = 0;
  const auto& colliderBox = thatBlock->Box;
  auto collision = GetCollisionRec(ourBox, colliderBox);
  if (IsEmptyRect(collision)) { return {x, y}; }
#if defined(DEBUG) && !defined(RLPLAYS_TRAIN)
  if (TGrid::DebugTrackCollisions)
  {
    TGrid::DebugCollisions.push_back({.collisionRect = collision, .targetRect = colliderBox});
  }
#endif

  const Vector2 movedByAbs = {std::abs(movedBy.x), std::abs(movedBy.y)};
  // Prefer backing out along the movedBy vector first if possible.
  if (!IsZeroVec(movedByAbs))
  {
    const auto minX = std::min(collision.width, movedByAbs.x);
    const auto minY = std::min(collision.height, movedByAbs.y);
    x = FloatSign(-movedBy.x, minX);
    y = FloatSign(-movedBy.y, minY);
    ourBox.x += x;
    ourBox.y += y;
    collision = GetCollisionRec(ourBox, colliderBox);

    if (IsEmptyRect(collision)) { return {x, y}; }
  }

  // If the vector moved the rectangle by moveBy and resulted in a collision
  // (i.e. intersection is non-empty), then we can back out.
  if (FloatEqual(collision.width, ourBox.width))
  {
    if (ourBox.y < colliderBox.y) { y -= collision.height; }
    else { y += collision.height; }
  }
  else
  {
    // Favor vertical movement first.
    if (collision.width * 4 > collision.height)
    {
      if (ourBox.y < colliderBox.y) { y -= collision.height; }
      else { y += collision.height; }
    }
    else
    {
      if (ourBox.x < colliderBox.x) { x -= collision.width; }
      else { x += collision.width; }
    }
  }
  return {x, y};
}

void TGridBlockInfo::ApplyCollisionBackoff(ABlock* t)
{
  MoveRectBy(t->Box, CollisionBackoff);
  MoveRectBy(ourRect_, CollisionBackoff);
}

void TGridBlockInfo::Reset(const ABlock* const currentBlock, const TFindBlocksInOptions& options,
  const Rectangle expandRect)
{
  ourBlockId_ = currentBlock->BlockId;
  ourRect_ = (options.overrideRect != nullptr) ? *options.overrideRect : currentBlock->Box;
  ourRect_ = ExpandRect(ourRect_, options.expandRect);
  searchRect_ = ExpandRect(ourRect_, expandRect);
  // Avoid per-frame allocs, use at most 10 blocks/collisions by default.
  foundBlocks_.reserve(10);
  foundBlocks_.clear();
}
