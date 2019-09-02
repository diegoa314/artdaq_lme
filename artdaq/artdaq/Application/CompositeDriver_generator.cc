#define TRACE_NAME "CompositeDriver"

#include "artdaq/Application/CompositeDriver.hh"

#include "artdaq/Application/GeneratorMacros.hh"
#include "artdaq/Application/makeCommandableFragmentGenerator.hh"
#include "canvas/Utilities/Exception.h"
#include "cetlib_except/exception.h"
#include <boost/algorithm/string.hpp>

using fhicl::ParameterSet;

artdaq::CompositeDriver::CompositeDriver(ParameterSet const& ps):
																CommandableFragmentGenerator(ps)
{
	std::vector<ParameterSet> psetList =
		ps.get<std::vector<ParameterSet>>("generator_config_list");
	for (auto pset : psetList)
	{
		if (! makeChildGenerator_(pset))
		{
			throw cet::exception("CompositeDriver")
				  << "Unable to create child generator for PSet= \""
				  << pset.to_string() << "\"";
		}
	}

	std::string compositeMetricName;
	for (auto& generator : generator_list_)
	{
		if (compositeMetricName.length() > 0) { compositeMetricName.append(", "); }
		compositeMetricName.append(generator->metricsReportingInstanceName());
	}
	this->metricsReportingInstanceName(compositeMetricName);
}

artdaq::CompositeDriver::~CompositeDriver() noexcept
{
	// 15-Feb-2014, KAB - explicitly destruct the generators so that
	// we can control the order in which they are destructed
	size_t listSize = generator_list_.size();
	while (listSize > 0)
	{
		--listSize;
		try
		{
			generator_list_.resize(listSize);
		}
		catch (...)
		{
			TLOG(TLVL_ERROR)
				<< "Unknown exception when destructing the generator at index "
				<< (listSize + 1) ;
		}
	}
}

void artdaq::CompositeDriver::start()
{
	for (size_t idx = 0; idx < generator_active_list_.size(); ++idx)
	{
		generator_active_list_[idx] = true;
	}
	for (auto& generator : generator_list_)
	{
		generator->StartCmd(run_number(), timeout(), timestamp());
	}
}

void artdaq::CompositeDriver::stopNoMutex()
{
	std::vector<std::unique_ptr<CommandableFragmentGenerator>>::reverse_iterator riter;
	for (riter = generator_list_.rbegin(); riter != generator_list_.rend(); ++riter)
	{
		(*riter)->StopCmd(timeout(), timestamp());
	}
}

void artdaq::CompositeDriver::stop()
{
	// 02/17/16 ELF: This logic is now handled in stopNoMutex
	/*
	std::vector<std::unique_ptr<CommandableFragmentGenerator>>::reverse_iterator riter;
	for (riter = generator_list_.rbegin(); riter != generator_list_.rend(); ++riter) {
	  (*riter)->StopCmd( timeout(), timestamp() );
	}
	*/
}

void artdaq::CompositeDriver::pause()
{
	std::vector<std::unique_ptr<CommandableFragmentGenerator>>::reverse_iterator riter;
	for (riter = generator_list_.rbegin(); riter != generator_list_.rend(); ++riter)
	{
		(*riter)->PauseCmd(timeout(), timestamp());
	}
}

void artdaq::CompositeDriver::resume()
{
	for (size_t idx = 0; idx < generator_active_list_.size(); ++idx)
	{
		generator_active_list_[idx] = true;
	}
	for (auto& generator : generator_list_)
	{
		generator->ResumeCmd(timeout(), timestamp());
	}
}

std::vector<artdaq::Fragment::fragment_id_t> artdaq::CompositeDriver::fragmentIDs()
{
	std::vector<artdaq::Fragment::fragment_id_t> workList;
	for (size_t idx = 0; idx < generator_list_.size(); ++idx)
	{
		std::vector<artdaq::Fragment::fragment_id_t> tempList =
			generator_list_[idx]->fragmentIDs();
		workList.insert(workList.end(), tempList.begin(), tempList.end());
	}
	return workList;
}

