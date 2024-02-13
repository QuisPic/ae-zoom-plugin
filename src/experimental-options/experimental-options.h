#pragma once
#include <nlohmann/json.hpp>

enum class ZOOM_AROUND { PANEL_CENTER, CURSOR_POSTION };

struct ViewPositionExperimentalOption {
  bool enabled;
  ZOOM_AROUND zoomAround;

  ViewPositionExperimentalOption()
      : enabled(false), zoomAround(ZOOM_AROUND::PANEL_CENTER){};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(ViewPositionExperimentalOption, enabled,
                                 zoomAround);
};

struct ExperimentalOptions {
  bool detectCursorInsideView;
  ViewPositionExperimentalOption fixViewportPosition;

  ExperimentalOptions()
      : detectCursorInsideView(false), fixViewportPosition(){};

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(ExperimentalOptions, detectCursorInsideView,
                                 fixViewportPosition);
};

extern ExperimentalOptions gExperimentalOptions;
