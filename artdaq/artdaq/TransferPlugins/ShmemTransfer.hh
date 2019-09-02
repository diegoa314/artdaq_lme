#ifndef artdaq_TransferPlugins_ShmemTransfer_hh
#define artdaq_TransferPlugins_ShmemTransfer_hh

#include "fhiclcpp/fwd.h"

#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "artdaq-core/Core/SharedMemoryFragmentManager.hh"

namespace artdaq
{
	/**
	 * \brief A TransferInterface implementation plugin that transfers data using Shared Memory
	 */
	class ShmemTransfer : public TransferInterface
	{
	public:

		/**
		 * \brief ShmemTransfer Constructor
		 * \param pset ParameterSet used to configure ShmemTransfer
		 * \param role Role of this ShmemTransfer instance (kSend or kReceive)
		 *
		 * \verbatim
		 * ShmemTransfer accepts the following Parameters:
		 * "shm_key_offset" (Default: 0): Offset to add to shared memory key (hash of uniqueLabel)
		 * \endverbatim
		 * ShmemTransfer also requires all Parameters for configuring a TransferInterface
		 * Additionally, an offset can be added via the ARTDAQ_SHMEM_TRANSFER_OFFSET envrionment variable.
		 * Note that this variable, if used, MUST have the same value for all artdaq processes communicating
		 * via ShmemTransfer.
		 */
		ShmemTransfer(fhicl::ParameterSet const& pset, Role role);

		/**
		 * \brief ShmemTransfer Destructor
		 */
		virtual ~ShmemTransfer() noexcept;

		/**
		* \brief Receive a Fragment from Shared Memory
		* \param[out] fragment Received Fragment
		* \param receiveTimeout Timeout for receive, in microseconds
		* \return Rank of sender or RECV_TIMEOUT
		*/
		int receiveFragment(Fragment& fragment,
			size_t receiveTimeout) override;

		/**
		* \brief Receive a Fragment Header from the transport mechanism
		* \param[out] header Received Fragment Header
		* \param receiveTimeout Timeout for receive
		* \return The rank the Fragment was received from (should be source_rank), or RECV_TIMEOUT
		*/
		int receiveFragmentHeader(detail::RawFragmentHeader& header, size_t receiveTimeout) override;

		/**
		* \brief Receive the body of a Fragment to the given destination pointer
		* \param destination Pointer to memory region where Fragment data should be stored
		 * \param wordCount Number of words of Fragment data to receive
		* \return The rank the Fragment was received from (should be source_rank), or RECV_TIMEOUT
		*/
		int receiveFragmentData(RawDataType* destination, size_t wordCount) override;

		/**
		* \brief Transfer a Fragment to the destination. May not necessarily be reliable, but will not block longer than send_timeout_usec.
		* \param fragment Fragment to transfer
		* \param send_timeout_usec Timeout for send, in microseconds
		* \return CopyStatus detailing result of transfer
		*/
		CopyStatus transfer_fragment_min_blocking_mode(Fragment const& fragment, size_t send_timeout_usec) override;

		/**
		* \brief Transfer a Fragment to the destination. This should be reliable, if the underlying transport mechanism supports reliable sending
		* \param fragment Fragment to transfer
		* \return CopyStatus detailing result of copy
		*/
		CopyStatus transfer_fragment_reliable_mode(Fragment&& fragment) override;

		/**
		* \brief Determine whether the TransferInterface plugin is able to send/receive data
		* \return True if the TransferInterface plugin is currently able to send/receive data
		*/
		bool isRunning() override;
	private:
		CopyStatus sendFragment(Fragment&& fragment,
			size_t send_timeout_usec, bool reliable = false);

		std::unique_ptr<SharedMemoryFragmentManager> shm_manager_;

	};
}

#endif // artdaq_TransferPlugins/ShmemTransfer_hh
