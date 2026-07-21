#include "input.hpp"

namespace fate {

bool (CKeyHandler::* CKeyHandler::KeyPressed)(unsigned int key) =
    AddrToFuncPtr<decltype(KeyPressed)>(0x592BD1);
bool (CKeyHandler::* CKeyHandler::KeyHeld)(unsigned int key) =
    AddrToFuncPtr<decltype(KeyPressed)>(0x592BEC);

bool (CMouseHandler::* CMouseHandler::ButtonPressed)(EButton key) =
    AddrToFuncPtr<decltype(ButtonPressed)>(0x5933B9);
bool (CMouseHandler::* CMouseHandler::ButtonHeld)(EButton key) =
    AddrToFuncPtr<decltype(ButtonHeld)>(0x5933D3);
bool (CMouseHandler::* CMouseHandler::ButtonDoubleClicked)(EButton key) =
    AddrToFuncPtr<decltype(ButtonDoubleClicked)>(0x5933EE);

}  // namespace fate
