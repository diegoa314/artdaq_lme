#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "artdaq/TransferPlugins/MakeTransferPlugin.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Utilities/ExceptionHandler.hh"
#include "cetlib/BasicPluginFactory.h"

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "fhiclcpp/ParameterSet.h"

#include <boost/tokenizer.hpp>

#include <sys/shm.h>
#include <memory>
#include <iostream>
#include <string>
#include <limits>
#include <sstream>

/**
 * \brief The artdaq namespace
 */
namespace artdaq
{
	/**
	 * \brief Demonstration TransferInterface plugin showing how to discard events
	 * Intended for use in the transfer_to_dispatcher case, NOT for primary data stream!
	 */
	class NthEventTransfer : public TransferInterface
	{
	public:
		/**
		 * \brief NthEventTransfer Constructor
		 * \param ps fhicl::ParameterSet used to configure TransferInterface. Contains "nth", the
		 * interval at which events will be transferred, and "physical_transfer_plugin", a table
		 * configuring the TransferInterface plugin used for those transfers
		 * \param role Either kSend or kReceive, see TransferInterface constructor
		 */
		NthEventTransfer(fhicl::ParameterSet const& ps, artdaq::TransferInterface::Role role);

		/**
		* \brief Transfer a Fragment to the destination. May not necessarily be reliable, but will not block longer than send_timeout_usec.
		* \param fragment Fragment to transfer
		* \param send_timeout_usec Timeout for send, in microseconds
		* \return CopyStatus detailing result of transfer
		*/
		TransferInterface::CopyStatus
			transfer_fragment_min_blocking_mode(artdaq::Fragment const& fragment, size_t send_timeout_usec) override;

		/**
		 * \brief Copy a fragment, using the reliable channel. moveFragment assumes ownership of the fragment
		 * \param fragment Fragment to copy
		 * \return CopyStatus (either kSuccess, kTimeout, kErrorNotRequiringException or an exception)
		 */
		TransferInterface::CopyStatus
			transfer_fragment_reliable_mode(artdaq::Fragment&& fragment) override;

		/**
		 * \brief Receive a fragment from the transfer plugin
		 * \param fragment Reference to output Fragment object
		 * \param receiveTimeout Timeout before aborting receive
		 * \return Rank of sender or RECV_TIMEOUT
		 */
		int receiveFragment(artdaq::Fragment& fragment,
			size_t receiveTimeout) override
		{
			// nth-event discarding is done at the send side. Pass receive calls through to underlying transfer
			return physical_transfer_->receiveFragment(fragment, receiveTimeout);
		}

		/**
		* \brief Receive a Fragment Header from the transport mechanism
		* \param[out] header Received Fragment Header
		* \param receiveTimeout Timeout for receive
		* \return The rank the Fragment was received from (should be source_rank), or RECV_TIMEOUT
		*/
		int receiveFragmentHeader(detail::RawFragmentHeader& header, size_t receiveTimeout) override
		{
			return physical_transfer_->receiveFragmentHeader(header, receiveTimeout);
		}

		/**
		* \brief Receive the body of a Fragment to the given destination pointer
		* \param destination Pointer to memory region where Fragment data should be stored
		* \param wordCount Number of words of Fragment data to receive
		* \return The rank the Fragment was received from (should be source_rank), or RECV_TIMEOUT
		*/
		int receiveFragmentData(RawDataType* destination, size_t wordCount) override
		{
			return physical_transfer_->receiveFragmentData(destination, wordCount);
		}

		/**
	   * \brief Get the source rank from the physical transfer
	   * \return The source rank from the physical transfer
	   */
		int source_rank() const override { return physical_transfer_->source_rank(); }

		/**
	   * \brief Get the destination rank from the physical transfer
	   * \return The destination rank from the physical transfer
	   */
		int destination_rank() const override { return physical_transfer_->destination_rank(); }


		/**
		* \brief Determine whether the TransferInterface plugin is able to send/receive data
		* \return True if the TransferInterface plugin is currently able to send/receive data
		*/
		bool isRunning() override { return physical_transfer_->isRunning(); }

	private:

		bool pass(const artdaq::Fragment&) const;

		std::unique_ptr<TransferInterface> physical_transfer_;
		size_t nth_;
		size_t offset_;
	};

	NthEventTransfer::NthEventTransfer(fhicl::ParameterSet const& pset, artdaq::TransferInterface::Role role) :
		TransferInterface(pset, role)
		, nth_(pset.get<size_t>("nth")),
		offset_(pset.get<size_t>("offset", 0))
	{
		if (pset.has_key("source_rank") || pset.has_key("destination_rank")) {
			throw cet::exception("NthEvent") << "The parameters \"source_rank\" and \"destination_rank\" must be explicitly defined in the body of the physical_transfer_plugin table, and not outside of it";
		}


		if (offset_ >= nth_) {
			throw cet::exception("NthEvent") << "Offset value of " << offset_ <<
				" must not be larger than the modulus value of " << nth_;
		}

		if (nth_ == 0)
		{
			mf::LogWarning("NthEventTransfer") << "0 was passed as the nth parameter to NthEventTransfer. Will change to 1 (0 is undefined behavior)";
			nth_ = 1;
		}
		// Instantiate the TransferInterface plugin used to effect transfers
		physical_transfer_ = MakeTransferPlugin(pset, "physical_transfer_plugin", role);
	}


	TransferInterface::CopyStatus
		NthEventTransfer::transfer_fragment_min_blocking_mode(artdaq::Fragment const& fragment,
			size_t send_timeout_usec)
	{

		if (!pass(fragment))
		{
			// Do not transfer but return success. Fragment is discarded
			return TransferInterface::CopyStatus::kSuccess;
		}

		// This is the nth Fragment, transfer
		return physical_transfer_->transfer_fragment_min_blocking_mode(fragment, send_timeout_usec);
	}

	TransferInterface::CopyStatus
		NthEventTransfer::transfer_fragment_reliable_mode(artdaq::Fragment&& fragment)
	{
		if (!pass(fragment))
		{
			// Do not transfer but return success. Fragment is discarded
			return TransferInterface::CopyStatus::kSuccess;
		}

		// This is the nth Fragment, transfer
		return physical_transfer_->transfer_fragment_reliable_mode(std::move(fragment));
	}

	bool
		NthEventTransfer::pass(const artdaq::Fragment& fragment) const
	{
		bool passed = false;

		if (fragment.type() == artdaq::Fragment::DataFragmentType) {
			passed = (fragment.sequenceID() + nth_ - offset_) % nth_ == 0 ? true : false;
		}
		else {
			passed = true;
		}

		return passed;
	}
}

DEFINE_ARTDAQ_TRANSFER(artdaq::NthEventTransfer)

// Local Variables:
// mode: c++
// End:
