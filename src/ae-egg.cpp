#include "ae-egg.h"
#include "logger.h"
#include "mangled-names/mangled-names.h"

#ifdef AE_OS_WIN
static std::string GetLastWindowsErrorStr() {
  // Retrieve the system error message for the last-error code

  std::string result;
  LPVOID lpMsgBuf;
  DWORD dw = GetLastError();

  DWORD message_length = FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0,
      NULL);

  if (message_length != 0) {
    result = std::string(static_cast<char *>(lpMsgBuf));
  }

  LocalFree(lpMsgBuf);

  return result;
}

template <typename T> T getFromAfterFXDll(const std::string &fn_name) {
  T result = nullptr;
  HINSTANCE hDLL = LoadLibrary("AfterFXLib.dll");

  if (!hDLL) {
    auto winapi_error = GetLastWindowsErrorStr();
    logger(LOG_LEVEL_ERROR, "\n\nFailed to load AfterFXLib.dll\n",
           winapi_error);
  } else {
    result = reinterpret_cast<T>(GetProcAddress(hDLL, fn_name.c_str()));

    if (!result) {
      auto winapi_error = GetLastWindowsErrorStr();
      const std::string error_string =
          "\n\nCan't find symbol " + fn_name + " in AfterFXLib.dll\n";
      logger(LOG_LEVEL_ERROR, error_string, winapi_error);
    }

    FreeLibrary(hDLL);
  }

  return result;
}
#endif

#ifdef AE_OS_MAC
template <typename T> T getFromAfterFXDll(const std::string &fn_name) {
  void *handle = dlopen("AfterFXLib.framework/AfterFXLib", RTLD_LAZY);

  if (!handle) {
    logger(LOG_LEVEL_ERROR, "Failed to load AfterFXLib.framework",
           std::string(dlerror()));
    return nullptr;
  }

  dlerror(); // reset errors
  T result = reinterpret_cast<T>(dlsym(handle, fn_name.c_str()));

  const char *dlsym_error = dlerror();

  if (dlsym_error) {
    logger(LOG_LEVEL_ERROR, "Cannot load symbol " + fn_name,
           std::string(dlsym_error));
    dlclose(handle);
    return nullptr;
  }

  dlclose(handle);
  return result;
}
#endif

template <typename T>
bool AeEgg::ExternalSymbols::loadExternalSymbol(
    T &symbol_storage, const std::string &symbol_name) {
  symbol_storage = getFromAfterFXDll<T>(symbol_name);

  return symbol_storage == nullptr ? false : true;
}

