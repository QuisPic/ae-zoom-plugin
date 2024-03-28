#include "mangled-names.h"
#include "AEConfig.h"

const std::string MangledNames::gEgg(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?gEgg@@3PEAVCEggApp@@EA";
#elif defined AE_OS_MAC
  return "gEgg";
#endif
}

const std::string MangledNames::GetActiveItem(const A_long major_version) {
#ifdef AE_OS_WIN
  if (major_version > 116) {
    return "?NIM_GetActiveItem@@YAPEAVBEE_Item@@XZ";
  } else {
    return "?NIM_GetActiveItem@@YAPEAUBEE_Item@@XZ";
  }
#elif defined AE_OS_MAC
  return "_Z17NIM_GetActiveItemv";
#endif
}

const std::string MangledNames::GetCItem(const A_long major_version) {
#ifdef AE_OS_WIN
  if (major_version > 116) {
    return "?GetCItem@@YAPEAVCItem@@PEAVBEE_Item@@E@Z";
  } else {
    return "?GetCItem@@YAPEAVCItem@@PEAUBEE_Item@@E@Z";
  }
#elif defined AE_OS_MAC
  return "_Z8GetCItemP8BEE_Itemh";
#endif
}

const std::string MangledNames::GetMRUItemDir(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetMRUItemDir@CItem@@QEAAPEAVCDirProjItem@@XZ";
#elif defined AE_OS_MAC
  return "_ZN5CItem13GetMRUItemDirEv";
#endif
}

const std::string MangledNames::GetMRUItemPano(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetMRUItemPano@CDirProjItem@@QEBAPEAVCPanoProjItem@@XZ";
#elif defined AE_OS_MAC
  return "_ZNK12CDirProjItem14GetMRUItemPanoEv";
#endif
}

const std::string MangledNames::CoordXf(const A_long major_version) {
#ifdef AE_OS_WIN
  if (major_version > 117) {
    return "?CoordXf@CView@@QEAA?AUM_Point@@W4FEE_CoordFxType@@U2@@Z";
  } else {
    return "?CoordXf@CView@@UEAA?AUM_Point@@W4FEE_CoordFxType@@U2@@Z";
  }
#elif defined AE_OS_MAC
  return "_ZN5CView7CoordXfE15FEE_CoordFxType7M_Point";
#endif
}

const std::string MangledNames::GetLocalMouse(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetLocalMouse@CView@@QEAA?AUM_Point@@XZ";
#elif defined AE_OS_MAC
  return "_ZN5CView13GetLocalMouseEv";
#endif
}

const std::string
MangledNames::PointFrameToFloatSource(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?PointFrameToFloatSource@CPanoProjItem@@QEAAXUM_Point@@PEAV?$M_"
         "Vector2T@N@@@Z";
#elif defined AE_OS_MAC
  return "_ZN13CPanoProjItem23PointFrameToFloatSourceE7M_PointP10M_"
         "Vector2TIdE";
#endif
}

const std::string MangledNames::SetFloatZoom(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?SetFloatZoom@CPanoProjItem@@QEAAXNULongPt@@EEEEE@Z";
#elif defined AE_OS_MAC
  return "_ZN13CPanoProjItem12SetFloatZoomEd6LongPthhhhh";
#endif
}

const std::string MangledNames::GetFloatZoom(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetFloatZoom@CPanoProjItem@@QEAANXZ";
#elif defined AE_OS_MAC
  return "_ZN13CPanoProjItem12GetFloatZoomEv";
#endif
}

const std::string MangledNames::GetWidth(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetWidth@CPane@@UEAAFXZ";
#elif defined AE_OS_MAC
  return "_ZN5CPane8GetWidthEv";
#endif
}

const std::string MangledNames::GetHeight(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetHeight@CPane@@UEAAFXZ";
#elif defined AE_OS_MAC
  return "_ZN5CPane9GetHeightEv";
#endif
}

const std::string MangledNames::ScrollTo(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?ScrollTo@CPanorama@@UEAAXPEAULongPt@@E@Z";
#elif defined AE_OS_MAC
  return "_ZN9CPanorama8ScrollToEP6LongPth";
#endif
}

const std::string MangledNames::GetPosition(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetPosition@CPanorama@@UEAAXPEAULongPt@@@Z";
#elif defined AE_OS_MAC
  return "_ZN9CPanorama11GetPositionEP6LongPt";
#endif
}

const std::string MangledNames::GetPaneAtCursor(const A_long major_version) {
#ifdef AE_OS_WIN
  return "?GetPaneAtCursor@CDesktopPlus@@QEAAPEAVCPane@@UM_Point@@@Z";
#elif defined AE_OS_MAC
  return "_ZN12CDesktopPlus15GetPaneAtCursorE7M_Point";
#endif
}
