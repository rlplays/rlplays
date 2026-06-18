#pragma once

#include <base_types.h>
#include <nlohmann/json.hpp>
#include <serialize.h>


namespace RLPlays
{
struct TCamera
{
  /**
   * @brief A cell holds a block.
   */
  Vector2 CellSize;
  Rectangle Viewport;
  Color BgColor = GREEN;

  template <class T_Vec>
  Vector2 SnapToCell(const T_Vec& v) const
  {
    return {(TO_INT(v.x / CellSize.x) * CellSize.x), (TO_INT(v.y / CellSize.y) * CellSize.y)};
  }

  Serializer(TCamera, CellSize, Viewport, BgColor)
};

} // namespace RLPlays
