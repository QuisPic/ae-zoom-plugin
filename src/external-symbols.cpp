#include "external-symbols.h"
#include "iohook.h"
#include "mangled-names.h"
#include "util-functions.h"

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
