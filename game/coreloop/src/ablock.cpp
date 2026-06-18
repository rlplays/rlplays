#include <base_types.h>
#include <world.h>

namespace RLPlays
{
void ABlock::AddBlock(TContextPtr context, const TBlock& block)
{
  // A subclass could (for instance) add multiple blocks to the context/grid.
  // Hence, we don't just add the list directly but let the block call us.
  context->AddBlock(block);
  if (block.Block->BlockId == InvalidBlockId) {}
}
#ifdef DEBUG
int ABlock::DEBUG_BLOCK_ID = 0;
#endif
} // namespace RLPlays
