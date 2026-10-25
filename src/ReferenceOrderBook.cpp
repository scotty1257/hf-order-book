#include "ReferenceOrderBook.hpp"
#include <iostream>

#include <numeric>
#include <optional>

std::vector<Fill> ReferenceOrderBook::addLimitOrder(std::uint64_t orderIdentifier,
    Side side,
    std::int64_t priceInTicks,
    std::uint32_t quantity) {

  // List of orders that are filled
  std::vector<Fill> fills;

  // Volume that is left to be traded
  Quantity remainingQuantity = quantity;

  // If the order is a buy side order (meaning someone wants to buy something)
  if (side == Side::Buy) {

    // Keep trading while the buyer still wants more and
    // There is someone still selling the instrument on the book
    while (remainingQuantity > 0 && !_askLevels.empty()) {

      // The best price a seller is willing to sell at  (the lowest price)
      std::optional<PriceInTicks> bestAskPrice = this->bestAskPrice();

      // if the order price is less than the lowest someone will sell,
      // the trade cannot happen because the buyer is a cheapskate
      if (priceInTicks < bestAskPrice) break;

      // List of orders that satisfy the best asking price
      std::list<Order> &orders = ordersAtBestAsk();

      // A resting order is the order that satisfies and is ready to go. (the first one in the
      // 'queue')
      Order &restingOrder = orders.front();

      // We trade the amount that is the minimum. If the resting order is less that the
      // remainingAmount to trade we will take it, but if the remaining quantity is lower, we only
      // trade that amount.
      Quantity tradeQuantity = std::min(remainingQuantity, restingOrder.quantity);

      // Since there is some quantity we have a filled order
      fills.push_back(Fill{restingOrder.id, orderIdentifier, bestAskPrice.value(), tradeQuantity});

      // Remove the traded volume from the queue
      remainingQuantity -= tradeQuantity;
      restingOrder.quantity -= tradeQuantity;

      // We dont have any more volume in that order
      if (restingOrder.quantity == 0) {
        // Remove the resting order from the list it was satisfied
        _locator.erase(restingOrder.id);
        orders.pop_front();

        // If there are no more orders, remove the first selling order (no more buyers for them)
        if (orders.empty()) {
          _askLevels.erase(_askLevels.begin());
        }
      }
    }
    // Matching is done, but the buyer still wants more, put the rest in the book for later as a
    // bid
    if (remainingQuantity > 0) {
      std::list<Order> &ordersAtPrice = _bidLevels[priceInTicks];

      ordersAtPrice.push_back(Order{orderIdentifier, side, priceInTicks, remainingQuantity});

      std::list<Order>::iterator positionInLevel = std::prev(ordersAtPrice.end());

      _locator[orderIdentifier] = {side, priceInTicks, positionInLevel};
    }
  }

  if (side == Side::Sell) {

    while (remainingQuantity > 0 && !_bidLevels.empty()) {

      std::optional<PriceInTicks> bestBidPrice = this->bestBidPrice();

      if (priceInTicks > bestBidPrice) break;

      std::list<Order> &orders = ordersAtBestBid();

      Order &restingOrder = orders.front();

      Quantity tradeQuantity = std::min(remainingQuantity, restingOrder.quantity);

      fills.push_back(Fill{restingOrder.id, orderIdentifier, bestBidPrice.value(), tradeQuantity});

      remainingQuantity -= tradeQuantity;
      restingOrder.quantity -= tradeQuantity;

      if (restingOrder.quantity == 0) {
        _locator.erase(restingOrder.id);
        orders.pop_front();

        if (orders.empty()) {
          _bidLevels.erase(_bidLevels.begin());
        }
      }
    }
    if (remainingQuantity > 0) {
      std::list<Order> &ordersAtPrice = _askLevels[priceInTicks];

      ordersAtPrice.push_back(Order{orderIdentifier, side, priceInTicks, remainingQuantity});

      std::list<Order>::iterator positionInLevel = std::prev(ordersAtPrice.end());

      _locator[orderIdentifier] = {side, priceInTicks, positionInLevel};
    }
  }

  return fills;
}

void ReferenceOrderBook::cancelOrder(std::uint64_t orderIdentifier) {

  OrderLocation &location = _locator.at(orderIdentifier);

  if (location.side == Side::Buy) {
    _bidLevels[location.price].erase(location.position);
  }
  if (location.side == Side::Sell) {
    _askLevels[location.price].erase(location.position);
  }
}

void ReferenceOrderBook::reduceOrder(std::uint64_t orderIdentifier,
    std::uint32_t quantityToRemove) {
  LevelsIterator position = _locator.at(orderIdentifier).position;
  position->quantity -= quantityToRemove;
}

void ReferenceOrderBook::replaceOrder(std::uint64_t oldOrderIdentifier,
    std::uint64_t newOrderIdentifier,
    std::int64_t newPriceInTicks,
    std::uint32_t newQuantity) {

  OrderLocation &originalOrder = _locator.at(oldOrderIdentifier);

  Side originalSide = originalOrder.side;

  this->cancelOrder(oldOrderIdentifier);

  this->addLimitOrder(newOrderIdentifier, originalSide, newPriceInTicks, newQuantity);
}

Quantity ReferenceOrderBook::quantityAtPrice(Side side, PriceInTicks price) const {

  Quantity total = 0;

  std::list<Order> orders = side == Side::Buy ? _bidLevels.at(price) : _askLevels.at(price);

  auto rTotal = [](double price, double currTotal, const Order &o) -> double {
    return o.price == price ? currTotal + price : currTotal;
  };

  total = std::accumulate(
      orders.begin(), orders.end(), 0.0, [rTotal, price](double runningTotal, const Order &order) {
    return rTotal(price, runningTotal, order);
  });

  return total;
}

std::optional<PriceInTicks> ReferenceOrderBook::bestBidPrice() const {
  std::cout << "Size of Bid Levels: " << _bidLevels.size() << std::endl;
  return _bidLevels.begin()->first;
}

std::optional<PriceInTicks> ReferenceOrderBook::bestAskPrice() const {

  std::cout << "Size of Ask Levels: " << _askLevels.size() << std::endl;
  return _askLevels.end()->first;
}

std::list<Order> &ReferenceOrderBook::ordersAtBestBid() {
  return _bidLevels.begin()->second;
}

std::list<Order> &ReferenceOrderBook::ordersAtBestAsk() {
  return _askLevels.begin()->second;
}

std::list<Order> ReferenceOrderBook::asksAtPrice(PriceInTicks price) const {
  return _askLevels.at(price);
}

std::list<Order> ReferenceOrderBook::bidsAtPrice(PriceInTicks price) const {
  return _bidLevels.at(price);
}
