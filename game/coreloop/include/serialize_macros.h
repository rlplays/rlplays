#pragma once

// Note: Using non-UPPER_CASE #defines to prevent SHOUTING at the user for a rather common use-case.
// Only specific to the serialization/deserialization macros here.
#define Serializer(Type, ...) NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Type, __VA_ARGS__)

#define SerializerWithBase(DerivedType, BaseType, ...)                                                                 \
  NLOHMANN_DEFINE_DERIVED_TYPE_INTRUSIVE_WITH_DEFAULT(DerivedType, BaseType, __VA_ARGS__)


#define SerializerType(Type, ...) NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Type, __VA_ARGS__)

//! @brief For composed types, we use this to serialize that sub-block (and allow replacing by the editor without
//! any other special glue code excepting this one place).
#ifdef RLPLAYS_EDITOR
#define AddSubSerializerBlock(Context, FullMemberName)                          \
  if (FullMemberName != nullptr)                                                \
    { Context->AddSubBlock(Context, *this, *(FullMemberName), #FullMemberName,  \
    [=](std::shared_ptr<TBlock> b) { this->FullMemberName = b; }                \
    );                                                                          \
  }                                                                             \
  (void) 0
#else
// In non-editor builds, we will not even fill the function (as it's not used).
#define AddSubSerializerBlock(Context, FullMemberName) \
  if (FullMemberName != nullptr)                                                \
    { Context->AddSubBlock(Context, *this, *(FullMemberName), #FullMemberName,  \
    [=](std::shared_ptr<TBlock> b) {  }                \
    );                                                                          \
  }                                                                             \
  (void) 0
#endif

// Macros to simplify conversion from/to types (note the _WITH_ARG and _GET_MACRO suffixes below that are different
// from the macros in macro_scope.hpp).
// Adapted from macro_scope.hpp (github.com/nlohmann MIT License, see LICENSE.txt)
// License pasted below.
//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013 - 2025 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#define NLOHMANN_JSON_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19,  \
  _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36,   \
  _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53,   \
  _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, NAME, ...)                      \
  NAME
#define NLOHMANN_JSON_PASTE_WITH_ARG(...)                                                                              \
  NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_GET_MACRO(                                                                        \
    __VA_ARGS__, NLOHMANN_JSON_PASTE_WITH_ARG63, NLOHMANN_JSON_PASTE_WITH_ARG62, NLOHMANN_JSON_PASTE_WITH_ARG61,       \
    NLOHMANN_JSON_PASTE_WITH_ARG60, NLOHMANN_JSON_PASTE_WITH_ARG59, NLOHMANN_JSON_PASTE_WITH_ARG58,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG57, NLOHMANN_JSON_PASTE_WITH_ARG56, NLOHMANN_JSON_PASTE_WITH_ARG55,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG54, NLOHMANN_JSON_PASTE_WITH_ARG53, NLOHMANN_JSON_PASTE_WITH_ARG52,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG51, NLOHMANN_JSON_PASTE_WITH_ARG50, NLOHMANN_JSON_PASTE_WITH_ARG49,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG48, NLOHMANN_JSON_PASTE_WITH_ARG47, NLOHMANN_JSON_PASTE_WITH_ARG46,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG45, NLOHMANN_JSON_PASTE_WITH_ARG44, NLOHMANN_JSON_PASTE_WITH_ARG43,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG42, NLOHMANN_JSON_PASTE_WITH_ARG41, NLOHMANN_JSON_PASTE_WITH_ARG40,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG39, NLOHMANN_JSON_PASTE_WITH_ARG38, NLOHMANN_JSON_PASTE_WITH_ARG37,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG36, NLOHMANN_JSON_PASTE_WITH_ARG35, NLOHMANN_JSON_PASTE_WITH_ARG34,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG33, NLOHMANN_JSON_PASTE_WITH_ARG32, NLOHMANN_JSON_PASTE_WITH_ARG31,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG30, NLOHMANN_JSON_PASTE_WITH_ARG29, NLOHMANN_JSON_PASTE_WITH_ARG28,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG27, NLOHMANN_JSON_PASTE_WITH_ARG26, NLOHMANN_JSON_PASTE_WITH_ARG25,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG24, NLOHMANN_JSON_PASTE_WITH_ARG23, NLOHMANN_JSON_PASTE_WITH_ARG22,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG21, NLOHMANN_JSON_PASTE_WITH_ARG20, NLOHMANN_JSON_PASTE_WITH_ARG19,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG18, NLOHMANN_JSON_PASTE_WITH_ARG17, NLOHMANN_JSON_PASTE_WITH_ARG16,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG15, NLOHMANN_JSON_PASTE_WITH_ARG14, NLOHMANN_JSON_PASTE_WITH_ARG13,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG12, NLOHMANN_JSON_PASTE_WITH_ARG11, NLOHMANN_JSON_PASTE_WITH_ARG10,                    \
    NLOHMANN_JSON_PASTE_WITH_ARG9, NLOHMANN_JSON_PASTE_WITH_ARG8, NLOHMANN_JSON_PASTE_WITH_ARG7,                       \
    NLOHMANN_JSON_PASTE_WITH_ARG6, NLOHMANN_JSON_PASTE_WITH_ARG5, NLOHMANN_JSON_PASTE_WITH_ARG4,                       \
    NLOHMANN_JSON_PASTE_WITH_ARG3, NLOHMANN_JSON_PASTE_WITH_ARG2)(__VA_ARGS__))
