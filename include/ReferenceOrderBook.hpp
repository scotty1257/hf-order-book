#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
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
 * - Side side;
 * - std::int64_t price;
 * - std::list<Order>::iterator position;
 **/
struct OrderLocation {
  Side side;
  std::int64_t price;
  std::list<Order>::iterator position;
};

/**
 * Order Book Rules
 *   1. Price-time priority with partial fills
 *   2. A reduce keeps the order's place in the queue
 *   3. A replace (new price or larger size) loses its place
 **/
class ReferenceOrderBook {
public:
  /** Typing that type everytime is annoying and has no semantic meaning */
  using PriceInTicks = std::int64_t;

  /** Can never have a negative ID */
  using Identifier = std::uint64_t;

  /**
   * Add an order to the order book on either Ask/Bid side.
   **/
  std::vector<Fill> addLimitOrder(std::uint64_t orderIdentifier, Side side,
      std::int64_t priceInTicks, std::uint32_t quantity);

  /**
   * Remove an order from the corresponding limit list
   **/
  void cancelOrder(std::uint64_t orderIdentifier);

  /**
   * Change the order volume in place
   **/
  void reduceOrder(std::uint64_t orderIdentifier, std::uint32_t quantityToRemove);

  /**
   * Replace the order completely in place
   **/
  void replaceOrder(std::uint64_t oldOrderIdentifier, std::uint64_t newOrderIdentifier,
      std::int64_t newPriceInTicks, std::uint32_t newQuantity);

  std::optional<std::int64_t> quantityAtPrice(Side side, PriceInTicks price);

  /**
   * Get the best price for buyer (bid):: best => highest
   * Bid is the highest price a buyer is willing to pay for an instrument
   **/
  std::optional<std::int64_t> bestBidPrice() const;

  /**
   * Get the best price for seller (ask):: best => lowest
   * Ask is the lowest price an instrument is listed for sell
   **/
  std::optional<std::int64_t> bestAskPrice() const;

private:
  std::int64_t _bestBid;
  std::int64_t _bestAsk;

  /** Default sort low to high (we want the sell price lowest...of course) */
  std::map<PriceInTicks, std::list<Order>> _askLevels;

  /** Sort high to low (we want the highest buy order......profit) */
  std::map<PriceInTicks, std::list<Order>, std::greater<>> _bidLevels;

  /** Map the order ID to is OrderLocation iterator container */
  std::unordered_map<Identifier, OrderLocation> _locator;
};
