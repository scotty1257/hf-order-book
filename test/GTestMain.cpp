#include "Reporter.hpp"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  pretty_gtest::install();
  return RUN_ALL_TESTS();
}
