<div align="right">

[![CI](https://github.com/e7ite/FateHacks/actions/workflows/ci.yml/badge.svg)](https://github.com/e7ite/FateHacks/actions/workflows/ci.yml)

</div>

# FateHacks

This is a reverse-engineering project for the FATE game on Steam. This project will produce a DLL file that can be injected into the game using your favorite cheat injector

## Features
- Left-click to be granted 100 gold in game

## Usage
1. Obtain [Microsoft Detours](https://github.com/microsoft/Detours) using [vcpkg](https://github.com/microsoft/vcpkg)
1. Build the project using VS 2019 (other VS versions may work, I have not attempted them). I currently build the project as on Debug x86 settings.
2. If compiled with Debug settings, navigate to the Debug/ directory to obtain the DLL created.
3. Inject with your favorite DLL injector.

## Debugging with IDA

The Debug build emits a full standalone PDB (`Debug/FateHacks.pdb`) so IDA can resolve this DLL's symbols while debugging the injected module. To load them:

1. Build and inject a matching DLL. A rebuild changes the PDB signature, so the injected DLL must be the one that produced the current PDB.
2. Attach IDA's Local Windows debugger to `fate.exe`. Use only one debugger at a time -- do not also attach Visual Studio to the same process.
3. Open `Debugger` -> `Debugger windows` -> `Module list`, find `FateHacks.dll`, right-click it and choose **Load debug symbols**.

The detour trampoline (shown as `JUMPOUT(0x...)` inside the game's `CGameUI::Render`) and any source breakpoints in `dllmain.cpp` will now resolve to named functions. Re-run step 3 after each rebuild, since the new PDB has a fresh signature.

![IDA debugging the injected DLL with symbols loaded: the call stack shows named FateHacks.dll frames (CheatMain, CGameUI::RenderDetour) and the decompiler resolves them](debugging_ida_poc.png)

# Dependencies
- [Microsoft Detours](https://github.com/microsoft/Detours)