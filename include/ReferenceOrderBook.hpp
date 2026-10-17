#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <ranges>
#include <unordered_map>
#include <vector>

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
  std::int64_t price;
  std::uint32_t quantity;
};

/**
 * OrderLocation
 * - Side *side*;
 * - std::int64_t price;
 * - std::list<Order>::iterator position;
 **/
struct OrderLocation {
  Side side;
  std::int64_t price;
  std::list<Order>::iterator position;
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

  using PriceInTicks = std::int64_t;
  using Identifier = std::uint64_t;

  // Default sort low to high (we want the sell price lowest...of course)
  std::map<PriceInTicks, std::list<Order>> _askLevels;

  // Sort high to low (we want the highest buy order......profit)
  std::map<PriceInTicks, std::list<Order>, std::greater<>> _bidLevels;

  std::unordered_map<Identifier, OrderLocation> _locator;
};
