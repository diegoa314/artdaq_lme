////////////////////////////////////////////////////////////////////////
// Class:       EventDump
// Module Type: analyzer
// File:        EventDump_module.cc
// Description: Prints out information about each event.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

namespace artdaq
{
	class EventDump;
}

/**
 * \brief Write Event information to the console
 */
class artdaq::EventDump : public art::EDAnalyzer
{
public:
	/**
	 * \brief EventDump Constructor
	 * \param pset ParameterSet used to configure EventDump
	 * 
	 * \verbatim
	 * EventDump accepts the following Parameters:
	 * "raw_data_label" (Default: "daq"): The label used to store artdaq data
	 * "verbosity" (Default: 0): verboseness level
	 * \endverbatim
	 */
	explicit EventDump(fhicl::ParameterSet const& pset);

	/**
	 * \brief Default virtual Destructor
	 */
	virtual ~EventDump() = default;

	/**
	 * \brief This method is called for each art::Event in a file or run
	 * \param e The art::Event to analyze
	 * 
	 * This module simply prints the event number, and art by default
	 * prints the products found in the event.
	 */
	void analyze(art::Event const& e) override;

private:
	std::string raw_data_label_;
	int verbosity_;
};


artdaq::EventDump::EventDump(fhicl::ParameterSet const& pset)
	: EDAnalyzer(pset)
	, raw_data_label_(pset.get<std::string>("raw_data_label", "daq"))
	, verbosity_(pset.get<int>("verbosity",0)) {}

void artdaq::EventDump::analyze(art::Event const& e)
{
    if (verbosity_ > 0) {
	std::cout << "***** Start of EventDump for event " << e.event() << " *****" << std::endl;

	std::vector< art::Handle< std::vector<artdaq::Fragment> > > fragmentHandles;
	e.getManyByType(fragmentHandles);

	for (auto const& handle : fragmentHandles)
        {
          if (handle->size() > 0) {
            std::string instance_name = handle.provenance()->productInstanceName();
            std::cout << instance_name << " fragments: " << std::endl;

            int jdx = 1;
            for (auto const& frag : *handle){
              std::cout << "  " << jdx << ") fragment ID " << frag.fragmentID() << " has type "
                        << (int) frag.type() << " and timestamp " << frag.timestamp();

              if (instance_name.compare(0,9,"Container")==0) {
                artdaq::ContainerFragment cf(frag);
                std::cout << " (contents: type = " << (int) cf.fragment_type() << ", count = "
                          << cf.block_count() << ", missing data = " << cf.missing_data()
                          << ")" << std::endl;;
                if (verbosity_ > 1) {
                  for (size_t idx = 0; idx < cf.block_count(); ++idx) {
                    std::cout << "    " << (idx+1) << ") fragment type " << (int) (cf.at(idx))->type()
                              << " fragment timestamp " << (cf.at(idx))->timestamp() << std::endl;
                  }
                }
              }
              else {
                std::cout << std::endl;
              }
              ++jdx;
            }
          }
	}

	std::cout << "***** End of EventDump for event " << e.event() << " *****" << std::endl;
    }
}

DEFINE_ART_MODULE(artdaq::EventDump)
