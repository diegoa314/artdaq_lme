#include "artdaq/DAQdata/Globals.hh"

int artdaq::Globals::my_rank_ = -1;
std::unique_ptr<artdaq::MetricManager> artdaq::Globals::metricMan_ = std::make_unique<artdaq::MetricManager>();
std::unique_ptr<artdaq::PortManager> artdaq::Globals::portMan_ = std::make_unique<artdaq::PortManager>();
std::string artdaq::Globals::app_name_ = "";
int artdaq::Globals::partition_number_ = -1;

std::mutex artdaq::Globals::mftrace_mutex_;
std::string artdaq::Globals::mftrace_iteration_ = "Booted";
std::string artdaq::Globals::mftrace_module_ = "DAQ";
