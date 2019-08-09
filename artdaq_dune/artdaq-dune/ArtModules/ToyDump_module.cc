////////////////////////////////////////////////////////////////////////
// Class:       ToyDump
// Module Type: analyzer
// File:        ToyDump_module.cc
// Description: Prints out information about each event.
////////////////////////////////////////////////////////////////////////

#define TRACE_NAME "ToyDump"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"

#include "artdaq-core-demo/Overlays/FragmentType.hh"
#include "artdaq-core-demo/Overlays/ToyFragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

namespace demo
{
	class ToyDump;
}

/**
 * \brief An art::EDAnalyzer module designed to display the data from demo::ToyFragment objects
 */
class demo::ToyDump : public art::EDAnalyzer
{
public:
	/**
	 * \brief ToyDump Constructor
	 * \param pset ParamterSet used to configure ToyDump
	 *
	 * \verbatim
	 * ToyDump accepts the following Parameters:
	 * "raw_data_label" (Default: "daq"): The label used to identify artdaq data
	 * "num_adcs_to_print" (Default: 10): How many ADCs to print to screen from each ToyFragment (-1 to disable, 0 for all)
	 * "num_adcs_to_write" (Default: 0): How many ADCs to write to file from each ToyFragment (-1 to disable, 0 for all)
	 * "output_file_name" (Default: "out.bin"): File to write ADC values to
	 * "binary_mode" (Default: true): Whether to write output in binary (true) or as tab-delimited ASCII text (false)
	 * "columns_to_display_on_screen" (Default: 10): How many ADC values to print in each row when writing to stdout
	 * \endverbatim
	 */
	explicit ToyDump(fhicl::ParameterSet const& pset);

	/**
	 * \brief ToyDump Destructor
	 */
	virtual ~ToyDump();

	/**
	* \brief Analyze an event. Called by art for each event in run (based on command line options)
	* \param evt The art::Event object to dump ToyFragments from
	*/
	virtual void analyze(art::Event const& evt);

private:
	std::string raw_data_label_;
	int num_adcs_to_write_;
	int num_adcs_to_print_;
	bool binary_mode_;
	uint32_t columns_to_display_on_screen_;
	std::string output_file_name_;
};


demo::ToyDump::ToyDump(fhicl::ParameterSet const& pset)
	: EDAnalyzer(pset)
	, raw_data_label_(pset.get<std::string>("raw_data_label", "daq"))
	, num_adcs_to_write_(pset.get<int>("num_adcs_to_write", 0))
	, num_adcs_to_print_(pset.get<int>("num_adcs_to_print", 10))
	, binary_mode_(pset.get<bool>("binary_mode", true))
	, columns_to_display_on_screen_(pset.get<uint32_t>("columns_to_display_on_screen", 10))
	, output_file_name_(pset.get<std::string>("output_file_name", "out.bin"))
{}

demo::ToyDump::~ToyDump() {}

