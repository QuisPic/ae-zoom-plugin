#pragma once

#include <set>
#include <nlohmann/json.hpp>
#include <uiohook.h>
#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "logger.h"

#ifdef AE_OS_MAC
#include <dlfcn.h>
#endif

#ifdef AE_OS_WIN
#include <windows.h>
#endif

class BEE_Project;
class BEE_Item;
class CPane;

struct LongPt { long y, x; };
struct DoublePt { double y, x; };
struct M_Point { short y, x; };

template<typename T>
struct M_Vector2T
{
	T x, y;
};

enum FEE_CoordFxType
{
	Zero,
	One,
	Two,
};

class CView;
class CItem;
class CPanorama;
class CPanoProjItem;
class CDirProjItem;

class CEggApp;

typedef BEE_Item*(__stdcall* GetActiveItem)();
typedef CItem*(__stdcall* GetCItem)(BEE_Item*, bool);
typedef CDirProjItem*(__stdcall* GetMRUItemDir)(CItem*);
typedef CPanoProjItem*(__stdcall* GetMRUItemPano)(CDirProjItem*);
typedef void(__stdcall* GetCurrentItem)(CEggApp**, BEE_Item**, CPanoProjItem**);
typedef M_Point(__stdcall* CoordXf)(CPanoProjItem*, M_Point*, FEE_CoordFxType, M_Point);
typedef M_Point(__stdcall* GetLocalMouse)(CPanoProjItem*, M_Point*);
typedef void(__stdcall* PointFrameToFloatSource)(CPanoProjItem*, M_Point, M_Vector2T<double>*);
typedef void(__stdcall* SetFloatZoom)(CPanoProjItem*, double, LongPt, bool, bool, bool, bool, bool);
typedef double(__stdcall* GetFloatZoom)(CPanoProjItem*);

enum class ZOOM_AROUND
{
	PANEL_CENTER,
	CURSOR_POSTION
};

class AeEgg
{
public:
    DoublePt last_view_pos;
    
	CEggApp* gEgg;
    GetActiveItem GetActiveItemFn;
	GetCItem GetCItemFn;
	GetMRUItemDir GetMRUItemDirFn;
	GetMRUItemPano GetMRUItemPanoFn;
	GetCurrentItem GetCurrentItemFn;
	CoordXf CoordXfFn;
	GetLocalMouse GetLocalMouseFn;
	PointFrameToFloatSource PointFrameToFloatSourceFn;
	SetFloatZoom SetFloatZoomFn;
	GetFloatZoom GetFloatZoomFn;

	bool isViewPanoExists();
	CPanoProjItem* getViewPano();
	void setViewPanoPosition(LongPt point);
	LongPt getViewPanoPosition();
	short getCPaneWidth();
	short getCPaneHeight();
	M_Point ScreenToCompMouse(POINT screen_p);
	M_Point getMouseRelativeToComp();
	LongPt getMouseRelativeToViewPano();
	bool isMouseInsideViewPano();
	void incrementViewZoomFixed(double zoom_delta, ZOOM_AROUND zoom_around);

	AeEgg() = default;
    AeEgg(A_long ae_major_version);
};

struct ViewPositionExperimentalOption
{
	bool enabled;
	ZOOM_AROUND zoomAround;

	ViewPositionExperimentalOption() : enabled(false), zoomAround(ZOOM_AROUND::PANEL_CENTER) {};

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ViewPositionExperimentalOption, enabled, zoomAround);
};

struct ExperimentalOptions
{
	bool detectCursorInsideView;
	ViewPositionExperimentalOption fixViewportPosition;

	ExperimentalOptions() : detectCursorInsideView(false), fixViewportPosition() {};

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ExperimentalOptions, detectCursorInsideView, fixViewportPosition);
};
