#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "fhiclcpp/ParameterSet.h"
#include "artdaq-core/Data/PackageBuildInfo.hh"

#include <iostream>


namespace artdaq
{
	/**
	 * \brief BuildInfo is an art::EDProducer which saves information about package builds to the data file
	 * \tparam instanceName Tag which the BuildInfo objects will be saved under
	 * \tparam Pkgs List of package BuildInfo types
	 */
	template <std::string* instanceName, typename... Pkgs>
	class BuildInfo : public art::EDProducer
	{
	public:
		/**
		 * \brief BuildInfo module Constructor
		 * \param p ParameterSet used to configure BuildInfo module
		 * 
		 * BuildInfo_module expects the following Parameters:
		 * "instance_name": Name which the BuildInfo information will be saved under
		 */
		explicit BuildInfo(fhicl::ParameterSet const& p);

		/**
		 * \brief Default Destructor
		 */
		virtual ~BuildInfo() = default;

		/**
		 * \brief Perform actions at the beginning of the Run
		 * \param r art::Run object
		 * 
		 * The BuildInfo information is stored in the Run-level provenance, so this
		 * method performs most of the "work" for this module.
		 */
		void beginRun(art::Run& r) override;

		/**
		 * \brief Perform actions for each event
		 * \param e art::Event object
		 * 
		 * This function is a required override for EDProducer, and is a No-Op in BuildInfo_module.
		 */
		void produce(art::Event& e) override;

	private:

		std::unique_ptr<std::vector<PackageBuildInfo>> packages_;
		std::string instanceName_;

		template <typename... Args>
		struct fill_packages;

		template <typename Arg>
		struct fill_packages<Arg>
		{
			static void doit(std::vector<PackageBuildInfo>& packages)
			{
				packages.emplace_back(Arg::getPackageBuildInfo());
			}
		};

		template <typename Arg, typename... Args>
		struct fill_packages<Arg, Args...>
		{
			static void doit(std::vector<PackageBuildInfo>& packages)
			{
				packages.emplace_back(Arg::getPackageBuildInfo());
				fill_packages<Args...>::doit(packages);
			}
		};
	};

	template <std::string* instanceName, typename... Pkgs>
	BuildInfo<instanceName, Pkgs...>::BuildInfo(fhicl::ParameterSet const& ps):
	                                                                          packages_(new std::vector<PackageBuildInfo>())
	                                                                          , instanceName_(ps.get<std::string>("instance_name", *instanceName))
	{
		fill_packages<Pkgs...>::doit(*packages_);

		produces<std::vector<PackageBuildInfo>, art::InRun>(instanceName_);
	}

	template <std::string* instanceName, typename... Pkgs>
	void BuildInfo<instanceName, Pkgs...>::beginRun(art::Run& e)
	{
		// JCF, 9/22/14                                                                                                                     

		// Previously, the vector pointed to by the member variable                                                                       
		// packages_ itself got stored in output on the call to "e.put()"                                                                   
		// below; what would then happen is that at the start of a new run                                                                  
		// or subrun, when e.put() got called again an exception would be                                                                   
		// thrown because packages_ would now be null thanks to the                                                                         
		// previous call to std::move. To make sure this doesn't happen, I                                                                  
		// now stash a copy of the vector pointed to by packages_, not the                                                                  
		// original member vector                                                                                                           

		auto packages_deep_copy_ptr = std::unique_ptr<std::vector<PackageBuildInfo>>(
			new std::vector<PackageBuildInfo>(*packages_));

		e.put(std::move(packages_deep_copy_ptr), instanceName_);
	}

	template <std::string* instanceName, typename... Pkgs>
	void BuildInfo<instanceName, Pkgs...>::produce(art::Event&)
	{
		// nothing to be done for individual events
	}
}
