#include "ReferenceOrderBook.hpp"

#include <numeric>
#include <optional>

std::vector<Fill> ReferenceOrderBook::addLimitOrder(std::uint64_t orderIdentifier,
    Side side,
    std::int64_t priceInTicks,
    std::uint32_t quantity) {

  // Create our order object for passing into levels;
  Order order{orderIdentifier, side, priceInTicks, quantity};

  if (side == Side::Sell) {
    _askLevels[order.price].push_back(order);

    std::list<Order>::iterator it = std::prev(_askLevels[order.price].end());

    OrderLocation location{side, priceInTicks, it};
    _locator[orderIdentifier] = location;
  }

  if (side == Side::Buy) {
    _bidLevels[order.price].push_back(order);
    std::list<Order>::iterator it = std::prev(_bidLevels[order.price].end());

    OrderLocation location{side, priceInTicks, it};
    _locator[orderIdentifier] = location;
  }

  return {};
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
  std::list<Order>::iterator position = _locator.at(orderIdentifier).position;
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

std::optional<std::int64_t> ReferenceOrderBook::bestBidPrice() const {
  return _askLevels.begin()->first;
}

std::optional<std::int64_t> ReferenceOrderBook::bestAskPrice() const {

  return _bidLevels.end()->first;
}
