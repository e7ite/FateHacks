// Entry point for the FateHacks test binary. vcpkg's gtest port ships no
// gtest_main library, so the runner lives here; test cases live in their own
// _test.cpp files and are linked into this binary.

#include "gmock/gmock.h"
#include "gtest/gtest.h"

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
