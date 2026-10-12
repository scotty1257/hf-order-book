#include "ReferenceOrderBook.hpp"
#include <optional>

// TEST(ReferenceOrderBookTest, PartialFillRespectsTimePriority) {
//     ReferenceOrderBook orderBook;
//     orderBook.addLimitOrder(1, Side::Sell, 10050, 100);
//     orderBook.addLimitOrder(2, Side::Sell, 10050, 200);
//     std::vector<Fill> fills = orderBook.addLimitOrder(3, Side::Buy, 10050, 150);

//     ASSERT_EQ(fills.size(), 2u);
//     EXPECT_EQ(fills[0].restingOrderIdentifier, 1u);
//     EXPECT_EQ(fills[0].quantity, 100u);
//     EXPECT_EQ(fills[1].restingOrderIdentifier, 2u);
//     EXPECT_EQ(fills[1].quantity, 50u);
// }

// Bid -> buy
// Ask -> Sell
// Spread -> gap  between highest bid and lowst ask
//    - gap between highest buy pricce and the lowest selling price
// struct Fill {
//     std::uint64_t restingOrderIdentifier;
//     std::uint64_t incomingOrderIdentifier;
//     std::int64_t priceInTicks;
//     std::uint32_t quantity;
// };
// `std::map<price, std::list<Order>>` plus `std::unordered_map<orderIdentifier, iterator>`

// struct Order {
//   std::uint64_t id;
//   Side side;
//   std::uint64_t price;
//   std::uint64_t quantity;
// };

std::vector<Fill> ReferenceOrderBook::addLimitOrder(std::uint64_t orderIdentifier, Side side,
    std::int64_t priceInTicks, std::uint32_t quantity) {

  if (crossover threshold) {
    create fills vector(match them)
  }
}

void ReferenceOrderBook::cancelOrder(std::uint64_t orderIdentifier) {}

void ReferenceOrderBook::reduceOrder(
    std::uint64_t orderIdentifier, std::uint32_t quantityToRemove) {}

void ReferenceOrderBook::replaceOrder(std::uint64_t oldOrderIdentifier,
    std::uint64_t newOrderIdentifier, std::int64_t newPriceInTicks,
    std::uint32_t newQuantity) {}

std::optional<std::int64_t> ReferenceOrderBook::bestBidPrice() const {}

std::optional<std::int64_t> ReferenceOrderBook::bestAskPrice() const {}
