#include "ae-zoom.h"
#include "ae-egg.h"

#ifdef AE_OS_WIN
template <typename T>
T getFromAfterFXDll(std::string fn_name)
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
#endif

static inline M_Point PointToMPoint(POINT p)
{
	return M_Point{ static_cast<short>(p.y), static_cast<short>(p.x) };
}

template <typename T> 
T getVirtualFn(long long* base_addr, int offset)
{
	return reinterpret_cast<T>(*base_addr + offset);
}

AeEgg::AeEgg()
{
	gEgg = getFromAfterFXDll<CEggApp*>("?gEgg@@3PEAVCEggApp@@EA");

	GetActualPrimaryPreviewItemFn =
		getFromAfterFXDll<GetActualPrimaryPreviewItem>(
			"?GetActualPrimaryPreviewItem@CEggApp@@QEBAXPEAPEAVCPanoProjItem@@PEAPEAVCDirProjItem@@PEAPEAVBEE_Item@@_N33@Z"
		);

	GetActiveItemFn = getFromAfterFXDll<GetActiveItem>("?NIM_GetActiveItem@@YAPEAVBEE_Item@@XZ");
	GetCItemFn = getFromAfterFXDll<GetCItem>("?GetCItem@@YAPEAVCItem@@PEAVBEE_Item@@E@Z");
	GetMRUItemDirFn = getFromAfterFXDll<GetMRUItemDir>("?GetMRUItemDir@CItem@@QEAAPEAVCDirProjItem@@XZ");
	GetMRUItemPanoFn = getFromAfterFXDll<GetMRUItemPano>("?GetMRUItemPano@CDirProjItem@@QEBAPEAVCPanoProjItem@@XZ");
	GetCurrentItemFn = getFromAfterFXDll<GetCurrentItem>("?GetCurrentItem@CEggApp@@QEAAXPEAPEAVBEE_Item@@PEAPEAVCPanoProjItem@@@Z");
	GetPaintCursorItemPanoFn = getFromAfterFXDll<GetPaintCursorItemPano>("?GetPaintCursorItemPano@CEggApp@@QEAAPEAVCPanoProjItem@@XZ");
	GetOpenedItemListFn = getFromAfterFXDll<GetOpenedItemList>("?GetOpenedItemList@CEggApp@@QEAAPEAV?$vector@PEAVCItem@@V?$allocator@PEAVCItem@@@std@@@std@@V?$basic_string@EU?$char_traits@E@std@@U?$STLAllocator@E@allocator@dvacore@@@3@@Z");
	GetPrimaryPreviewPanoFn = getFromAfterFXDll<GetPrimaryPreviewPano>("?GetPrimaryPreviewPano@CEggApp@@QEAAPEAVCPanoProjItem@@_N@Z");
	GetPreviewingPanosFn = getFromAfterFXDll<GetPreviewingPanos>("?GetPreviewingPanos@CEggApp@@QEAA_N_NAEAV?$set@PEAVCPanoProjItem@@U?$less@PEAVCPanoProjItem@@@std@@V?$allocator@PEAVCPanoProjItem@@@3@@std@@@Z");
	GetPanosInPreviewSetFn = getFromAfterFXDll<GetPanosInPreviewSet>("?GetPanosInPreviewSet@CEggApp@@QEAAXAEAV?$set@PEAVCPanoProjItem@@U?$less@PEAVCPanoProjItem@@@std@@V?$allocator@PEAVCPanoProjItem@@@3@@std@@_N@Z");
	CoordXfFn = getFromAfterFXDll<CoordXf>("?CoordXf@CView@@QEAA?AUM_Point@@W4FEE_CoordFxType@@U2@@Z");
	GetLocalMouseFn = getFromAfterFXDll<GetLocalMouse>("?GetLocalMouse@CView@@QEAA?AUM_Point@@XZ");
	PointFrameToFloatSourceFn = getFromAfterFXDll<PointFrameToFloatSource>("?PointFrameToFloatSource@CPanoProjItem@@QEAAXUM_Point@@PEAV?$M_Vector2T@N@@@Z");
	SetFloatZoomFn = getFromAfterFXDll<SetFloatZoom>("?SetFloatZoom@CPanoProjItem@@QEAAXNULongPt@@EEEEE@Z");
	GetFloatZoomFn = getFromAfterFXDll<GetFloatZoom>("?GetFloatZoom@CPanoProjItem@@QEAANXZ");

	last_view_pos = { 999999.0, 999999.0 };
}

CPanoProjItem* AeEgg::getViewPano()
{
	CPanoProjItem* view_pano = nullptr;

	GetCurrentItemFn(&gEgg, nullptr, &view_pano);

	if (!view_pano)
	{
		BEE_Item* bee_item = GetActiveItemFn();

		if (bee_item)
		{
			CItem* c_item = GetCItemFn(bee_item, false);
			
			if (c_item)
			{
				CDirProjItem* dir_item = GetMRUItemDirFn(c_item);

				if (dir_item)
				{
					view_pano = GetMRUItemPanoFn(dir_item);
				}
			}
		}
	}

	return view_pano;
}

void AeEgg::setViewPanoPosition(LongPt point)
{
	typedef void (__fastcall** ScrollToP)(CPanoProjItem*, LongPt*, unsigned char);

	CPanoProjItem* view_pano = getViewPano();

	// call to "virtual void __cdecl CPanorama::ScrollTo(struct LongPt * __ptr64,unsigned char) __ptr64" from AfterFXLib.dll AE CC2024
	const ScrollToP ScrollTo = getVirtualFn<ScrollToP>(reinterpret_cast<long long*>(view_pano), 0x490);

	(*ScrollTo)(view_pano, &point, 1);
}

