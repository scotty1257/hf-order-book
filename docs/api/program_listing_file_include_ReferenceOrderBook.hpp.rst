
.. _program_listing_file_include_ReferenceOrderBook.hpp:

Program Listing for File ReferenceOrderBook.hpp
===============================================

|exhale_lsh| :ref:`Return to documentation for file <file_include_ReferenceOrderBook.hpp>` (``include/ReferenceOrderBook.hpp``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #pragma once
   
   #include <cstdint>
   #include <functional>
   #include <list>
   #include <map>
   #include <optional>
   #include <unordered_map>
   #include <vector>
   
   using Quantity = std::uint32_t;
   
   using PriceInTicks = std::int64_t;
   
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
   
   struct OrderLocation {
     Side side;
     PriceInTicks price;
     std::list<Order>::iterator position;
   };
   
   class ReferenceOrderBook {
   public:
     std::vector<Fill> addLimitOrder(Identifier orderIdentifier,
         Side side,
         PriceInTicks priceInTicks,
         Quantity quantity);
   
     void cancelOrder(Identifier orderIdentifier);
   
     void reduceOrder(Identifier orderIdentifier, Quantity quantityToRemove);
   
     void replaceOrder(Identifier oldOrderIdentifier,
         Identifier newOrderIdentifier,
         PriceInTicks newPriceInTicks,
         Quantity newQuantity);
   
     Quantity quantityAtPrice(Side side, PriceInTicks price);
   
     std::optional<PriceInTicks> bestBidPrice() const;
   
     std::optional<PriceInTicks> bestAskPrice() const;
   
     std::list<Order> bidsAtPrice(PriceInTicks price) const;
     std::list<Order> asksAtPrice(PriceInTicks price) const;
   
   private:
     PriceInTicks _bestBid;
     PriceInTicks _bestAsk;
   
     std::map<PriceInTicks, std::list<Order>> _askLevels;
   
     std::map<PriceInTicks, std::list<Order>, std::greater<>> _bidLevels;
   
     std::unordered_map<Identifier, OrderLocation> _locator;
   };
