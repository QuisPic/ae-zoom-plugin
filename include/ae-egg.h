#pragma once

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
class CPanorama;
class CPanoProjItem;
class CDirProjItem;

class CEggApp;

typedef void(__stdcall* GetActualPrimaryPreviewItem)(CEggApp*, CPanoProjItem**, CDirProjItem**, BEE_Item**, bool, bool, bool);
typedef M_Point(__stdcall* CoordXf)(CPanoProjItem*, M_Point*, FEE_CoordFxType, M_Point);
typedef M_Point(__stdcall* GetLocalMouse)(CPanoProjItem*, M_Point*);
typedef void(__stdcall* PointFrameToFloatSource)(CPanoProjItem*, M_Point, M_Vector2T<double>*);
typedef void(__stdcall* SetFloatZoom)(CPanoProjItem*, double, LongPt, bool, bool, bool, bool, bool);
typedef double(__stdcall* GetFloatZoom)(CPanoProjItem*);

class AeEgg
{
public:
	CEggApp* gEgg;
	GetActualPrimaryPreviewItem GetActualPrimaryPreviewItemFn;
	CoordXf CoordXfFn;
	GetLocalMouse GetLocalMouseFn;
	PointFrameToFloatSource PointFrameToFloatSourceFn;
	SetFloatZoom SetFloatZoomFn;
	GetFloatZoom GetFloatZoomFn;

	DoublePt last_view_pos;

	AeEgg();
	CPanoProjItem* getViewPano();
	void setViewPanoPosition(LongPt point);
	LongPt getViewPanoPosition();
	short getCPaneWidth();
	short getCPaneHeight();
	M_Point ScreenToCompMouse(POINT screen_p);
	M_Point getMouseRelativeToComp();
	void incrementViewZoomFixed(double zoom_delta);
};