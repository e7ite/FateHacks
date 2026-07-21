# Builds and runs the Google Test suite. The FateHacksTests project runs the
# tests in its post-build step, so a non-zero exit means they failed or did not
# build. The pre-commit hook uses this to block a commit whose tests do not pass.
$ErrorActionPreference = 'Stop'
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
& $msbuild "FateHacks\FateHacksTests.vcxproj" /p:Configuration=Debug /p:Platform=Win32 /nologo /v:minimal
exit $LASTEXITCODE
