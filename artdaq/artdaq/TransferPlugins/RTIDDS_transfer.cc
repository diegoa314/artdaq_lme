#include "artdaq/TransferPlugins/TransferInterface.hh"

#include "artdaq/RTIDDS/RTIDDS.hh"

#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include <memory>
#include <iostream>


namespace fhicl
{
	class ParameterSet;
}

// ----------------------------------------------------------------------

namespace artdaq
{
	/**
	* \brief RTIDDSTransfer is a TransferInterface implementation plugin that transfers data using RTI DDS
	*/
	class RTIDDSTransfer : public TransferInterface
	{
	public:
		/**
		 * \brief RTIDDSTransfer default Destructor
		 */
		virtual ~RTIDDSTransfer() = default;

		/**
		* \brief RTIDDSTransfer Constructor
		* \param ps ParameterSet used to configure RTIDDSTransfer
		* \param role Role of this RTIDDSTransfer instance (kSend or kReceive)
		*
		* RTIDDSTransfer only requires the Parameters for configuring a TransferInterface
		*/
		RTIDDSTransfer(fhicl::ParameterSet const& ps, Role role) :
																 TransferInterface(ps, role)
																 , rtidds_reader_(std::make_unique<artdaq::RTIDDS>("RTIDDSTransfer_reader", artdaq::RTIDDS::IOType::reader))
																 , rtidds_writer_(std::make_unique<artdaq::RTIDDS>("RTIDDSTransfer_writer", artdaq::RTIDDS::IOType::writer)) { }

		/**
		* \brief Receive a Fragment using DDS
		* \param[out] fragment Received Fragment
		* \param receiveTimeout Timeout for receive, in microseconds
		* \return Rank of sender or RECV_TIMEOUT
		*/
		int receiveFragment(artdaq::Fragment& fragment,
									   size_t receiveTimeout) override;

		/**
		* \brief Transfer a Fragment to the destination. May not necessarily be reliable, but will not block longer than send_timeout_usec.
		* \param fragment Fragment to transfer
		* \param send_timeout_usec Timeout for send, in microseconds
		* \return CopyStatus detailing result of transfer
		*/
		CopyStatus transfer_fragment_min_blocking_mode(artdaq::Fragment const& fragment,
										size_t send_timeout_usec = std::numeric_limits<size_t>::max()) override;

		/**
		* \brief Transfer a Fragment to the destination. This should be reliable, if the underlying transport mechanism supports reliable sending
		* \param fragment Fragment to transfer
		* \return CopyStatus detailing result of copy
		*/
		CopyStatus transfer_fragment_reliable_mode(artdaq::Fragment&& fragment) override;

		/**
		* \brief Determine whether the TransferInterface plugin is able to send/receive data
		* \return True if the TransferInterface plugin is currently able to send/receive data
		*/
		bool isRunning() override
		{
			switch (role())
			{
			case TransferInterface::Role::kSend:
				return rtidds_writer_ != nullptr;
			case TransferInterface::Role::kReceive:
				return rtidds_reader_ != nullptr;
			}
		}
	private:
		std::unique_ptr<artdaq::RTIDDS> rtidds_reader_;
		std::unique_ptr<artdaq::RTIDDS> rtidds_writer_;
	};
}

int artdaq::RTIDDSTransfer::receiveFragment(artdaq::Fragment& fragment,
											   size_t receiveTimeout)
{
	bool receivedFragment = false;
	//  static std::size_t consecutive_timeouts = 0;
	//  std::size_t message_after_N_timeouts = 10;

	//  while (!receivedFragment) {

	try
	{
		receivedFragment = rtidds_reader_->octets_listener_.receiveFragmentFromDDS(fragment, receiveTimeout);
	}
	catch (...)
	{
		ExceptionHandler(ExceptionHandlerRethrow::yes,
						 "Error in RTIDDS transfer plugin: caught exception in call to OctetsListener::receiveFragmentFromDDS, rethrowing");
	}

	//    if (!receivedFragment) {

	//      consecutive_timeouts++;

	//      if (consecutive_timeouts % message_after_N_timeouts == 0) {
	//	TLOG_INFO("RTIDDSTransfer") << consecutive_timeouts << " consecutive " << 
	//	  static_cast<float>(receiveTimeout)/1e6 << "-second timeouts calling OctetsListener::receiveFragmentFromDDS, will continue trying..." ;
	//      }
	//    } else {
	//      consecutive_timeouts = 0;
	//    }
	//  }

	//  return 0;

	return receivedFragment ? source_rank() : TransferInterface::RECV_TIMEOUT;
}

artdaq::TransferInterface::CopyStatus
artdaq::RTIDDSTransfer::transfer_fragment_reliable_mode(artdaq::Fragment&& fragment)
{
	rtidds_writer_->transfer_fragment_reliable_mode_via_DDS_(std::move(fragment));
	return CopyStatus::kSuccess;
}

artdaq::TransferInterface::CopyStatus
artdaq::RTIDDSTransfer::transfer_fragment_min_blocking_mode(artdaq::Fragment const& fragment,
									 size_t send_timeout_usec)
{
	(void) &send_timeout_usec; // No-op to get the compiler not to complain about unused parameter

	rtidds_writer_->transfer_fragment_min_blocking_mode_via_DDS_(fragment);
	return CopyStatus::kSuccess;
}

DEFINE_ARTDAQ_TRANSFER(artdaq::RTIDDSTransfer)

// Local Variables:
// mode: c++
// End:
