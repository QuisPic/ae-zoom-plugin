#include "ae-zoom.h"
#include "experimental.h"

#ifdef AE_OS_WIN
template <typename T>
T GetFromAfterFXDll(std::string fn_name)
{
	HINSTANCE hDLL = LoadLibrary("AfterFXLib.dll");

	if (!hDLL) {
		logger(LOG_LEVEL_ERROR, "Failed to load AfterFXLib.dll", "");
		return nullptr;
	}

	T result = reinterpret_cast<T>(GetProcAddress(hDLL, fn_name.c_str()));

	FreeLibrary(hDLL);
	return result;
}

void HackBipBop()
{
	//AEGP_SuiteHandler	suites(sP);
	//suites.UtilitySuite3()->AEGP_ReportInfo(S_zoom_id, std::to_string(helpCtx).c_str());

	auto gEgg = GetFromAfterFXDll<CEggApp*>("?gEgg@@3PEAVCEggApp@@EA");

	typedef void (__stdcall* GetActualPrimaryPreviewItem)(CEggApp*, CPanoProjItem**, CDirProjItem**, BEE_Item**, bool, bool, bool);
	auto GetActualPrimaryPreviewItemFn =
		GetFromAfterFXDll<GetActualPrimaryPreviewItem>(
			"?GetActualPrimaryPreviewItem@CEggApp@@QEBAXPEAPEAVCPanoProjItem@@PEAPEAVCDirProjItem@@PEAPEAVBEE_Item@@_N33@Z"
		);

	CPanoProjItem* view_pano = nullptr;
	BEE_Item* bee_item = nullptr;

	if (gEgg)
	{
		GetActualPrimaryPreviewItemFn(gEgg, &view_pano, nullptr, &bee_item, true, true, true);

		if (view_pano)
		{
			// call to "virtual void __cdecl CPanorama::ScrollTo(struct LongPt * __ptr64,unsigned char) __ptr64" from AfterFXLib.dll AE CC2024
			typedef void (__fastcall** ScrollToP)(CPanoProjItem*, LongPt*, unsigned char);
			__int64 ScrollTo_addr = *reinterpret_cast<__int64*>(view_pano) + 0x490;
			const ScrollToP ScrollTo = reinterpret_cast<ScrollToP>(ScrollTo_addr);

			LongPt pt = { 1, 1 };

			(*ScrollTo)(view_pano, &pt, 1);
		}
	}
}
#endif
