#include "scenes.h"

#include <serialize.h>

#include <context.h>
#include <timer.h>
#include <game_types.h>

namespace RLPlays
{
bool TScenes::BeginScenes(TContextPtr context, bool repeating)
{
  if (state_ == TSceneState::EndedAllScenes) { return false; }
  if (context_ != nullptr)
  {
    TLOG(LOG_ERROR, "Invalid state: Cannot begin scenes - scenes did not end correctly before!");
    return false;
  }
  context_ = context;
  runningSceneIndex_ = -1;
  repeating_ = repeating;
  return true;
}

bool TScenes::RunScene(const TimeMillis sceneDuration, const TimeMillis sceneDelay, const TimeMillis postDelay)
{
  // First check which scene we are part of.
  if (state_ == TSceneState::EndedAllScenes) { return false; }
  ++runningSceneIndex_;
  if (currentScene_ != runningSceneIndex_) { return false; }

  // We are in the correct scene, so track transitions.
  if (state_ == TSceneState::StartScene)
  {
    percent_ = 0;
    TLOG(LOG_TRACE, "Starting scene %d", (currentScene_ + 1));
    const auto timerVal = sceneDelay > 0 ? sceneDelay : sceneDuration;
    countDownTimer_ = TCountdownTimer(NanosFromMillis(timerVal));
    countDownTimer_.Start();
    if (sceneDelay > 0)
    {
      TLOG(LOG_TRACE, "--PreSceneDelay scene %d, (%f)", (currentScene_ + 1), percent_);
      state_ = TSceneState::InDelayScene;
    }
    else
    {
      TLOG(LOG_TRACE, "--Running scene %d, (%f)", (currentScene_ + 1), percent_);
      state_ = TSceneState::RunningScene;
    }
  }
  if (state_ == TSceneState::InDelayScene)
  {
    percent_ = 0;

    countDownTimer_.TickTimerPerFrame(context_);
    if (!countDownTimer_.IsRunning())
    {
      if (sceneDuration == 0) { state_ = TSceneState::StopScene; }
      else
      {
        TLOG(LOG_TRACE, "--Running scene %d, (%f)", (currentScene_ + 1), percent_);
        state_ = TSceneState::RunningScene;
        countDownTimer_ = TCountdownTimer(NanosFromMillis(sceneDuration));
        countDownTimer_.Start();
      }
    }
  }
  if (state_ == TSceneState::RunningScene)
  {
    countDownTimer_.TickTimerPerFrame(context_);
    if (!countDownTimer_.IsRunning())
    {
      // Return StopScene and in the sameframe, proceed to the next scene.
      percent_ = 1;
      countDownTimer_.Stop();
      if (postDelay > 0)
      {
        state_ = TSceneState::PostDelay;
        TLOG(LOG_TRACE, "--PostDelay scene %d, (%f)", (currentScene_ + 1), percent_);
        countDownTimer_ = TCountdownTimer(NanosFromMillis(postDelay));
        countDownTimer_.Start();
      }
      else { state_ = TSceneState::StopScene; }
    }
    else
    {
      percent_ = countDownTimer_.Percent();
    }
  }
  if (state_ == TSceneState::PostDelay)
  {
    percent_ = 1;
    countDownTimer_.TickTimerPerFrame(context_);
    if (!countDownTimer_.IsRunning())
    {
      TLOG(LOG_TRACE, "Stopping scene %d, (%f)", (currentScene_ + 1), percent_);
      percent_ = 1;
      state_ = TSceneState::StopScene;
    }
  }
  if (state_ == TSceneState::StopScene)
  {
    TLOG(LOG_TRACE, "Stopped scene %d, (%f)", (currentScene_ + 1), percent_);

    ++currentScene_;
    state_ = TSceneState::StartScene;
    // Make sure to have a gap between two scenes so we don't run them in the same frame.
    // Unless we are repeating, where we want a seamless transition.
    return repeating_;
  }
  return true;
}

bool TScenes::EndScenes()
{
  context_ = nullptr;
  if (state_ == TSceneState::StartScene)
  {
    state_ = TSceneState::EndedAllScenes;
    percent_ = 0;
    if (repeating_) { return Restart(); }
    return true;
  }
  return false;
}

void TScenes::ForceStop()
{
  state_ = TSceneState::EndedAllScenes;
  percent_ = 0;
  currentScene_ = 0;
  runningSceneIndex_ = -1;
  context_ = nullptr;
}

bool TScenes::Restart()
{
  const auto ret = (state_ == TSceneState::EndedAllScenes);
  state_ = TSceneState::StartScene;
  percent_ = 0;
  currentScene_ = 0;
  runningSceneIndex_ = -1;
  context_ = nullptr;
  return ret;
}
}
