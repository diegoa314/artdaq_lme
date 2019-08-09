#ifndef ARTDAQ_TEST_DAQRATE_TRANSFERTEST_HH
#define ARTDAQ_TEST_DAQRATE_TRANSFERTEST_HH

#include <vector>
#include <string>
#include <chrono>
#include <cmath>

#include "fhiclcpp/ParameterSet.h"
#include "artdaq-utilities/Plugins/MetricManager.hh"

namespace artdaq
{
	/**
	 * \brief Test a set of TransferInterface plugins
	 */
	class TransferTest
	{
	public:
		/**
		 * \brief TransferTest Constructor
		 * \param psi ParameterSet used to configure TransferTest
		 * 
		 * \verbatim
		 * TransferTest accepts the following Parameters:
		 * "num_senders" (REQUIRED): Number of sending TransferTest instances
		 * "num_receivers" (REQUIRED): Number of receiving TransferTest instances
		 * "sends_per_sender" (REQUIRED): Number of sends each sender will perform
		 * "sending_threads" (Default: 1): Number of TransferInterface instances to send fragments from for each source rank
		 * "buffer_count" (Default: 10): Buffer count for TransferInterfaces
		 * "fragment_size" (Default: 0x100000): Size of Fragments to transfer
		 * "metrics": FHiCL table used to configure MetricManager (see documentation)
		 * "transfer_plugin_type" (Default: Shmem): TransferInterface plugin to load
		 * "hostmap" (OPTIONAL): Host map to use for "host_map" parameter of TransferInterface plugins (i.e. TCPSocketTransfer)
		 * \endverbatim
		 */
		explicit TransferTest(fhicl::ParameterSet psi);

		/**
		 * \brief Run the test as configured
		 * \return 0 upon success
		 */
		int runTest();

	private:
		std::pair<size_t, double> do_sending(int thread_index);

		std::pair<size_t, double> do_receiving();

		//Helper functions
		const std::vector<std::string> suffixes{" B", " KB", " MB", " GB", " TB"};

		std::string formatBytes(double bytes, size_t suffixIndex = 0);

		int senders_;
		int receivers_;
		int sending_threads_;
		int sends_each_sender_;
		int receives_each_receiver_; // Should be sends_each_sender * sending_threads * sending_ranks / receiving_ranks
		int buffer_count_;
		int error_count_max_;
		size_t fragment_size_;
		std::chrono::steady_clock::time_point start_time_;
		fhicl::ParameterSet ps_;
		bool validate_mode_;
		int partition_number_;
	};

	inline std::string TransferTest::formatBytes(double bytes, size_t suffixIndex)
	{
		auto b = fabs(bytes);

		if (b > 1024.0 && suffixIndex < suffixes.size())
		{
			return formatBytes(bytes / 1024.0, suffixIndex + 1);
		}

		return std::to_string(bytes) + suffixes[suffixIndex];
	}
}
#endif //ARTDAQ_TEST_DAQRATE_TRANSFERTEST_HH
