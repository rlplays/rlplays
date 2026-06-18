#pragma once
#include <memory>

#include <string>
#include <actions.h>

#include <serialize.h>
#include <set>
#include "log.h"
#include <cassert>

#include <raylib_utils.h>
#include "game_types.h"
#include <fstream>
#include <sstream>

namespace RLPlays
{
//! @brief Player actions stored and indexed by frame index.
struct TPlayerFrameActions
{
  // TODO(perumaal): Frame index is absolute, but for all practical purposes, it should be relative from the previous frame.
  //                 A minor optimization, needed when we have lots of frames beyond the 18 min mark. We have to be careful here
  //                 though, as lots of empty frames is currently optimized well as we can fast-forward with very few stored bits...
  uint16_t FrameIndex = 0;
  uint8_t Action = 0;
  uint8_t RunLength = 0;
  Vector2 Position = {0, 0};

  inline uint32_t ActionsToUInt32() const
  {
    return uint32_t(FrameIndex) << 16 | (uint32_t(Action) << 8) | uint32_t(RunLength);
  }

  static inline uint32_t PositionToUInt32(const Vector2& p)
  {
    return (uint16_t(p.x) & 0xFFFF) << 16 | (uint16_t(p.y) & 0xFFFF);
  }

  static inline Vector2 UInt32ToPosition(const uint32_t pos)
  {
    return {float((pos >> 16) & 0xFFFF), float(pos & 0xFFFF)};
  }

  void AddPosition(const Vector2& pos) { Position = pos; }

  static TPlayerFrameActions ParseFrame(const uint32_t action, const uint32_t pos)
  {
    TPlayerFrameActions actions;
    actions.FrameIndex = (action >> 16) & 0xFFFF;
    actions.Action = (action >> 8) & 0xFF;
    actions.RunLength = action & 0xFF;
    actions.Position = UInt32ToPosition(pos);
    return actions;
  }

  static bool IsEmptyAction(const TPlayerAction actions) { return actions == TPlayerAction::None; }
  bool Contains(const int frame) const { return FrameIndex <= frame && (FrameIndex + RunLength) >= frame; }
};

//! @brief Stored game actions indexed by frame index.
//! @note Uses a sparse array of frames. This is allocated during gameplay and is perhaps the only
//!       important allocation per-frame - it's optimized to store/retrieve a serialized, sparse actions using RLE.
struct TGameActions
{
  std::vector<TPlayerFrameActions> FrameActions;
  int PlayerId = 0;
  int CurrentRound = 0;
  float Score = -1;
  std::string Filename = "";
  uint32_t Version = 0;

  // Filled in v1.4.0
  Vector2 StartPosition = INVALID_POS;
  
  // NOTE: If you add anything here, make sure it's version compatible, and add 
  // relevant copying code to TGameActions::Copy in context.cpp.

  static std::shared_ptr<TGameActions> FromSerialized(const std::string& str)
  {
    auto actions = std::make_shared<TGameActions>();
    TSerializerOutput v;
    actions->Version = ParseUInt32(str, v);
    if (actions->Version > TVersion::BlockVersion) { return nullptr; }
    actions->CurrentRound = ParseInt32(str, v);
    actions->Score = ParseFloat(str, v);
    actions->Filename = DecodeStringWithLen(str, v);
    actions->PlayerId = ParseInt32(str, v);
    if (actions->Version > TVersion::BlockVersion_1_4_0)
    {
      actions->StartPosition = TPlayerFrameActions::UInt32ToPosition(ParseUInt32(str, v));
    } else { actions->StartPosition = INVALID_POS; }
    int numFrames = ParseInt32(str, v);
    while (numFrames-- > 0)
    {
      const auto action = ParseUInt32(str, v);
      const auto pos = ParseUInt32(str, v);
      actions->FrameActions.push_back(TPlayerFrameActions::ParseFrame(action, pos));
    }

    return actions;
  }

