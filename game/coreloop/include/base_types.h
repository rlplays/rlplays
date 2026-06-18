#pragma once
#include <cmath>
#include <cstdint>
#include <memory>
#include <raylib.h>
#include <string>
#include <time.h>
#include "log.h"

/** @brief Pain to type static_cast, so a shortcut! */
#define TO_FLT(x) static_cast<float>(x)

/** @brief Pain to type static_cast, so a shortcut! */
#define TO_INT(x) static_cast<int>(x)

/** In cases where we test a unit that is an API for other private users but may be inaccessible to a test. */
#ifdef RLPLAYS_TEST
#define PUBLIC_TEST public
#else
#define PUBLIC_TEST private
#endif

//! @brief Empty block to help break to prevent compiler optimizing away the code.
#ifdef DEBUG
#define EMPTY_DEBUG_BREAK(...) \
      volatile int dummy = 0; \
      do { ++dummy; }         \
      while (dummy == 0)
#else
#define EMPTY_DEBUG_BREAK(...) static_assert(false, "MUST NOT HAVE ANY EMPTY DEBUG BREAKS IN RELEASE MODE (REMOVE BEFORE COMMITTING!).")
#endif


#if defined(DEBUG) || defined(RLPLAYS_TEST)
#define DBG_ASSERT(x) assert((x))
#else
#define DBG_ASSERT(x) (void (0))
#endif

namespace RLPlays
{
// Blocks can be created on the fly (such as bullets) or be part of the world (such as platforms).
// Given that the sizeof(int) ==  bytes, we can have at most a ~2 billion blocks in the world.
// It's okay as we want to start small (and change this one place if needed).
typedef int TBlockId;
constexpr TBlockId InvalidBlockId = -1;


typedef long long TimeNanos;
static_assert(sizeof(TimeNanos) == 8, "TimeNanos must be 64bits wide!");

typedef long long TimeMillis;
static_assert(sizeof(TimeMillis) == 8, "TimeNanos must be 64bits wide!");

inline double SecondsFromNanos(const TimeNanos ns) { return static_cast<double>(ns) / (1000.0 * 1000.0 * 1000.0); }
inline TimeMillis MillisFromNanos(const TimeNanos ns) { return ns / (1000 * 1000); }
inline TimeNanos NanosFromMillis(const TimeMillis ms) { return ms * (1000 * 1000); }
inline TimeNanos NanosFromSeconds(const float seconds) { return (seconds * (1000.0 * 1000.0 * 1000.0)); }
inline TimeNanos DurationFromFPS(const double fps) { return lround((1000.0 * 1000.0 * 1000.0) / fps); }

inline double CurrentTimeMs() { return (1000.0 * static_cast<double>(clock())) / static_cast<double>(CLOCKS_PER_SEC); }

//! @brief Converts the given milliseconds to TimeMillis.
//! A bit fancy operator overloading - I don't like this, but it simplifies things elsewhere.
inline auto operator ""_ms(unsigned long long ms) { return static_cast<TimeMillis>(ms); }

//! @brief Depth of the block (negative depth values are behind positive depth values).
enum class TLayerDepth : int8_t
{
  Background = -100,
  Foreground = 0,
  Overlay    = 100
};

/**
 * @brief Useful for comparing floats (rectangles, points) whether they are close enough
          in world coords.
 */
constexpr float RLPLAYS_EPSILON = 0.0001f;

/** @brief Returns true if x and y are within a small epsilon. */
static bool FloatEqual(float x, float y, float epsilon = RLPLAYS_EPSILON) { return fabs(x - y) < epsilon; }

/** @brief Returns true if x and y are within a small epsilon. */
static bool FloatIsZero(float x, float epsilon = RLPLAYS_EPSILON) { return fabs(x) < epsilon; }


//! @brief Homage to MonoGame/XNA that raylib is based upon. :)
constexpr Color CORN_FLOWER_BLUE = {100, 149, 237, 255};

//! @brief Empty vector.
constexpr Vector2 VEC_ZERO = {0, 0};

//! @brief Empty rectangle.
constexpr Rectangle RECT_ZERO = {0, 0, 0, 0};

//! @brief Invalid position we can use to distinguish between 'valid' and 'invalid' vectors.
constexpr Vector2 INVALID_POS = {-100000, -100000};

constexpr Rectangle INVALID_RECT = {-100000, -100000, 0, 0};

/**
 * @brief Adapted from and mirroring raylib.h: We want ints for strong guarantees (for certain uses).

 */
struct Vec2i
{
  int x; // Vector x component
  int y; // Vector y component
  [[nodiscard]]
  bool Empty() const { return (x == 0 || y == 0); }

