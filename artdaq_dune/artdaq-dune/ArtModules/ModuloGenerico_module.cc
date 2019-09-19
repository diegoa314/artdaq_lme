////////////////////////////////////////////////////////////////////////
// Class:       ModuloGenerico
// Plugin Type: analyzer (art v2_10_02)
// File:        ModuloGenerico_module.cc
//
// Generated at Wed Sep 18 13:51:58 2019 by root using cetskelgen
// from cetlib version v3_02_00.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

namespace prueba {
  class ModuloGenerico;
}


class prueba::ModuloGenerico : public art::EDAnalyzer {
public:
  explicit ModuloGenerico(fhicl::ParameterSet const & p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  ModuloGenerico(ModuloGenerico const &) = delete;
  ModuloGenerico(ModuloGenerico &&) = delete;
  ModuloGenerico & operator = (ModuloGenerico const &) = delete;
  ModuloGenerico & operator = (ModuloGenerico &&) = delete;

  // Required functions.
  void analyze(art::Event const & e) override;

  // Selected optional functions.
  void beginRun(art::Run const & r) override;
  void endRun(art::Run const & r) override;

private:

  // Declare member data here.

};


prueba::ModuloGenerico::ModuloGenerico(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p)  // ,
 // More initializers here.
{}

void prueba::ModuloGenerico::analyze(art::Event const & e)
{
  // Implementation of required member function here.
}

void prueba::ModuloGenerico::beginRun(art::Run const & r)
{
  // Implementation of optional member function here.
}

void prueba::ModuloGenerico::endRun(art::Run const & r)
{
  // Implementation of optional member function here.
}

DEFINE_ART_MODULE(prueba::ModuloGenerico)
