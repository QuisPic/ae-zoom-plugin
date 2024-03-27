#pragma once

#include "external-symbols.h"
#include "options.h"

class ViewPano {
public:
  void setViewPanoPosition(LongPt point);
  LongPt getViewPanoPosition();
  short getCPaneWidth();
  short getCPaneHeight();
  M_Point getMouseRelativeToComp();
  LongPt getMouseRelativeToViewPano();
  double getZoom();
  void setZoom(double zoom_value);
  void setZoomFixed(double zoom_value, ZOOM_AROUND zoom_around);

  CPanoProjItem *pano;
  ExternalSymbols::SymbolPointers *extSymbols;
  DoublePt last_view_pos = {999999.0, 999999.0};

  ViewPano(CPanoProjItem *pano, ExternalSymbols::SymbolPointers *extSymbols)
      : pano(pano), extSymbols(extSymbols) {}
};
