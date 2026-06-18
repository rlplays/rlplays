#include <actions.h>
#include <base_block.h>
#include <base_types.h>
#include <context.h>
#include <cstdint>
#include <memory>
#include <serialize.h>

namespace RLPlays
{
int ABlock::GetTotalCells(TContextPtr context)
{
  return ((int)Box.width / (int)context->GetCamera().CellSize.x) * 
    ((int)Box.height / (int)context->GetCamera().CellSize.y);
}

#if RLPLAYS_EDITOR
void ABlock::EditorDraw(const std::shared_ptr<TEditorData>& editorData, TContextPtr context)
{
  if (editorData->selectedBlockIds.find(GetBlockId()) != editorData->selectedBlockIds.end())
  {
    ::DrawRectangleLinesEx(Box, 10.0f, {0, 255, 0, 128});
  }
  else
  {
    ::DrawRectangleLinesEx(Box, 6.0f, {0, 255, 0, 128});
  }
}
#endif
} // namespace RLPlays
