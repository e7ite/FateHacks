# FateHacks

This is a reverse-engineering project for the FATE game on Steam. This project will produce a DLL file that can be injected into the game using your favorite cheat injector

## Features
- Left-click to be granted 100 gold in game

## Usage
1. Obtain [Microsoft Detours](https://github.com/microsoft/Detours) using [vcpkg](https://github.com/microsoft/vcpkg)
1. Build the project using VS 2019 (other VS versions may work, I have not attempted them). I currently build the project as on Debug x86 settings.
2. If compiled with Debug settings, navigate to the Debug/ directory to obtain the DLL created.
3. Inject with your favorite DLL injector.

# Dependencies
- [Microsoft Detours](https://github.com/microsoft/Detours)