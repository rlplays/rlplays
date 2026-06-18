#include <raylib_utils.h>

namespace RLPlays
{
std::unordered_set<std::string> THeadless::ContentFilenames;

// Whether headless mode is enabled for this build. For RL training/tests/converter runs, skip all rendering/raylib work.
bool THeadless::IsHeadless =
#if RLPLAYS_HEADLESS
// May be overriden by rlplays.cpp during EVAL mode.
    true;
#else
    false;
#endif
}
