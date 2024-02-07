#pragma once

#include "A.h"
#include "AEConfig.h"
#include <nlohmann/json.hpp>
#include <optional>
#include <uiohook.h>

#ifdef AE_OS_MAC
#include <dlfcn.h>
#endif

#ifdef AE_OS_WIN
#include <windows.h>
#endif

class BEE_Project;
class BEE_Item;
class CPane;

struct LongPt {
  long y, x;
};
struct DoublePt {
  double y, x;
};
struct M_Point {
  short y, x;
};

template <typename T> struct M_Vector2T {
  T x, y;
};

enum FEE_CoordFxType {
  Zero,
  One,
  Two,
};

class CView;
class CItem;
class CPanorama;
class CPanoProjItem;
class CDirProjItem;

class CEggApp;

typedef BEE_Item *(*GetActiveItem)();
typedef CItem *(*GetCItem)(BEE_Item *, bool);
typedef CDirProjItem *(*GetMRUItemDir)(CItem *);
typedef CPanoProjItem *(*GetMRUItemPano)(CDirProjItem *);
typedef void (__fastcall *GetCurrentItem)(CEggApp *, BEE_Item **, CPanoProjItem **);
typedef M_Point (*CoordXf)(CPanoProjItem *, M_Point *, FEE_CoordFxType,
                           M_Point);
typedef M_Point (*GetLocalMouse)(CPanoProjItem *, M_Point *);
typedef void (*PointFrameToFloatSource)(CPanoProjItem *, M_Point,
                                        M_Vector2T<double> *);
typedef void (*SetFloatZoom)(CPanoProjItem *, double, LongPt, bool, bool, bool,
                             bool, bool);
typedef double (*GetFloatZoom)(CPanoProjItem *);
typedef short (*GetWidth)(CPanoProjItem *);
typedef short (*GetHeight)(CPanoProjItem *);
typedef void (*ScrollTo)(CPanoProjItem *, LongPt *, bool);
typedef void (*GetPosition)(CPanoProjItem *, LongPt *);

enum class ZOOM_AROUND { PANEL_CENTER, CURSOR_POSTION };

class ExternalSymbols {
public:
  struct SymbolPointers {
    CEggApp *gEgg = nullptr;
    GetActiveItem GetActiveItemFn = nullptr;
    GetCItem GetCItemFn = nullptr;
    GetMRUItemDir GetMRUItemDirFn = nullptr;
    GetMRUItemPano GetMRUItemPanoFn = nullptr;
    GetCurrentItem GetCurrentItemFn = nullptr;
    CoordXf CoordXfFn = nullptr;
    GetLocalMouse GetLocalMouseFn = nullptr;
    PointFrameToFloatSource PointFrameToFloatSourceFn = nullptr;
    SetFloatZoom SetFloatZoomFn = nullptr;
    GetFloatZoom GetFloatZoomFn = nullptr;
    GetWidth GetWidhtFn = nullptr;
    GetHeight GetHeightFn = nullptr;
    ScrollTo ScrollToFn = nullptr;
    GetPosition GetPositionFn = nullptr;
  };

private:
  SymbolPointers mSymbols;
  A_long ae_major_version = 0;

  template <typename T>
  bool loadExternalSymbol(T &symbol_storage, const std::string &symbol_name);

  void load();

public:
  enum class SYMBOLS_LOADING_STATE {
    NOT_LOADED,
    LOADING,
    SOME_NOT_LOADED,
    ALL_LOADED,
  };

  SYMBOLS_LOADING_STATE mLoadingState = SYMBOLS_LOADING_STATE::NOT_LOADED;

  const std::optional<SymbolPointers *> get();

  ExternalSymbols() = default;
  ExternalSymbols(A_long ae_major_version)
      : ae_major_version(ae_major_version){};
};

class ViewPano {
public:
  void setViewPanoPosition(LongPt point);
  LongPt getViewPanoPosition();
  short getCPaneWidth();
  short getCPaneHeight();
  // M_Point ScreenToCompMouse(M_Point screen_p);
  M_Point getMouseRelativeToComp();
  LongPt getMouseRelativeToViewPano();

  CPanoProjItem *pano;
  ExternalSymbols::SymbolPointers extSymbols;

  ViewPano(CPanoProjItem *pano, ExternalSymbols::SymbolPointers extSymbols)
      : pano(pano), extSymbols(extSymbols) {}
};

class AeEgg {
private:
  DoublePt last_view_pos = {999999.0, 999999.0};

public:
  ExternalSymbols extSymbols;

  std::optional<ViewPano> getViewPano();
  bool isMouseInsideViewPano();
  void incrementViewZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around);

  AeEgg() = default;
  AeEgg(A_long ae_major_version) : extSymbols(ae_major_version){};
};

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
