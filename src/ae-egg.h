#pragma once

#include "A.h"
#include "AEConfig.h"
#include "options/options.h"
#include <cstdint>
#include <optional>
#include <string>
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
  int32_t y, x;
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
class CDesktopPlus;

class CEggApp;

typedef BEE_Item *(*GetActiveItem)();
typedef CItem *(*GetCItem)(BEE_Item *, bool);
typedef CDirProjItem *(*GetMRUItemDir)(CItem *);
typedef CPanoProjItem *(*GetMRUItemPano)(CDirProjItem *);
typedef M_Point (*CoordXf)(CPanoProjItem *, M_Point *, FEE_CoordFxType,
                           M_Point);

#ifdef AE_OS_WIN
typedef M_Point (*GetLocalMouse)(CPanoProjItem *, M_Point *);
#elifdef AE_OS_MAC
typedef M_Point (*GetLocalMouse)(CPanoProjItem *);
#endif

typedef void (*PointFrameToFloatSource)(CPanoProjItem *, M_Point,
                                        M_Vector2T<double> *);
typedef void (*SetFloatZoom)(CPanoProjItem *, double, LongPt, bool, bool, bool,
                             bool, bool);
typedef double (*GetFloatZoom)(CPanoProjItem *);
typedef short (*GetWidth)(CPanoProjItem *);
typedef short (*GetHeight)(CPanoProjItem *);
typedef void (*ScrollTo)(CPanoProjItem *, LongPt *, bool);
typedef bool (*MemberOfCPanoProjItem)(CDesktopPlus *, M_Point);
typedef CPane *(*GetPaneAtCursor)(CDesktopPlus *, M_Point);

#ifdef AE_OS_WIN
typedef void (*GetPosition)(CPanoProjItem *, LongPt *);
#elifdef AE_OS_MAC
typedef LongPt (*GetPosition)(CPanoProjItem *, LongPt *);
#endif

class ExternalSymbols {
public:
  struct SymbolPointers {
    CEggApp *gEgg = nullptr;
    GetActiveItem GetActiveItemFn = nullptr;
    GetCItem GetCItemFn = nullptr;
    GetMRUItemDir GetMRUItemDirFn = nullptr;
    GetMRUItemPano GetMRUItemPanoFn = nullptr;
    CoordXf CoordXfFn = nullptr;
    GetLocalMouse GetLocalMouseFn = nullptr;
    PointFrameToFloatSource PointFrameToFloatSourceFn = nullptr;
    SetFloatZoom SetFloatZoomFn = nullptr;
    GetFloatZoom GetFloatZoomFn = nullptr;
    GetWidth GetWidhtFn = nullptr;
    GetHeight GetHeightFn = nullptr;
    ScrollTo ScrollToFn = nullptr;
    GetPosition GetPositionFn = nullptr;
    GetPaneAtCursor GetPaneAtCursorFn = nullptr;
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
  M_Point getMouseRelativeToComp();
  LongPt getMouseRelativeToViewPano();
  double getZoom();
  void setZoom(double zoom_value);
  void incrementZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around);

  CPanoProjItem *pano;
  ExternalSymbols::SymbolPointers *extSymbols;
  DoublePt last_view_pos = {999999.0, 999999.0};

  ViewPano(CPanoProjItem *pano, ExternalSymbols::SymbolPointers *extSymbols)
      : pano(pano), extSymbols(extSymbols) {}
};

class AeEgg {
private:
  std::optional<int64_t> CPanoProjItemBasePtr;

public:
  ExternalSymbols extSymbols;

  std::optional<int64_t> getCPanoProjItemBasePtr();
  std::optional<ViewPano> getActiveViewPano();
  std::optional<ViewPano> getViewPanoUnderCursor();

  AeEgg() = default;
  AeEgg(A_long ae_major_version) : extSymbols(ae_major_version){};
};

extern AeEgg gAeEgg;
