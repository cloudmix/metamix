#pragma once

#include <functional>
#include <iostream>

#include "log.h"

namespace metamix {

template<typename F>
auto
supervised(F f, bool restart = true)
{
  return [f, restart](auto... args) -> void {
    for (;;) {
      try {
        f(args...);
        if (restart) {
          LOG(info) << "Restarting (user-issued)...";
          continue;
        } else {
          return;
        }
      } catch (const std::exception &ex) {
        LOG(fatal) << ex.what();
        if (restart) {
          LOG(debug) << "Restarting (caused by fatal error)...";
          continue;
        } else {
          return;
        }
      } catch (...) {
        LOG(fatal) << "Unknown fatal error.";
        if (restart) {
          LOG(debug) << "Restarting (caused by fatal error)...";
          continue;
        } else {
          return;
        }
      }
    }
  };
}
}