#define NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) func(arg, v1)
#define NLOHMANN_JSON_PASTE_WITH_ARG3(func, arg, v1, v2)                                                               \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v2)
#define NLOHMANN_JSON_PASTE_WITH_ARG4(func, arg, v1, v2, v3)                                                           \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG3(func, arg, v2, v3)
#define NLOHMANN_JSON_PASTE_WITH_ARG5(func, arg, v1, v2, v3, v4)                                                       \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG4(func, arg, v2, v3, v4)
#define NLOHMANN_JSON_PASTE_WITH_ARG6(func, arg, v1, v2, v3, v4, v5)                                                   \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG5(func, arg, v2, v3, v4, v5)
#define NLOHMANN_JSON_PASTE_WITH_ARG7(func, arg, v1, v2, v3, v4, v5, v6)                                               \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG6(func, arg, v2, v3, v4, v5, v6)
#define NLOHMANN_JSON_PASTE_WITH_ARG8(func, arg, v1, v2, v3, v4, v5, v6, v7)                                           \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG7(func, arg, v2, v3, v4, v5, v6, v7)
#define NLOHMANN_JSON_PASTE_WITH_ARG9(func, arg, v1, v2, v3, v4, v5, v6, v7, v8)                                       \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG8(func, arg, v2, v3, v4, v5, v6, v7, v8)
#define NLOHMANN_JSON_PASTE_WITH_ARG10(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9)                                  \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1) NLOHMANN_JSON_PASTE_WITH_ARG9(func, arg, v2, v3, v4, v5, v6, v7, v8, v9)
#define NLOHMANN_JSON_PASTE_WITH_ARG11(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10)                             \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG10(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define NLOHMANN_JSON_PASTE_WITH_ARG12(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)                        \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG11(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define NLOHMANN_JSON_PASTE_WITH_ARG13(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)                   \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG12(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define NLOHMANN_JSON_PASTE_WITH_ARG14(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)              \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG13(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define NLOHMANN_JSON_PASTE_WITH_ARG15(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)         \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG14(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define NLOHMANN_JSON_PASTE_WITH_ARG16(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)    \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG15(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define NLOHMANN_JSON_PASTE_WITH_ARG17(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16)                                                                            \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG16(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define NLOHMANN_JSON_PASTE_WITH_ARG18(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17)                                                                       \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG17(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define NLOHMANN_JSON_PASTE_WITH_ARG19(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18)                                                                  \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG18(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18)
#define NLOHMANN_JSON_PASTE_WITH_ARG20(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19)                                                             \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG19(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19)
#define NLOHMANN_JSON_PASTE_WITH_ARG21(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20)                                                        \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG20(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20)
#define NLOHMANN_JSON_PASTE_WITH_ARG22(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21)                                                   \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG21(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21)
#define NLOHMANN_JSON_PASTE_WITH_ARG23(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22)                                              \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG22(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22)
#define NLOHMANN_JSON_PASTE_WITH_ARG24(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23)                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG23(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23)
#define NLOHMANN_JSON_PASTE_WITH_ARG25(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24)                                    \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG24(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24)
#define NLOHMANN_JSON_PASTE_WITH_ARG26(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25)                               \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG25(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25)
#define NLOHMANN_JSON_PASTE_WITH_ARG27(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26)                          \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG26(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26)
#define NLOHMANN_JSON_PASTE_WITH_ARG28(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)                     \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG27(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)
#define NLOHMANN_JSON_PASTE_WITH_ARG29(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)                \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG28(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)
#define NLOHMANN_JSON_PASTE_WITH_ARG30(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)           \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG29(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)
#define NLOHMANN_JSON_PASTE_WITH_ARG31(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)      \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG30(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)
#define NLOHMANN_JSON_PASTE_WITH_ARG32(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31) \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG31(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)
#define NLOHMANN_JSON_PASTE_WITH_ARG33(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32)                                                                            \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG32(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)
#define NLOHMANN_JSON_PASTE_WITH_ARG34(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33)                                                                       \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG33(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)
#define NLOHMANN_JSON_PASTE_WITH_ARG35(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34)                                                                  \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG34(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)
#define NLOHMANN_JSON_PASTE_WITH_ARG36(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35)                                                             \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG35(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35)
#define NLOHMANN_JSON_PASTE_WITH_ARG37(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36)                                                        \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG36(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36)
#define NLOHMANN_JSON_PASTE_WITH_ARG38(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37)                                                   \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG37(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37)
#define NLOHMANN_JSON_PASTE_WITH_ARG39(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38)                                              \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG38(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38)
#define NLOHMANN_JSON_PASTE_WITH_ARG40(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39)                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG39(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39)
#define NLOHMANN_JSON_PASTE_WITH_ARG41(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40)                                    \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG40(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40)
#define NLOHMANN_JSON_PASTE_WITH_ARG42(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41)                               \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG41(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41)
#define NLOHMANN_JSON_PASTE_WITH_ARG43(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42)                          \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG42(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42)
#define NLOHMANN_JSON_PASTE_WITH_ARG44(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43)                     \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG43(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43)
#define NLOHMANN_JSON_PASTE_WITH_ARG45(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44)                \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG44(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44)
#define NLOHMANN_JSON_PASTE_WITH_ARG46(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45)           \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG45(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45)
#define NLOHMANN_JSON_PASTE_WITH_ARG47(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46)      \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG46(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46)
#define NLOHMANN_JSON_PASTE_WITH_ARG48(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47) \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG47(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)
#define NLOHMANN_JSON_PASTE_WITH_ARG49(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48)                                                                            \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG48(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)
#define NLOHMANN_JSON_PASTE_WITH_ARG50(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49)                                                                       \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG49(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)
#define NLOHMANN_JSON_PASTE_WITH_ARG51(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50)                                                                  \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG50(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)
#define NLOHMANN_JSON_PASTE_WITH_ARG52(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51)                                                             \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG51(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)
#define NLOHMANN_JSON_PASTE_WITH_ARG53(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52)                                                        \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG52(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52)
#define NLOHMANN_JSON_PASTE_WITH_ARG54(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53)                                                   \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG53(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53)
#define NLOHMANN_JSON_PASTE_WITH_ARG55(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54)                                              \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG54(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54)
#define NLOHMANN_JSON_PASTE_WITH_ARG56(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55)                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG55(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55)
#define NLOHMANN_JSON_PASTE_WITH_ARG57(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56)                                    \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG56(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56)
#define NLOHMANN_JSON_PASTE_WITH_ARG58(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57)                               \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG57(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57)
#define NLOHMANN_JSON_PASTE_WITH_ARG59(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58)                          \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG58(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57, v58)
#define NLOHMANN_JSON_PASTE_WITH_ARG60(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59)                     \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG59(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57, v58, v59)
#define NLOHMANN_JSON_PASTE_WITH_ARG61(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60)                \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG60(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57, v58, v59, v60)
#define NLOHMANN_JSON_PASTE_WITH_ARG62(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61)           \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG61(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57, v58, v59, v60, v61)
#define NLOHMANN_JSON_PASTE_WITH_ARG63(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62)      \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG62(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62)
#define NLOHMANN_JSON_PASTE_WITH_ARG64(func, arg, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15,    \
  v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, \
  v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, \
  v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63) \
  NLOHMANN_JSON_PASTE_WITH_ARG2(func, arg, v1)                                                                         \
  NLOHMANN_JSON_PASTE_WITH_ARG63(func, arg, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17,    \
                                 v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34,  \
                                 v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51,  \
                                 v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63)
