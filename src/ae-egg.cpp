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

	this->GetActualPrimaryPreviewItemFn(this->gEgg, &view_pano, nullptr, nullptr, true, true, true);

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

void AeEgg::incrementViewZoomFixed(double zoom_delta)
{
	CPanoProjItem* view_pano = getViewPano();

	short cpane_width = getCPaneWidth();
	short cpane_height = getCPaneHeight();

	double current_zoom = GetFloatZoomFn(view_pano);
	double new_zoom = current_zoom + zoom_delta;

	POINT cursor_pos;
	GetCursorPos(&cursor_pos); // Get the cursor position in screen coordinates

	LongPt actual_view_pos = getViewPanoPosition();
	M_Point comp_mouse_pos = getMouseRelativeToComp();

	DoublePt cpane_mouse_pos = {
		-actual_view_pos.y + comp_mouse_pos.y,
		-actual_view_pos.x + comp_mouse_pos.x,
	};

	if (lround(last_view_pos.y) == actual_view_pos.y)
	{
		cpane_mouse_pos.y += abs(last_view_pos.y) - abs(actual_view_pos.y);
	}

	if (lround(last_view_pos.x) == actual_view_pos.x)
	{
		cpane_mouse_pos.x += abs(last_view_pos.x) - abs(actual_view_pos.x);
	}

	DoublePt view_pos = {
		-(-(comp_mouse_pos.y * (new_zoom / current_zoom)) + cpane_mouse_pos.y),
		-(-(comp_mouse_pos.x * (new_zoom / current_zoom)) + cpane_mouse_pos.x),
	};

	/*
	new_view_pos = {
		cpane_height / 2 + view_pos.y,
		cpane_width / 2 + view_pos.x,
	};
	*/

	SetFloatZoomFn(view_pano, new_zoom, { 0, 0 }, true, true, false, false, true);
	setViewPanoPosition({ lround(view_pos.y), lround(view_pos.x) });

	last_view_pos = view_pos;
}

