#pragma once

#include "A.h"
#include "options.h"

class ZoomActions {
  std::vector<KeyBindAction> v;
  std::mutex mutex;

public:
  const std::vector<KeyBindAction> &getActions() { return v; }
  const std::mutex &getMutex() { return mutex; }
  auto size() { return v.size(); }
  void clear() { v.clear(); }
  void post(const KeyBindAction &action);
  A_Err runActions();
};
