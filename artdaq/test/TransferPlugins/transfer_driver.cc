#define TRACE_NAME "transfer_driver"

#include "artdaq/DAQrate/TransferTest.hh"
#include "artdaq/DAQdata/Globals.hh"
#include "fhiclcpp/make_ParameterSet.h"


int main(int argc, char* argv[])
{
	artdaq::configureMessageFacility("transfer_driver");
	TLOG(TLVL_INFO) << "BEGIN" ;

	std::cout << "argc:" << argc << std::endl;
	for (int i = 0; i < argc; ++i)
	{
		std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
	}

	if (argc != 3)
	{
		std::cerr << argv[0] << " requires 2 arguments, " << argc - 1 << " provided" << std::endl;
		std::cerr << "Usage: " << argv[0] << " <my_rank> <fhicl_document>" << std::endl;
		return 1;
	}

	my_rank = atoi(argv[1]);

	cet::filepath_lookup lookup_policy("FHICL_FILE_PATH");
	fhicl::ParameterSet ps;

	auto fhicl = std::string(argv[2]);
	make_ParameterSet(fhicl, lookup_policy, ps);

	if (ps.has_key("partition_number")) artdaq::Globals::partition_number_ = ps.get<int>("partition_number");

	artdaq::TransferTest theTest(ps);
	theTest.runTest();

	TLOG(TLVL_INFO) << "END" ;
	return 0;
}