bool artdaq::CompositeDriver::getNext_(artdaq::FragmentPtrs& frags)
{
	bool anyGeneratorIsActive = false;
	for (size_t idx = 0; idx < generator_list_.size(); ++idx)
	{
		if (generator_active_list_[idx])
		{
			bool status = generator_list_[idx]->getNext(frags);

			// 08-May-2015, KAB & JCF: if the generator getNext() method returns
			// false (which indicates that the data flow has stopped) *and* the
			// reason that it has stopped is because there was an exception that
			// wasn't handled by the experiment-specific FragmentGenerator class,
			// we throw an exception so that the process will move to the
			// InRunError state. We do our best to try to reproduce the original
			// exception message.
			if (! status && generator_list_[idx]->exception())
			{
				std::string reportString =
					generator_list_[idx]->ReportCmd("latest_exception");
				if (std::string::npos !=
					boost::algorithm::to_lower_copy(reportString).find("exception"))
				{
					throw cet::exception("CompositeDriver_generator")
						  << "The FragmentGenerator for "
						  << generator_list_[idx]->metricsReportingInstanceName()
						  << " threw an exception: " << reportString;
				}
				else
				{
					throw cet::exception("CompositeDriver_generator")
						  << "The FragmentGenerator for "
						  << generator_list_[idx]->metricsReportingInstanceName()
						  << " threw an exception.";
				}
			}
			generator_active_list_[idx] = status;
			if (status) { anyGeneratorIsActive = true; }
		}
	}
	return anyGeneratorIsActive;
}

bool artdaq::CompositeDriver::makeChildGenerator_(fhicl::ParameterSet const& pset)
{
	// pull out the relevant parts of the ParameterSet
	fhicl::ParameterSet daq_pset;
	try
	{
		daq_pset = pset.get<fhicl::ParameterSet>("daq");
	}
	catch (...)
	{
		TLOG(TLVL_ERROR)
			<< "Unable to find the DAQ parameters in the initialization "
			<< "ParameterSet: \"" + pset.to_string() + "\"." ;
		return false;
	}
	fhicl::ParameterSet fr_pset;
	try
	{
		fr_pset = daq_pset.get<fhicl::ParameterSet>("fragment_receiver");
	}
	catch (...)
	{
		TLOG(TLVL_ERROR)
			<< "Unable to find the fragment_receiver parameters in the DAQ "
			<< "initialization ParameterSet: \"" + daq_pset.to_string() + "\"." ;
		return false;
	}

	// create the requested FragmentGenerator
	std::string frag_gen_name = fr_pset.get<std::string>("generator", "");
	if (frag_gen_name.length() == 0)
	{
		TLOG(TLVL_ERROR)
			<< "No fragment generator (parameter name = \"generator\") was "
			<< "specified in the fragment_receiver ParameterSet.  The "
			<< "DAQ initialization PSet was \"" << daq_pset.to_string() << "\"." ;
		return false;
	}

	std::unique_ptr<artdaq::CommandableFragmentGenerator> tmp_gen_ptr;
	try
	{
		tmp_gen_ptr = artdaq::makeCommandableFragmentGenerator(frag_gen_name, fr_pset);
	}
	catch (art::Exception& excpt)
	{
		TLOG(TLVL_ERROR)
			<< "Exception creating a FragmentGenerator of type \""
			<< frag_gen_name << "\" with parameter set \"" << fr_pset.to_string()
			<< "\", exception = " << excpt ;
		return false;
	}
	catch (cet::exception& excpt)
	{
		TLOG(TLVL_ERROR)
			<< "Exception creating a FragmentGenerator of type \""
			<< frag_gen_name << "\" with parameter set \"" << fr_pset.to_string()
			<< "\", exception = " << excpt ;
		return false;
	}
	catch (...)
	{
		TLOG(TLVL_ERROR)
			<< "Unknown exception creating a FragmentGenerator of type \""
			<< frag_gen_name << "\" with parameter set \"" << fr_pset.to_string()
			<< "\"." ;
		return false;
	}

	std::unique_ptr<CommandableFragmentGenerator> generator_ptr;
	generator_ptr.reset(nullptr);
	try
	{
		CommandableFragmentGenerator* tmp_cmdablegen_bareptr =
			dynamic_cast<CommandableFragmentGenerator*>(tmp_gen_ptr.get());
		if (tmp_cmdablegen_bareptr)
		{
			tmp_gen_ptr.release();
			generator_ptr.reset(tmp_cmdablegen_bareptr);
		}
	}
	catch (...) {}
	if (! generator_ptr)
	{
		TLOG(TLVL_ERROR)
			<< "Error: The requested fragment generator type (" << frag_gen_name
			<< ") is not a CommandableFragmentGenerator, and only "
			<< "CommandableFragmentGenerators are currently supported." ;
		return false;
	}

	generator_list_.push_back(std::move(generator_ptr));
	generator_active_list_.push_back(true);
	return true;
}

DEFINE_ARTDAQ_COMMANDABLE_GENERATOR(artdaq::CompositeDriver)