void AeEgg::ExternalSymbols::load() {
  bool loading_err = false;

  mLoadingState = SYMBOLS_LOADING_STATE::LOADING;

#define SYMB_ERR(FUNC)                                                         \
  if (!loading_err) {                                                          \
    loading_err = !(FUNC);                                                     \
  }

  SYMB_ERR(
      loadExternalSymbol(mSymbols.gEgg, MangledNames::gEgg(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetActiveItemFn,
                              MangledNames::GetActiveItem(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetCItemFn,
                              MangledNames::GetCItem(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetMRUItemDirFn,
                              MangledNames::GetMRUItemDir(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetMRUItemPanoFn,
                              MangledNames::GetMRUItemPano(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetCurrentItemFn,
                              MangledNames::GetCurrentItem(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.CoordXfFn,
                              MangledNames::CoordXf(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetLocalMouseFn,
                              MangledNames::GetLocalMouse(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(
      mSymbols.PointFrameToFloatSourceFn,
      MangledNames::PointFrameToFloatSource(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.SetFloatZoomFn,
                              MangledNames::SetFloatZoom(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetFloatZoomFn,
                              MangledNames::GetFloatZoom(ae_major_version)));

#undef SYMB_ERR

  if (loading_err) {
    mLoadingState = SYMBOLS_LOADING_STATE::SOME_NOT_LOADED;
  } else {
    mLoadingState = SYMBOLS_LOADING_STATE::ALL_LOADED;
  }
}

const std::optional<AeEgg::ExternalSymbols::SymbolPointers *>
AeEgg::ExternalSymbols::get() {
  if (mLoadingState == SYMBOLS_LOADING_STATE::NOT_LOADED) {
    load();
  }

  if (mLoadingState == SYMBOLS_LOADING_STATE::ALL_LOADED) {
    return std::optional(&mSymbols);
  } else {
    return std::nullopt;
  }
}

#ifdef AE_OS_WIN
static inline M_Point PointToMPoint(POINT p) {
  return M_Point{static_cast<short>(p.y), static_cast<short>(p.x)};
}

template <typename T> T getVirtualFn(long long *base_addr, int offset) {
  return reinterpret_cast<T>(*base_addr + offset);
}

CPanoProjItem *AeEgg::getViewPano() {
  CPanoProjItem *view_pano = nullptr;

  GetCurrentItemFn(&gEgg, nullptr, &view_pano);

  if (!view_pano) {
    BEE_Item *bee_item = GetActiveItemFn();

    if (bee_item) {
      CItem *c_item = GetCItemFn(bee_item, false);

      if (c_item) {
        CDirProjItem *dir_item = GetMRUItemDirFn(c_item);

        if (dir_item) {
          view_pano = GetMRUItemPanoFn(dir_item);
        }
      }
    }
  }

  return view_pano;
}

void AeEgg::setViewPanoPosition(LongPt point) {
  typedef void(__fastcall * *ScrollToP)(CPanoProjItem *, LongPt *,
                                        unsigned char);

  CPanoProjItem *view_pano = getViewPano();

  // call to "virtual void __cdecl CPanorama::ScrollTo(struct LongPt *
  // __ptr64,unsigned char) __ptr64" from AfterFXLib.dll AE CC2024
  const ScrollToP ScrollTo =
      getVirtualFn<ScrollToP>(reinterpret_cast<long long *>(view_pano), 0x490);

  (*ScrollTo)(view_pano, &point, 1);
}

LongPt AeEgg::getViewPanoPosition() {
  typedef void(__fastcall * *GetPositionP)(CPanoProjItem *, LongPt *);
  LongPt pos = {0, 0};

  CPanoProjItem *view_pano = getViewPano();

  // call to "virtual void __cdecl CPanorama::GetPosition(struct LongPt *
  // __ptr64) __ptr64" from AfterFXLib.dll AE CC2024
  const GetPositionP GetPosition = getVirtualFn<GetPositionP>(
      reinterpret_cast<long long *>(view_pano), 0x448);

  (*GetPosition)(view_pano, &pos);

  return pos;
}

short AeEgg::getCPaneWidth() {
  typedef short(__fastcall * *GetWidthP)(CPanoProjItem *);

  CPanoProjItem *view_pano = getViewPano();

  // call to "virtual short __cdecl CPane::GetWidth(void) __ptr64" from
  // AfterFXLib.dll AE CC2024
  const GetWidthP GetWidth =
      getVirtualFn<GetWidthP>(reinterpret_cast<long long *>(view_pano), 0x318);

  short cpane_width = (*GetWidth)(view_pano);

  return cpane_width;
}

short AeEgg::getCPaneHeight() {
  typedef short(__fastcall * *GetHeightP)(CPanoProjItem *);

  CPanoProjItem *view_pano = getViewPano();

  // call to "virtual short __cdecl CPane::GetHeight(void) __ptr64" from
  // AfterFXLib.dll AE CC2024
  const GetHeightP GetHeight =
      getVirtualFn<GetHeightP>(reinterpret_cast<long long *>(view_pano), 0x320);

  short cpane_height = (*GetHeight)(view_pano);

  return cpane_height;
}

M_Point AeEgg::ScreenToCompMouse(POINT screen_p) {
  CPanoProjItem *view_pano = getViewPano();

  M_Point pp = {0, 0};
  CoordXfFn(view_pano, &pp, FEE_CoordFxType::Two, PointToMPoint(screen_p));

  return pp;
}

M_Point AeEgg::getMouseRelativeToComp() {
  CPanoProjItem *view_pano = getViewPano();

  M_Point pp = {0, 0};
  GetLocalMouseFn(view_pano, &pp);

  return pp;
}

bool AeEgg::isViewPanoExists() {
  CPanoProjItem *view_pano = getViewPano();

  if (view_pano) {
    return true;
  } else {
    return false;
  }
}

LongPt AeEgg::getMouseRelativeToViewPano() {
  auto mouse_rel_to_comp = getMouseRelativeToComp();
  auto view_pano_position = getViewPanoPosition();

  /* view pano position is negative */
  return {
      mouse_rel_to_comp.y - view_pano_position.y,
      mouse_rel_to_comp.x - view_pano_position.x,
  };
}

bool AeEgg::isMouseInsideViewPano() {
  if (!isViewPanoExists()) {
    return false;
  }

  auto mouse_rel_to_view = getMouseRelativeToViewPano();
  auto view_pano_width = getCPaneWidth();
  auto view_pano_height = getCPaneHeight();

  return (mouse_rel_to_view.x >= 0 && mouse_rel_to_view.y >= 0 &&
          mouse_rel_to_view.x <= view_pano_width &&
          mouse_rel_to_view.y <= view_pano_height);
}

void AeEgg::incrementViewZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around) {
  if (!isViewPanoExists()) {
    return;
  }

  CPanoProjItem *view_pano = getViewPano();

  double current_zoom = GetFloatZoomFn(view_pano);
  double new_zoom = current_zoom + zoom_delta;

  POINT cursor_pos;
  GetCursorPos(&cursor_pos); // Get the cursor position in screen coordinates

  LongPt actual_view_pos = getViewPanoPosition();

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
      -(dist_to_zoom_pt.y * (new_zoom / current_zoom) + zoom_pt.y),
      -(dist_to_zoom_pt.x * (new_zoom / current_zoom) + zoom_pt.x),
  };

  SetFloatZoomFn(view_pano, new_zoom, {0, 0}, true, true, false, false, true);
  setViewPanoPosition({lround(new_view_pos.y), lround(new_view_pos.x)});

  last_view_pos = new_view_pos;
}
#elif defined AE_OS_MAC
void AeEgg::incrementViewZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around) {
  return;
}

bool AeEgg::isMouseInsideViewPano() { return true; }
#endif
