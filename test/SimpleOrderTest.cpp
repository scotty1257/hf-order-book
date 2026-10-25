#include "SimpleOrderBook.hpp"

#include <gtest/gtest.h>

#include <list>
#include <optional>
#include <ostream>
#include <vector>

TEST(SimleOrderBookTest, SimpleOrderLength) {
  SimpleOrderBook orderBook;
  orderBook.AddOrder(1, SimpleSide::Sell, 10050, 100);
  orderBook.AddOrder(2, SimpleSide::Sell, 10050, 200);
  orderBook.AddOrder(3, SimpleSide::Sell, 10050, 200);

  orderBook.AddOrder(4, SimpleSide::Buy, 10050, 100);
  orderBook.AddOrder(5, SimpleSide::Buy, 10050, 200);
  orderBook.AddOrder(6, SimpleSide::Buy, 10050, 200);
  orderBook.AddOrder(7, SimpleSide::Buy, 10050, 200);
  orderBook.AddOrder(8, SimpleSide::Buy, 10050, 200);
  orderBook.AddOrder(9, SimpleSide::Buy, 10050, 200);
  orderBook.AddOrder(10, SimpleSide::Buy, 10050, 200);
  orderBook.AddOrder(11, SimpleSide::Buy, 10050, 250);

  // orderBook.PrintOrders();
}
