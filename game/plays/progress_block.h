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
#include <scenes.h>
#include "game_actions.h"
#include "raymath.h"
#include "raylib.h"

namespace RLPlays
{
//! @brief Show progress - before game starts (shows menu with choice), when the user wins/loses, round progression.
//! LOTS of UI Code. I am bad at UI, so some of this is generated using Claude (credited as necessary below).
//! The RL agent does not see any of this, this is purely for the human.
struct TProgressBlock final : ABlock
{
  // Shown during progress/transitions.
  Color BgColor = CORN_FLOWER_BLUE;
  Color BgColor2 = {0x30, 0xf0, 0x30, 0xff};

  TFont TitleFont = {"fonts/pixantiqua.ttf", 80, DARKPURPLE};
  TFont MenuFont = {"fonts/pixantiqua.ttf", 48, BLUE};
  TFont SceneFont = {"fonts/pixantiqua.ttf", 64, RED};
  TFont ProgressFont = {"fonts/pixantiqua.ttf", 32, BLUE};

  SerializerWithBase(TProgressBlock, ABlock, SceneFont, BgColor, BgColor2, ProgressFont, TitleFont, MenuFont)


  void Init(TContextPtr context) override
  {
    Depth = TLayerDepth::Overlay;
    Traits = TBlockTraits::Cosmetic;
  }


  void LoadContent(TContextPtr context) override
  {
    context->LoadFont(TitleFont);
    context->LoadFont(MenuFont);
    context->LoadFont(SceneFont);
    context->LoadFont(ProgressFont);
  }

  void ApplyProgressText_(TGameProgress& progress)
  {
    progress.NextProgressText1 = "Your goal:";
    if (progress.GameType == TGameType::SinglePlayer)
    {
      // Switch progress text based on game type.
      progress.NextProgressText2 = "1. Collect Fruit";
      progress.NextProgressText3 = "2. Reach Goal";
      progress.NextProgressText4 = "3. Beat Timer";
    }
    else if (progress.GameType == TGameType::PriorVsPlayer || progress.GameType == TGameType::PlayerVsPrior)
    {
      std::string playerText = (progress.GameType == TGameType::PriorVsPlayer ? "Player1" : "Player2");
      progress.NextProgressText2 = "1. Collect More Fruit Than " + playerText;
      progress.NextProgressText3 = "2. Reach Goal Before Your Time Ghost";
      progress.NextProgressText4 = "3. Beat Your Time Ghost";
    }
    else if (progress.GameType == TGameType::PlayerVsAI || progress.GameType == TGameType::AIVsPlayer)
    {
      std::string playerText = (progress.GameType == TGameType::AIVsPlayer ? "Player1" : "Player2");
      progress.NextProgressText2 = "1. Collect More Fruit Than " + playerText;
      progress.NextProgressText3 = "2. Reach Goal Before The AI Agent";
      progress.NextProgressText4 = "3. Beat The AI";
    }
  }

