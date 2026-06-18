#pragma once
#if RLPLAYS_EDITOR

#include <base_types.h>
#include <memory>
#include <tblock.h>

#include <world.h>
#include <world_editor.h>

namespace RLPlays
{
enum class TEditorChangeType : uint8_t
{
  None        = 0,
  MoveBlock   = 1,
  ResizeBlock = 2,
  DeleteBlock = 3,
  CloneBlock  = 4,
};

struct TEditor
{
  void DrawEditor(TContextPtr context);
  void HandleEditor(TGameInfo& gameInfo);
  bool ShowEditor = false;
  Vec2i nextPos_;

  void Reset();
  TEditor(const TEditor&& that) = delete;

  TEditor();
  ~TEditor();
  static std::shared_ptr<TEditor> Editor;
  TGameInfo GetGameInfo() const { return gameInfo_; }
private:
  [[nodiscard]] std::string GetBlockName_(std::string altName, const std::shared_ptr<ABlock>& block) const;
  void HandleMirrorMode(TContextPtr context, TWorld& world);
  bool ShowBlockEditor(TContextPtr context, TWorld& world, TBlock& block);
  void DrawMainButtons_(TContextPtr context);
  bool ShowPrimitiveEditor_(json& data, nlohmann::detail::iter_impl<nlohmann::basic_json<>> it);
  bool ShowObjectEditor_(TContextPtr context, const TBlock* block, json& data, int depth = 0);
  bool ShowWorldEditor_(TContextPtr context, TWorld& world);
  void UpdateBlock_(TContextPtr context, const TBlock& updatedBlock, int blockId, TBlockType blockType);
  void DeleteBlock_(TContextPtr context, const TBlockId& blockId);
  void AddBlock_(TContextPtr context, const TBlock& block);

  std::vector<TBlock::TReflection_ABlock> blockDefs_;
  TTemplateBlocks templateBlocks_;
  std::shared_ptr<TEditorData> editorData_;
  TEditorChangeType editorChange_ = TEditorChangeType::None;
  const Vec2i DEFAULT_POS = {-10000, -10000};
  bool halfCellSnapH_ = false;
  bool halfCellSnapV_ = false;
  bool isImGUISetup_ = false;
  bool reloadGame_ = false;
  std::shared_ptr<TWorldFiles> worldFiles_;
  int editorId_ = 0;
  int invalidatedCount_ = 0;
  TGameInfo gameInfo_;
  std::shared_ptr<TGame> GetGame() { return gameInfo_.Game; }
};


void SetupEditor();

void UnloadEditor();

TGameInfo HandleEditor(TGameInfo& gameInfo);

bool IsEditorOpen();

// Defined in editor_utils.cpp
extern int editorId_;
extern int invalidatedCount_;
} // namespace RLPlays
#endif
