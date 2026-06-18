#include <context.h>

#include <game.h>
#include <world.h>

#include "log.h"

namespace RLPlays
{
TimeNanos TCountdownTimer::TickTimerPerFrame(TContextPtr context)
{
  return TickTimerPerFrame(context->FrameDurationNanos);
}
} // namespace RLPlays
