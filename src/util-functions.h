#pragma once

#include "A.h"
#include <string>
#include <tuple>

void logger(unsigned int, std::tuple<std::string, std::string>);

void logger(unsigned int, const std::string &, const std::string &);

std::tuple<A_Err, std::string> RunExtendscript(const std::string &script_str);
