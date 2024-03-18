#include "zoom-actions.h"
#include "JS/JS.h"
#include "ae-egg.h"
#include "util-functions.h"
#include "view-pano.h"

void ZoomActions::post(const KeyBindAction &act) {
  std::lock_guard lock(mutex);

  if (!v.empty() && v.back().action == act.action) {
    if (act.action == KB_ACTION::CHANGE || act.action == KB_ACTION::DECREMENT) {
      v.back().amount += act.amount;
    } else if (act.action == KB_ACTION::SET_TO) {
      v.back().amount = act.amount;
    }
  } else {
    v.push_back(act);
  }
}

A_Err ZoomActions::runActions() {
  /** Lock actions queue while we iterate over it */
  const std::lock_guard lock(mutex);

  A_Err err = A_Err_NONE;

  for (const auto &act : v) {
    double zoomValue = act.getAmount(gHighDpiOptions);

    if (gExperimentalOptions.fixViewportPosition.enabled) {
      std::optional<ViewPano> view_pano;
      if (gExperimentalOptions.detectCursorInsideView &&
          (act.keyCodes.type == EVENT_MOUSE_CLICKED ||
           act.keyCodes.type == EVENT_MOUSE_WHEEL)) {
        view_pano = gAeEgg.getViewPanoUnderCursor();
      } else {
        view_pano = gAeEgg.getActiveViewPano();
      }

      if (view_pano) {
        ZOOM_AROUND zoom_around = ZOOM_AROUND::PANEL_CENTER;

        switch (act.action) {
        case KB_ACTION::DECREMENT: // support deprecated action
          zoomValue = -zoomValue;
        case KB_ACTION::CHANGE: {
          if (gExperimentalOptions.fixViewportPosition.zoomAround ==
                  ZOOM_AROUND::CURSOR_POSTION &&
              act.keyCodes.type == EVENT_MOUSE_WHEEL) {
            zoom_around = ZOOM_AROUND::CURSOR_POSTION;
          }

          auto current_zoom = view_pano->getZoom();
          view_pano->setZoomFixed(current_zoom + zoomValue / 100.0,
                                  zoom_around);
          break;
        }
        case KB_ACTION::SET_TO: {
          view_pano->setZoomFixed(zoomValue / 100.0, zoom_around);
          break;
        }
        default:
          break;
        }
      }
    } else {
      std::string script_str;

      switch (act.action) {
      case KB_ACTION::DECREMENT: // support deprecated action
        zoomValue = -zoomValue;
      case KB_ACTION::CHANGE:
        script_str = zoom_increment_js + "(" + std::to_string(zoomValue) + ")";
        break;
      case KB_ACTION::SET_TO:
        script_str = zoom_set_to_js + "(" + std::to_string(zoomValue) + ")";
        break;
      default:
        break;
      }

      std::tie(err, std::ignore) = RunExtendscript(script_str);
    }
  }

  v.clear();
  return err;
}