  // Co-written by Claude.
  std::string GetSerialized()
  {
    std::string s;
    s.reserve(FrameActions.size() * 12 + 50);

    Version = TVersion::BlockVersion;
    UInt32ToString(Version, s) += ",";
    s += std::to_string(CurrentRound) + ",";
    s += std::to_string(Score) + ",";
    EncodeStringWithLen(Filename, s) += ",";
    s += std::to_string(PlayerId) + ",";
    UInt32ToString(TPlayerFrameActions::PositionToUInt32(StartPosition), s) += ",";
    s += std::to_string(FrameActions.size()) + ",";
    const int size = FrameActions.size();
    for (int i = 0; i < size; ++i)
    {
      const auto& frame = FrameActions[i];
      UInt32ToString(frame.ActionsToUInt32(), s) += ",";
      UInt32ToString(TPlayerFrameActions::PositionToUInt32(frame.Position), s);
      if (i + 1 != size) { s += ","; }
    }

    return s;
  }

  inline int GetNumFrames()
  {
    if (FrameActions.empty()) { return 0; }
    const auto& lastFrame = FrameActions[FrameActions.size() - 1];
    return lastFrame.FrameIndex + lastFrame.RunLength + 1;
  }


  // Written with some Claude/Copilot support.
  static std::vector<std::shared_ptr<TGameActions>> FromSerializedList(std::stringstream& ss,
    const std::string& matchingFilename)
  {
    std::vector<std::shared_ptr<TGameActions>> actionsList;
    std::string line;
    while (std::getline(ss, line))
    {
      // Check if line starts with a quote and ends with a quote followed by comma
      if (!line.empty() && line.front() == '"' && line.size() >= 2)
      {
        size_t lastPos = line.size() - 1;
        if (line[lastPos] == ',' && line[lastPos - 1] == '"')
        {
          // Extract the text between the quotes
          std::string serializedActions = line.substr(1, lastPos - 2);

          // Create TGameActions from the extracted string
          try
          {
            auto actions = FromSerialized(serializedActions);
            if (actions)
            {
              if (matchingFilename.empty() || (actions->Filename.find(matchingFilename) != std::string::npos))
              {
                actionsList.push_back(actions);
              }
            }
          }
          catch (std::exception& ex)
          {
            TLOG(LOG_ERROR, "Unable to parse recorded actions. Ignoring.");
          }
        }
      }
    }
    if (actionsList.empty())
    {
      TLOG(LOG_DEBUG, "No ghost actions found for %s - maybe in a different level?", matchingFilename.c_str());
    }
    return actionsList;
  }

  bool GetAction(const int frame, TPlayerActions* actions)
  {
    for (; lastFrameIndex_ < int(FrameActions.size()); ++lastFrameIndex_)
    {
      const auto& currFrame = FrameActions[lastFrameIndex_];
      if (currFrame.Contains(frame))
      {
        actions->Action = static_cast<TPlayerAction>(currFrame.Action);
        actions->FrameIndex = frame;
        actions->Position = currFrame.Position;
        return true;
      }
      if (currFrame.FrameIndex > frame)
      {
        break;
      }
    }
    return false;
  }

  void AddAction(const int frame, const TPlayerAction actions)
  {
#if DEBUG
    // Warn when we hit ~65535 frames (about 18 minutes of continuous actions at 60FPS).
    assert(frame < 65500 && "Frame index about to exceed 16-bit limit.");
#endif
    bool addedToLast = false;
    if (!FrameActions.empty())
    {
      auto& lastAction = FrameActions.back();
      if (lastAction.Action == static_cast<uint8_t>(actions) && lastAction.RunLength < 255
        && lastAction.Contains(frame - 1))
      {
        ++lastAction.RunLength;
        addedToLast = true;
      }
    }

    // Either it's a new action that doesn't match the run-length encoded one, or the number of
    // run-length encoded entries is too large; so add a new entry.
    if (!addedToLast)
    {
      FrameActions.reserve(FrameActions.size() + 100);
      FrameActions.push_back({static_cast<uint16_t>(frame), static_cast<uint8_t>(actions), 0});
    }
  }

  void AddPositionToLastFrame(const Vector2& pos)
  {
    if (!FrameActions.empty()) { FrameActions.back().AddPosition(pos); }
  }

  static std::shared_ptr<TGameActions> Copy(const std::shared_ptr<TGameActions>& actions, int round,
    const std::string& filename);

private:
  int lastFrameIndex_ = 0;
};


// TODO: All of this stuff below should be moved into a relevant header file. Right now, quick dirty hack to get RL training to work.
struct TDebugGameInfo
{
  bool ShowDebugView = false;
  bool ShowGhostActions = false;
  bool ShowRLViz = false;
  bool RLControlMainPlayer = false;
};
}
