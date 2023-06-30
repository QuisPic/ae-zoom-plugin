struct M_Point { short y, x; };

class CEggApp;
class CPanoProjItem;
class BEE_Item;
class CDirProjItem;

typedef int (*GetSelectedTool)(CEggApp*);
typedef void (*GetActualPrimaryPreviewItem)(CEggApp*, CPanoProjItem**, CDirProjItem**, BEE_Item**, bool, bool, bool);
typedef void (*GetCurrentItem)(CEggApp*, BEE_Item**, CPanoProjItem**);
typedef BEE_Item* (*GetCurrentItemH)(CEggApp*);
typedef M_Point (*GetLocalMouse)(void);
