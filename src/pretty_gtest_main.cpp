// Drop-in replacement for gtest_main.
#include <pretty_gtest/reporter.hpp>

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  pretty_gtest::install();
  return RUN_ALL_TESTS();
}