  [[nodiscard]]
  Vector2 ToVector() const { return {TO_FLT(x), TO_FLT(y)}; }
};

inline void MoveRectBy(Rectangle& rect, const Vector2& vec)
{
  rect.x += vec.x;
  rect.y += vec.y;
}


inline float Bottom(const Rectangle& rect) { return rect.y + rect.height; }

inline float Right(const Rectangle& rect) { return rect.x + rect.width; }

inline bool DoesRectContainPos(const Rectangle& rect, const Vector2& pos)
{
  return (pos.x >= rect.x && pos.x < Right(rect) && pos.y >= rect.y && pos.y < Bottom(rect));
}

inline bool DoesRectContainRect(const Rectangle& outerRect, const Rectangle& innerRect)
{
  const auto innerR = innerRect.x + innerRect.width;
  const auto innerB = innerRect.y + innerRect.height;
  const auto outerR = outerRect.x + outerRect.width;
  const auto outerB = outerRect.y + outerRect.height;
  return (innerRect.x >= outerRect.x && innerRect.x < outerR && innerRect.y >= outerRect.y && innerRect.y < outerB) &&
      (innerR >= outerRect.x && innerR < outerR && innerB >= outerRect.y && innerB < outerB);
}

inline bool DoesRectContainX(const Rectangle& rect, const float x) { return (x >= rect.x && x < Right(rect)); }

inline bool DoesRectContainY(const Rectangle& rect, const float y) { return (y >= rect.y && y < Bottom(rect)); }

struct RectI
{
  int x;      // Rectangle top-left corner position x
  int y;      // Rectangle top-left corner position y
  int width;  // Rectangle width
  int height; // Rectangle height
  inline Rectangle ToRect() const { return {TO_FLT(x), TO_FLT(y), TO_FLT(width), TO_FLT(height)}; }
};

//! @brief Returns @param that with the same sign as @param x .
inline float FloatSign(const float x, const float that = 1.0f)
{
  if (FloatIsZero(x)) { return 0.0f; }
  if (x < 0) { return -that; }
  return that;
}


inline Vector2 AddVector(const Vector2& vec1, const Vector2& vec2) { return {(vec1.x + vec2.x), (vec1.y + vec2.y)}; }

inline Vector2 InvertVector(const Vector2& vec) { return {-vec.x, -vec.y}; }

inline Vec2i FromVector(const Vector2& vec) { return {TO_INT(lround(vec.x)), TO_INT(lround(vec.y))}; }
inline Vector2 ToVector(const Vec2i& vec) { return {TO_FLT(vec.x), TO_FLT(vec.y)}; }

inline bool AreVectorsSame(const Vec2i& vec1, const Vec2i& vec2) { return (vec1.x == vec2.x && vec1.y == vec2.y); }

inline bool AreVectorsSame(const Vector2& vec1, const Vector2& vec2)
{
  return (FloatEqual(vec1.x, vec2.x) && FloatEqual(vec1.y, vec2.y));
}

inline bool IsZeroVec(const Vec2i& vec) { return (vec.x == 0 && vec.y == 0); }

inline bool IsZeroVec(const Vector2& vec, const float epsilon = RLPLAYS_EPSILON)
{
  return AreVectorsSame(vec, VEC_ZERO);
}

inline bool AreVectorsSame(const Vector2& vec1, const Vector2& vec2, const float precision)
{
  return (FloatEqual(vec1.x, vec2.x, precision) && FloatEqual(vec1.y, vec2.y, precision));
}

inline bool IsInvalidVector(const Vector2& v) { return AreVectorsSame(v, INVALID_POS); }

inline float RectBottom(const Rectangle& rect) { return rect.y + rect.height; }
inline float RectRight(const Rectangle& rect) { return rect.x + rect.width; }
inline Vector2 RectTopLeft(const Rectangle& rect) { return {rect.x, rect.y}; }
inline Vector2 RectBottomRight(const Rectangle& rect) { return {rect.x + rect.width, rect.y + rect.height}; }

inline Vector2 RectCenter(const Rectangle& rect)
{
  return {rect.x + (rect.width / 2.0f), rect.y + (rect.height / 2.0f)};
}

inline Vector2 RectSize(const Rectangle& rect) { return {rect.width, rect.height}; }


inline void SetRectPos(Rectangle& rect, const Vector2 pos)
{
  rect.x = pos.x;
  rect.y = pos.y;
}

inline void OffsetRectBy(Rectangle& rect, const Vector2& offset)
{
  rect.x += offset.x;
  rect.y += offset.y;
}

[[nodiscard]]
inline Rectangle OffsetRect(const Rectangle& rect, const Vector2& offset)
{
  return {rect.x + offset.x, rect.y + offset.y, rect.width, rect.height};
}

//! @brief Returns a new rectangle scaled by the given factor while maintaining the same center point.
[[nodiscard]]
inline Rectangle ScaleRect(const Rectangle& rect, const float scaleFactor, const Vector2& center = {0.5f, 0.5f})
{
  float centerX = rect.x + rect.width * center.x;
  float centerY = rect.y + rect.height * center.y;
  float newWidth = rect.width * scaleFactor;
  float newHeight = rect.height * scaleFactor;
  float newX = centerX - newWidth * center.x;
  float newY = centerY - newHeight * center.y;

  return {newX, newY, newWidth, newHeight};
}


//! @brief Returns a new rectangle where the width is inverted (negative) from the original one.
inline Rectangle RectXInvert(const Rectangle& rect)
{
  Rectangle invertXRect = rect;
  invertXRect.width = -invertXRect.width;
  return invertXRect;
}

inline RectI FromRect(const Rectangle& rect)
{
  return {TO_INT(lround(rect.x)), TO_INT(lround(rect.y)), TO_INT(lround(rect.width)), TO_INT(lround(rect.height))};
}

//! @brief Returns the union rect that contains both {@param rect1} and {@param rect2}.
inline Rectangle UnionRect(const Rectangle& rect1, const Rectangle& rect2)
{
  const float x = std::min(rect1.x, rect2.x);
  const float y = std::min(rect1.y, rect2.y);
  const float r = std::max(Right(rect1), Right(rect2));
  const float b = std::max(Bottom(rect1), Bottom(rect2));
  return {x, y, r - x, b - y};
}

inline bool AreRectsSame(const Rectangle& rect1, const Rectangle& rect2)
{
  return (FloatEqual(rect1.x, rect2.x) && FloatEqual(rect1.y, rect2.y) && FloatEqual(rect1.width, rect2.width) &&
    FloatEqual(rect1.height, rect2.height));
}

inline float RoundFloatPrecisionToOneDecimalPlace(float x)
{
  return TO_FLT(TO_INT(lround(x * 10.0f))) / 10.0f;
}

inline void EnsureSnapToInt(Rectangle& rect)
{
#ifdef DEBUG
  const float oldX = rect.x;
#endif
  rect.x = RoundFloatPrecisionToOneDecimalPlace(rect.x);
  rect.y = RoundFloatPrecisionToOneDecimalPlace(rect.y);
  rect.width = RoundFloatPrecisionToOneDecimalPlace(rect.width);
  rect.height = RoundFloatPrecisionToOneDecimalPlace(rect.height);
#ifdef DEBUG
  const float newX = rect.x;
  if (std::abs(newX - oldX) > 0.05)
  {
    TLOG(LOG_INFO, "Adjusted rect by a large amount %.2f -> %.2f", oldX, newX);
  }
#endif
}

//! @brief Whether this rect matches (0, 0, 0, 0).
inline bool IsZeroRect(const Rectangle& rect)
{
  return FloatEqual(rect.width, 0) && FloatEqual(rect.height, 0) && FloatEqual(rect.x, 0) || FloatEqual(rect.y, 0);
}

//! @brief Whether this rect has zero size (width is zero or height is zero).
inline bool IsEmptyRect(const Rectangle& rect) { return (FloatEqual(rect.width, 0) || FloatEqual(rect.height, 0)); }

inline bool IsEmptyVec(const Vector2& vec) { return (FloatEqual(vec.x, 0.0f) && FloatEqual(vec.y, 0.0f)); }

//! @brief Returns rect2-rect1 position alone as a Vector2.
inline Vector2 DiffRectPos(const Rectangle& rect1, const Rectangle& rect2)
{
  return {rect2.x - rect1.x, rect2.y - rect1.y};
}

//! @brief Expands the @param rect by the given @param expandBy rectangle.
//! For example if rect = { 100, 100, w=100, h=100 } and expandBy = {10, 10, 10, 10 }
//! then the result will be { x=90, y=90, w=120, h=120 }.
//! This also works if expandBy is negative (i.e. it will contract instead).
[[nodiscard]] inline Rectangle ExpandRect(const Rectangle& rect, const Rectangle& expandBy)
{
  return {
    rect.x - expandBy.x, rect.y - expandBy.y, rect.width + (expandBy.width + expandBy.x),
    rect.height + (expandBy.height + expandBy.y)
  };
}

//! @brief Expand rect on all sides by the same amount.
inline Rectangle ExpandRect(const Rectangle& rect, const float expandBy)
{
  return {rect.x - expandBy, rect.y - expandBy, rect.width + (expandBy * 2), rect.height + (expandBy * 2)};
}

//! @brief Expands the rect on all sides by the given percent (in the range [0, 1]) of its width and height.
[[nodiscard]] inline Rectangle ExpandRectSizePercent(const Rectangle& rect, const float percent)
{
  return ExpandRect(rect, {rect.width * percent, rect.height * percent, rect.width * percent, rect.height * percent});
}

//! @brief Expand rect on all sides by the given width/height.
inline Rectangle ExpandRect(const Rectangle& rect, const Vector2& expandBy)
{
  return {rect.x - expandBy.x, rect.y - expandBy.y, rect.width + (expandBy.x * 2), rect.height + (expandBy.y * 2)};
}

[[nodiscard]] inline Rectangle ExpandRectSizePercent(const Rectangle& rect, const float percentX, const float percentY)
{
  return ExpandRect(rect,
    {rect.width * percentX, rect.height * percentY, rect.width * percentX, rect.height * percentY});
}

inline std::string RectStr(const Rectangle& rect)
{
  return std::string("{") + std::to_string(rect.x) + ", " + std::to_string(rect.y) + "}x{" +
      std::to_string(rect.width) + ", " + std::to_string(rect.height) + "}";
}

inline std::string VecStr(const Vector2& v)
{
  return std::string("{") + std::to_string(v.x) + ", " + std::to_string(v.y) + "}";
}

inline std::string VecStr(const Vec2i& v)
{
  return std::string("{") + std::to_string(v.x) + ", " + std::to_string(v.y) + "}";
}

//! @brief Returns a random integer between [startIncl, endExcl) rounding off down to the nearest snap value.
template <typename T>
inline T Random(T startIncl, T endExcl, T snap = 1)
{
  T ret = (rand() % int(endExcl - startIncl)) + startIncl;
  if (snap != 1)
  {
    ret = T(((int)(ret / snap)) * snap);
  }

  return ret;
}

/**
 * @brief Returns (rect1-rect2) assuming they are intersecting.
 * If they are not intersecting, returns rect1.
 * This obviously does not handle the complex 'region diff' case, but a very simple
 * form of diff.
 */
inline Rectangle DiffRect(const Rectangle& rec1, const Rectangle& rec2)
{
  // Copied from GetCollisionRec and tweaked.
  // CollisionRec = rec1 INTERSECT rec2
  // Diff = rec1 - (rec1 INTERSECT rec2)
  // License:

  /**
   *   LICENSE: zlib/libpng
   *
   *   Copyright (c) 2013-2025 Ramon Santamaria (@raysan5)
   *
   *   This software is provided "as-is", without any express or implied warranty. In no event
   *   will the authors be held liable for any damages arising from the use of this software.
   *
   *   Permission is granted to anyone to use this software for any purpose, including commercial
   *   applications, and to alter it and redistribute it freely, subject to the following restrictions:
   *
   *     1. The origin of this software must not be misrepresented; you must not claim that you
   *     wrote the original software. If you use this software in a product, an acknowledgment
   *     in the product documentation would be appreciated but is not required.
   *
   *     2. Altered source versions must be plainly marked as such, and must not be misrepresented
   *     as being the original software.
   *
   *     3. This notice may not be removed or altered from any source distribution.
   */

  float left = (rec1.x > rec2.x) ? rec1.x : rec2.x;
  float right1 = rec1.x + rec1.width;
  float right2 = rec2.x + rec2.width;
  float right = (right1 < right2) ? right1 : right2;
  float top = (rec1.y > rec2.y) ? rec1.y : rec2.y;
  float bottom1 = rec1.y + rec1.height;
  float bottom2 = rec2.y + rec2.height;
  float bottom = (bottom1 < bottom2) ? bottom1 : bottom2;

  if ((left < right) && (top < bottom))
  {
    if (left > rec1.x)
    {
      right = left;
      left = rec1.x;
    }
    else
    {
      left = right;
      right = right1;
    }
    if (top > rec1.y)
    {
      bottom = top;
      top = rec1.y;
    }
    else
    {
      top = bottom;
      bottom = bottom1;
    }


    return {left, top, right - left, bottom - top};
  }
  return rec1;
}

//! @brief  Type of the current game to ensure the right player1 and player2 behavior.
enum class TGameType : uint8_t
{
  //! @brief User plays Single player, while Player2 is dormant.
  SinglePlayer = 0,

