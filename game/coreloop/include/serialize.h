#ifndef SERIALIZE_H_
#define SERIALIZE_H_

#include <fstream>

#include <base_types.h>
#include <nlohmann/json.hpp>
#include <serialize_macros.h>
#include "raylib.h"


SerializerType(Rectangle, x, y, width, height);
SerializerType(Vector2, x, y);
SerializerType(Vector3, x, y, z);
SerializerType(Vector4, x, y, z, w);
SerializerType(Color, r, g, b, a);

namespace RLPlays
{
SerializerType(Vec2i, x, y);
SerializerType(RectI, x, y);
} // namespace RLPlays

//
// TODO(perumaal): Convert all json calls to use int keys instead of strings. And avoid using json itself, just
// use BSON or something else. For now, it's quick to implement and easy to debug (and it's human-readable too!).
//

// These are specific to TBlock -> (derived)Block conversion.

// Use a simple lookup table (if conditions essentially) instead of dynamic casting/dispatching.
// Use the enum as a strong identifier (similar in spirit to FlatBuffers/ProtoBuf/Cap'nProto style unions/oneofs)

// Assumes Derived class is of TDerivedNameDerivedTypeSuffix (e.g. TPlayerBlock)
// and the type enum is TDerivedTypeSuffixType (e.g. TBlockType)
// and the base class is explicitly specified (e.g. ABlock)

// e.g. If DerivedName = Player, then json["TPlayerBlock"] =
// *std::static_pointer_cast<TPlayerBlock>(nlohmann_json_t.Block);
#define SerializeDerivedType(DerivedTypeSuffix, DerivedName)                                                           \
  if (nlohmann_json_t.DerivedTypeSuffix##Type == T##DerivedTypeSuffix##Type::DerivedName)                              \
  {                                                                                                                    \
    nlohmann_json_j[#DerivedName #DerivedTypeSuffix] =                                                                 \
      *std::static_pointer_cast<T##DerivedName##DerivedTypeSuffix>(nlohmann_json_t.DerivedTypeSuffix);                 \
  }

// If DerivedName = Player, then assign Block = make_shared<TPlayerBlock>(j.value("PlayerBlock", TPlayerBlock()); (i.e.
// use default if the PlayerBlock key/value pair is not found)
#define DeserializeDerivedType(DerivedTypeSuffix, DerivedName)                                                         \
  if (nlohmann_json_t.DerivedTypeSuffix##Type == T##DerivedTypeSuffix##Type::DerivedName)                              \
  {                                                                                                                    \
    auto default_##DerivedName##_##DerivedTypeSuffix = T##DerivedName##DerivedTypeSuffix();                            \
    nlohmann_json_t.DerivedTypeSuffix = std::make_shared<T##DerivedName##DerivedTypeSuffix>(                           \
      nlohmann_json_j.value(#DerivedName #DerivedTypeSuffix, default_##DerivedName##_##DerivedTypeSuffix));            \
  }

#define DefineReflection(DerivedTypeSuffix, DerivedName)                                                               \
  {                                                                                                                    \
    ret.push_back(                                                                                                     \
      {T##DerivedTypeSuffix##Type::DerivedName, #DerivedName, std::make_shared<T##DerivedName##DerivedTypeSuffix>()}); \
  }


/*
Actual expansion:
template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value,
int> = 0> friend void to_json(BasicJsonType& nlohmann_json_j, const TBlock& nlohmann_json_t) {
    nlohmann_json_j["BlockType"] = nlohmann_json_t.BlockType;
    if (nlohmann_json_t.BlockType == TBlockType::Player) {
        nlohmann_json_j["Player""Block"] = *std::static_pointer_cast<TPlayerBlock>(nlohmann_json_t.Block);
    }
}

template<typename BasicJsonType, nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value,
int> = 0> friend void from_json(const BasicJsonType& nlohmann_json_j, TBlock& nlohmann_json_t) { const TBlock
nlohmann_json_default_obj{}; nlohmann_json_t.BlockType = !nlohmann_json_j.is_null() ? nlohmann_json_j.value("BlockType",
nlohmann_json_default_obj.BlockType) : nlohmann_json_default_obj.BlockType; if (nlohmann_json_t.BlockType ==
TBlockType::Player) { auto default_Player_Block = TPlayerBlock(); nlohmann_json_t.Block =
std::make_shared<TPlayerBlock>(nlohmann_json_j.value("Player""Block", default_Player_Block));
    }
}

*/


/**
@brief Serializes derived class definitions based on a provided container (not base class) holding
       a type enum and a shared pointer to a derived class.

Assumes there is a class like this:

class ContainerType {
  TTypeEnum TypeEnum;
  std::shared_ptr<BaseClass> DerivedType;

  // Use this to generate to/from serialization methods to ContainerType (intrusive with default).
  SerializeDerived(ContainerType, TypeEnum, DerivedType, DerivedType1, DerivedType2...)
}

*/
#if RLPLAYS_EDITOR
#define SerializerDerivedReflection(ContainerType, BaseClass, TypeEnum, DerivedType, ...)                            \
struct TReflection_##BaseClass                                                                                       \
{                                                                                                                    \
  ContainerType##Type TypeEnum;                                                                                      \
  std::string TypeName;                                                                                              \
  std::shared_ptr<BaseClass> BlockInstance;                                                                          \
};                                                                                                                   \
static std::vector<TReflection_##BaseClass> GetReflection()                                                          \
{                                                                                                                    \
  std::vector<TReflection_##BaseClass> ret;                                                                          \
  NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE_WITH_ARG(DefineReflection, DerivedType, __VA_ARGS__))                     \
  return ret;                                                                                                        \
}
#else
// Remove reflection from non-editor builds.
#define SerializerDerivedReflection(ContainerType, BaseClass, TypeEnum, DerivedType, ...)
#endif

#define SerializerDerived(ContainerType, BaseClass, TypeEnum, DerivedType, ...)                                        \
  SerializerDerivedReflection(ContainerType, BaseClass, TypeEnum, DerivedType, __VA_ARGS__)                            \
  template <typename BasicJsonType,                                                                                    \
            nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0>             \
  friend void to_json(BasicJsonType& nlohmann_json_j, const ContainerType& nlohmann_json_t)                            \
  {                                                                                                                    \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, TypeEnum))                                              \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE_WITH_ARG(SerializeDerivedType, DerivedType, __VA_ARGS__))                 \
  }                                                                                                                    \
  template <typename BasicJsonType,                                                                                    \
            nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0>             \
  friend void from_json(const BasicJsonType& nlohmann_json_j, ContainerType& nlohmann_json_t)                          \
  {                                                                                                                    \
    const ContainerType nlohmann_json_default_obj{};                                                                   \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM_WITH_DEFAULT, TypeEnum))                               \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE_WITH_ARG(DeserializeDerivedType, DerivedType, __VA_ARGS__))               \
  }


