#pragma once

#include <A.h>
#include <string>

class MangledNames {
public:
  static const std::string gEgg(const A_long major_version);
  static const std::string GetActiveItem(const A_long major_version);
  static const std::string GetCItem(const A_long major_version);
  static const std::string GetMRUItemDir(const A_long major_version);
  static const std::string GetMRUItemPano(const A_long major_version);
  static const std::string GetCurrentItem(const A_long major_version);
  static const std::string CoordXf(const A_long major_version);
  static const std::string GetLocalMouse(const A_long major_version);
  static const std::string PointFrameToFloatSource(const A_long major_version);
  static const std::string SetFloatZoom(const A_long major_version);
  static const std::string GetFloatZoom(const A_long major_version);
  static const std::string GetWidth(const A_long major_version);
  static const std::string GetHeight(const A_long major_version);
  static const std::string ScrollTo(const A_long major_version);
  static const std::string GetPosition(const A_long major_version);
  static const std::string GetPaneAtCursor(const A_long major_version);
};
