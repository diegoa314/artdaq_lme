////////////////////////////////////////////////////////////////////////
// Class:       MissingDataCheck
// Module Type: analyzer
// File:        MissingDataCheck_module.cc
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
#include <set>
#include <iostream>

#include "art/Framework/Services/Optional/TFileService.h" 
#include "TTree.h"

namespace artdaq
{
  class MissingDataCheck;
}

/**
 * \brief Check art::Event for potentially missing data
 */
class artdaq::MissingDataCheck : public art::EDAnalyzer
{
public:
  /**
   * \brief MissingDataCheck Constructor
   * \param pset ParameterSet used to configure MissingDataCheck
   * 
   * \verbatim
   * MissingDataCheck accepts the following Parameters:
   * "raw_data_label" (Default: "daq"): The label used to store artdaq data
   * "expected_n_fragments" (Default: -1): number of expected fragments. Uses n_frags in first event if -1
   * "verbosity" (Default: 0): verboseness level
   * \endverbatim
   */
  explicit MissingDataCheck(fhicl::ParameterSet const& pset);
  
  /**
   * \brief Default virtual Destructor
   */
  virtual ~MissingDataCheck() = default;
  
  /**
   * \brief This method is called for each art::Event in a file or run
   * \param e The art::Event to analyze
   * 
   */
  void analyze(art::Event const& e) override;

  void endJob() override;
  
private:
  std::string raw_data_label_;
  int         expected_n_fragments_;
  int         verbosity_;
  bool        make_tree_;

  TTree* evtree_;
  TTree* fragtree_;
  unsigned int run_;
  unsigned int subrun_;
  unsigned int event_;
  unsigned int timeHigh_;
  unsigned int timeLow_;

  unsigned int total_n_frags_;
  unsigned int total_data_size_;
  unsigned int total_n_CFs_;
  unsigned int total_n_CFs_missing_;
  unsigned int total_n_frags_in_CFs_;
  unsigned int total_n_frags_broken_;


  unsigned int frag_id_;
  unsigned int seq_id_;
  unsigned int frag_type_;
  int contained_frags_;
  unsigned int frag_data_size_;
  int missing_data_;
  
};


artdaq::MissingDataCheck::MissingDataCheck(fhicl::ParameterSet const& pset)
  : EDAnalyzer(pset)
  , raw_data_label_(pset.get<std::string>("raw_data_label", "daq"))
  , expected_n_fragments_(pset.get<int>("expected_n_fragments",-1))
  , verbosity_(pset.get<int>("verbosity",0))
  , make_tree_(pset.get<bool>("make_tree",true))
{
  if(make_tree_){
    art::ServiceHandle<art::TFileService> tfs;
    evtree_ = tfs->make<TTree>("evtree","MissingDataCheck Event Tree");
    evtree_->Branch("run",&run_,"run/i");
    evtree_->Branch("subrun",&subrun_,"subrun/i");
    evtree_->Branch("event",&event_,"event/i");
    evtree_->Branch("timeHigh",&timeHigh_,"timeHigh/i");
    evtree_->Branch("timeLow",&timeLow_,"timeLow/i");
    
    evtree_->Branch("n_frag_exp",&expected_n_fragments_,"nfrag_exp/I");
    evtree_->Branch("n_frag",&total_n_frags_,"n_frag/i");
    evtree_->Branch("data_size",&total_data_size_,"data_size/i");
    evtree_->Branch("n_cont_frag",&total_n_CFs_,"n_cont_frag/i");
    evtree_->Branch("n_miss_data",&total_n_CFs_missing_,"n_miss_data/i");
    evtree_->Branch("n_frag_in_cont",&total_n_frags_in_CFs_,"n_frag_in_cont/i");
    evtree_->Branch("n_frag_broken",&total_n_frags_broken_,"n_frag_broken/i");

    fragtree_ = tfs->make<TTree>("fragtree","MissingDataCheck Fragment Tree");
    fragtree_->Branch("run",&run_,"run/i");
    fragtree_->Branch("subrun",&subrun_,"subrun/i");
    fragtree_->Branch("event",&event_,"event/i");
    fragtree_->Branch("timeHigh",&timeHigh_,"timeHigh/i");
    fragtree_->Branch("timeLow",&timeLow_,"timeLow/i");

    fragtree_->Branch("frag_id",&frag_id_,"frag_id/i");
    fragtree_->Branch("seq_id",&seq_id_,"seq_id/i");
    fragtree_->Branch("frag_type",&frag_type_,"frag_type/i");
    fragtree_->Branch("frag_data_size",&frag_data_size_,"frag_data_size/i");
    fragtree_->Branch("contained_frags",&contained_frags_,"contained_frags/I");
    fragtree_->Branch("missing_data",&missing_data_,"missing_data/I");
  }
}


