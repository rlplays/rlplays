#include <serialize.h>

#include <actions.h>
#include <base_block.h>
#include <context.h>
#include <activator_blocks.h>
#include <tblock.h>

namespace RLPlays
{
#if RLPLAYS_EDITOR

void TSwitchActivatorBlock::EditorEnsureMirror(
  const std::shared_ptr<TEditorData>& editorData, TContextPtr context, const TBlock& originalBlock)
{
  if (ActivatorBlock == nullptr) return;
  std::swap(ActiveTex, InactiveTex);
  context->MirrorBlockViaEditor(editorData, context, *originalBlock.GetABlockWithType<TSwitchActivatorBlock>()->
                                                               ActivatorBlock, this->ActivatorBlock->Block.get());
}
#endif
}