void demo::ToyDump::analyze(art::Event const& evt)
{
	art::EventNumber_t eventNumber = evt.event();

	// ***********************
	// *** Toy Fragments ***
	// ***********************

	artdaq::Fragments fragments;
	artdaq::FragmentPtrs containerFragments;
	std::vector<std::string> fragment_type_labels{ "TOY1", "TOY2", "ContainerTOY1", "ContainerTOY2" };

	for (auto label : fragment_type_labels)
	{
		art::Handle<artdaq::Fragments> fragments_with_label;

		evt.getByLabel(raw_data_label_, label, fragments_with_label);
		if (!fragments_with_label.isValid()) continue;

		if (label == "Container" || label == "ContainerTOY1" || label == "ContainerTOY2")
		{
			for (auto cont : *fragments_with_label)
			{
				artdaq::ContainerFragment contf(cont);
				for (size_t ii = 0; ii < contf.block_count(); ++ii)
				{
					containerFragments.push_back(contf[ii]);
					fragments.push_back(*containerFragments.back());
				}
			}
		}
		else
		{
			for (auto frag : *fragments_with_label)
			{
				fragments.emplace_back(frag);
			}
		}
	}

	// look for raw Toy data
	TLOG(TLVL_INFO) << "Run " << evt.run() << ", subrun " << evt.subRun()
		<< ", event " << eventNumber << " has " << fragments.size()
		<< " fragment(s) of type TOY1 or TOY2";

	for (const auto& frag : fragments)
	{
		ToyFragment bb(frag);

		TLOG(TLVL_INFO) << fragmentTypeToString(static_cast<demo::detail::FragmentType>(frag.type())) << " fragment " << frag.fragmentID()
			<< " w/ seqID " << frag.sequenceID() << " and timestamp " << frag.timestamp()
			<< " has total ADC counts = " << bb.total_adc_values() << ", trig # = " << bb.hdr_trigger_number() << ", dist_type = " << bb.hdr_distribution_type();

		if (frag.hasMetadata())
		{
			ToyFragment::Metadata const* md =
				frag.metadata<ToyFragment::Metadata>();
			TLOG(TLVL_DEBUG) << "Fragment metadata: " << std::showbase << "Board serial number = "
				<< md->board_serial_number << ", sample bits = "
				<< md->num_adc_bits
				<< " -> max ADC value = "
				<< bb.adc_range((int)md->num_adc_bits);
		}

		if (num_adcs_to_write_ >= 0)
		{
			uint32_t numAdcs = num_adcs_to_write_;
			if (num_adcs_to_write_ == 0) numAdcs = bb.total_adc_values();
			else if (static_cast<uint32_t>(num_adcs_to_write_) > bb.total_adc_values())
			{
				TLOG(TLVL_WARNING) << "Asked for more ADC values to file than are in Fragment. Only writing what's here...";
				numAdcs = bb.total_adc_values();
			}
			if (binary_mode_) {
				std::ofstream output(output_file_name_, std::ios::out | std::ios::app | std::ios::binary);
				for (uint32_t i_adc = 0; i_adc < numAdcs; ++i_adc)
				{
					output.write((char*)(bb.dataBeginADCs() + i_adc), sizeof(ToyFragment::adc_t));
				}
				output.close();
			}
			else {
				std::ofstream output(output_file_name_, std::ios::out | std::ios::app);
				output << fragmentTypeToString(static_cast<demo::detail::FragmentType>(frag.type())) << "\t" << frag.fragmentID();

				for (uint32_t i_adc = 0; i_adc < numAdcs; ++i_adc)
				{
					output << "\t" << std::to_string(*(bb.dataBeginADCs() + i_adc));
				}
				output << std::endl;
				output.close();
				
			}
		}

		if (num_adcs_to_print_ >= 0)
		{
			uint32_t numAdcs = num_adcs_to_print_;
			if (num_adcs_to_print_ == 0) numAdcs = bb.total_adc_values();
			else if (static_cast<uint32_t>(num_adcs_to_print_) > bb.total_adc_values())
			{
				TLOG(TLVL_WARNING) << "Asked for more ADC values to file than are in Fragment. Only writing what's here...";
				numAdcs = bb.total_adc_values();
			}

			TLOG(TLVL_INFO) << "First " << numAdcs << " ADC values in the fragment:";
			int rows = 1 + (int)((num_adcs_to_print_ - 1) / columns_to_display_on_screen_);
			uint32_t adc_counter = 0;
			for (int idx = 0; idx < rows; ++idx)
			{
				std::ostringstream o;
				o << std::right;
				o << std::setw(4) << std::setfill('.');
				o << (idx * columns_to_display_on_screen_) << ": ";
				for (uint32_t jdx = 0; jdx < columns_to_display_on_screen_; ++jdx)
				{
					if (adc_counter >= numAdcs) { break; }
					o << std::setw(6) << std::setfill(' ');
					o << bb.adc_value(adc_counter);
					++adc_counter;
				}
				TLOG(TLVL_INFO) << o.str();
			}
		}
	}
}

DEFINE_ART_MODULE(demo::ToyDump)
