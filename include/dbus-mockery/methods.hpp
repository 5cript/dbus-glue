#pragma once

namespace DBusMock
{
   template <typename>
   struct method{};

   template <typename R, typename... Params>
   struct method<R(Params...)>
   {
   };

   template <typename...>
   struct methods
   {
   };

   template <typename... List>
   struct methods <method <List>...>
   {

   };
}
