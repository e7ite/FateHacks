# Overlay triplet used by CI (--overlay-triplets) when building gtest/gmock.
# Same as vcpkg's community x86-windows-static-md (static libraries, dynamic
# CRT), but compiles the port with the STL's vectorized algorithm helpers
# disabled.
#
# gtest's std::rotate otherwise emits a call to __std_rotate, one of the
# separately-compiled "vector algorithm" helpers. The CI runner's x86 import
# libraries don't provide that symbol, so linking gtest-all.cc.obj fails with an
# unresolved external. Building gtest with _USE_STD_VECTOR_ALGORITHMS=0 makes it
# use the header-only algorithm implementations, which don't reference the
# helper at all. The test project (FateHacksTests.vcxproj) sets the same define
# so both sides match.
set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CXX_FLAGS "/D_USE_STD_VECTOR_ALGORITHMS=0")
set(VCPKG_C_FLAGS "/D_USE_STD_VECTOR_ALGORITHMS=0")
