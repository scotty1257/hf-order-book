#include <cstdint>
#include <functional>
#include <list>
#include <map>

struct TOrder {};

/*
Asks (Selling Price)

$100 [1, 50] [2, 60] [3, 100] [4, 20] [5, 30]
$200 [6, 200] [7, 300]
$300 [8, 10] [9, 15] [10, 80] [11, 200] [12, 100]

Bids (Buying Price)

$300 [13, 50] [14, 60] [15, 70]
$240 [16, 80] [17, 90] [18, 40] [19, 30]
$200 [20, 100] [21, 200]
*/

enum TSide { Buy, Sell };
struct TOrder {
  int id;
  TSide side;
  int price;
  int amount;
};

struct TLoc {
  TSide side;
  int price;
  std::list<TOrder>::iterator pos;
};

std::map<int, std::list<TOrder>> sellOrders;
std::map<int, std::list<TOrder>, std::greater<>> buyOrders;
std::map<int, TLoc> orderLocatorMap;

void order(int id, TSide side, int price, int amount) {

  if (side == TSide::Buy) {
    // Okay, someone sent an order in to buy something
    // They sent the order with a price for an amount of an instrument

    // If we are buying, we have to check what is being sold
    // so lets get the best price someone is selling an instrument at
    //    int sellingPrice = sellOrders

    // if the price we are buying something at is less than the lowest
    // selling price, we cant do anything...

    //  in matching systems we send an order and we keep matching until we are done
    // For exampel: I send an order to buy 200 units, there is a resting order that can
    // sell to me 100 units, and thats it. So I create a new order of 100 units for the price
    // i am willing to buy at.

    // send order 200 dollars for 100 units to buy
    // left to buy is 100 units
    // someone is selling at 195 80 units
    // fill the order
    // left to sell remove 80 units
    // left to sell is now 20 units
    // filled order with buying order id and selling order id for price and amount
  }
}
