#include <string>

class BEE_Project;
class BEE_Item;
class CPane;
class CDirTabPanel;
class CDirProjItem;
class BEE_CompItem;
class CDesktopPlus;
class CPanoDrawbot;

struct LongPt { long y, x; };

class CView;
class CPanorama;
class CPanoProjItem;

class CEggApp
{
public:
	// CPanoProjItem* GetPrimaryPreviewPano(bool);
	// void GetActualPrimaryPreviewItem(CPanoProjItem**, CDirProjItem**, BEE_Item**, bool, bool, bool);
};

// _declspec(dllimport) CEggApp* gEgg;

template <typename T>
T GetFromAfterFXDll(std::string fn_name);

void HackBipBop();
