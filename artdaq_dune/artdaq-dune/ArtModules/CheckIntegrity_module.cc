////////////////////////////////////////////////////////////////////////
// Class:       CheckIntegrity
// Module Type: analyzer
// File:        CheckIntegrity_module.cc
// Description: Prints out information about each event.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Utilities/Exception.h"

#include "artdaq-core-dune/Overlays/PruebaFragmento.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq-core/Data/Fragment.hh"

#include "tracemf.h"			// TLOG
#define TRACE_NAME "CheckIntegrity"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <vector>
#include <iostream>

namespace prueba
{
	class CheckIntegrity;
}

/**
 * \brief Demonstration art::EDAnalyzer which checks that all ToyFragment ADC counts are in the defined range
 */
class prueba::CheckIntegrity : public art::EDAnalyzer
{
public:
	/**
	 * \brief CheckIntegrity Constructor
	 * \param pset ParameterSet used to configure CheckIntegrity
	 *
	 * CheckIntegrity has the following paramters:
	 * "raw_data_label": The label applied to data (usually "daq")
	 * "frag_type": The fragment type to analyze ("TOY1" or "TOY2")
	 */
	explicit CheckIntegrity(fhicl::ParameterSet const& pset);

	/**
	 * \brief Default destructor
	 */
	virtual ~CheckIntegrity() = default;

	/**
	* \brief Analyze an event. Called by art for each event in run (based on command line options)
	* \param evt The art::Event object containing ToyFragments to check
	*/
	virtual void analyze(art::Event const& evt);

private:
	std::string raw_data_label_;
};


prueba::CheckIntegrity::CheckIntegrity(fhicl::ParameterSet const& pset)
	: EDAnalyzer(pset)
	, raw_data_label_(pset.get<std::string>("raw_data_label"))
{}

void prueba::CheckIntegrity::analyze(art::Event const& evt)
{


	artdaq::Fragments fragments;
	artdaq::FragmentPtrs containerFragments;
	std::vector<std::string> fragment_type_labels{ "PRUEBA", "ContainerPRUEBA"};

	for (auto label : fragment_type_labels)
	{
		art::Handle<artdaq::Fragments> fragments_with_label;

		evt.getByLabel(raw_data_label_, label, fragments_with_label);
		if (!fragments_with_label.isValid()) continue;

		if (label == "Container" || label == "ContainerPRUEBA")
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

	TLOG(TLVL_INFO) << "Run " << evt.run() << ", subrun " << evt.subRun()
		<< ", event " << evt.event() << " has " << fragments.size()
		<< " fragment(s) of type TOY1 or TOY2";

	bool err = false;
	for (const auto& frag : fragments)
	{
		PruebaFragmento bb(frag);

		if (bb.hdr_event_size() * sizeof(PruebaFragmento::Header::dato_t) != frag.dataSize() * sizeof(artdaq::RawDataType))
		{
			TLOG(TLVL_ERROR) << "Error: in run " << evt.run() << ", subrun " << evt.subRun() <<
				", event " << evt.event() << ", seqID " << frag.sequenceID() <<
				", fragID " << frag.fragmentID() << ": Size mismatch!" <<
				" ToyFragment Header reports size of " << bb.hdr_event_size() * sizeof(PruebaFragmento::Header::dato_t) << " bytes, Fragment report size of " << frag.dataSize() * sizeof(artdaq::RawDataType) << " bytes.";
			continue;
		}


		{
			auto adc_iter = bb.dataBeginADCs();
			PruebaFragmento::adc_t expected_adc = 1;

			for (; adc_iter != bb.dataEndADCs(); adc_iter++, expected_adc++)
			{
				//if (expected_adc > bb.adc_range(frag.metadata<PruebaFragment::Metadato>()->num_adc_bits)) expected_adc = 0;

				// ELF 7/10/18: Distribution type 2 is the monotonically-increasing one
				/*if (bb.hdr_distribution_type() == 2 && *adc_iter != expected_adc)
				{
					TLOG(TLVL_ERROR) << "Error: in run " << evt.run() << ", subrun " << evt.subRun() <<
						", event " << evt.event() << ", seqID " << frag.sequenceID() <<
						", fragID " << frag.fragmentID() << ": expected an ADC value of " << expected_adc <<
						", got " << *adc_iter;
					err = true;
					break;
				}*/

				// ELF 7/10/18: As of now, distribution types 3 and 4 are uninitialized, and can therefore produce out-of-range counts.
				/*if (bb.hdr_distribution_type() < 3 && *adc_iter > bb.adc_range(frag.metadata<ToyFragment::Metadata>()->num_adc_bits))
				{
					TLOG(TLVL_ERROR) << "Error: in run " << evt.run() << ", subrun " << evt.subRun() <<
						", event " << evt.event() << ", seqID " << frag.sequenceID() <<
						", fragID " << frag.fragmentID() << ": " << *adc_iter << " is out-of-range for this Fragment type";
					err = true;
					break;
				}*/
			}

		}
	}
	if (!err) {
		TLOG(TLVL_DEBUG) << "In run " << evt.run() << ", subrun " << evt.subRun() <<
			", event " << evt.event() << ", everything is fine";
	}
}

DEFINE_ART_MODULE(prueba::CheckIntegrity)