  void Draw(TContextPtr context) override
  {
    const auto& cellSize = context->Grid()->CellSize;
    auto& progress = *context->UpdateGameProgress();
    if (progress.GetGameState() == TGameState::EditorMode || progress.GetGameState() == TGameState::PauseGame)
    {
      return;
    }
    const auto rect = context->ScreenRect();

    constexpr auto bgMult = 0.9f;
    auto drawBeginScene = false;
    auto fade = 0.0;
    auto fade2 = 1.0; // When menu is dismissed
    //bool finished = progress.CheckFinishedAllRounds();
    auto won = progress.LastPlayerState == TPlayerState::Alive || progress.LastPlayerState == TPlayerState::Won;
    const auto& gameTypes = GetGameTypes_(progress.CurrentRound);

    // This is the only place where we change the state from within Draw. Noone else should touch this.
    // And only in scenarios where Draw is being called (i.e. progress.ShowSceneTransitions is true).
    if (progress.GetGameState() == TGameState::StartGame)
    {
      if (startScenes_.BeginScenes(context))
      {
        // This happens when we reload the game type (for the same round) and we don't want to keep showing the anim.
        if (startScenes_.RunScene(progress.ShowMenuAnim ? 300_ms : 0_ms, progress.ShowMenuAnim ? 200_ms : 0_ms,
          progress.ShowMenuAnim ? 200_ms : 0_ms))
        {
          fade = startScenes_.PercentTime();
          drawBeginScene = true;
        }

        //DrawRectangleRec(rect, Fade(BgColor, bgMult * fade));
        if (startScenes_.EndScenes())
        {
          drawBeginScene = true;
          fade = 1;
          progress.RequestGameState(TGameState::AboutToRun);
        }
      }
    }
    if (progress.GetGameState() == TGameState::AboutToRun || progress.GetGameState() == TGameState::MenuDismissing
      || progress.GetGameState() == TGameState::MenuDismissedBeforeRunning)
    {
      if (progress.GetGameState() == TGameState::AboutToRun)
      {
        // Choose menu using the <- -> arrow keys or controller buttons.
        if (HasEnumValue(progress.CurrentMenuInput, TPlayerAction::GameMenu))
        {
          // Debounce the menu input as the key press may be a few ms.
          for (int i = 0; i < gameTypes.size(); ++i)
          {
            if (gameTypes[i] == progress.GameType)
            {
              int newIndex = (i + 1 + gameTypes.size()) % gameTypes.size();
              progress.GameType = gameTypes[newIndex];
              progress.ShouldReloadGame = true;
              break;
              // We have to reload the game because switching between say single player vs RL agent vs replay actions
              // require loading models (RL) or setting up correct replay actions during init.
              // Also prevents us from loading RL models or replay actions when not needed.
              // In the future, if we were to support online realtime multiplayer, we can setup connection at that junction.
            }
          }
        }
        else if (HasEnumValue(progress.CurrentMenuInput, TPlayerAction::GameStart))
        {
          dismissingMenu_.Restart();
          progress.RequestGameState(TGameState::MenuDismissing);
        }
      }
      if (progress.GetGameState() == TGameState::MenuDismissing)
      {
        if (dismissingMenu_.BeginScenes(context))
        {
          if (dismissingMenu_.RunScene(200_ms)) { fade2 = 1 - dismissingMenu_.PercentTime(); }
          dismissingMenu_.EndScenes();
        }
        else
        {
          fade2 = 0;
          if (!HasEnumValue(progress.CurrentMenuInput, TPlayerAction::GameStart))
          {
            progress.RequestGameState(TGameState::MenuDismissedBeforeRunning);
          }
        }
      }
      if (progress.GetGameState() == TGameState::MenuDismissedBeforeRunning)
      {
        fade2 = 0;
        if (HasAnyEnumValue(progress.CurrentMenuInput))
        {
          progress.RequestGameState(TGameState::RunningGame);
          return;
        }
      }
      bool controllerFound = IsGamepadAvailable(0);
      auto fadeTopText = 1.0f - fade2;
      auto rect = context->ScreenRect();
      rect.height = context->Grid()->CellSize.y;
      DrawRectangleRec(rect, Fade(BgColor2, fadeTopText));
      std::string text = "Press any key to start | Space: Shoot/Use | E: Activate";
      context->DrawCenteredText(MenuFont, rect, text, Fade(SceneFont.FontColor, fadeTopText));
      std::string controllerText = "(Controller not found)";
      /* From raylib.h (MIT License from raysan5)
    GAMEPAD_BUTTON_RIGHT_FACE_UP,       // Gamepad right button up (i.e. PS3: Triangle, Xbox: Y)
    GAMEPAD_BUTTON_RIGHT_FACE_RIGHT,    // Gamepad right button right (i.e. PS3: Circle, Xbox: B)
    GAMEPAD_BUTTON_RIGHT_FACE_DOWN,     // Gamepad right button down (i.e. PS3: Cross, Xbox: A)
    GAMEPAD_BUTTON_RIGHT_FACE_LEFT,     // Gamepad right button left (i.e. PS3: Square, Xbox: X)
      
      */
      if (controllerFound)
      {
        controllerText = "Controller: Jump (A) | Shoot/Use (B) | Activate (X/Y)";
        // TODO: Replace with kenney.nl icon!!
      }
      context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, cellSize.y * 1.0f}), controllerText,
        Fade(MenuFont.FontColor, fadeTopText));

