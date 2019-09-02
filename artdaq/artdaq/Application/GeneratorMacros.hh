#ifndef artdaq_Application_GeneratorMacros_hh
#define artdaq_Application_GeneratorMacros_hh

#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "fhiclcpp/fwd.h"

#include "cetlib/compiler_macros.h"
#include <memory>

namespace artdaq
{
	/**
	* \brief Constructs a CommandableFragmentGenerator instance, and returns a pointer to it
	* \param ps Parameter set for initializing the CommandableFragmentGenerator
	* \return A smart pointer to the CommandableFragmentGenerator
	*/
	typedef std::unique_ptr<artdaq::CommandableFragmentGenerator> makeFunc_t(fhicl::ParameterSet const& ps);
}

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

#define DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(klass)                    \
  EXTERN_C_FUNC_DECLARE_START                                      \
  std::unique_ptr<artdaq::CommandableFragmentGenerator>               \
  make(fhicl::ParameterSet const & ps) {                              \
    return std::unique_ptr<artdaq::CommandableFragmentGenerator>(new klass(ps)); \
  }}

#endif /* artdaq_Application_GeneratorMacros_hh */
