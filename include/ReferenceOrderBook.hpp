#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

/** An amount of orders */
using Quantity = std::uint32_t;

/** Typing that type everytime is annoying and has no semantic meaning */
using PriceInTicks = std::int64_t;

/** Can never have a negative ID */
using Identifier = std::uint64_t;

enum class Side : std::uint8_t { Buy, Sell };

struct Fill {
  Identifier restingOrderIdentifier;
  Identifier incomingOrderIdentifier;
  PriceInTicks priceInTicks;
  Quantity quantity;
};

struct Order {
  Identifier id;
  Side side;
  PriceInTicks price;
  Quantity quantity;
};

/// Holds the information about an order and its position in line.
struct OrderLocation {
  Side side;
  PriceInTicks price;
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
  /**
   * Add an order to the order book on either Ask/Bid side.
   **/
  std::vector<Fill> addLimitOrder(Identifier orderIdentifier,
      Side side,
      PriceInTicks priceInTicks,
      Quantity quantity);

  /**
   * Remove an order from the corresponding limit list
   **/
  void cancelOrder(Identifier orderIdentifier);

  /**
   * Change the order volume in place
   **/
  void reduceOrder(Identifier orderIdentifier, Quantity quantityToRemove);

  /**
   * Replace the order completely in place
   **/
  void replaceOrder(Identifier oldOrderIdentifier,
      Identifier newOrderIdentifier,
      PriceInTicks newPriceInTicks,
      Quantity newQuantity);

  Quantity quantityAtPrice(Side side, PriceInTicks price);

  /**
   * Get the best price for buyer (bid):: best => highest
   * Bid is the highest price a buyer is willing to pay for an instrument
   **/
  std::optional<PriceInTicks> bestBidPrice() const;

  /**
   * Get the best price for seller (ask):: best => lowest
   * Ask is the lowest price an instrument is listed for sell
   **/
  std::optional<PriceInTicks> bestAskPrice() const;

  std::list<Order> bidsAtPrice(PriceInTicks price) const;
  std::list<Order> asksAtPrice(PriceInTicks price) const;

private:
  PriceInTicks _bestBid;
  PriceInTicks _bestAsk;

  /** Default sort low to high (we want the sell price lowest...of course) */
  std::map<PriceInTicks, std::list<Order>> _askLevels;

  /** Sort high to low (we want the highest buy order......profit) */
  std::map<PriceInTicks, std::list<Order>, std::greater<>> _bidLevels;

  /** Map the order ID to is OrderLocation iterator container */
  std::unordered_map<Identifier, OrderLocation> _locator;
};
