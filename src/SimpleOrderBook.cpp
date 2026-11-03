#include "SimpleOrderBook.hpp"

#include <algorithm>
#include <map>
#include <print>
#include <string>

void SimpleOrderBook::AddOrder(std::uint32_t id,
    SimpleSide side,
    std::uint32_t volume,
    std::uint64_t price) {

  if (side == SimpleSide::Buy) {
    // order is a buy side order

    std::uint64_t bestAskPrice = mAsks.begin()->first;

    std::uint32_t remainingVolumeToBuy = volume;

    // Matching loop here, after this we just add to the book
    while (remainingVolumeToBuy > 0 && !mAsks.empty()) {

      // buy order less than lowest selling price
      if (price < bestAskPrice) break;

      SimpleOrder &restingOrder = mAsks.at(bestAskPrice).front();

      std::uint32_t soldOrderId = restingOrder.id;

      std::uint32_t volumeTraded = remainingVolumeToBuy;
      if (remainingVolumeToBuy > restingOrder.volume) {
        volumeTraded = restingOrder.volume;
      }

      FillOrder(restingOrder.id, id, bestAskPrice, volumeTraded);

      remainingVolumeToBuy -= volumeTraded;
      // restingOrder.quantity -= volumeTraded;

      // if resting order volume is more than remaining we have enough for the entire order
      // if (remainingVolumeToBuy < restingOrder.volume) {
      //   std::uint32_t boughtVolume = remainingVolumeToBuy;
      //   // resting order is less than the remaining volume to buy
      //   // so we have to remove it from the list and go to the next order

      //   FillOrder(soldOrderId, id, bestAskPrice, boughtVolume);

      //   SimpleOrderIterator &it = mIterator.at(soldOrderId);

      // } else {
      //   // resting order is more than the remaining volume.
      //   // dont remove from the list, just change the order amount
      //   // Only having to sell the resting order
      //   FillOrder(soldOrderId, id, bestAskPrice, restingOrder.volume);

      //   std::list<SimpleOrder>::iterator pos = mIterator.at(soldOrderId).position;

      //   remainingVolumeToBuy -= volume;
      //   pos->volume = remainingVolumeToBuy;
      // }
    }

    // mBids[price].push_back(SimpleOrder{id, volume, price});
  }

  if (side == SimpleSide::Sell) {
    mAsks[price].push_back(SimpleOrder{id, volume, price});
  }
}

void SimpleOrderBook::FillOrder(std::uint32_t restingOrderId,
    std::uint32_t incomingId,
    std::uint64_t price,
    std::uint32_t volume) {
  mActiveFilledOrders.emplace_back(SimpleFilledOrder{
      .restingOrderId = restingOrderId,
      .incomingOrderId = incomingId,
      .filledPrice = price,
      .filledVolume = volume,
  });
}

int SimpleOrderBook::FlushFilledOrders() {

  int quantity = mActiveFilledOrders.size();

  // std::for_each(
  //     mActiveFilledOrders.begin(), mActiveFilledOrders.end(), [](SimpleFilledOrder &order) {
  //   std::print("A: {}, B: {}, ${:2f}, Q: {}", order.restingOrderId, order.incomingOrderId,
  //       order.filledPrice, order.filledVolume);
  // });

  mActiveFilledOrders.clear();

  return quantity;
}

SimpleFilledOrder &SimpleOrderBook::GetMostRecentFilledOrder() {
  return mActiveFilledOrders.front();
}
