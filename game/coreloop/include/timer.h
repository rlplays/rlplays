#pragma once
#include <base_types.h>

namespace RLPlays
{
//! \brief A simple countdown timer with a default (const) set time.
//! \details Allows start/stop and ticking based on the context's frame time.
struct TCountdownTimer
{
  TimeNanos TimeSet = {0};

  inline void Start() { currentTime_ = 0; }

  inline void Stop() { currentTime_ = -1; }

  [[nodiscard]] inline bool HasStarted() const { return currentTime_ >= 0; }
  [[nodiscard]] inline bool IsRunning() const { return currentTime_ >= 0 && currentTime_ < TimeSet; }

  //! \brief Tick the timer by the given amount of time and returns the total time.
  //! \details Ensures that the total time does not exceed the set countdown time.
  inline TimeNanos TickTimerPerFrame(const TimeNanos diffNanos)
  {
    if (currentTime_ < 0 || currentTime_ >= TimeSet)
    {
      return -1;
    }
    return (currentTime_ = std::min(currentTime_ + diffNanos, TimeSet));
  }

  TimeNanos TickTimerPerFrame(TContextPtr context);

  [[nodiscard]] double PercentLeft() const
  {
    return (currentTime_ >= 0) ? (1.0 - (static_cast<double>(currentTime_) / static_cast<double>(TimeSet))) : 0.0;
  }

  [[nodiscard]] double Percent() const
  {
    return (currentTime_ >= 0) ? (static_cast<double>(currentTime_) / static_cast<double>(TimeSet)) : 0.0;
  }

  [[nodiscard]] TimeNanos TimeRemaining() const
  {
    if (currentTime_ < 0 || currentTime_ >= TimeSet)
    {
      return -1;
    }
    return TimeSet - currentTime_;
  }

  [[nodiscard]] TimeNanos TimeElapsed() const
  {
    if (currentTime_ < 0) { return -1; }
    return currentTime_;
  }


  [[nodiscard]] TimeMillis TimeRemainingMillis() const { return MillisFromNanos(TimeRemaining()); }

  Serializer(TCountdownTimer, TimeSet)

  //! @brief Returns true if count-down timer has started and has a valid time set.
  bool IsValid() const { return TimeSet > 0 && currentTime_ >= 0; }

  TCountdownTimer() = default;
  TCountdownTimer(const TimeNanos timeSet) : TimeSet(timeSet) {}

private:
  TimeNanos currentTime_ = -1;
};
} // namespace RLPlays
