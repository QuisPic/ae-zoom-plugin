#pragma once

#include <string>
#include <AE_GeneralPlug.h>
#include "AEConfig.h"

class MangledNames
{
public:
	static const std::string gEgg(const A_long major_version);
	const std::string GetActiveItem(const A_long major_version);
	const std::string GetCItem(const A_long major_version);
	const std::string GetMRUItemDir(const A_long major_version);
	const std::string GetMRUItemPano(const A_long major_version);
	const std::string GetCurrentItem(const A_long major_version);
	const std::string CoordXf(const A_long major_version);
	const std::string GetLocalMouse(const A_long major_version);
	const std::string PointFrameToFloatSource(const A_long major_version);
	const std::string SetFloatZoom(const A_long major_version);
	const std::string GetFloatZoom(const A_long major_version);
};