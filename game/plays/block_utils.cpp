#include <serialize.h>

#include <actions.h>
#include <base_block.h>
#include <context.h>
#include <block_utils.h>
#include <world.h>

namespace RLPlays
{
Rectangle TBlockUtils::RandomizePlayerStartPos(TContextPtr context)
{
  if (context->GetGameProgress()->GetGameState() != TGameState::StartGame) return INVALID_RECT;
  const auto playerBlock = context->GetActionsHandlerBlock();
  if (playerBlock == nullptr) return INVALID_RECT;
  const auto viewport = context->World()->WorldInfo.Camera.Viewport;
  const auto& worldSize = RectSize(viewport);
  const auto& box = playerBlock->Box;
  const auto& cellSize = context->World()->WorldInfo.Camera.CellSize;
  do
  {
    const auto dx = cellSize.x * 40;
    const auto dy = cellSize.y * 7;
    // Favor boxes around the area of the active player's original box (more so vertically above ground).
    const Rectangle newBox = {
      Random(box.x - dx, box.x + dx, cellSize.x), Random(box.y - (dy / 2), box.y + (dy), cellSize.y), box.width,
      box.height
    };
    if (!DoesRectContainRect(viewport, newBox)) { continue; }
    bool foundMatch = true;
    bool hasSolidBlockUnderneath = false;
    for (const auto& [_, block] : context->GetBlocks())
    {
      if (HasEnumValue(block->Traits, TBlockTraits::Cosmetic)) continue;
      if (block->GetBlockId() == playerBlock->GetBlockId()) continue;
      if (CheckCollisionRecs(newBox, block->Box))
      {
        foundMatch = false;
        break;
      }

      if (!hasSolidBlockUnderneath)
      {
        if (HasEnumValue(block->Traits, TBlockTraits::Solid) && !HasEnumValue(block->Traits, TBlockTraits::GoalBlock))
        {
          // A solid block is below the new player box in Y but not too far down the player block so they don't end up falling too much:
          if (newBox.y + newBox.height + (cellSize.y / 2) <= block->Box.y && block->Box.y <= newBox.y + newBox.height
            + (cellSize.y * 4))
          {
            // A solid block is correctly below the new player box in X and Y.
            if ((newBox.x >= block->Box.x) && (newBox.x <= block->Box.x + block->Box.width))
            {
              hasSolidBlockUnderneath = true;
            }
          } // if (newBox is above block...)
        }   // if (HasEnumValue(block->Traits...))
      }     // if (HasEnumValue(block->Traits...))
    }       // for (block...)

    if (foundMatch && hasSolidBlockUnderneath)
    {
      return newBox;
    }
  } // do
  while (true);
}

void TBlockUtils::SetPlayerStartPos(TContextPtr context, const Rectangle& box)
{
  if (context->GetGameProgress()->GetGameState() != TGameState::StartGame) return;
  context->UpdateActivePlayerPos(RectTopLeft(box));
}
}
