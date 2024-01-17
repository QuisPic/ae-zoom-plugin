#include "mangled-names.h"

#ifdef AE_OS_MAC
const std::string MangledNames::gEgg(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "gEgg";
		break;
	}
}

const std::string MangledNames::GetActiveItem(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_Z17NIM_GetActiveItemv";
		break;
	}
}

const std::string MangledNames::GetCItem(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_Z8GetCItemP8BEE_Itemh";
		break;
	}
}

const std::string MangledNames::GetMRUItemDir(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN5CItem13GetMRUItemDirEv";
		break;
	}
}

const std::string MangledNames::GetMRUItemPano(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZNK12CDirProjItem14GetMRUItemPanoEv";
		break;
	}
}

const std::string MangledNames::GetCurrentItem(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN7CEggApp14GetCurrentItemEPP8BEE_ItemPP13CPanoProjItem";
		break;
	}
}

const std::string MangledNames::CoordXf(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN5CView7CoordXfE15FEE_CoordFxType7M_Point";
		break;
	}
}

const std::string MangledNames::GetLocalMouse(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN5CView13GetLocalMouseEv";
		break;
	}
}

const std::string MangledNames::PointFrameToFloatSource(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN13CPanoProjItem23PointFrameToFloatSourceE7M_PointP10M_Vector2TIdE";
		break;
	}
}

const std::string MangledNames::SetFloatZoom(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN13CPanoProjItem12SetFloatZoomEd6LongPthhhhh";
		break;
	}
}

const std::string MangledNames::GetFloatZoom(const A_long major_version)
{
	switch (major_version)
	{
	default:
		return "_ZN13CPanoProjItem12GetFloatZoomEv";
		break;
	}
}
#endif // AE_OS_MAC