      if (startScenes2_.BeginScenes(context))
      {
        if (startScenes2_.RunScene(progress.ShowMenuAnim ? 300_ms : 0_ms)) { fade = startScenes2_.PercentTime(); }
        if (startScenes2_.EndScenes()) { fade = 1; }
      }
      else { fade = 1; }
      fade *= fade2;
      if (!FloatIsZero(fade))
      {
        rect = context->ScreenRect();

        auto inset = ExpandRectSizePercent(rect, -0.25 * fade, -0.25 * fade);
        auto bgColor = won ? GREEN : WHITE;
        //if (finished) { bgColor = YELLOW; }
        auto fgColor = won ? BLUE : RED;
        auto fgColor2 = DARKPURPLE;
        auto titleRect = OffsetRect(inset, {0, -cellSize.y * 2.5f});
        titleRect.height = cellSize.y * 9.0f;
        DrawRectangleRounded(titleRect, 0.2f, 10, Fade(YELLOW, bgMult * fade));
        inset.height += cellSize.y * 3.0f;
        DrawRectangleRounded(inset, 0.2f, 10, Fade(bgColor, bgMult * fade));
        //context->DrawCenteredText(SceneFont, rect, text, Fade(SceneFont.FontColor, fade));
        context->DrawCenteredText(TitleFont, OffsetRect(rect, {0, -cellSize.y * 6.0f}), "RLPlays Game",
          Fade(fgColor2, fade));
        context->DrawCenteredText(SceneFont, OffsetRect(rect, {0, -cellSize.y * 5.0f}),
          "Explore Self-play and Reinforcement Learning", Fade(fgColor2, fade));

        context->DrawCenteredText(SceneFont, OffsetRect(rect, {0, -cellSize.y * 3.5f}),
          progress.GetRoundName(progress.CurrentRound), Fade(fgColor, fade));

        bool controllerFound = GetGamepadId() >= 0;
        std::string controllerText = "Press \"C\" Key";
        if (controllerFound) { controllerText += " or DPAD"; }
        controllerText += " to choose gametype";
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, -cellSize.y * 2.5f}), controllerText,
          Fade(DARKPURPLE, fade));
        controllerText = "Press ENTER";
        if (controllerFound) { controllerText += " or A button"; }
        controllerText += " to start";
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, -cellSize.y * 1.5f}), controllerText,
          Fade(DARKPURPLE, fade));
        if (!controllerFound)
        {
          context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, -cellSize.y * 0.5f}), "(Controller Not Found)",
            Fade(DARKBROWN, fade));
        } else
        {
          context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, -cellSize.y * 0.5f}), "(Controller Found)",
            Fade(DARKGREEN, fade));
        }
        float menuItemSizeX = 450;
        float menuOffsetX = -menuItemSizeX * (static_cast<float>(gameTypes.size()) - 1) * 0.5f;
        float currentOffsetX = menuOffsetX;

        for (auto& choiceGameType : gameTypes)
        {
          const auto isChosen = choiceGameType == progress.GameType;
          auto strs = GetGameTypeStrs_(progress.CurrentRound, choiceGameType);
          auto rec = context->DrawCenteredText(MenuFont, OffsetRect(rect, {currentOffsetX, cellSize.y * 0.5f}),
            strs[0],
            Fade(isChosen ? DARKBLUE : DARKGREEN, fade));
          if (strs.size() > 1)
          {
            auto rec2 = context->DrawCenteredText(MenuFont, OffsetRect(rect, {currentOffsetX, cellSize.y * 1.5f}),
              strs[1],
              Fade(isChosen ? BLUE : DARKBROWN, fade));
            rec = UnionRect(rec, rec2);
          }
          if (choiceGameType == progress.GameType)
          {
            DrawRectangleRoundedLinesEx(ExpandRect(rec, 20), 0.1f, 10, 10, Fade({230, 80, 80, 255}, bgMult * fade));
          }
          currentOffsetX += menuItemSizeX;
        }


        //if (progress.NextProgressText1.empty() && progress.NextProgressText2.empty() && progress.NextProgressText3.empty()
        //  && progress.NextProgressText4.empty())
        ApplyProgressText_(progress);
        auto textOffsetY = 0;
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, textOffsetY + cellSize.y * 3.0f}),
          progress.NextProgressText1,
          Fade(fgColor, fade));
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, textOffsetY + cellSize.y * 4.0f}),
          progress.NextProgressText2,
          Fade(fgColor, fade));
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, textOffsetY + cellSize.y * 5.0f}),
          progress.NextProgressText3,
          Fade(fgColor, fade));
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {0, textOffsetY + cellSize.y * 6.0f}),
          progress.NextProgressText4,
          Fade(fgColor, fade));

        DrawRectangleRec(OffsetRect(rect, {0, cellSize.y * 17.0f}), Fade(BgColor2, bgMult * fade));

        context->DrawCenteredText(MenuFont, OffsetRect(rect, {cellSize.x * 8.0f, textOffsetY + cellSize.y * 8.5f}),
          "Built with PufferLib/Raylib/ImGui",
          Fade(RED, fade));
        context->DrawCenteredText(MenuFont, OffsetRect(rect, {-cellSize.x * 8.0f, textOffsetY + cellSize.y * 8.5f}),
          "Assets from kenney.nl",
          Fade(RED, fade));
      }
    }


    if (progress.GetGameState() == TGameState::RunningGame)
    {
      auto rect = context->ScreenRect();
      rect.height = context->Grid()->CellSize.y;
      std::stringstream text;
      const auto timeRemaining = progress.TimeLimit.TimeRemaining();
      if (progress.TimeLimit.IsValid())
      {
        text << "Time left: " << std::fixed << std::setprecision(1) << SecondsFromNanos(timeRemaining) << " seconds"
            << " Round " << progress.CurrentRound;
        if (!progress.InfiniteRounds) { text << "/" << progress.MaxRounds; }
        text << "\n";
        DrawRectangleRec(rect, Fade(BgColor2, 0.8f));
        context->DrawCenteredText(SceneFont, rect, text.str(), SceneFont.FontColor);

        auto& playerL = progress.GetPlayerProgress(progress.GetLeftSidePlayer());
        auto& playerR = progress.GetPlayerProgress(progress.GetRightSidePlayer());
        rect.width = context->Grid()->CellSize.x * 10.0f;

        // Determine highlight colors based on fruit comparison
        Color p1BgColor, p2BgColor;
        if (playerL.NumRewards > playerR.NumRewards)
        {
          p1BgColor = {0, 160, 0, 255};
          p2BgColor = {180, 30, 30, 255};
        }
        else if (playerR.NumRewards > playerL.NumRewards)
        {
          p1BgColor = {180, 30, 30, 255};
          p2BgColor = {0, 160, 0, 255};
        }
        else
        {
          p1BgColor = {0, 160, 0, 255};
          p2BgColor = {0, 160, 0, 255};
        }

        // Detect fruit count increases and trigger blink
        if (playerL.NumRewards > prevP1Fruit_) { p1FruitBlink_.Restart(); }
        prevP1Fruit_ = playerL.NumRewards;
        if (playerR.NumRewards > prevP2Fruit_) { p2FruitBlink_.Restart(); }
        prevP2Fruit_ = playerR.NumRewards;

        // Compute blink alpha for Player 1 (flashes bright then settles)
        float p1Alpha = 0.55f;
        if (p1FruitBlink_.BeginScenes(context))
        {
          if (p1FruitBlink_.RunScene(400_ms))
          {
            p1Alpha = 0.55f + 0.4f * (1.0f - static_cast<float>(p1FruitBlink_.PercentTime()));
          }
          p1FruitBlink_.EndScenes();
        }

        // Compute blink alpha for Player 2
        float p2Alpha = 0.55f;
        if (p2FruitBlink_.BeginScenes(context))
        {
          if (p2FruitBlink_.RunScene(400_ms))
          {
            p2Alpha = 0.55f + 0.4f * (1.0f - static_cast<float>(p2FruitBlink_.PercentTime()));
          }
          p2FruitBlink_.EndScenes();
        }

        // Player 1: draw highlight box then text
        DrawRectangleRounded(rect, 0.3f, 10, Fade(p1BgColor, p1Alpha));
        text = {};
        auto youText = ":";
        if (progress.GetLeftSidePlayer() == progress.GetActivePlayerId())
        {
          youText = "(You): ";
        }
        text << "Player 1 " << youText << playerL.NumRewards << " Fruit";
        context->DrawCenteredText(SceneFont, rect, text.str(), WHITE);

        // Player 2: draw highlight box then text
        rect.x = context->ScreenRect().width - rect.width;
        DrawRectangleRounded(rect, 0.3f, 10, Fade(p2BgColor, p2Alpha));
        text = {};
        youText = ": ";
        if (progress.GetRightSidePlayer() == progress.GetActivePlayerId())
        {
          youText = "(You): ";
        }
        text << "Player 2" << youText << playerR.NumRewards << " Fruit";
        context->DrawCenteredText(SceneFont, rect, text.str(), WHITE);
      }
    }

    if (progress.GetGameState() == TGameState::StoppingGame)
    {
      auto activePlayerState = progress.GetPlayerState(progress.GetActivePlayerId());
      std::stringstream text;
      if (progress.TimeLimit.IsValid())
      {
        const auto timeTaken = progress.TimeLimit.TimeElapsed();
        text << "Time taken: " << std::fixed << std::setprecision(1) << SecondsFromNanos(timeTaken) <<
            " seconds\n";
        progress.NextProgressText3 = text.str();
        text = {};
      }
      // Add controller/keyboard info
      // TODO(perumaal): Add rewards here to text4
      if (activePlayerState == TPlayerState::Won)
      {
        progress.NextProgressText1 = "You Won!";
        if (progress.CurrentRound < progress.MaxRounds)
        {
          text << "Round " << progress.CurrentRound;
          if (!progress.InfiniteRounds) { text << "/" << progress.MaxRounds; }
          text << " completed!";
        }
        else
        {
          text << "You completed all rounds! Press R To Restart.";
        }
        progress.NextProgressText2 = text.str();
      }
      else if (activePlayerState == TPlayerState::Dead || activePlayerState == TPlayerState::TimeOut)
      {
        progress.NextProgressText1 = "You Lost!";
        progress.NextProgressText2 = "Restarting Round " + std::to_string(progress.CurrentRound) + "";
      }
      progress.NextProgressText4 = "Collected: " + std::to_string(
            progress.GetPlayerProgress(progress.GetActivePlayerId()).NumRewards) + " Fruit"
          + "  vs   " + std::to_string(progress.GetPlayerProgress(progress.GetInactivePlayerId()).NumRewards) +
          " Fruit";

      progress.LastPlayerState = activePlayerState;
      progress.RequestGameState(TGameState::StoppingSequence);
      stoppingScenes_.Restart();
    }

    if (progress.GetGameState() == TGameState::StoppingSequence)
    {
      if (stoppingScenes_.PercentTime() >= 1.0)
      {
        if (HasEnumValue(progress.CurrentMenuInput, TPlayerAction::GameStart)) { stoppingScenes_.ForceStop(); }
      }
      if (stoppingScenes_.BeginScenes(context))
      {
        if (stoppingScenes_.RunScene(2000_ms, 100_ms, 35000_ms))
        {
          if (won) { ShowVictory_(context); }
          else { ShowLost_(context); }
        }
        stoppingScenes_.EndScenes();
      }
      else
      {
        progress.RequestGameState(TGameState::StopGame);
      }
    }
  }

