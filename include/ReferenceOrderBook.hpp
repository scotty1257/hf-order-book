#pragma once

#include <cstdint>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <map>
#include <vector>
#include <list>

enum class Side : std::uint8_t { Buy, Sell };

struct Fill {
    std::uint64_t restingOrderIdentifier;
    std::uint64_t incomingOrderIdentifier;
    std::int64_t priceInTicks;
    std::uint32_t quantity;
};

struct Order {
  std::uint64_t id;
  Side side;
  std::uint64_t price;
  std::uint64_t quantity;
};

class ReferenceOrderBook {
public:
    std::vector<Fill> addLimitOrder(std::uint64_t orderIdentifier, Side side,
                                    std::int64_t priceInTicks, std::uint32_t quantity);
    void cancelOrder(std::uint64_t orderIdentifier);
    void reduceOrder(std::uint64_t orderIdentifier, std::uint32_t quantityToRemove);
    void replaceOrder(std::uint64_t oldOrderIdentifier, std::uint64_t newOrderIdentifier,
                      std::int64_t newPriceInTicks, std::uint32_t newQuantity);
    std::optional<std::int64_t> bestBidPrice() const;
    std::optional<std::int64_t> bestAskPrice() const;

private:
        std::int64_t _bestBid;
        std::int64_t _bestAsk;

// map<id, list<>>
    std::map<std::int64_t, std::list<Order>> _orders;
    std::unordered_map<std::uint64_t, std::ranges::iterator_t<std::uint64_t>> mOrders;
};
