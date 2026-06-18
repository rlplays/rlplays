#pragma once
#include <cstdint>

#include <serialize.h>
#include "serialize_macros.h"
#include <base_types.h>

namespace RLPlays
{
//! @brief Bit-mask for player actions mapped from keyboard/controller etc.
enum class TPlayerAction : uint8_t
{
  None      = 0,
  WalkLeft  = 0x1,
  WalkRight = 0x2,
  Jump      = 0x4,
  Activate  = 0x8,
  Use       = 0x10,
  Crouch    = 0x20,
  GameStart = 0x40,
  GameMenu  = 0x80,
  // NOTE: If you add actions here, make sure to adjust the number of actions below.
};

// For now, Walk/Jump alone (ignoring None).
constexpr int MAX_NUM_ACTIONS = 3;


struct TPlayerActions
{
  TPlayerAction Action = TPlayerAction::None;
  int FrameIndex = -1;
  Vector2 Position = {0, 0};
};

/**
 * @brief Checks whether the enum @paramref values contains @paramref value.
 */
template <class T_Enum>
[[nodiscard]]
static bool HasEnumValue(const T_Enum& values, const T_Enum& value)
{
  if (static_cast<uint64_t>(value) == 0)
  {
    return values == static_cast<T_Enum>(0);
  }
  return (static_cast<uint64_t>(values) & static_cast<uint64_t>(value)) == static_cast<uint64_t>(value);
}

/**
 * @brief Checks whether the enum @paramref values contains at least one of the provided bitmask @paramref value.
 */
template <class T_Enum>
[[nodiscard]]
static bool HasOneOfEnumValue(const T_Enum& values, const T_Enum& valueSet)
{
  if (static_cast<uint64_t>(valueSet) == 0)
  {
    return values == static_cast<T_Enum>(0);
  }
  return (static_cast<uint64_t>(values) & static_cast<uint64_t>(valueSet)) != static_cast<uint64_t>(0);
}

/**
 * @brief Adds enum @paramref value to @paramref values.
 */
template <class T_Enum>
[[nodiscard]]
static T_Enum AddEnumValue(const T_Enum& values, const T_Enum value)
{
  return static_cast<T_Enum>(static_cast<uint64_t>(values) | static_cast<uint64_t>(value));
}

/**
 * @brief Adds enum @paramref value to @paramref values.
 */
template <class T_Enum>
[[nodiscard]]
static T_Enum AddEnumValue(const T_Enum& values, const T_Enum value1, const T_Enum value2)
{
  return static_cast<T_Enum>(static_cast<uint64_t>(values) | static_cast<uint64_t>(value1) |
    static_cast<uint64_t>(value2));
}

/**
 * @brief Adds enum @paramref value to @paramref values.
 */
template <class T_Enum>
[[nodiscard]]
static T_Enum AddEnumValue(const T_Enum& values, const T_Enum value1, const T_Enum value2, const T_Enum value3)
{
  return static_cast<T_Enum>(static_cast<uint64_t>(values) | static_cast<uint64_t>(value1) |
    static_cast<uint64_t>(value2) | static_cast<uint64_t>(value3));
}

/**
 * @brief Adds enum @paramref value to @paramref values.
 */
template <class T_Enum>
[[nodiscard]]
static T_Enum AddEnumValue(const T_Enum& values, const T_Enum value1, const T_Enum value2, const T_Enum value3,
  const T_Enum value4)
{
  return static_cast<T_Enum>(static_cast<uint64_t>(values) | static_cast<uint64_t>(value1) |
    static_cast<uint64_t>(value2) | static_cast<uint64_t>(value3) | static_cast<uint64_t>(value4));
}


/**
 * @brief Removes enum @paramref value from @paramref values.
 */
template <class T_Enum>
[[nodiscard]]
static T_Enum RemoveEnumValue(const T_Enum& values, const T_Enum& value)
{
  return static_cast<T_Enum>(static_cast<uint64_t>(values) & ~static_cast<uint64_t>(value));
}


//! @brief Adds or removes the enum @paramref value from @paramref values based on whether @paramref that has
//! the specified @paramref value set.
template <class T_Enum>
[[nodiscard]]
static T_Enum CopyEnumValue(const T_Enum& values, const T_Enum& that, const T_Enum value)
{
  if (HasEnumValue(that, value)) { return AddEnumValue(values, value); }
  return RemoveEnumValue(values, value);
}


/**
 * @brief Replaces enum @paramref fromValue with @paramref toValue from @paramref values.
 * NOTE: Using && purely to indicate that the value is a temporary value.
 */
template <class T_Enum>
[[nodiscard]]
static T_Enum ReplaceEnumValue(const T_Enum& values, const T_Enum&& fromValue, const T_Enum&& toValue)
{
  return static_cast<T_Enum>((static_cast<uint64_t>(values) & ~static_cast<uint64_t>(fromValue)) |
    static_cast<uint64_t>(toValue));
}

//! @brief Returns true if the enum @paramref values has any value set (i.e. not zero).
template <class T_Enum>
[[nodiscard]]
static bool HasAnyEnumValue(const T_Enum& values) { return static_cast<uint64_t>(values) != 0U; }


//! @brief Converts an action value in the range of [0, MAX_NUM_ACTIONS) into a TPlayerAction enum (bitmask) val.
inline TPlayerAction ConvertToPlayerAction(const int* actions, const int numActions)
{
  TPlayerAction playerAction = TPlayerAction::None;
  for (uint32_t i = 0; i < numActions; ++i)
  {
    TASSERT(actions[i] == 0 || actions[i] == 1, "Invalid action value %d at index %d", actions[i], i);
    if (actions[i] != 0) { playerAction = AddEnumValue(playerAction, TPlayerAction(1 << i)); }
  }
  return playerAction;
}

//! @brief Converts player action into a simple int.
inline void ConvertFromPlayerAction(const TPlayerAction actionVal, int* actions, const int maxActions)
{
  auto action = TPlayerAction(uint32_t(TPlayerAction::None) + 1);
  for (int i = 0; i < maxActions; ++i)
  {
    if (HasEnumValue(actionVal, action)) { actions[i] = 1; }
    else { actions[i] = 0; }
    action = TPlayerAction(uint32_t(action) << 1);
  }
}
} // namespace RLPlays