LongPt AeEgg::getViewPanoPosition()
{
	typedef void(__fastcall** GetPositionP)(CPanoProjItem*, LongPt*);
	LongPt pos = { 0, 0 };

	CPanoProjItem* view_pano = getViewPano();

	// call to "virtual void __cdecl CPanorama::GetPosition(struct LongPt * __ptr64) __ptr64" from AfterFXLib.dll AE CC2024
	const GetPositionP GetPosition = getVirtualFn<GetPositionP>(reinterpret_cast<long long*>(view_pano), 0x448);

	(*GetPosition)(view_pano, &pos);

	return pos;
}

short AeEgg::getCPaneWidth()
{
	typedef short(__fastcall** GetWidthP)(CPanoProjItem*);

	CPanoProjItem* view_pano = getViewPano();

	// call to "virtual short __cdecl CPane::GetWidth(void) __ptr64" from AfterFXLib.dll AE CC2024
	const GetWidthP GetWidth = getVirtualFn<GetWidthP>(reinterpret_cast<long long*>(view_pano), 0x318);

	short cpane_width = (*GetWidth)(view_pano);

	return cpane_width;
}

short AeEgg::getCPaneHeight()
{
	typedef short(__fastcall** GetHeightP)(CPanoProjItem*);

	CPanoProjItem* view_pano = getViewPano();

	// call to "virtual short __cdecl CPane::GetHeight(void) __ptr64" from AfterFXLib.dll AE CC2024
	const GetHeightP GetHeight = getVirtualFn<GetHeightP>(reinterpret_cast<long long*>(view_pano), 0x320);

	short cpane_height = (*GetHeight)(view_pano);

	return cpane_height;
}

M_Point AeEgg::ScreenToCompMouse(POINT screen_p)
{
	CPanoProjItem* view_pano = getViewPano();

	M_Point pp = { 0, 0 };
	CoordXfFn(view_pano, &pp, FEE_CoordFxType::Two, PointToMPoint(screen_p));

	return pp;
}

M_Point AeEgg::getMouseRelativeToComp()
{
	CPanoProjItem* view_pano = getViewPano();

	M_Point pp = { 0, 0 };
	GetLocalMouseFn(view_pano, &pp);

	return pp;
}

bool AeEgg::isViewPanoExists()
{
	CPanoProjItem* view_pano = getViewPano();

	if (view_pano)
	{
		return true;
	}
	else
	{
		return false;
	}
}

LongPt AeEgg::getMouseRelativeToViewPano()
{
	auto mouse_rel_to_comp = getMouseRelativeToComp();
	auto view_pano_position = getViewPanoPosition();

	/* view pano position is negative */
	return {
		mouse_rel_to_comp.y - view_pano_position.y,
		mouse_rel_to_comp.x - view_pano_position.x,
	};
}

bool AeEgg::isMouseInsideViewPano()
{
	if (!isViewPanoExists())
	{
		return false;
	}

	auto mouse_rel_to_view = getMouseRelativeToViewPano();
	auto view_pano_width = getCPaneWidth();
	auto view_pano_height = getCPaneHeight();

	return (
		mouse_rel_to_view.x >= 0 &&
		mouse_rel_to_view.y >= 0 &&
		mouse_rel_to_view.x <= view_pano_width &&
		mouse_rel_to_view.y <= view_pano_height
	);
}

void AeEgg::incrementViewZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around)
{
	if (!isViewPanoExists())
	{
		return;
	}

	CPanoProjItem* view_pano = getViewPano();

	double current_zoom = GetFloatZoomFn(view_pano);
	double new_zoom = current_zoom + zoom_delta;

	POINT cursor_pos;
	GetCursorPos(&cursor_pos); // Get the cursor position in screen coordinates

	LongPt actual_view_pos = getViewPanoPosition();

	DoublePt view_pos = {
		lround(last_view_pos.y) == actual_view_pos.y ? last_view_pos.y : actual_view_pos.y,
		lround(last_view_pos.x) == actual_view_pos.x ? last_view_pos.x : actual_view_pos.x,
	};

	DoublePt zoom_pt;
	DoublePt dist_to_zoom_pt;

	switch (zoom_around)
	{
	case ZOOM_AROUND::PANEL_CENTER:
	{
		double cpane_width2 = getCPaneWidth() / 2.0;
		double cpane_height2 = getCPaneHeight() / 2.0;

		dist_to_zoom_pt = {
			-cpane_height2 - view_pos.y,
			-cpane_width2 - view_pos.x,
		};

		zoom_pt = { cpane_height2, cpane_width2 };

		break;
	}
	case ZOOM_AROUND::CURSOR_POSTION:
	{
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

	/*
	if (lround(last_view_pos.y) == actual_view_pos.y)
	{
		double float_diff = -last_view_pos.y + actual_view_pos.y;

		dist_to_zoom_pt.y += float_diff;
		zoom_pt.y += float_diff;
	}

	if (lround(last_view_pos.x) == actual_view_pos.x)
	{
		double float_diff = -last_view_pos.x + actual_view_pos.x;

		dist_to_zoom_pt.x += float_diff;
		zoom_pt.x += float_diff;
	}
	*/

	DoublePt new_view_pos = {
		-(dist_to_zoom_pt.y * (new_zoom / current_zoom) + zoom_pt.y),
		-(dist_to_zoom_pt.x * (new_zoom / current_zoom) + zoom_pt.x),
	};

	SetFloatZoomFn(view_pano, new_zoom, { 0, 0 }, true, true, false, false, true);
	setViewPanoPosition({ lround(new_view_pos.y), lround(new_view_pos.x) });

	last_view_pos = new_view_pos;
}

