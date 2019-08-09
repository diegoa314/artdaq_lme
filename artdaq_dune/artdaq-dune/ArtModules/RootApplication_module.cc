////////////////////////////////////////////////////////////////////////
// Class:       RootApplication
// Module Type: analyzer
// File:        RootApplication_module.cc
//
// Generated at Sun Dec  2 12:23:06 2012 by Alessandro Razeto & Nicola Rossi using artmod
// from art v1_02_04.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Core/ModuleMacros.h"

#include <iostream>
#include <future>
#include <TApplication.h>
#include <TSystem.h>
#include <unistd.h>

namespace demo
{
	/**
	 * \brief Provides a wrapper for displaying ROOT canvases
	 */
	class RootApplication : public art::EDAnalyzer
	{
	public:
		/**
		 * \brief RootApplication Constructor
		 * \param p ParameterSet for configuring RootApplication
		 * 
		 * RootApplication accepts the following Paramters:
		 * "force_new" (Default: true): Always create a new window
		 * "dont_quit" (Default: false): Keep window open after art exits
		 */
		explicit RootApplication(fhicl::ParameterSet const& p);

		/**
		 * \brief RootApplication Destructor
		 */
		virtual ~RootApplication();

		/**
		 * \brief Called by art at the beginning of the job. RootApplication will create a window unless one already exists and force_new == false.
		 */
		void beginJob() override;

		/**
		 * \brief Called by art for each event
		 * \param e The art::Event object
		 * 
		 * RootApplication checks for ROOT system events, it does not touch the art::Event
		 */
		void analyze(art::Event const& e) override;

		/**
		 * \brief Called by art at the end of the job. RootApplication will close the findow if dont_quit == false.
		 */
		void endJob() override;

	private:
		std::unique_ptr<TApplication> app_;
		bool force_new_;
		bool dont_quit_;
	};
}

demo::RootApplication::RootApplication(fhicl::ParameterSet const& ps): art::EDAnalyzer(ps)
                                                                     , force_new_(ps.get<bool>("force_new", true))
                                                                     , dont_quit_(ps.get<bool>("dont_quit", false)) {}

demo::RootApplication::~RootApplication() { }

void demo::RootApplication::analyze(art::Event const&)
{
	gSystem->ProcessEvents();
}

void demo::RootApplication::beginJob()
{
	if (!gApplication || force_new_)
	{
		int tmp_argc(0);
		app_ = std::unique_ptr<TApplication>(new TApplication("noapplication", &tmp_argc, 0));
	}
}

void demo::RootApplication::endJob()
{
	if (dont_quit_) app_->Run(true);
}

DEFINE_ART_MODULE(demo::RootApplication)
