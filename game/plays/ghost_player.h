#pragma once
#include <serialize.h>

#include <actions.h>
#include <base_block.h>
#include <context.h>
#include <block_utils.h>
#include <timer.h>
#include "game_progress.h"
#include <sstream>
#include <iomanip>
#include <game_types.h>

#include "raymath.h"
#include "scenes.h"

namespace RLPlays
{
struct TGhostActions
{
  std::shared_ptr<TGameActions> Actions;
  Vector2 LastPos = {0, 0};
};

// Show non-interactive sampled ghost(s) from recorded gameplay actions for the given map/level/round.
// Thse may come from a prior RL training recording (sampled from the 'good' runs) or from
// recorded actions from human players.
struct TGhostPlayerBlocks
{
  void Init(TContextPtr context, TBlockTraits traits)
  {
    if (context->GetDebugInfo().ShowGhostActions)
    {
      isGhostBlockEnabled_ = HasEnumValue(traits, TBlockTraits::MainPlayerBlock) && context->GetGameProgress()->GameType
          == TGameType::SinglePlayer;
    }
    else { isGhostBlockEnabled_ = false; }
  }


  void LoadContent(TContextPtr context)
  {
    if (!CheckGhost_(context)) return;

    auto matchingFilename = context->GetWorldFilename();
    auto slashPos = matchingFilename.find_last_of('/');
    if (slashPos != std::string::npos) { matchingFilename = matchingFilename.substr(slashPos + 1); }
    std::stringstream buffer;
    { // Scope to read file into the buffer.
      std::string recordedFilename = RECORDED_FILENAME;
      auto gamePos = recordedFilename.find("game/");
      // Remove 'game' but keep the trailing slash (if 'game/' is found).
      if (gamePos != std::string::npos) { recordedFilename = recordedFilename.substr(gamePos + 4); }
      std::ifstream file(GetRootGameDir() + recordedFilename);
      if (!file.is_open()) { return; }
      buffer << file.rdbuf();
      file.close();
    }


    auto recordedActions = TGameActions::FromSerializedList(buffer, matchingFilename);
    // If this is from a training run, then the best actions are usually at the end (as the perf/score increases over
    // time).
    //for (int i = rand()%recordedActions.size(); i < recordedActions.size(); i++)
    for (int i = recordedActions.size() - 1; i >= 0; i--)
    {
      auto& a = recordedActions[i];
      recordedActions_.push_back(TGhostActions{a, INVALID_POS});
      // TODO(perumaal): Also filter for 'best rewards' or 'interesting play' - especially when replaying
      //                 other 'human players' (not RL agent which is too perfect).
      if (recordedActions_.size() >= MAX_RECORDED_ACTIONS_) { break; }
    }
  }


  void Draw(TContextPtr context, TSpriteSheet& sprite)
  {
    if (!CheckGhost_(context) || recordedActions_.empty()) return;

    TPlayerActions playerActions;
    Vector2 pos;
    Vector2 cellSize = context->GetCamera().CellSize;
    const auto cellDist = Vector2LengthSqr(cellSize);
    for (int i = 0; i < recordedActions_.size(); ++i)
    {
      auto& g = recordedActions_[i];
      if (g.Actions->GetAction(context->Frame(), &playerActions))
      {
        Vector2 p = {playerActions.Position.x, playerActions.Position.y};
        if (IsInvalidVector(g.LastPos) || Vector2DistanceSqr(p, g.LastPos) < cellDist)
        {
          DrawGhost_(i, p, context, sprite, 128);
        }
        else
        {
          for (float t = 0; t <= 1.0f; t += 0.05f)
          {
            Vector2 np = Vector2Lerp(p, g.LastPos, t);
            DrawGhost_(i, np, context, sprite, static_cast<unsigned char>(t * 128));
          }
        }
        g.LastPos = p;
      }
    }
  }

private:
  void DrawGhost_(const int index, Vector2& p, TContextPtr context, TSpriteSheet& sprite, const unsigned char alpha)
  {
    Rectangle dest = {
        p.x, p.y, static_cast<float>(sprite.SpriteSize.x),
        static_cast<float>(sprite.SpriteSize.y)
    };
    const unsigned char r = ((index % 255) % 2) * 128;
    const unsigned char g = ((index % 63) % 2) * 128;
    const unsigned char b = ((index % 127) % 2) * 128;
    context->DrawAnimSprite(sprite, dest, {r, g, b, alpha});
  }

  bool CheckGhost_(TContextPtr context)
  {
    if (!isGhostBlockEnabled_) return false;
    if (!context->GetDebugInfo().ShowGhostActions)
    {
      isGhostBlockEnabled_ = false;
      return false;
    }
    return true;
  }

  bool isGhostBlockEnabled_ = false;
  std::vector<TGhostActions> recordedActions_;
  // const char* RECORDED_FILENAME = "/private/rec_actions/recorded_actions.txt";
  // const char* RECORDED_FILENAME = "/private/logs/rl_2025_10_10_11_39.json.txt"; // Very good run for demo purposes.
  // const char* RECORDED_FILENAME = "/private/logs/rl_2025_10_10_20_55.json.txt"; // "Perfect" self-run with no weapons/enemies - simple level.
  // const char* RECORDED_FILENAME = "/private/logs/rl_2025_11_11_13_30.json.txt";  One of the best runs: Syllabus training in full display (rlplays_level1-4, nov2_enemies, lots_enemies, pg_blocked7, mirror)
  const char* RECORDED_FILENAME = "game/private/logs/rl_2026_03_24_11_39.json.txt";

  static constexpr int MAX_RECORDED_ACTIONS_ = 50;
};
} // namespace RLPlays
