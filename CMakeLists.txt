cmake_minimum_required(VERSION 3.24)
project(order_book LANGUAGES CXX)

# ==============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(googletest)

# ==============================================================================
add_library(reference_order_book 
  src/ReferenceOrderBook.cpp 
  src/SimpleOrderBook.cpp 
  src/ThreadPool.cpp)

target_include_directories(reference_order_book PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_options(reference_order_book PRIVATE -Wall -Wextra -Wpedantic)

# ==============================================================================
enable_testing()

add_executable(order_book_test
  test/OrderBookTest.cpp
  test/SimpleOrderTest.cpp
  test/GTestMain.cpp
)

target_compile_options(order_book_test PRIVATE -Wall -Wextra)

target_link_libraries(order_book_test PRIVATE
  reference_order_book GTest::gtest)

include(GoogleTest)
gtest_discover_tests(order_book_test
  TEST_PREFIX "orderBook."
  PROPERTIES
  ENVIRONMENT "GTEST_COLOR=yes"
TIMEOUT 10)
