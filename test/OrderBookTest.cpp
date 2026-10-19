#include "ReferenceOrderBook.hpp"

#include <gtest/gtest.h>

#include <list>
#include <optional>
#include <ostream>
#include <vector>

// Fill has no operator== in the header, so EXPECT_EQ on std::vector<Fill> needs
// one. It must live in the global namespace (same as Fill) so gtest finds it
// through ADL.
static bool operator==(const Fill &leftFill, const Fill &rightFill) {
  return leftFill.restingOrderIdentifier == rightFill.restingOrderIdentifier &&
         leftFill.incomingOrderIdentifier ==
             rightFill.incomingOrderIdentifier &&
         leftFill.priceInTicks == rightFill.priceInTicks &&
         leftFill.quantity == rightFill.quantity;
}

// Readable failure output instead of a raw byte dump.
static void PrintTo(const Fill &fill, std::ostream *outputStream) {
  *outputStream << "{resting " << fill.restingOrderIdentifier << ", incoming "
                << fill.incomingOrderIdentifier << ", price "
                << fill.priceInTicks << ", quantity " << fill.quantity << "}";
}

// Queue order at one price level, front of the line first.
static std::vector<Identifier> orderIdentifiersInQueue(
    const std::list<Order> &level) {
  std::vector<Identifier> orderIdentifiers;
  for (const Order &order : level) {
    orderIdentifiers.push_back(order.id);
  }
  return orderIdentifiers;
}

using Fills = std::vector<Fill>;
using Identifiers = std::vector<Identifier>;

TEST(ReferenceOrderBookTest, PartialFillRespectsTimePriority) {
  ReferenceOrderBook orderBook;
  orderBook.addLimitOrder(1, Side::Sell, 10050, 100);
  orderBook.addLimitOrder(2, Side::Sell, 10050, 200);
  Fills fills = orderBook.addLimitOrder(3, Side::Buy, 10050, 150);

  ASSERT_EQ(fills.size(), 2u);
  EXPECT_EQ(fills[0].restingOrderIdentifier, 1u);
  EXPECT_EQ(fills[0].quantity, 100u);
  EXPECT_EQ(fills[1].restingOrderIdentifier, 2u);
  EXPECT_EQ(fills[1].quantity, 50u);
  EXPECT_EQ(
      orderIdentifiersInQueue(orderBook.asksAtPrice(10050)), (Identifiers{2}));
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10050), 150u);
}

