#include "ae-egg.h"
#include <cstdint>
#include <optional>

#ifdef AE_OS_MAC
#include <CoreGraphics/CoreGraphics.h>
#endif

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
#elif defined AE_OS_MAC
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

AeEgg gAeEgg;
