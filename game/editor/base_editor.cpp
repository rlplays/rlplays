#if RLPLAYS_EDITOR
#include <base_editor.h>
#include <block_templates.h>
#include <main_game.h>
#include <world.h>
#include "imgui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"
#include "rlImGuiColors.h"

namespace RLPlays
{
// DPI scaling functions
float ScaleToDPIF(float value) { return GetWindowScaleDPI().x * value; }

int ScaleToDPII(int value) { return int(GetWindowScaleDPI().x * value); }


void TEditor::DrawEditor(TContextPtr context)
{
  if (!ShowEditor) { return; }
  const auto game = GetGame();
  if (game == nullptr) { return; }

  rlImGuiBegin();
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
  {
    auto pos = GetMousePosition();
    // Select the topmost block (or all blocks under the cursor, if shift key is pressed).
    editorData_->selectedBlockIds = game->Context->GetSelectedObjects(pos, /* onlySelectOne*/ !IsShiftKeyDown());
  }
  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
  {
    const auto pos = GetMousePosition();
    const auto worldPos = game->Context->GetWorldPosFromScreenPos(pos);
    nextPos_ = FromVector(worldPos);
    if ((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
    {
      editorChange_ = TEditorChangeType::MoveBlock;
    }
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
    {
      editorChange_ = TEditorChangeType::ResizeBlock;
    }
  }
  else
  {
    editorChange_ = TEditorChangeType::None;
    if (IsControlKeyDown() && IsKeyReleased(KEY_D)) { editorChange_ = TEditorChangeType::DeleteBlock; }
    else { nextPos_ = DEFAULT_POS; }
    if (IsControlKeyDown() && IsKeyReleased(KEY_C)) { editorChange_ = TEditorChangeType::CloneBlock; }
  }

  if (IsShiftKeyDown() && IsKeyReleased(KEY_H)) { halfCellSnapH_ = !halfCellSnapH_; }
  if (IsShiftKeyDown() && IsKeyReleased(KEY_V)) { halfCellSnapV_ = !halfCellSnapV_; }

  // (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)))
  // ImGui::ShowDemoWindow(&open);
  DrawMainButtons_(context);

  // if (ImGui::Begin("Test Window", &open))
  //{
  //   ImGui::TextUnformatted(ICON_FA_JEDI);
  //
  // }
  // ImGui::End();
  rlImGuiEnd();
}

void TEditor::HandleEditor(TGameInfo& gameInfo)
{
  gameInfo_ = gameInfo;
  if (GetGame() == nullptr || GetGame()->Context == nullptr)
  {
    return;
  }

  auto lkgShowEditor = ShowEditor;
  // Press F6 or CTRL+T;
  if (IsKeyReleased(KEY_F6) || (IsControlKeyDown() && IsKeyReleased(KEY_T)))
  {
    ShowEditor = !ShowEditor;
    if (ShowEditor)
    {
      UnloadGame(gameInfo);
      ReloadGame(gameInfo, DEFAULT_FPS);
      if (!isImGUISetup_)
      {
        rlImGuiSetup(true);
        isImGUISetup_ = true;
      }
    }
    else
    {
      SaveGame(gameInfo);
      UnloadEditor();
      gameInfo.Game->Context->UpdateGameProgress()->SetGameState(TGameState::StartGame);
      // MUST RETURN HERE, AS THE EDITOR INSTANCE IS GONE.
      return;
    }
  }

  if (ShowEditor)
  {
    // We have to re-get context here as may have reloaded the game/context etc.
    gameInfo_ = gameInfo;
    const auto context = GetGame()->Context;
    if (lkgShowEditor != ShowEditor)
    {
      // Init template blocks just before the editor restarts.
      blockDefs_ = TBlock::GetReflection();
      worldFiles_ = TWorldFiles::Load("", /* useCached */ false); // We might add a new file to the list.
      CreateTemplates(context, templateBlocks_, *worldFiles_);
    }
    context->DrawEditor(editorData_, context);
    context->UpdateGameProgress()->SetGameState(TGameState::EditorMode);
    DrawEditor(context);
  }

  if (reloadGame_)
  {
    ResetGame(gameInfo);
    reloadGame_ = false;
  }
}


void TEditor::Reset() { editorData_ = std::make_shared<TEditorData>(); }

TEditor::TEditor()
{
  editorData_ = std::make_shared<TEditorData>();
}

TEditor::~TEditor()
{
  if (isImGUISetup_)
  {
    rlImGuiShutdown();
    isImGUISetup_ = false;
  }
}

std::string TEditor::GetBlockName_(std::string altName, const std::shared_ptr<ABlock>& block) const
{
  for (auto& def : blockDefs_)
  {
    if (def.BlockType == block->BlockType)
    {
      if (altName.empty()) { return def.TypeName; }
      return altName;
    }
  }
  return "<unknown>";
}
} // namespace RLPlays

#endif