TEST(ReferenceOrderBookTest, FullSessionScenario) {
  ReferenceOrderBook orderBook;

  // ---- Build the starting book (nothing crosses) ----
  EXPECT_EQ(orderBook.addLimitOrder(1, Side::Sell, 10050, 100), Fills{});
  EXPECT_EQ(orderBook.addLimitOrder(2, Side::Sell, 10050, 200), Fills{});
  EXPECT_EQ(orderBook.addLimitOrder(3, Side::Sell, 10051, 200), Fills{});
  EXPECT_EQ(orderBook.addLimitOrder(4, Side::Sell, 10052, 300), Fills{});
  EXPECT_EQ(orderBook.addLimitOrder(5, Side::Buy, 10049, 150), Fills{});
  EXPECT_EQ(orderBook.addLimitOrder(6, Side::Buy, 10048, 400), Fills{});
  // ASKS 10052:[#4 300]  10051:[#3 200]  10050:[#1 100, #2 200]
  // BIDS 10049:[#5 150]  10048:[#6 400]
  EXPECT_EQ(orderBook.bestBidPrice(), std::optional<PriceInTicks>{10049});
  EXPECT_EQ(orderBook.bestAskPrice(), std::optional<PriceInTicks>{10050});
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10050), 300u);
  EXPECT_EQ(orderIdentifiersInQueue(orderBook.asksAtPrice(10050)),
      (Identifiers{1, 2}));

  // ---- Buy sweeps two ask levels, partial fill on #3, resting price wins ----
  EXPECT_EQ(orderBook.addLimitOrder(7, Side::Buy, 10051, 350),
      (Fills{
          {1, 7, 10050, 100},
          {2, 7, 10050, 200},
          {3, 7, 10051,  50}
  }));
  // ASKS 10052:[#4 300]  10051:[#3 150]
  // BIDS 10049:[#5 150]  10048:[#6 400]
  EXPECT_EQ(orderBook.bestAskPrice(), std::optional<PriceInTicks>{10051});
  EXPECT_EQ(
      orderBook.quantityAtPrice(Side::Sell, 10050), 0u); // empty level removed
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10051), 150u);
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10051),
      0u); // #7 fully filled, did not rest

  // ---- Sell partially fills, remainder rests on the ask side ----
  EXPECT_EQ(orderBook.addLimitOrder(8, Side::Sell, 10049, 500),
      (Fills{
          {5, 8, 10049, 150}
  }));
  // ASKS 10052:[#4 300]  10051:[#3 150]  10049:[#8 350]
  // BIDS 10048:[#6 400]
  EXPECT_EQ(orderBook.bestBidPrice(), std::optional<PriceInTicks>{10048});
  EXPECT_EQ(orderBook.bestAskPrice(), std::optional<PriceInTicks>{10049});
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10049), 350u);

  // ---- Reduce keeps time priority ----
  EXPECT_EQ(orderBook.addLimitOrder(9, Side::Buy, 10048, 200), Fills{});
  orderBook.reduceOrder(6, 100);
  // BIDS 10048:[#6 300, #9 200]
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10048), 500u);
  EXPECT_EQ(orderIdentifiersInQueue(orderBook.bidsAtPrice(10048)),
      (Identifiers{6, 9}));
  EXPECT_EQ(orderBook.addLimitOrder(10, Side::Sell, 10048, 100),
      (Fills{
          {6, 10, 10048, 100}
  })); // #6 still first
  // BIDS 10048:[#6 200, #9 200]

  // ---- Replace loses time priority ----
  orderBook.replaceOrder(6, 11, 10048, 250);
  // BIDS 10048:[#9 200, #11 250]
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10048), 450u);
  EXPECT_EQ(orderIdentifiersInQueue(orderBook.bidsAtPrice(10048)),
      (Identifiers{9, 11}));
  EXPECT_EQ(orderBook.addLimitOrder(12, Side::Sell, 10048, 250),
      (Fills{
          { 9, 12, 10048, 200},
          {11, 12, 10048,  50}
  })); // #9 now first
  // BIDS 10048:[#11 200]

  // ---- Replace that crosses must match (replaceOrder returns void, so check
  // the book) ----
  orderBook.replaceOrder(11, 13, 10049, 200);
  // #13 buys 200 from #8 at 10049, nothing left to rest
  // ASKS 10052:[#4 300]  10051:[#3 150]  10049:[#8 150]
  // BIDS (empty)
  EXPECT_EQ(orderBook.bestBidPrice(), std::nullopt);
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10048), 0u); // #11 gone
  EXPECT_EQ(
      orderBook.quantityAtPrice(Side::Buy, 10049), 0u); // #13 did not rest
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10049), 150u);
  EXPECT_EQ(
      orderIdentifiersInQueue(orderBook.asksAtPrice(10049)), (Identifiers{8}));

  // ---- Cancels (cancelOrder returns void, so check the book) ----
  orderBook.cancelOrder(3);
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10051), 0u);
  EXPECT_TRUE(orderBook.asksAtPrice(10051).empty());
  orderBook.cancelOrder(999); // never existed: book unchanged
  orderBook.cancelOrder(1);   // already filled: book unchanged
  orderBook.cancelOrder(3);   // already cancelled: book unchanged
  // ASKS 10052:[#4 300]  10049:[#8 150]
  EXPECT_EQ(orderBook.bestAskPrice(), std::optional<PriceInTicks>{10049});
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10049), 150u);
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10052), 300u);

  // ---- Buy larger than the whole ask side: sweep everything, rest the
  // remainder ----
  EXPECT_EQ(orderBook.addLimitOrder(14, Side::Buy, 10052, 1000),
      (Fills{
          {8, 14, 10049, 150},
          {4, 14, 10052, 300}
  }));
  // ASKS (empty)
  // BIDS 10052:[#14 550]
  EXPECT_EQ(orderBook.bestAskPrice(), std::nullopt);
  EXPECT_EQ(orderBook.bestBidPrice(), std::optional<PriceInTicks>{10052});
  EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10052), 550u);
  EXPECT_EQ(
      orderIdentifiersInQueue(orderBook.bidsAtPrice(10052)), (Identifiers{14}));
}
