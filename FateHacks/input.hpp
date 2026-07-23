#ifndef FATEHACKS_INPUT_HPP_
#define FATEHACKS_INPUT_HPP_

// The game's keyboard and mouse handlers. Both index their state by Windows
// virtual-key / button code and expose per-frame "pressed" edges.

#include "abi.hpp"

namespace fate {

struct CKeyHandler {
  char _pad00[0x4];

  static bool (CKeyHandler::*KeyPressed)(unsigned int key);
  static bool (CKeyHandler::*KeyHeld)(unsigned int key);
};

struct CMouseHandler {
  enum class EButton : int {
    LEFT_CLICK,
    RIGHT_CLICK,
  };

  char buttonData[0x4];

  static bool (CMouseHandler::*ButtonPressed)(EButton key);
  static bool (CMouseHandler::*ButtonHeld)(EButton key);
  static bool (CMouseHandler::*ButtonDoubleClicked)(EButton key);
};

}  // namespace fate

#endif  // FATEHACKS_INPUT_HPP_
