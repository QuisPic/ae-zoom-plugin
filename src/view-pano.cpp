#include "view-pano.h"

void ViewPano::setViewPanoPosition(LongPt point) {
  extSymbols->ScrollToFn(pano, &point, true);
}

LongPt ViewPano::getViewPanoPosition() {
  LongPt pos = {0, 0};
  extSymbols->GetPositionFn(pano, &pos);

  return pos;
}

short ViewPano::getCPaneWidth() {
  auto cpane_width = extSymbols->GetWidhtFn(pano);

  return cpane_width;
}

short ViewPano::getCPaneHeight() {
  auto cpane_height = extSymbols->GetHeightFn(pano);

  return cpane_height;
}

M_Point ViewPano::getMouseRelativeToComp() {
#ifdef AE_OS_WIN
  M_Point comp_mouse;
  extSymbols->GetLocalMouseFn(pano, &comp_mouse);
#elifdef AE_OS_MAC
  auto comp_mouse = extSymbols->GetLocalMouseFn(pano);
#endif

  return comp_mouse;
}

LongPt ViewPano::getMouseRelativeToViewPano() {
  auto mouse_rel_to_comp = getMouseRelativeToComp();
  auto view_pano_position = getViewPanoPosition();

  /* view pano position is negative */
  return {
      mouse_rel_to_comp.y - view_pano_position.y,
      mouse_rel_to_comp.x - view_pano_position.x,
  };
}

double ViewPano::getZoom() { return extSymbols->GetFloatZoomFn(pano); }

void ViewPano::setZoom(double zoom_value) {
  extSymbols->SetFloatZoomFn(pano, zoom_value, {0, 0}, true, true, false, false,
                             true);
}

void ViewPano::setZoomFixed(double zoom_value, ZOOM_AROUND zoom_around) {
  double current_zoom = getZoom();
  // double zoom_value = current_zoom + zoom_delta;

  if (zoom_value < 0.008) {
    zoom_value = 0.008;
  }

  auto actual_view_pos = getViewPanoPosition();

  DoublePt view_pos = {
      lround(last_view_pos.y) == actual_view_pos.y ? last_view_pos.y
                                                   : actual_view_pos.y,
      lround(last_view_pos.x) == actual_view_pos.x ? last_view_pos.x
                                                   : actual_view_pos.x,
  };

  DoublePt zoom_pt;
  DoublePt dist_to_zoom_pt;

  switch (zoom_around) {
  case ZOOM_AROUND::PANEL_CENTER: {
    double cpane_width2 = getCPaneWidth() / 2.0;
    double cpane_height2 = getCPaneHeight() / 2.0;

    dist_to_zoom_pt = {
        -cpane_height2 - view_pos.y,
        -cpane_width2 - view_pos.x,
    };

    zoom_pt = {cpane_height2, cpane_width2};

    break;
  }
  case ZOOM_AROUND::CURSOR_POSTION: {
    M_Point comp_mouse_pos = getMouseRelativeToComp();

    dist_to_zoom_pt = {
        static_cast<double>(-comp_mouse_pos.y),
        static_cast<double>(-comp_mouse_pos.x),
    };

    zoom_pt = {
        static_cast<double>(-view_pos.y + comp_mouse_pos.y),
        static_cast<double>(-view_pos.x + comp_mouse_pos.x),
    };

    break;
  }
  default:
    /* exit the function */
    return;
  }

  DoublePt new_view_pos = {
      -(dist_to_zoom_pt.y * (zoom_value / current_zoom) + zoom_pt.y),
      -(dist_to_zoom_pt.x * (zoom_value / current_zoom) + zoom_pt.x),
  };

  setZoom(zoom_value);

  setViewPanoPosition({static_cast<int32_t>(lround(new_view_pos.y)),
                       static_cast<int32_t>(lround(new_view_pos.x))});

  last_view_pos = new_view_pos;
}
