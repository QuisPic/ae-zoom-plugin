#pragma once

#include "AEConfig.h"
#include "A.h"
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

typedef BEE_Item *(__stdcall *GetActiveItem)();
typedef CItem *(__stdcall *GetCItem)(BEE_Item *, bool);
typedef CDirProjItem *(__stdcall *GetMRUItemDir)(CItem *);
typedef CPanoProjItem *(__stdcall *GetMRUItemPano)(CDirProjItem *);
typedef void(__stdcall *GetCurrentItem)(CEggApp **, BEE_Item **,
                                        CPanoProjItem **);
typedef M_Point(__stdcall *CoordXf)(CPanoProjItem *, M_Point *, FEE_CoordFxType,
                                    M_Point);
typedef M_Point(__stdcall *GetLocalMouse)(CPanoProjItem *, M_Point *);
typedef void(__stdcall *PointFrameToFloatSource)(CPanoProjItem *, M_Point,
                                                 M_Vector2T<double> *);
typedef void(__stdcall *SetFloatZoom)(CPanoProjItem *, double, LongPt, bool,
                                      bool, bool, bool, bool);
typedef double(__stdcall *GetFloatZoom)(CPanoProjItem *);

enum class ZOOM_AROUND { PANEL_CENTER, CURSOR_POSTION };

class AeEgg {
private:
  class ExternalSymbols {
  private:
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
    };

    SymbolPointers mSymbols;
    A_long ae_major_version = 0;

    template <typename T>
    bool loadExternalSymbol(T &symbol_storage, const std::string &symbol_name);

  public:
    enum class SYMBOLS_LOADING_STATE {
      NOT_LOADED,
      LOADING,
      SOME_NOT_LOADED,
      ALL_LOADED,
    };

    SYMBOLS_LOADING_STATE mLoadingState = SYMBOLS_LOADING_STATE::NOT_LOADED;

    ExternalSymbols() = default;
    ExternalSymbols(A_long ae_major_version)
        : ae_major_version(ae_major_version){};

    void load();
    const std::optional<SymbolPointers *> get();
  };

  ExternalSymbols mExSymbols;
  DoublePt last_view_pos = {999999.0, 999999.0};

  bool isViewPanoExists();
  CPanoProjItem *getViewPano();
  void setViewPanoPosition(LongPt point);
  LongPt getViewPanoPosition();
  short getCPaneWidth();
  short getCPaneHeight();
  M_Point ScreenToCompMouse(POINT screen_p);
  M_Point getMouseRelativeToComp();
  LongPt getMouseRelativeToViewPano();

public:
  bool isMouseInsideViewPano();
  void incrementViewZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around);

  AeEgg() = default;
  AeEgg(A_long ae_major_version) : mExSymbols(ae_major_version){};
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
