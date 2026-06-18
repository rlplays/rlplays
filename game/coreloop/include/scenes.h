#pragma once
#include <serialize.h>

#include <context.h>
#include <timer.h>
#include <game_types.h>

namespace RLPlays
{
enum class TSceneState : uint8_t
{
  StartScene     = 0,
  InDelayScene   = 1,
  RunningScene   = 2,
  PostDelay      = 3,
  StopScene      = 4,
  EndedAllScenes = 5,
};

/**
 *! @brief A simple scene transition util class.
 * Usage:
 * private: TScenes scenes_;
 * void Draw(TContextPtr context) {
 *  scenes_.BeginScenes(context);
 *  if (scenes_.RunScene(250)) {
 *    context->Draw...(..., Fade(Color, scenes_.Percent());
 *  }
 *  if (scenes_.RunScene(500, 200)) {
 *    context->Draw...(scenes_.PercentVector(v), Fade(color, scenes_.Percent() * 2));
 *  }
 *  scenes_.EndScenes();
 * This is very much in similar style as ImGui APIs letting code be the (scene) description.
 */
struct TScenes
{
  //! @brief Tracks current time via the context object. Returns true if there are any scenes left.
  //! Context is stored between the BeginScenes/EndScenes calls - the only valid place to store context
  //! in this project as context is ephemeral.
  bool BeginScenes(TContextPtr context, bool repeating = false);

  //! @brief Runs the scene timing and returns true if this scene is still ongoing (or just ended).
  //! Time ins inclusive on start/end: if the duration is 200ms, then this method returns true at 0ms
  //! and at 200ms (so fades/percent calculations work in [0, 1] range correctly).
  //! Percent() returns the time elapsed as a ratio to the scene duration (past the delay).
  [[nodiscard]] bool RunScene(TimeMillis sceneDuration, TimeMillis sceneDelay = 0, TimeMillis postSceneDelay = 0);

  //! @brief If the current scene is running (i.e. post-delay and within the duration inclusive of start/end), returns the percent
  //! in the inclusive range [0, 1].
  [[nodiscard]] double PercentTime() const { return percent_; }

  [[nodiscard]] TimeNanos Time() const { return countDownTimer_.TimeElapsed(); }

  [[nodiscard]] double ReversePercentTime() const { return 1.0 - percent_; }

  //! @brief Returns a scaled version of @param v based on the current percent time.
  [[nodiscard]] Vector2 PercentTimeForVector(const Vector2& v) const
  {
    const auto p = static_cast<float>(PercentTime());
    return {v.x * p, v.y * p};
  }

  // TODO(perumaal): Provide a Bezier curve from Percent() -> Interpolate() to allow for more complex transitions.
  // Returns true if all scenes ended.
  bool EndScenes();
  void ForceStop();
  bool Restart();

private:
  TCountdownTimer countDownTimer_ = {};
  double percent_ = 0;
  int currentScene_ = 0;
  int runningSceneIndex_ = -1;
  TSceneState state_ = TSceneState::StartScene;
  std::shared_ptr<TContext> context_;
  bool repeating_ = false;
};
}