void artdaq::MissingDataCheck::analyze(art::Event const& e)
{
  run_ = e.run();
  subrun_ = e.subRun();
  event_ = e.event();
  timeHigh_ = e.time().timeHigh();
  timeLow_ = e.time().timeLow();

  //print basic run info
  if(verbosity_>2)
    std::cout << "Processing:" 
	      << "  Run " << e.run() 
	      << ", Subrun " << e.subRun() 
	      << ", Event " << e.event()
	      << " (Time=" << e.time().timeHigh() << " " << e.time().timeLow() << ")" 
	      << std::endl;

  //get all the artdaq fragment collections in the event.
  std::vector< art::Handle< std::vector<artdaq::Fragment> > > fragmentHandles;
  e.getManyByType(fragmentHandles);

  //print basic fragment number info
  if(verbosity_>2){
    std::cout << "\tFound " << fragmentHandles.size() << " fragment collections." << std::endl;
    if(verbosity_>2){
      for(auto const& h : fragmentHandles)
	std::cout << "\t\tCollection " << h.provenance()->productInstanceName()
		  << ":\t" << h->size() << " fragments." << std::endl;
    }
  }

  //count total fragments
  total_n_frags_=0;
  total_data_size_=0;
  for(auto const& h : fragmentHandles){
    total_n_frags_ += h->size();
    for(auto const& f : *h)
      total_data_size_ += f.dataSizeBytes();
  }

  //first time through, if this is -1, set it to total fragments seen
  if(expected_n_fragments_==-1)
    expected_n_fragments_=total_n_frags_;

  if(verbosity_>2){
    std::cout << "\tTotal fragments = " << total_n_frags_ 
	      << " / " << expected_n_fragments_ << std::endl;
    std::cout << "\tTotal data size in fragments = " << total_data_size_ << std::endl;

  }
  //count number of container fragments
  total_n_CFs_=0;
  total_n_CFs_missing_=0;
  total_n_frags_in_CFs_=0;
  total_n_frags_broken_=0;
  for(auto const& h : fragmentHandles){
    std::string instance_name = h.provenance()->productInstanceName();
    
    if(instance_name.compare(0,9,"Container")==0){
      total_n_CFs_ += h->size();

      for(auto const& f : *h){
	artdaq::ContainerFragment cf(f);
	if(cf.missing_data()){
	  ++total_n_CFs_missing_;
	}
	total_n_frags_in_CFs_ += cf.block_count();
	frag_id_ = f.fragmentID();
	seq_id_ = f.sequenceID();
	frag_type_ = f.type();
	contained_frags_ = cf.block_count();
	missing_data_ = cf.missing_data()?1:0;
	frag_data_size_ = f.dataSizeBytes();
	fragtree_->Fill();
      }

    }

    else{
      for(auto const& f : *h){

	if(instance_name.compare(0,4,"Data")==0 || 
	   instance_name.compare(0,5,"Error")==0 || 
	   instance_name.compare(0,6,"Broken")==0)
	  ++total_n_frags_broken_;

	frag_id_ = f.fragmentID();
	seq_id_ = f.sequenceID();
	frag_type_ = f.type();
	contained_frags_ = -1;
	missing_data_ = -1;
	frag_data_size_ = f.dataSizeBytes();
	fragtree_->Fill();
      }

    }

  }
  
  if(verbosity_>2){
    std::cout << "\tTotal container fragments = " << total_n_CFs_
	      << " of which " << total_n_CFs_missing_ << " have missing data."
	      << std::endl;
    std::cout << "\tTotal fragments in containers = " << total_n_frags_in_CFs_ << std::endl;
  }

  if(make_tree_){
    evtree_->Fill();
  }

}

void artdaq::MissingDataCheck::endJob()
{
  if(verbosity_>0){
    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << "----------- MISSING DATA CHECK SUMMARY ------------" << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;

    std::cout << "Total events processed: " << evtree_->GetEntries()
	      << ", expected fragments: " << expected_n_fragments_ << std::endl;

    unsigned int run;    evtree_->SetBranchAddress("run",&run);
    unsigned int subrun; evtree_->SetBranchAddress("subrun",&subrun);
    unsigned int event;  evtree_->SetBranchAddress("event",&event);
    int n_frag_exp; evtree_->SetBranchAddress("n_frag_exp",&n_frag_exp);
    unsigned int n_frag; evtree_->SetBranchAddress("n_frag",&n_frag);
    unsigned int n_frag_broken; evtree_->SetBranchAddress("n_frag_broken",&n_frag_broken);
    unsigned int n_miss_data; evtree_->SetBranchAddress("n_miss_data",&n_miss_data);

    std::cout << "Events missing fragments:\t\t" 
	      << evtree_->GetEntries("n_frag<n_frag_exp") 
	      << " / " << evtree_->GetEntries() << std::endl;
    if(verbosity_>1){
      if(evtree_->GetEntries("n_frag<n_frag_exp")>0){
        for(int i=0; i<evtree_->GetEntries(); ++i)
	{
	  evtree_->GetEvent(i);
	  if((int)n_frag<n_frag_exp){
	    std::cout << "\tEvent (" << run << "," << subrun << "," << event << ")"
		      << " is missing " << n_frag_exp-n_frag << " fragments."
		      << std::endl;
	  }
	}
      }
    }
    std::cout << "Events with broken fragments:\t\t" 
	      << evtree_->GetEntries("n_frag_broken>0") 
	      << " / " << evtree_->GetEntries() << std::endl;
    if(evtree_->GetEntries("n_frag_broken>0")>0){
      for(int i=0; i<evtree_->GetEntries(); ++i)
	{
	  evtree_->GetEvent(i);
	  if(n_frag_broken > 0){
	    std::cout << "\tEvent (" << run << "," << subrun << "," << event << ")"
		      << " has " << n_frag_broken << " fragments."
		      << std::endl;
	  }
	}
    }
    std::cout << "Events missing data in fragments:\t"
	      << evtree_->GetEntries("n_miss_data>0")
	      << " / " << evtree_->GetEntries() << std::endl;
    if(verbosity_>1){
      if(evtree_->GetEntries("n_miss_data>0")>0){
	for(int i=0; i<evtree_->GetEntries(); ++i)
	{
	  evtree_->GetEvent(i);
	  if(n_miss_data>0){
	    std::cout << "\tEvent (" << run << "," << subrun << "," << event << ")"
		      << " has " << n_miss_data << " fragments missing data."
		      << std::endl;
	  }
	}
      }
    }

    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;
  }
}

DEFINE_ART_MODULE(artdaq::MissingDataCheck)
