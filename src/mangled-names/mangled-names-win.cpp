#ifdef AE_OS_WIN
const std::string MangledNames::gEgg(const A_long major_version) {
  switch (major_version) {
  default:
    return "?gEgg@@3PEAVCEggApp@@EA";
    break;
  }
}

const std::string MangledNames::GetActiveItem(const A_long major_version) {
  switch (major_version) {
  default:
    return "?NIM_GetActiveItem@@YAPEAVBEE_Item@@XZ";
    break;
  }
}

const std::string MangledNames::GetCItem(const A_long major_version) {
  switch (major_version) {
  default:
    return "?GetCItem@@YAPEAVCItem@@PEAVBEE_Item@@E@Z";
    break;
  }
}

const std::string MangledNames::GetMRUItemDir(const A_long major_version) {
  switch (major_version) {
  default:
    return "?GetMRUItemDir@CItem@@QEAAPEAVCDirProjItem@@XZ";
    break;
  }
}

const std::string MangledNames::GetMRUItemPano(const A_long major_version) {
  switch (major_version) {
  default:
    return "?GetMRUItemPano@CDirProjItem@@QEBAPEAVCPanoProjItem@@XZ";
    break;
  }
}

const std::string MangledNames::GetCurrentItem(const A_long major_version) {
  switch (major_version) {
  default:
    return "?GetCurrentItem@CEggApp@@QEAAXPEAPEAVBEE_Item@@"
           "PEAPEAVCPanoProjItem@@@Z";
    break;
  }
}

const std::string MangledNames::CoordXf(const A_long major_version) {
  switch (major_version) {
  default:
    return "?CoordXf@CView@@QEAA?AUM_Point@@W4FEE_CoordFxType@@U2@@Z";
    break;
  }
}

const std::string MangledNames::GetLocalMouse(const A_long major_version) {
  switch (major_version) {
  default:
    return "?GetLocalMouse@CView@@QEAA?AUM_Point@@XZ";
    break;
  }
}

const std::string
MangledNames::PointFrameToFloatSource(const A_long major_version) {
  switch (major_version) {
  default:
    return "?PointFrameToFloatSource@CPanoProjItem@@QEAAXUM_Point@@PEAV?$M_"
           "Vector2T@N@@@Z";
    break;
  }
}

const std::string MangledNames::SetFloatZoom(const A_long major_version) {
  switch (major_version) {
  default:
    return "?SetFloatZoom@CPanoProjItem@@QEAAXNULongPt@@EEEEE@Z";
    break;
  }
}

const std::string MangledNames::GetFloatZoom(const A_long major_version) {
  switch (major_version) {
  default:
    return "?GetFloatZoom@CPanoProjItem@@QEAANXZ";
    break;
  }
}
#endif // AE_OS_WIN
