#ifndef artdaq_Application_Routing_PolicyMacros_hh
#define artdaq_Application_Routing_PolicyMacros_hh

#include "artdaq/Application/Routing/RoutingMasterPolicy.hh"
#include "fhiclcpp/fwd.h"
#include "cetlib/compiler_macros.h"

#include <memory>

namespace artdaq
{
/**
* \brief Constructs a RoutingMasterPolicy instance, and returns a pointer to it
* \param ps Parameter set for initializing the RoutingMasterPolicy
* \return A smart pointer to the RoutingMasterPolicy
*/
	typedef std::unique_ptr<artdaq::RoutingMasterPolicy> makeFunc_t(fhicl::ParameterSet const& ps);
}

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

#define DEFINE_ARTDAQ_ROUTING_POLICY(klass)                    \
  EXTERN_C_FUNC_DECLARE_START                                      \
  std::unique_ptr<artdaq::RoutingMasterPolicy>               \
  make(fhicl::ParameterSet const & ps) {                              \
	return std::unique_ptr<artdaq::RoutingMasterPolicy>(new klass(ps)); \
  }}

#endif /* artdaq_Application_Routing_PolicyMacros_hh */
