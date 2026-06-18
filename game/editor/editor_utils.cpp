#ifdef RLPLAYS_EDITOR
#include <algorithm>
#include <base_editor.h>
#include <tblock.h>
#include <world.h>

#include <raylib.h>
#include <imgui.h>

// Contains all generic editor functions - to not clog the editors.cpp file.
namespace RLPlays
{
using namespace nlohmann;

bool TEditor::ShowPrimitiveEditor_(json& data, basic_json<>::iterator it)
{
  bool changed = false;
  auto key = it.key().c_str();
  if (it->is_number_float())
  {
    float x;
    it.value().get_to(x);
    ImGui::PushID(++editorId_);
    if (ImGui::InputFloat(key, &x) && ImGui::IsItemDeactivatedAfterEdit())
    {
      data[key] = x;
      changed = true;
    }
    ImGui::PopID();
  }
  else if (it->is_number_integer())
  {
    long long x;
    it.value().get_to(x);
    ImGui::PushID(++editorId_);
    int x1 = static_cast<int>(x);
    if (ImGui::InputInt(key, &x1) && ImGui::IsItemDeactivatedAfterEdit())
    {
      data[key] = (long long)x1;
      changed = true;
    }


    if (!changed && it.key().find("TimeSet") != std::string::npos)
    {
      auto keyMs = it.key() + " (Ms)";
      int xMs = static_cast<int>(MillisFromNanos(x));
      ImGui::SameLine();
      if (ImGui::InputInt(keyMs.c_str(), &xMs) && ImGui::IsItemDeactivatedAfterEdit())
      {
        data[key] = (long long)NanosFromMillis(xMs);
        changed = true;
      }
    }
    ImGui::PopID();
  }
  else if (it->is_string())
  {
    std::string str;
    it.value().get_to(str);
    char buffer[256];
    strncpy(buffer, str.c_str(), sizeof(buffer));
    buffer[sizeof(buffer) - 1] = '\0';
    ImGui::PushID(++editorId_);
    if (ImGui::InputText(it.key().c_str(), buffer, sizeof(buffer)) && ImGui::IsItemDeactivatedAfterEdit())
    {
      data[key] = buffer;
      changed = true;
    }
    ImGui::PopID();
  }
  else if (it->is_boolean())
  {
    bool b;
    it.value().get_to(b);
    ImGui::PushID(++editorId_);
    if (ImGui::Checkbox(it.key().c_str(), &b))
    {
      data[key] = b;
      changed = true;
    }
    ImGui::PopID();
  }
  return changed;
}

bool TEditor::ShowObjectEditor_(TContextPtr context, const TBlock* block, json& data, int depth)
{
  bool changed = false;
  if (!data.is_object())
  {
    ImGui::Text("ERROR: %s is a primitive", data.dump().c_str());
    return changed;
  }
  for (auto it = data.begin(); it != data.end(); ++it)
  {
    if (!it->is_object())
    {
      if (ShowPrimitiveEditor_(data, it))
      {
        changed = true;
      }
      continue;
    }
    ImGui::PushID(++editorId_);

    // ImGui::Text("%s: ", it.key().c_str());
    if (ImGui::TreeNodeEx("%s_%d", depth < 2 ? ImGuiTreeNodeFlags_DefaultOpen : 0, it.key().c_str(), ++editorId_))
    {
      if (ShowObjectEditor_(context, block, data[it.key()], depth + 1))
      {
        changed = true;
      }
      ImGui::TreePop();
    }

    ImGui::PopID();
  }
  return changed;
}

// Add this after the template function definition
bool TEditor::ShowWorldEditor_(TContextPtr context, TWorld& world)
{
  bool changed = false;
  json data(world.WorldInfo);
  changed |= ShowObjectEditor_(context, nullptr, data);
  if (changed)
  {
    world.WorldInfo = data.template get<TWorldInfo>();
  }
  return changed;
}


//! @brief Updates the block either as a sub-block (with an ancestor tree) or as a top-level block in the root World object.
void TEditor::UpdateBlock_(TContextPtr context, const TBlock& updatedBlock, const int blockId,
                           const TBlockType blockType)
{
  auto parentBlockInfo = context->GetParentBlockInfo(blockId);
  auto world = context->World();
  auto childBlock = updatedBlock;
  while (parentBlockInfo != nullptr)
  {
    const auto parentBlockId = parentBlockInfo->BlockId;
    auto parentBlock = context->GetBlock(parentBlockId);
    parentBlockInfo->UpdateBlockFn(std::make_shared<TBlock>(childBlock));

    parentBlockInfo = context->GetParentBlockInfo(parentBlockId);
    if (parentBlockInfo != nullptr)
    {
      const auto blockPtr = world->GetBlockWithIdForEditor(parentBlockId);
      if (blockPtr != nullptr) { childBlock = *blockPtr; }
      else
      {
        TLOG(LOG_ERROR, "Invalid block id %d for parent starting with original child block %d",
             parentBlockId, blockId);
      }
    }
  }
  world->UpdateBlock(childBlock, blockId, blockType);
}

void TEditor::DeleteBlock_(TContextPtr context, const TBlockId& blockId)
{
  auto parentBlockInfo = context->GetParentBlockInfo(blockId);
  auto world = context->World();
  if (parentBlockInfo == nullptr) { world->DeleteBlock(blockId); }
  else { TLOG(LOG_INFO, "Deleting sub-blocks not supported yet."); }
}

void TEditor::AddBlock_(TContextPtr context, const TBlock& block)
{
  auto parentBlockInfo = context->GetParentBlockInfo(block.Block->GetBlockId());
  auto world = context->World();
  if (parentBlockInfo == nullptr) { world->AddBlock(block); }
  else { TLOG(LOG_INFO, "Adding sub-blocks not supported yet."); }
}
}

#endif