  //! @brief Player1 (user) Vs a recorded version of a prior game as Player2 usually odd-numbered rounds (sans round 1).
  PlayerVsPrior = 1,

  // TODO(perumaal): Not Yet Implemented.
  //! @brief Player1 (user) vs a recorded version of ANOTHER player (not coop)
  PlayerVsPlayer = 2,

  //! @brief Player1 (user) vs Player2 (AI)
  PlayerVsAI = 3,

  //! @brief Player1 is a recorded version of a prior game Vs Player2 (user) usually even numbered rounds.
  PriorVsPlayer = 4,

  //! @brief Player1 (AI) vs Player2 (user)
  AIVsPlayer = 5,

  // TODO(perumaal): Add realtime co-op here.
};

//! @brief State of the game mainly to track scene transitions (in non-headless mode) and when the game ends.
enum class TGameState : uint8_t
{
  //! @brief Started the game (but will not tick).
  StartGame = 0,

  //! @brief Pre-run sequence (after starting, before running).
  AboutToRun = 2,

  //! Menu is being dismissed.
  MenuDismissing = 3,

  //! Menu dismissed, game is showing (but not running).
  MenuDismissedBeforeRunning = 4,

  //! @brief Game has started and is running.
  RunningGame = 5,

  //! @brief Game about to be stopped.
  StoppingGame = 10,

  //! @brief Game running Stopping sequence.
  StoppingSequence = 15,

  //! @brief Game has stopped.
  StopGame = 25,

  //! @brief Game has paused (no updates).
  PauseGame = 40,

  //! @brief Game is in editor mode (no unnecessary drawing).
  EditorMode = 50,
};

constexpr int PlayerId1 = 1;
constexpr int PlayerId2 = 2;


struct TPlayerBlock;
struct TCamera;
struct TWorld;
struct TContext;
struct TBlock;
struct TGrid;
struct ABlock;


// Should be optimized to a simple pointer as it's a const shared_ptr.
typedef const std::shared_ptr<TContext>& TContextPtr;

inline bool IsControlKeyDown() { return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL); }
inline bool IsShiftKeyDown() { return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT); }


enum class TEditorCommandType : uint8_t
{
  None      = 0,
  ShowBlock = 1,
};

struct TEditorCommand
{
  std::string NodeName;
  TEditorCommandType CommandType = TEditorCommandType::ShowBlock;
};

struct TEditorOutput
{
  bool handled = false;
};

enum class TPlayerState : uint8_t
{
  Alive   = 0,
  Won     = 1,
  Dead    = 2,
  TimeOut = 3,
};

} // namespace RLPlays