/**
 * @brief Useful for shared pointer conversions.
 */
// From https://github.com/nlohmann/json/discussions/2377
namespace nlohmann
{
template <typename T>
struct adl_serializer<std::shared_ptr<T>>
{
  // If you find an error here, likely you have not added a Serializer block to your shared_ptr<T>'s T.
  static void to_json(json& j, const std::shared_ptr<T>& opt)
  {
    if (opt)
    {
      j = *opt;
    }
    else
    {
      j = nullptr;
    }
  }

  static void from_json(const json& j, std::shared_ptr<T>& opt)
  {
    if (j.is_null())
    {
      opt = nullptr;
    }
    else
    {
      opt.reset(new T(j.get<T>()));
    }
  }
};
}

#define SaveToJson(encoded, obj) \
    const json data(obj); \
    std::string encoded = data.dump()

#define LoadFromJson(encoded, obj) \
     obj = json::parse(encoded).template get<decltype(obj)>()

namespace RLPlays
{
//! @brief Unfolded as a pair in C++.
struct TSerializerOutput
{
  int NextIndex = 0;

  union DataType
  {
    uint8_t Byte;
    uint16_t UInt16;
    uint32_t UInt32;
    int32_t Int32;
    float Float;
    double Double;
  } Data;
};

inline int GoToNext(const std::string& str, TSerializerOutput& prev)
{
  const auto prevIndex = prev.NextIndex;
  while (prev.NextIndex < str.size())
  {
    if (str[prev.NextIndex] == ',')
    {
      prev.NextIndex++;
      break;
    }
    prev.NextIndex++;
  }
  return prevIndex;
}

inline int ParseInt32(const std::string& str, TSerializerOutput& prev)
{
  int prevIndex = GoToNext(str, prev);
  if (prevIndex < (int)str.size())
  {
    prev.Data.Int32 = atoi(str.c_str() + prevIndex);
  }
  return prev.Data.Int32;
}

//! @brief Parses an LSB-first (see UInt32ToString) hex string into an uint32_t.
inline uint32_t ParseUInt32(const std::string& str, TSerializerOutput& prev)
{
  // Co-written by Claude.
  prev.Data.UInt32 = 0;
  int i = 0;
  while (prev.NextIndex < (int)str.size() && i < 8)
  {
    char hexChar = str[prev.NextIndex];
    // Encoded in 
    if (hexChar >= 'A' && hexChar <= 'F')
    {
      prev.Data.UInt32 = (prev.Data.UInt32) | (uint32_t(hexChar - 'A' + 10) << (i * 4));
    }
    else if (hexChar >= 'a' && hexChar <= 'f')
    {
      prev.Data.UInt32 = (prev.Data.UInt32) | (uint32_t(hexChar - 'a' + 10) << (i * 4));
    }
    else if (hexChar >= '0' && hexChar <= '9')
    {
      prev.Data.UInt32 = (prev.Data.UInt32) | (uint32_t(hexChar - '0') << (i * 4));
    }
    else
    {
      break;
    }
    prev.NextIndex++;
    i++;
  }
  GoToNext(str, prev);
  return prev.Data.UInt32;
}


inline float ParseFloat(const std::string& str, TSerializerOutput& prev)
{
  int prevIndex = GoToNext(str, prev);
  if (prevIndex < prev.NextIndex)
  {
    prev.Data.Float = atof(str.c_str() + prevIndex);
  }
  return prev.Data.Float;
}


inline double ParseDouble(const std::string& str, TSerializerOutput& prev)
{
  int prevIndex = GoToNext(str, prev);
  if (prevIndex < prev.NextIndex)
  {
    prev.Data.Double = std::strtod(str.c_str() + prevIndex, nullptr);
  }
  return prev.Data.Double;
}

//! @brief Returns a LSB-first string (with leading zeros stripped out). This is simpler to process
//! from a strictly encode/decode perspective. Pass in a pre-reserved string to avoid allocations.
inline std::string& UInt32ToString(uint32_t val, std::string& s)
{
  // NOTE: Writing LSB first here.
  // Co-written by Claude.
  for (int i = 0; i < 8; ++i)
  {
    uint8_t nibble = (val & 0xF);
    const char hexChar = nibble < 10 ? ('0' + nibble) : ('A' + (nibble - 10));
    s += hexChar;
    val >>= 4;
    if (val == 0) break;
  }
  return s;
}

inline std::string& EncodeStringWithLen(const std::string& in, std::string& out)
{
  UInt32ToString(static_cast<uint32_t>(in.size()), out) += ',';
  out += in;
  return out;
}

inline std::string DecodeStringWithLen(const std::string& in, TSerializerOutput& prev)
{
  const auto len = ParseUInt32(in, prev);
  const auto startIndex = prev.NextIndex;
  const auto endIndex = std::min(startIndex + static_cast<int>(len), static_cast<int>(in.size()));
  std::string out = in.substr(startIndex, endIndex - startIndex);
  prev.NextIndex = endIndex;
  GoToNext(in, prev);
  return out;
}

constexpr const char* base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

//! @brief Convert a string to base-64 encoded string to ensure it's ASCII-safe.
inline std::string& ToBase64(const std::string& input, std::string& output)
{
  // Written with Claude's help.
  int val = 0, valb = -6;
  int outputSize = output.size();
  for (unsigned char c : input)
  {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0)
    {
      output.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while ((output.size() - outputSize) % 4) output.push_back('=');
  return output;
}

//! @brief Convert a string from base-64 encoded to a normal string.
inline std::string FromBase64(const std::string& str, TSerializerOutput& prev)
{
  // Create inverse lookup table for decoding
  static constexpr unsigned char lookup[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63, // '+' is 62, '/' is 63
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64, // '0'-'9' are 52-61
    64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,           // 'A'-'O' are 0-14
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, // 'P'-'Z' are 15-25
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 'a'-'o' are 26-40
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, // 'p'-'z' are 41-51
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
  };

  int val = 0;
  int valb = -8;

  std::string output;
  int prevIndex = GoToNext(str, prev);
  if (prevIndex + 1 >= prev.NextIndex) { return ""; }
  for (int i = prevIndex; i < prev.NextIndex; ++i)
  {
    const unsigned char c = str[i];
    // Skip padding characters
    if (c == '=')
    {
      break;
    }

    // Get the decoded value for this character
    unsigned char decoded = lookup[c];
    if (decoded == 64)
    {
      // Invalid base64 character, skip it
      continue;
    }

    // Process each valid base64 character
    val = (val << 6) | decoded;
    valb += 6;

    // Every 8 bits, we have a complete byte to output
    if (valb >= 0)
    {
      output.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }

  return output;
}
} // namespace RLPlays
#endif // !SERIALIZE_H_
