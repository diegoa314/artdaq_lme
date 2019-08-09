#include "artdaq/TransferPlugins/MakeTransferPlugin.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"

#include "cetlib/BasicPluginFactory.h"
#include "fhiclcpp/ParameterSet.h"

#include <sstream>

namespace artdaq
{
	std::unique_ptr<artdaq::TransferInterface>
	MakeTransferPlugin(const fhicl::ParameterSet& pset,
	                   std::string plugin_label,
	                   TransferInterface::Role role)
	{
		static cet::BasicPluginFactory bpf("transfer", "make");

		fhicl::ParameterSet transfer_pset;

		try
		{
			transfer_pset = pset.get<fhicl::ParameterSet>(plugin_label);
		}
		catch (...)
		{
			std::stringstream errmsg;
			errmsg
				<< "Error in artdaq::MakeTransferPlugin: Unable to find the transfer plugin parameters in the FHiCL code \"" << transfer_pset.to_string()
				<< "\"; FHiCL table with label \"" << plugin_label
				<< "\" may not exist, or if it does, one or more parameters may be missing.";
			ExceptionHandler(ExceptionHandlerRethrow::yes, errmsg.str());
		}

		try
		{
			auto transfer =
				bpf.makePlugin<std::unique_ptr<TransferInterface>,
				               const fhicl::ParameterSet&,
				               TransferInterface::Role>(
					transfer_pset.get<std::string>("transferPluginType"),
					transfer_pset,
					std::move(role));

			return transfer;
		}
		catch (...)
		{
			std::stringstream errmsg;
			errmsg
				<< "Unable to create transfer plugin using the FHiCL parameters \""
				<< transfer_pset.to_string()
				<< "\"";
			ExceptionHandler(ExceptionHandlerRethrow::yes, errmsg.str());
		}

		return nullptr;
	}
}
