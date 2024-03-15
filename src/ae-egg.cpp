#include "ae-egg.h"
#include "logger.h"
#include "mangled-names/mangled-names.h"
#include "options/options.h"
#include <cstdint>
#include <optional>

#ifdef AE_OS_MAC
#include <CoreGraphics/CoreGraphics.h>
#endif

AeEgg gAeEgg;

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
#elif defined AE_OS_MAC
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
bool ExternalSymbols::loadExternalSymbol(T &symbol_storage,
                                         const std::string &symbol_name) {
  symbol_storage = getFromAfterFXDll<T>(symbol_name);

  return symbol_storage == nullptr ? false : true;
}

void ExternalSymbols::load() {
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
  SYMB_ERR(loadExternalSymbol(mSymbols.GetWidhtFn,
                              MangledNames::GetWidth(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetHeightFn,
                              MangledNames::GetHeight(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.ScrollToFn,
                              MangledNames::ScrollTo(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetPositionFn,
                              MangledNames::GetPosition(ae_major_version)));
  SYMB_ERR(loadExternalSymbol(mSymbols.GetPaneAtCursorFn,
                              MangledNames::GetPaneAtCursor(ae_major_version)));

#undef SYMB_ERR

  if (loading_err) {
    mLoadingState = SYMBOLS_LOADING_STATE::SOME_NOT_LOADED;
  } else {
    mLoadingState = SYMBOLS_LOADING_STATE::ALL_LOADED;
  }
}

const std::optional<ExternalSymbols::SymbolPointers *> ExternalSymbols::get() {
  if (mLoadingState == SYMBOLS_LOADING_STATE::NOT_LOADED) {
    load();
  }

  if (mLoadingState == SYMBOLS_LOADING_STATE::ALL_LOADED) {
    return std::optional(&mSymbols);
  } else {
    return std::nullopt;
  }
}

template <typename T> T getVirtualFn(long long *base_addr, int offset) {
  return reinterpret_cast<T>(*base_addr + offset);
}

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

void ViewPano::incrementZoomFixed(double zoom_delta) {
  double current_zoom = getZoom();
  double new_zoom = current_zoom + zoom_delta;

  if (new_zoom < 0.008) {
    new_zoom = 0.008;
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

  switch (gExperimentalOptions.fixViewportPosition.zoomAround) {
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

  setZoom(new_zoom);

  setViewPanoPosition({static_cast<int32_t>(lround(new_view_pos.y)),
                       static_cast<int32_t>(lround(new_view_pos.x))});

  last_view_pos = new_view_pos;
}

std::optional<ViewPano> AeEgg::getActiveViewPano() {
  CPanoProjItem *view_pano = nullptr;
  auto externalSymbolsOpt = extSymbols.get();

  if (externalSymbolsOpt) {
    auto extSymbols = externalSymbolsOpt.value();
    BEE_Item *bee_item = extSymbols->GetActiveItemFn();

    if (bee_item) {
      CItem *c_item = extSymbols->GetCItemFn(bee_item, false);

      if (c_item) {
        CDirProjItem *dir_item = extSymbols->GetMRUItemDirFn(c_item);

        if (dir_item) {
          view_pano = extSymbols->GetMRUItemPanoFn(dir_item);
        }
      }
    }

    return view_pano && externalSymbolsOpt
               ? std::optional(ViewPano(view_pano, externalSymbolsOpt.value()))
               : std::nullopt;
  }

  return std::nullopt;
}

std::optional<int64_t> AeEgg::getCPanoProjItemBasePtr() {
  if (!CPanoProjItemBasePtr) {
    auto activeViewPanoPtr = getActiveViewPano();

    if (activeViewPanoPtr) {
      CPanoProjItemBasePtr =
          *reinterpret_cast<int64_t *>(activeViewPanoPtr->pano);
    }
  }

  return CPanoProjItemBasePtr;
}

std::optional<ViewPano> AeEgg::getViewPanoUnderCursor() {
  CPanoProjItem *view_pano = nullptr;
  auto externalSymbolsOpt = extSymbols.get();

  if (externalSymbolsOpt) {
#ifdef AE_OS_WIN
    POINT native_cursor_pos;
    GetCursorPos(&native_cursor_pos);
#elifdef AE_OS_MAC
    CGEventRef event = CGEventCreate(NULL);
    CGPoint native_cursor_pos = CGEventGetLocation(event);
    CFRelease(event);
#endif

    M_Point cursor_pos = {static_cast<short>(native_cursor_pos.y),
                          static_cast<short>(native_cursor_pos.x)};

    auto cpane =
        externalSymbolsOpt.value()->GetPaneAtCursorFn(nullptr, cursor_pos);
    auto CPanoProjItemBasePtr = getCPanoProjItemBasePtr();

    if (cpane && CPanoProjItemBasePtr &&
        *reinterpret_cast<int64_t *>(cpane) == CPanoProjItemBasePtr.value()) {
      view_pano = reinterpret_cast<CPanoProjItem *>(cpane);
    }
  }

  return view_pano && externalSymbolsOpt
             ? std::optional(ViewPano(view_pano, externalSymbolsOpt.value()))
             : std::nullopt;
}
