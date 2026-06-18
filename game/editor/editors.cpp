#if RLPLAYS_EDITOR
#include "game_actions.h"
#include <algorithm>
#include <base_editor.h>
#include <imgui.h>
#include <main_game.h>
#include <raylib.h>
#include <tblock.h>
#include <world.h>
//! @brief Include the editor utils (which is simply separated out to avoid confusion, hence cpp instead of h).


namespace RLPlays
{
using namespace nlohmann;

bool TEditor::ShowBlockEditor(TContextPtr context, TWorld& world, TBlock& block)
{
  bool changed = false;
  if (!AreVectorsSame(nextPos_, DEFAULT_POS))
  {
    auto cellSize = world.WorldInfo.Camera.CellSize;
    Vector2 snapSize = {cellSize.x / (halfCellSnapH_ ? 2.0f : 1.0f), cellSize.y / (halfCellSnapV_ ? 2.0f : 1.0f)};
    Vector2 pos = {TO_INT(nextPos_.x / snapSize.x) * snapSize.x, TO_INT(nextPos_.y / snapSize.y) * snapSize.y};
    Rectangle newBox = block.Block->Box;
    if (editorChange_ == TEditorChangeType::MoveBlock)
    {
      newBox.x = pos.x;
      newBox.y = pos.y;
    }
    else if (editorChange_ == TEditorChangeType::ResizeBlock)
    {
      Vector2 newSize = {pos.x - block.Block->Box.x, pos.y - block.Block->Box.y};
      newSize = {TO_INT(newSize.x / snapSize.x) * snapSize.x, TO_INT(newSize.y / snapSize.y) * snapSize.y};
      newSize.x = std::max(newSize.x, cellSize.x);
      newSize.y = std::max(newSize.y, cellSize.y);
      newBox.width = newSize.x;
      newBox.height = newSize.y;
    }
    if (!AreRectsSame(newBox, block.Block->Box))
    {
      block.Block->Box = newBox;
      changed = true;
    }
  }

  json data(block);

  changed |= ShowObjectEditor_(context, &block, data);
  if (changed)
  {
    auto changedBlock = data.template get<TBlock>();
    UpdateBlock_(context, changedBlock, block.Block->GetBlockId(), block.BlockType);
    // SaveGame();
  }
  else
  {
    if (editorChange_ == TEditorChangeType::DeleteBlock)
    {
      DeleteBlock_(context, block.Block->GetBlockId());
      changed = true;
    }
    else if (editorChange_ == TEditorChangeType::CloneBlock)
    {
      TBlock clonedBlock = CopyBlock(block);
      if (clonedBlock.Block->Box.height > clonedBlock.Block->Box.width)
      {
        clonedBlock.Block->Box.x += clonedBlock.Block->Box.width;
      }
      else
      {
        clonedBlock.Block->Box.y += clonedBlock.Block->Box.height;
      }
      AddBlock_(context, clonedBlock);
      editorData_->selectedBlockIds = {clonedBlock.Block->GetBlockId()};
      changed = true;
    }
  }

  if (changed)
  {
    reloadGame_ = true;
  }

  return changed;
}

void TEditor::HandleMirrorMode(TContextPtr context, TWorld& world)
{
  // Mirror all left-side blocks to the right side, and remove all prior blocks that are on the right side first.
  const auto& blocks = world.Blocks;
  std::vector < TBlockId > toRemove;
  std::vector < TBlock > mirroredBlocks;
  if (blocks.size() >= 150)
  {
    TLOG(TINFO, "Too many blocks (%d) to mirror, skipping.", blocks.size());
  }
  for (const auto& block : blocks)
  {
    if (block.Block == nullptr || !block.Block->IsCandidateForMirroring(context)) { continue; }

    if (world.ShouldMirrorBlock(block))
    {
      // If the block is on the right side, we need to mirror it.
      auto mirroredBlock = CopyBlock(block);
      mirroredBlock.Block->Box.x =
          world.WorldInfo.Camera.Viewport.width - mirroredBlock.Block->Box.x - mirroredBlock.Block->Box.width;
      mirroredBlock.Block->EditorEnsureMirror(editorData_, GetGame()->Context, block);
      mirroredBlocks.push_back(mirroredBlock);
    }
    else
    {
      // Check for blocks that may start on the left side but protrude past the halfway point.
      const auto halfWidth = world.WorldInfo.Camera.Viewport.width / 2.0f;
      if (block.Block->Box.x >= halfWidth && Right(block.Block->Box) > halfWidth)
      {
        // If the block is *strictly* on the right side (by mistake or from a previous mirror clone), we need to first remove it.
        toRemove.push_back(block.Block->GetBlockId());
      }
    }
  }

  // Remove the blocks now.
  for (const auto& blockId : toRemove)
  {
    DeleteBlock_(context, blockId);
  }

  // Add the mirrored blocks.
  for (const auto& mirrorBlock : mirroredBlocks)
  {
    AddBlock_(context, mirrorBlock);
  }
}


void TEditor::DrawMainButtons_(TContextPtr context)
{
  auto game = GetGame();
  if (!ShowEditor || game == nullptr || game->World == nullptr)
    return;

  editorId_ = 0;
  auto world = game->World;

  static bool isWorldEditorOpen_;
  static bool isBlocksEditorOpen_;
  static bool isBlockEditorOpen_;
  static bool isWorldsEditorOpen_;

  ImGui::SetNextWindowPos(ImVec2(10, 20), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Worlds", &isWorldsEditorOpen_,
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding |
    ImGuiWindowFlags_NoFocusOnAppearing))
  {
    ImGui::SetWindowCollapsed(true, ImGuiCond_FirstUseEver);
    invalidatedCount_ = 0;
    std::map<std::string, std::vector<std::string>> dirFiles;
    for (const auto& worldFile : worldFiles_->Files)
    {
      auto file = worldFile.Filename;
      auto slashPos = file.find_last_of("/");
      if (slashPos != std::string::npos)
      {
        dirFiles[file.substr(0, slashPos)].push_back(file);
      }
      else { dirFiles["."].push_back(file); }
    }

    for (auto& [dir, files] : dirFiles)
    {
      auto shouldOpen = (dir.find("levels") == 0) || (dir.find("playground") == 0);
      // !((dir.find("test") == 0) || (dir.find("archive/") == 0));
      if (ImGui::TreeNodeEx(dir.c_str(), shouldOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_OpenOnArrow))
      {
        for (auto file : files)
        {
          const auto selected = (worldFiles_->SelectedFile.Filename == file) ? "Selected " : "Load ";

          const auto slashPos = file.find_last_of("/");
          auto displayFile = file;
          if (slashPos != std::string::npos) { displayFile = file.substr(slashPos + 1); }
          if (ImGui::Button((selected + displayFile).c_str()))
          {
            worldFiles_->SelectedFile = {file};
            worldFiles_->Save();
            TGameLoadInfo loadInfo = {.Filename = file};
            gameInfo_ = LoadGame(*worldFiles_, loadInfo);
            SaveGame(gameInfo_);
          }
        }
        ImGui::TreePop();
      }
    }
  }
  ImGui::End();
  if (ImGui::Begin("World", &isWorldEditorOpen_,
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding |
    ImGuiWindowFlags_NoFocusOnAppearing))
  {
    ImGui::SetWindowCollapsed(true, ImGuiCond_FirstUseEver);
    invalidatedCount_ = 0;
    if (ShowWorldEditor_(context, *world))
    {
      reloadGame_ = true;
    }
  }
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(10, 50), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Blocks", &isBlocksEditorOpen_,
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding))
  {
    for (auto& templateBlock : templateBlocks_.Blocks)
    {
      auto& block = templateBlock.Block;
      auto blockName = GetBlockName_(templateBlock.Name, block->Block);
      if (templateBlock.IsFromFile)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));        // Normal
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f)); // Hovered
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));  // Active/Pressed
      }
      if (ImGui::Button(blockName.c_str()))
      {
        auto newTBlock = CopyBlock(*block);
        auto newBlock = world->AddBlock(newTBlock);
        auto& box = newBlock->Block->Box;
        if (box.width <= 100 && box.height <= 100)
        {
          box.x = world->WorldInfo.Camera.Viewport.width / 2;
          box.y = world->WorldInfo.Camera.Viewport.height / 2;
        }
        reloadGame_ = true;
        editorData_->selectedBlockIds = {newBlock->Block->GetBlockId()};

        // ShowBlockEditor(context, *world, *newBlock);
      }
      if (templateBlock.IsFromFile) { ImGui::PopStyleColor(3); }
    }

    ImGui::Separator();
    ImGui::Spacing();
    auto selectedFilename = world->WorldInfo.Filename;
    char buffer[256] = {0};
    if (ImGui::InputText(selectedFilename.c_str(), buffer, sizeof(buffer)) && ImGui::IsItemDeactivatedAfterEdit())
    {
      selectedFilename = buffer;
      // Ensure the filename has a .json extension
      if (!selectedFilename.empty() && selectedFilename.find(".json") == std::string::npos)
      {
        selectedFilename += ".json";
      }

      world->WorldInfo.Filename = selectedFilename;
      world->WorldInfo.Filename = selectedFilename;
    }
    ImGui::Separator();
    if (ImGui::Button("Save World"))
    {
      SaveGame(gameInfo_);
      worldFiles_->SelectedFile = {selectedFilename};
      worldFiles_->Save();
    }
    if (ImGui::Button("Load World"))
    {
      worldFiles_->SelectedFile = {selectedFilename};
      worldFiles_->Save();
      TGameLoadInfo loadInfo = {.Filename = selectedFilename};
      gameInfo_ = LoadGame(*worldFiles_, loadInfo);
    }
    if (world->WorldInfo.GameProgress.HasMirrorMode)
    {
      if (ImGui::Button("Mirror World"))
      {
        HandleMirrorMode(context, *world);
        reloadGame_ = true;
        SaveGame(gameInfo_);
      }
    }
  }
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(GetScreenWidth() - 250.0f, 20), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Block", &isBlockEditorOpen_,
    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding))
  {
    // Copy the selected blocks as the editor might change the selected blocks.
    // (Usually, just 1 or at most a few selected blocks - so it's okay to copy these).
    const auto selectedBlocks = editorData_->selectedBlockIds;
    for (const auto& blockId : selectedBlocks)
    {
      auto block = GetGame()->Context->GetBlock(blockId);
      if (block == nullptr) { continue; }
      auto blockName = GetBlockName_("", block);
      ImGui::Text("%s (id %d)", blockName.c_str(), block->GetBlockId());
      if (ImGui::TreeNodeEx(blockName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
      {
        TBlock b = {block->BlockType, block};
        ShowBlockEditor(context, *world, b);
        ImGui::TreePop();
      }
    }
  }
  ImGui::End();
}

void SetupEditor()
{
  if (TEditor::Editor == nullptr) { TEditor::Editor = std::make_shared<TEditor>(); }
}

void UnloadEditor()
{
  if (TEditor::Editor != nullptr)
  {
    TEditor::Editor->Reset();
    TEditor::Editor = nullptr;
  }
}

TGameInfo HandleEditor(TGameInfo& gameInfo)
{
  SetupEditor();
  if (TEditor::Editor != nullptr)
  {
    TEditor::Editor->HandleEditor(gameInfo);
    // TODO: This is the only remaining unnecessary alloc in the game in editor mode. It's fine for now as it's a few bytes,
    //       but we don't need to keep calling this per-frame!
    if (TEditor::Editor != nullptr) { return TEditor::Editor->GetGameInfo(); }
  }
  return gameInfo;
}

bool IsEditorOpen() { return (TEditor::Editor != nullptr && TEditor::Editor->ShowEditor); }

void TContext::MirrorBlockViaEditor(const std::shared_ptr<TEditorData>& editorData, TContextPtr context,
  const TBlock& block, ABlock* mirroredBlock)
{
  if (TEditor::Editor == nullptr) return;
  const auto& world = *context->World();
  if (world.ShouldMirrorBlock(block))
  {
    // If the block is on the right side, we need to mirror it.
    mirroredBlock->Box.x =
        world.WorldInfo.Camera.Viewport.width - mirroredBlock->Box.x - mirroredBlock->Box.width;
    mirroredBlock->EditorEnsureMirror(editorData, context, block);
  }
  else
  {
    // Check for blocks that may start on the left side but protrude past the halfway point.
    const auto halfWidth = world.WorldInfo.Camera.Viewport.width / 2.0f;
    // TODO(perumaal): What should we do about sub-blocks that are on the right side?
  }
}

std::shared_ptr<TEditor> TEditor::Editor = nullptr;
} // namespace RLPlays
#endif