#if RLPLAYS_EDITOR
  void EditorDraw(const std::shared_ptr<TEditorData>& editorData, TContextPtr context) override
  {
    context->DrawCenteredText(this->ProgressFont, Box, "PROGRESSBOX", RED);
    ABlock::EditorDraw(editorData, context);
  }
#endif
  bool IsCandidateForMirroring(TContextPtr _) const override { return false; }

private:
  [[nodiscard]] const std::vector<TGameType>& GetGameTypes_(const int round) const
  {
    static const std::vector Round1GameTypes = {TGameType::SinglePlayer, TGameType::PlayerVsAI};
    static const std::vector Round2GameTypes = {
      TGameType::PriorVsPlayer, TGameType::AIVsPlayer /*, TGameType::SinglePlayer*/
    };
    static const std::vector Round3GameTypes = {
      TGameType::PlayerVsPrior, TGameType::PlayerVsAI /*, TGameType::SinglePlayer*/
    };
    if (round == 1) { return Round1GameTypes; }
    if (round % 2 == 0) { return Round2GameTypes; }
    if (round % 2 != 0) { return Round3GameTypes; }
    return Round1GameTypes;
  }


  [[nodiscard]] const std::vector<std::string>& GetGameTypeStrs_(const int round, const TGameType gameType) const
  {
    static const std::vector<std::string> SinglePlayerRound1Strs = {"You", "Single Player"};
    static const std::vector<std::string> SinglePlayerRound2Strs = {"You (again!)", "Single Player"};
    static const std::vector<std::string> PlayerVsPriorStrs = {"You vs Yourself", "Self-Play!"};
    static const std::vector<std::string> PlayerVsAIStrs = {"You vs Computer", "RL Agent!"};
    static const std::vector<std::string> PlayerVsPlayerStrs = {"You vs Them", "Human Opponent"};
    switch (gameType)
    {
    case TGameType::SinglePlayer: if (round == 1) return SinglePlayerRound1Strs;
      return SinglePlayerRound2Strs;

    case TGameType::AIVsPlayer:
    case TGameType::PlayerVsAI: return PlayerVsAIStrs;

    case TGameType::PlayerVsPrior:
    case TGameType::PriorVsPlayer: return PlayerVsPriorStrs;

    case TGameType::PlayerVsPlayer: return PlayerVsPlayerStrs;
    default: return SinglePlayerRound1Strs;
    }
  }

  // Draws a styled stats box showing player fruit/enemy counts and time taken.
  void DrawEndGameStatsBox_(TContextPtr context, float alpha, float yOffset)
  {
    auto& progress = *context->UpdateGameProgress();
    const auto rect = context->ScreenRect();
    const auto& cellSize = context->Grid()->CellSize;
    const bool isSinglePlayer = (progress.GameType == TGameType::SinglePlayer);

    // Box dimensions
    const float boxW = cellSize.x * 18.0f;
    const float lineH = cellSize.y * 1.2f;
    const float padY = cellSize.y * 0.4f;
    const int numLines = isSinglePlayer ? 2 : 3; // player1 + time, or player1 + player2 + time
    const float boxH = padY * 2.0f + lineH * numLines;
    const float boxX = rect.x + rect.width / 2.0f - boxW / 2.0f;
    const float boxY = rect.y + rect.height / 2.0f + yOffset - boxH / 2.0f;
    const Rectangle statsBox = {boxX, boxY, boxW, boxH};

    // Draw box background and border
    Color boxColor = progress.LastPlayerState == TPlayerState::Won ? DARKGREEN : MAROON;
    DrawRectangleRounded(statsBox, 0.15f, 10, Fade(boxColor, alpha));
    DrawRectangleRoundedLinesEx(statsBox, 0.15f, 10, 3.0f, Fade(GOLD, alpha));

    // Decorative header line
    const Rectangle headerLine = {boxX + 15.0f, boxY + padY + lineH * 0.05f, boxW - 30.0f, 2.0f};
    DrawRectangleRec(headerLine, Fade(GOLD, alpha * 0.4f));

    float currentY = boxY + padY;

    // Helper lambda for drawing a player stats line
    auto drawPlayerLine = [&](int playerId, const char* label, Color nameColor, Color statColor)
    {
      const auto& pp = progress.GetPlayerProgress(playerId);
      const bool isYou = progress.IsActivePlayer(playerId);

      std::stringstream ss;
      ss << label;
      if (isYou) { ss << " (You)"; }
      ss << ":  " << pp.NumRewards << " Fruit   |   " << pp.NumEnemies << " Enemies";

      const Rectangle lineRect = {boxX, currentY, boxW, lineH};
      context->DrawCenteredText(MenuFont, lineRect, ss.str(), Fade(isYou ? GOLD : statColor, alpha));
      currentY += lineH;
    };

    // Player 1 stats
    drawPlayerLine(PlayerId1, "Player 1", {100, 180, 255, 255}, {100, 180, 255, 255});

    // Player 2 stats (only in multiplayer)
    if (!isSinglePlayer)
    {
      drawPlayerLine(PlayerId2, "Player 2", {255, 140, 80, 255}, {255, 140, 80, 255});
    }

    // Time taken
    if (progress.TimeLimit.IsValid())
    {
      const auto timeTaken = progress.TimeLimit.TimeElapsed();
      std::stringstream ts;
      ts << "Time: " << std::fixed << std::setprecision(1) << SecondsFromNanos(timeTaken) << "s";
      const Rectangle timeRect = {boxX, currentY, boxW, lineH};
      context->DrawCenteredText(MenuFont, timeRect, ts.str(), Fade(WHITE, alpha));
    }
  }

  // Written with Copilot assistance (Claude Sonnet 3.7) - "Add confetti here using raylib here"
  void ShowVictory_(TContextPtr context)
  {
    const auto rect = context->ScreenRect();
    const auto& cellSize = context->Grid()->CellSize;


    // Add confetti celebration animation when player wins
    constexpr int numConfettiParticles = 100;
    constexpr float confettiSize = 32.0f;

    // Get animation progress from stoppingScenes to control confetti intensity
    const float animProgress = stoppingScenes_.PercentTime();

    // Use screen dimensions for confetti distribution
    const Rectangle screenRect = context->ScreenRect();

    // Create confetti particles with different colors
    Color confettiColor;
    for (int i = 0; i < numConfettiParticles * animProgress; i++)
    {
      // Randomize position across screen
      const float x = screenRect.x + GetRandomValue(0, static_cast<int>(screenRect.width));
      const float y = screenRect.y + GetRandomValue(0, static_cast<int>(screenRect.height * animProgress));

      // Randomize color from festive palette
      switch (GetRandomValue(0, 5))
      {
      case 0: confettiColor = RED;
        break;
      case 1: confettiColor = GREEN;
        break;
      case 2: confettiColor = BLUE;
        break;
      case 3: confettiColor = YELLOW;
        break;
      case 4: confettiColor = PINK;
        break;
      default: confettiColor = PURPLE;
        break;
      }

      // Apply fade based on animation progress
      confettiColor = Fade(confettiColor, animProgress * 0.8f);

      // Draw confetti particle as small rectangle with random rotation
      const float rotation = GetRandomValue(0, 360);
      const Vector2 origin = {confettiSize / 2.0f, confettiSize / 2.0f};
      DrawRectanglePro(
        {x, y, confettiSize, confettiSize * GetRandomValue(5, 10) / 10.0f},
        origin,
        rotation,
        confettiColor
      );
    }

    // Add celebratory text that appears with animation
    if (animProgress > 0.3f)
    {
      const float pulsingOffset = 1.0f + sin(stoppingScenes_.PercentTime() * 5) * 0.2f; // Pulsing text effect
      const float textAlpha = animProgress > 0.7f ? 1.0f : animProgress / 0.7f;

      // Draw celebration text
      context->DrawCenteredText(
        TitleFont,
        OffsetRect(rect, {0, pulsingOffset + cellSize.y * 1.0f}),
        "YOU WIN!",
        Fade(confettiColor, textAlpha * 0.9f)
      );

      // Draw stats box below VICTORY! text
      if (animProgress > 0.5f)
      {
        const float boxAlpha = animProgress > 0.8f ? 1.0f : (animProgress - 0.5f) / 0.3f;
        DrawEndGameStatsBox_(context, boxAlpha, cellSize.y * 5.0f);
      }
    }
  }

  // Written with Copilot assistance (Claude Sonnet 3.7) - "Now implement a losing screen here with relevant graphics using raylib."
  void ShowLost_(TContextPtr context)
  {
    const auto rect = context->ScreenRect();
    const auto& cellSize = context->Grid()->CellSize;

    // Get animation progress for fade-in effects
    const float animProgress = stoppingScenes_.PercentTime();

    // Add visual effects for losing scenario
    // Draw dark smoke/cloud particles rising from bottom
    constexpr int numSmokeParticles = 30;
    constexpr float smokeSize = 25.0f;

    // Use screen dimensions for smoke distribution
    const Rectangle screenRect = context->ScreenRect();

    // Create smoke/debris particles
    for (int i = 0; i < numSmokeParticles * animProgress; i++)
    {
      // Position particles rising from bottom of screen
      const float x = screenRect.x + GetRandomValue(0, static_cast<int>(screenRect.width));
      // Rising effect - particles start from bottom and rise up
      const float y = screenRect.height - (GetRandomValue(0, static_cast<int>(screenRect.height * animProgress)));

      // Smoke colors - dark grays and reds for defeat
      Color smokeColor;
      switch (GetRandomValue(0, 4))
      {
      case 0: smokeColor = DARKGRAY;
        break;
      case 1: smokeColor = Fade(RED, 0.7f);
        break;
      case 2: smokeColor = Fade(MAROON, 0.8f);
        break;
      case 3: smokeColor = GRAY;
        break;
      default: smokeColor = Fade(BLACK, 0.6f);
        break;
      }

      // Apply fade based on animation progress
      smokeColor = Fade(smokeColor, animProgress * 0.6f);

      // Draw smoke particle as circle with varying size
      const float radius = smokeSize * (GetRandomValue(5, 15) / 10.0f);
      DrawCircle(
        static_cast<int>(x),
        static_cast<int>(y),
        radius,
        smokeColor
      );
    }

    // Add screen shake effect based on animation progress
    float shakeAmount = 0.0f;
    if (animProgress > 0.2f && animProgress < 0.6f)
    {
      // More intense at the beginning, then calms down
      shakeAmount = (0.6f - animProgress) * 10.0f;
      float offsetX = GetRandomValue(-static_cast<int>(shakeAmount), static_cast<int>(shakeAmount));
      float offsetY = GetRandomValue(-static_cast<int>(shakeAmount), static_cast<int>(shakeAmount));

      // Draw red flash/vignette effect for losing
      DrawRectangleGradientEx(
        rect,
        Fade(MAROON, animProgress * 0.2f),
        Fade(RED, animProgress * 0.15f),
        Fade(DARKBLUE, animProgress * 0.3f),
        Fade(MAROON, animProgress * 0.2f)
      );
    }

    // Add defeat text that appears with animation
    if (animProgress > 0.4f)
    {
      // Slight shake/wobble effect for text
      const float shakeOffset = (sin(stoppingScenes_.PercentTime() * 8) * shakeAmount * 0.5f);
      float textScale = 1.0f + sin(stoppingScenes_.PercentTime() * 3) * 0.1f; // Subtle pulsing effect
      const float textAlpha = animProgress > 0.7f ? 1.0f : (animProgress - 0.4f) / 0.3f;

      // Draw main defeat text
      context->DrawCenteredText(
        TitleFont,
        OffsetRect(rect, {shakeOffset, -cellSize.y * 3.0f + shakeOffset}),
        "YOU LOSE!",
        Fade(MAROON, textAlpha * 0.9f)
      );

      // Draw stats box below DEFEAT! text
      if (animProgress > 0.5f)
      {
        const float boxAlpha = animProgress > 0.8f ? 1.0f : (animProgress - 0.5f) / 0.3f;
        DrawEndGameStatsBox_(context, boxAlpha, cellSize.y * 2.0f);
      }

      // Draw secondary text
      if (animProgress > 0.6f)
      {
        context->DrawCenteredText(
          MenuFont,
          OffsetRect(rect, {0, cellSize.y * 4.5f}),
          "Try again...",
          Fade(GRAY, (animProgress - 0.6f) / 0.4f * 0.9f)
        );
      }
    }

    // Add cracked/broken frame effect around the screen
    if (animProgress > 0.3f)
    {
      // Draw cracked lines radiating from center
      const Vector2 center = {rect.x + rect.width / 2, rect.y + rect.height / 2};
      const int numCracks = 12;

      for (int i = 0; i < numCracks; i++)
      {
        // Create zigzag crack lines
        const float angle = GetRandomValue(0, 360);
        const float length = GetRandomValue(100, static_cast<int>(rect.width / 1.5f)) * animProgress;

        const Vector2 end = {
          center.x + cos(angle * DEG2RAD) * length,
          center.y + sin(angle * DEG2RAD) * length
        };

        // Create zigzag pattern for each crack
        Vector2 current = center;
        Vector2 next;
        const int segments = GetRandomValue(3, 8);

        for (int j = 1; j <= segments; j++)
        {
          const float segmentPercent = static_cast<float>(j) / segments;
          next = {
            Lerp(center.x, end.x, segmentPercent),
            Lerp(center.y, end.y, segmentPercent)
          };

          // Add zigzag randomness
          next.x += GetRandomValue(-15, 15);
          next.y += GetRandomValue(-15, 15);

          // Draw crack segment
          DrawLineEx(
            current,
            next,
            2.0f * animProgress,
            Fade(BLACK, animProgress * 0.7f)
          );

          current = next;
        }
      }
    }
  }

  TScenes startScenes_;
  TScenes startScenes2_;
  TScenes stoppingScenes_;
  TScenes transitionScenes_;
  TScenes dismissingMenu_;

  // Fruit highlight blink state
  int prevP1Fruit_ = 0;
  int prevP2Fruit_ = 0;
  TScenes p1FruitBlink_;
  TScenes p2FruitBlink_;
};
} // namespace RLPlays
