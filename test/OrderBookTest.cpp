#include "ReferenceOrderBook.hpp"
#include <vector>
#include <gtest/gtest.h>

TEST(ReferenceOrderBookTest, PartialFillRespectsTimePriority) {
    ReferenceOrderBook orderBook;
    orderBook.addLimitOrder(1, Side::Sell, 10050, 100);
    orderBook.addLimitOrder(2, Side::Sell, 10050, 200);
    std::vector<Fill> fills = orderBook.addLimitOrder(3, Side::Buy, 10050, 150);

    ASSERT_EQ(fills.size(), 2u);
    EXPECT_EQ(fills[0].restingOrderIdentifier, 1u);
    EXPECT_EQ(fills[0].quantity, 100u);
    EXPECT_EQ(fills[1].restingOrderIdentifier, 2u);
    EXPECT_EQ(fills[1].quantity, 50u);
}

TEST(ReferenceOrderBookTest, FullSessionScenario) {
    ReferenceOrderBook orderBook;
    using Fills = std::vector<Fill>;

    // ---- Build the starting book (nothing crosses) ----
    EXPECT_EQ(orderBook.addLimitOrder(1, Side::Sell, 10050, 100), Fills{});
    EXPECT_EQ(orderBook.addLimitOrder(2, Side::Sell, 10050, 200), Fills{});
    EXPECT_EQ(orderBook.addLimitOrder(3, Side::Sell, 10051, 200), Fills{});
    EXPECT_EQ(orderBook.addLimitOrder(4, Side::Sell, 10052, 300), Fills{});
    EXPECT_EQ(orderBook.addLimitOrder(5, Side::Buy, 10049, 150), Fills{});
    EXPECT_EQ(orderBook.addLimitOrder(6, Side::Buy, 10048, 400), Fills{});
    // ASKS 10052:[#4 300]  10051:[#3 200]  10050:[#1 100, #2 200]
    // BIDS 10049:[#5 150]  10048:[#6 400]
    EXPECT_EQ(orderBook.bestBidPrice(), 10049);
    EXPECT_EQ(orderBook.bestAskPrice(), 10050);
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10050), 300u);

    // ---- Buy sweeps two ask levels, partial fill on #3, resting price wins ----
    EXPECT_EQ(orderBook.addLimitOrder(7, Side::Buy, 10051, 350),
              (Fills{{1, 7, 10050, 100}, {2, 7, 10050, 200}, {3, 7, 10051, 50}}));
    // ASKS 10052:[#4 300]  10051:[#3 150]
    // BIDS 10049:[#5 150]  10048:[#6 400]
    EXPECT_EQ(orderBook.bestAskPrice(), 10051);
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10050), 0u);   // empty level removed
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10051), 150u);
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10051), 0u);    // #7 fully filled, did not rest

    // ---- Sell partially fills, remainder rests on the ask side ----
    EXPECT_EQ(orderBook.addLimitOrder(8, Side::Sell, 10049, 500),
              (Fills{{5, 8, 10049, 150}}));
    // ASKS 10052:[#4 300]  10051:[#3 150]  10049:[#8 350]
    // BIDS 10048:[#6 400]
    EXPECT_EQ(orderBook.bestBidPrice(), 10048);
    EXPECT_EQ(orderBook.bestAskPrice(), 10049);
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10049), 350u);

    // ---- Reduce keeps time priority ----
    EXPECT_EQ(orderBook.addLimitOrder(9, Side::Buy, 10048, 200), Fills{});
    orderBook.reduceOrder(6, 100);
    // BIDS 10048:[#6 300, #9 200]
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10048), 500u);
    EXPECT_EQ(orderBook.addLimitOrder(10, Side::Sell, 10048, 100),
              (Fills{{6, 10, 10048, 100}}));                      // #6 still first
    // BIDS 10048:[#6 200, #9 200]

    // ---- Replace loses time priority ----
    EXPECT_EQ(orderBook.replaceOrder(6, 11, 10048, 250), Fills{});
    // BIDS 10048:[#9 200, #11 250]
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10048), 450u);
    EXPECT_EQ(orderBook.addLimitOrder(12, Side::Sell, 10048, 250),
              (Fills{{9, 12, 10048, 200}, {11, 12, 10048, 50}}));  // #9 now first
    // BIDS 10048:[#11 200]

    // ---- Replace that crosses must match ----
    EXPECT_EQ(orderBook.replaceOrder(11, 13, 10049, 200),
              (Fills{{8, 13, 10049, 200}}));
    // ASKS 10052:[#4 300]  10051:[#3 150]  10049:[#8 150]
    // BIDS (empty)
    EXPECT_EQ(orderBook.bestBidPrice(), std::nullopt);
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10049), 150u);

    // ---- Cancels ----
    EXPECT_TRUE(orderBook.cancelOrder(3));
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Sell, 10051), 0u);
    EXPECT_FALSE(orderBook.cancelOrder(999));   // never existed
    EXPECT_FALSE(orderBook.cancelOrder(1));     // already filled
    // ASKS 10052:[#4 300]  10049:[#8 150]

    // ---- Buy larger than the whole ask side: sweep everything, rest the remainder ----
    EXPECT_EQ(orderBook.addLimitOrder(14, Side::Buy, 10052, 1000),
              (Fills{{8, 14, 10049, 150}, {4, 14, 10052, 300}}));
    // ASKS (empty)
    // BIDS 10052:[#14 550]
    EXPECT_EQ(orderBook.bestAskPrice(), std::nullopt);
    EXPECT_EQ(orderBook.bestBidPrice(), 10052);
    EXPECT_EQ(orderBook.quantityAtPrice(Side::Buy, 10052), 550u);
}
