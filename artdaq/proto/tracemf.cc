/* This file (just.cc) was created by Ron Rechenmacher <ron@fnal.gov> on
// Feb 19, 2014. "TERMS AND CONDITIONS" governing this file are in the README
// or COPYING file. If you do not have such a file, one can be obtained by
// contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
// $RCSfile: just_user.cc,v $
*/

#include "artdaq/DAQdata/Globals.hh"
#include <boost/program_options.hpp>
#include <iomanip>
namespace bpo = boost::program_options;

std::string formatTime(double time)
{
	std::ostringstream o;
	o << std::fixed << std::setprecision(3);
	if (time > 60) o << static_cast<int>(time / 60) << " m " << time - 60 * static_cast<int>(time / 60) << " s";
	else if (time > 1) o << time << " s";
	else
	{
		time *= 1000;
		if (time > 1) o << time << " ms";
		else
		{
			time *= 1000;
			if (time > 1) o << time << " us";
			else
			{
				time *= 1000;
				o << time << " ns";
			}
		}
	}

	return o.str();
}

int main(int argc, char *argv[])
{
	std::ostringstream descstr;
	descstr << argv[0]
		<< " <-l test loops> [csutdi]";
	bpo::options_description desc(descstr.str());
	desc.add_options()
		("loops,l", bpo::value<size_t>(), "Number of times to run each test")
		("C,c", "Run TRACEC test")
		("S,s", "Run TRACES test")
		("U,u", "Run TRACE_ test")
		("T,t", "Run TRACE_STREAMER test")
		("D,d", "Run TLOG_DEBUG test")
		("I,i", "Run TLOG_INFO test")
		("console,x", "Enable MessageFacility output to console")
		("help,h", "produce help message");
	bpo::variables_map vm;
	try
	{
		bpo::store(bpo::command_line_parser(argc, argv).options(desc).run(), vm);
		bpo::notify(vm);
	}
	catch (bpo::error const & e)
	{
		std::cerr << "Exception from command line processing in " << argv[0]
			<< ": " << e.what() << "\n";
		return -1;
	}
	if (vm.count("help"))
	{
		std::cout << desc << std::endl;
		return 1;
	}
	if (!vm.count("loops"))
	{
		std::cerr << "Exception from command line processing in " << argv[0]
			<< ": no loop count given.\n"
			<< "For usage and an options list, please do '"
			<< argv[0] << " --help"
			<< "'.\n";
		return 2;
	}
	size_t loops = vm["loops"].as<size_t>();

	artdaq::configureMessageFacility("tracemf", vm.count("console"));

	if (vm.count("C"))
	{
		std::cout << "Starting TRACEC test" << std::endl;
		auto start = std::chrono::steady_clock::now();
		for (size_t l = 0; l < loops; ++l)
		{
			TRACE(TLVL_DEBUG, "Test TRACEC with an int %i and a float %.1f", 42, 5.56);
		}
		auto time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
		std::cout << "TRACEC test took " << formatTime(time) << ", avg: " << formatTime(time / loops) << std::endl;
	}

	if (vm.count("S"))
	{
		std::cout << "Starting TRACES test" << std::endl;
		auto start = std::chrono::steady_clock::now();
		for (size_t l = 0; l < loops; ++l)
		{
			std::string test = "Test TRACE with an int %d";
			std::string test2 = " and a float %.1f";
			TRACE(TLVL_DEBUG, test + test2, 42, 5.56);
		}
		auto time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
		std::cout << "TRACES test took " << formatTime(time) << ", avg: " << formatTime(time / loops) << std::endl;
	}

	if (vm.count("U"))
	{
		std::cout << "Starting TRACE_ test" << std::endl;
		auto start = std::chrono::steady_clock::now();
		for (size_t l = 0; l < loops; ++l)
		{
			TRACEN_( "tracemf", TLVL_DEBUG, "Test TRACE_ with an int " << 42 << " and a float %.1f", 5.56);
		}
		auto time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
		std::cout << "TRACE_ test took " << formatTime(time) << ", avg: " << formatTime(time / loops) << std::endl;
	}

	if (vm.count("D"))
	{
		std::cout << "Starting TLOG_DEBUG test" << std::endl;
		auto start = std::chrono::steady_clock::now();
		for (size_t l = 0; l < loops; ++l)
		{
			TLOG_DEBUG("tracemf") << "Test TLOG_DEBUG with an int " << 42 << " and a float " << std::setprecision(1) << 5.56 << ", and another float " << 7.3 << TRACE_ENDL;
		}
		auto time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
		std::cout << "TLOG_DEBUG test took " << formatTime(time) << ", avg: " << formatTime(time / loops) << std::endl;
	}

	if (vm.count("I"))
	{
		std::cout << "Starting TLOG_INFO test" << std::endl;
		auto start = std::chrono::steady_clock::now();
		for (size_t l = 0; l < loops; ++l)
		{
			TLOG_INFO("tracemf") << "Test TLOG_INFO with an int " << 42 << " and a float " << std::setprecision(1) << 5.56 << ", and another float " << std::fixed << 7.3 << TRACE_ENDL;
		}
		auto time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start).count();
		std::cout << "TLOG_INFO test took " << formatTime(time) << ", avg: " << formatTime(time / loops) << std::endl;
	}

	return (0);
}   /* main */
