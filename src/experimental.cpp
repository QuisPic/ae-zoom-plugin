#include "experimental.h"

#ifdef AE_OS_WIN
template <typename T>
T GetFromAfterFXDll(std::string fn_name)
{
	HINSTANCE hDLL = LoadLibrary("AfterFXLib.dll");

	if (!hDLL) {
		logger(LOG_LEVEL_ERROR, "Failed to load AfterFXLib.dll");
		return nullptr;
	}

	T result = reinterpret_cast<T>(GetProcAddress(hDLL, fn_name.c_str()));

	FreeLibrary(hDLL);
	return result;
}

void HackBipBop()
{
	AEGP_SuiteHandler	suites(sP);
	//suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, std::to_string(helpCtx).c_str());

	auto gEgg = GetFromAfterFXDll<CEggApp*>("?gEgg@@3PEAVCEggApp@@EA");
	//auto GetSelectedToolFn = GetFromAfterFXDll<GetSelectedTool>("?GetSelectedTool@CEggApp@@QEBA?AW4ToolID@@XZ");
	auto GetActualPrimaryPreviewItemFn = 
		GetFromAfterFXDll<GetActualPrimaryPreviewItem>(
			"?GetActualPrimaryPreviewItem@CEggApp@@QEAAXPEAPEAVCPanoProjItem@@PEAPEAVCDirProjItem@@PEAPEAVBEE_Item@@_N33@Z"
		);
	auto GetCurrentItemFn = 
		GetFromAfterFXDll<GetCurrentItem>(
			"?GetCurrentItem@CEggApp@@QEAAXPEAPEAVBEE_Item@@PEAPEAVCPanoProjItem@@@Z"
		);
	auto GetCurrentItemHFn = 
		GetFromAfterFXDll<GetCurrentItemH>(
			"?GetCurrentItemH@CEggApp@@QEAAPEAVBEE_Item@@XZ"
		);

	CPanoProjItem* view_pano = nullptr;
	CDirProjItem* ccdir = nullptr;
	//BEE_Item* bee_item = nullptr;
	//GetCurrentItemFn(gEgg, &bee_item, &view_pano);
	BEE_Item* bee_item = GetCurrentItemHFn(gEgg);
	//GetActualPrimaryPreviewItemFn(gEgg, &view_pano, &ccdir, &bee_item, false, false, false);
	//auto selectedTool = GetSelectedTool(gEgg);

	if (bee_item)
	{
		suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, "yesssssssssssssss");
	}
}
#endif
