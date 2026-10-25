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

    std::uin32_t remainingVolumeToBuy = volume;

    while (remainingVolumeToBuy > 0 && !mAsks.empty()) {

      // buy order less than lowest selling price
      if (price < bestAskPrice) break;

      SimpleOrder &restingOrder = mAsks.at(bestAskPrice).front();

      std::uint32_t soldOrderId = restingOrder.id;

      // if resting order volume is more than remaining we have enough for the entire order
      if (remainingVolumeToBuy < restingOrder.volume) {
        std::uint32_t boughtVolume = remainingVolumeToBuy;
        // resting order is less than the remaining volume to buy
        // so we have to remove it from the lsit and go to the next order

        FillOrder(soldOrderId, id, bestAskPrice, boughtVolume);
      } else {
        // resting order is more than the remaining volume.
        // dont remove from the list, just change the order amount
        // Only having to sell the resting order
        FillOrder(soldOrderId, id, bestAskPrice, restingOrder.volume);
      }
    }

    // mBids[price].push_back(SimpleOrder{id, volume, price});
  }

  if (side == SimpleSide::Sell) {
    mAsks[price].push_back(SimpleOrder{id, volume, price});
  }
}
