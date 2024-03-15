#include "mangled-names.h"
#include "AEConfig.h"

const std::string MangledNames::gEgg(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?gEgg@@3PEAVCEggApp@@EA";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "gEgg";
    break;
  }
#endif
}

const std::string MangledNames::GetActiveItem(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?NIM_GetActiveItem@@YAPEAVBEE_Item@@XZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_Z17NIM_GetActiveItemv";
    break;
  }
#endif
}

const std::string MangledNames::GetCItem(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetCItem@@YAPEAVCItem@@PEAVBEE_Item@@E@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_Z8GetCItemP8BEE_Itemh";
    break;
  }
#endif
}

const std::string MangledNames::GetMRUItemDir(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetMRUItemDir@CItem@@QEAAPEAVCDirProjItem@@XZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN5CItem13GetMRUItemDirEv";
    break;
  }
#endif
}

const std::string MangledNames::GetMRUItemPano(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetMRUItemPano@CDirProjItem@@QEBAPEAVCPanoProjItem@@XZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZNK12CDirProjItem14GetMRUItemPanoEv";
    break;
  }
#endif
}

const std::string MangledNames::GetCurrentItem(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetCurrentItem@CEggApp@@QEAAXPEAPEAVBEE_Item@@"
           "PEAPEAVCPanoProjItem@@@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN7CEggApp14GetCurrentItemEPP8BEE_ItemPP13CPanoProjItem";
    break;
  }
#endif
}

const std::string MangledNames::CoordXf(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?CoordXf@CView@@QEAA?AUM_Point@@W4FEE_CoordFxType@@U2@@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN5CView7CoordXfE15FEE_CoordFxType7M_Point";
    break;
  }
#endif
}

const std::string MangledNames::GetLocalMouse(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetLocalMouse@CView@@QEAA?AUM_Point@@XZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN5CView13GetLocalMouseEv";
    break;
  }
#endif
}

const std::string
MangledNames::PointFrameToFloatSource(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?PointFrameToFloatSource@CPanoProjItem@@QEAAXUM_Point@@PEAV?$M_"
           "Vector2T@N@@@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN13CPanoProjItem23PointFrameToFloatSourceE7M_PointP10M_"
           "Vector2TIdE";
    break;
  }
#endif
}

const std::string MangledNames::SetFloatZoom(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?SetFloatZoom@CPanoProjItem@@QEAAXNULongPt@@EEEEE@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN13CPanoProjItem12SetFloatZoomEd6LongPthhhhh";
    break;
  }
#endif
}

const std::string MangledNames::GetFloatZoom(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetFloatZoom@CPanoProjItem@@QEAANXZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN13CPanoProjItem12GetFloatZoomEv";
    break;
  }
#endif
}

const std::string MangledNames::GetWidth(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetWidth@CPane@@UEAAFXZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN5CPane8GetWidthEv";
    break;
  }
#endif
}

const std::string MangledNames::GetHeight(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetHeight@CPane@@UEAAFXZ";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN5CPane9GetHeightEv";
    break;
  }
#endif
}

const std::string MangledNames::ScrollTo(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?ScrollTo@CPanorama@@UEAAXPEAULongPt@@E@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN9CPanorama8ScrollToEP6LongPth";
    break;
  }
#endif
}

const std::string MangledNames::GetPosition(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetPosition@CPanorama@@UEAAXPEAULongPt@@@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN9CPanorama11GetPositionEP6LongPt";
    break;
  }
#endif
}

const std::string MangledNames::GetPaneAtCursor(const A_long major_version) {
#ifdef AE_OS_WIN
  switch (major_version) {
  default:
    return "?GetPaneAtCursor@CDesktopPlus@@QEAAPEAVCPane@@UM_Point@@@Z";
    break;
  }
#elifdef AE_OS_MAC
  switch (major_version) {
  default:
    return "_ZN12CDesktopPlus15GetPaneAtCursorE7M_Point";
    break;
  }
#endif
}
