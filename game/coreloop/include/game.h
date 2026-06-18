#pragma once

#include <base_block.h>
#include <context.h>
#include <game_blocks.h>
#include <game_actions.h>
#include <game_loop.h>
#include <main_game.h>
#include <nlohmann/json.hpp>
#include <serialize.h>

#include <game_types.h>
#include <memory>
#include <player_block.h>
#include <string>
#include <world.h>
#include <world_fileset.h>

namespace RLPlays
{
// Forward declaration for json
using json = nlohmann::json;


struct TGame
{
  std::shared_ptr<TWorld> World;
  std::shared_ptr<TContext> Context;

  //! @brief Set this to a positive value to fast-forward; 0 to pause; < 0 for slow-motion updates.
  int NumFrameSkips;

  /**
   * @brief Constructor for TGame.
   * @param fps Frames per second target for the game.
   */
  TGame(int fps);

  /**
   * @brief Advances the game state by one frame.
   */
  void Tick();

  /**
   * @brief Renders the current game state.
   */
  void Draw() const;

  /**
   * @brief Unloads the current game state.
   */
  void Unload();

  /**
   * @brief Loads a game state from a file.
   * @param filename The name of the file to load from.
   * @param actions Previous recorded timelined actions to replay from.
   * @return A shared pointer to the loaded TWorld object.
   */
  std::shared_ptr<TWorld> LoadFile( TGameLoadInfo& loadInfo);

  /**
   * @brief Saves the current game state to a file.
   * @param filename The name of the file to save to.
   * @return True if the save was successful, false otherwise.
   */
  bool SaveFile(std::string filename) const;


  /**
   * @brief Checks if the game state is valid.
   * @return True if the game state is valid, false otherwise.
   */
  bool IsValid() const;

  //! @brief Handle input actions from the player or from action replay.
  void HandleActions(const TPlayerAction& action);
  void ResetGame(bool resetBlocks);

  void TogglePause();

  [[nodiscard]] int GetRound() const;
  
private:
  void LoadGame_(TGameLoadInfo& loadInfo);
  //! @brief Check for next round (if requested via {@param goToNextRound} or end of game and load that.
  void AutoContinue_(bool goToNextRound);

  //! @brief Track slow-mo frame skips (i.e. hold the current frame for a specific number of frames via negative TGame::NumFrameSkips).
  int frameSkips_ = 0;

  //! @brief As we reload the game when the menu game type changes, ensure we don't keep reloading as the key press is
  //! held down.
  TCountdownTimer menuDebounceTimer_ = {NanosFromMillis(150)};
  std::shared_ptr<TRLTrain> rlTrain_;


  //! @brief The current state of the game that controls various sequences/Update patterns.
  //TGameState GameState = TGameState::StartGame;
};
} // namespace RLPlays
