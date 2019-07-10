#pragma once

#include "methods.hpp"
#include "properties.hpp"
#include "signals.hpp"

#include "bindings/sdbus_core.hpp"

namespace DBusMock
{
   template <typename M, typename P, typename S>
   struct declare_interface
   {
   };

   // MPS - main implementation
   template <typename... Methods, typename... Properties, typename ...Signals>
   struct declare_interface <methods<Methods...>, properties<Properties...>, signals<Signals...>>

   {
	  using interface_methods = methods <Methods...>;
	  using interface_properties = properties <Properties...>;
	  using interface_signals = signals <Signals...>;
   };

   // SPM
   template <typename... Methods, typename... Properties, typename ...Signals>
   struct declare_interface <signals<Signals...>, properties<Properties...>, methods<Methods...>>
         : public declare_interface <methods<Methods...>, properties<Properties...>, signals<Signals...>>
   {
   };

   // PMS
   template <typename... Methods, typename... Properties, typename ...Signals>
   struct declare_interface <properties<Properties...>, methods<Methods...>, signals<Signals...>>
         : public declare_interface <methods<Methods...>, properties<Properties...>, signals<Signals...>>
   {
   };

   // PSM
   template <typename... Methods, typename... Properties, typename ...Signals>
   struct declare_interface <properties<Properties...>, signals<Signals...>, methods<Methods...>>
         : public declare_interface <methods<Methods...>, properties<Properties...>, signals<Signals...>>
   {
   };

   // MSP
   template <typename... Methods, typename... Properties, typename ...Signals>
   struct declare_interface <methods<Methods...>, signals<Signals...>, properties<Properties...>>
         : public declare_interface <methods<Methods...>, properties<Properties...>, signals<Signals...>>
   {
   };

   // SMP
   template <typename... Methods, typename... Properties, typename ...Signals>
   struct declare_interface <signals<Signals...>, methods<Methods...>, properties<Properties...>>
         : public declare_interface <methods<Methods...>, properties<Properties...>, signals<Signals...>>
   {
   };
}
