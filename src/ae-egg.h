#pragma once

#include "A.h"
#include "external-symbols.h"
#include "view-pano.h"
#include <cstdint>
#include <optional>
#include <uiohook.h>

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
