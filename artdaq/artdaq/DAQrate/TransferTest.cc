#include "artdaq/DAQrate/TransferTest.hh"

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq/DAQrate/FragmentReceiverManager.hh"
#include "artdaq/DAQrate/DataSenderManager.hh"

#define TRACE_NAME "TransferTest"
#include "artdaq/DAQdata/Globals.hh"

#include "fhiclcpp/make_ParameterSet.h"

#include <future>

artdaq::TransferTest::TransferTest(fhicl::ParameterSet psi)
	: senders_(psi.get<int>("num_senders"))
	, receivers_(psi.get<int>("num_receivers"))
	, sending_threads_(psi.get<int>("sending_threads", 1))
	, sends_each_sender_(psi.get<int>("sends_per_sender"))
	, receives_each_receiver_(0)
	, buffer_count_(psi.get<int>("buffer_count", 10))
	, error_count_max_(psi.get<int>("max_errors_before_abort", 3))
	, fragment_size_(psi.get<size_t>("fragment_size", 0x100000))
	, ps_()
	, validate_mode_(psi.get<bool>("validate_data_mode", false))
	, partition_number_(psi.get<int>("partition_number", rand() % 0x7F))
{
	TLOG(10) << "CONSTRUCTOR";

	if (fragment_size_ < artdaq::detail::RawFragmentHeader::num_words() * sizeof(artdaq::RawDataType))
	{
		fragment_size_ = artdaq::detail::RawFragmentHeader::num_words() * sizeof(artdaq::RawDataType);
	}

	fhicl::ParameterSet metric_pset;

	try
	{
		metric_pset = psi.get<fhicl::ParameterSet>("metrics");
	}
	catch (...) {} // OK if there's no metrics table defined in the FHiCL                                    

	try
	{
		std::string name = "TransferTest" + std::to_string(my_rank);
		metricMan->initialize(metric_pset, name);
		metricMan->do_start();
	}
	catch (...) {}

	std::string type(psi.get<std::string>("transfer_plugin_type", "Shmem"));

	bool broadcast_mode = psi.get<bool>("broadcast_sends", false);
	if (broadcast_mode)
	{
		receives_each_receiver_ = senders_ * sending_threads_ * sends_each_sender_;
	}
	else
	{
		if (receivers_ > 0)
		{
			if (senders_ * sending_threads_ * sends_each_sender_ % receivers_ != 0)
			{
				TLOG(TLVL_TRACE) << "Adding sends so that sends_each_sender * num_sending_ranks is a multiple of num_receiving_ranks" << std::endl;
				while (senders_ * sends_each_sender_ % receivers_ != 0)
				{
					sends_each_sender_++;
				}
				receives_each_receiver_ = senders_ * sending_threads_ * sends_each_sender_ / receivers_;
				TLOG(TLVL_TRACE) << "sends_each_sender is now " << sends_each_sender_ << std::endl;
				psi.put_or_replace("sends_per_sender", sends_each_sender_);
			}
			else
			{
				receives_each_receiver_ = senders_ * sending_threads_ * sends_each_sender_ / receivers_;
			}
		}
	}

	std::string hostmap = "";
	if (psi.has_key("hostmap"))
	{
		hostmap = " host_map: @local::hostmap";
	}

	std::stringstream ss;
	ss << psi.to_string() << std::endl;

	ss << " sources: {";
	for (int ii = 0; ii < senders_; ++ii)
	{
		ss << "s" << ii << ": { transferPluginType: " << type << " source_rank: " << ii << " max_fragment_size_words : " << fragment_size_ << " buffer_count : " << buffer_count_ << " partition_number : " << partition_number_ << hostmap << " }" << std::endl;
	}
	ss << "}" << std::endl << " destinations: {";
	for (int jj = senders_; jj < senders_ + receivers_; ++jj)
	{
		ss << "d" << jj << ": { transferPluginType: " << type << " destination_rank: " << jj << " max_fragment_size_words : " << fragment_size_ << " buffer_count : " << buffer_count_ << " partition_number : " << partition_number_ << hostmap << " }" << std::endl;
	}
	ss << "}" << std::endl;

	make_ParameterSet(ss.str(), ps_);


	TLOG(TLVL_DEBUG) << "Going to configure with ParameterSet: " << ps_.to_string() << std::endl;
}

int artdaq::TransferTest::runTest()
{
	TLOG(TLVL_INFO) << "runTest BEGIN: " << (my_rank < senders_ ? "sending" : "receiving");
	start_time_ = std::chrono::steady_clock::now();
	std::pair<size_t, double> result;
	if (my_rank >= senders_ + receivers_) return 0;
	if (my_rank < senders_)
	{
		std::vector<std::future<std::pair<size_t, double>>> results_futures(sending_threads_);
		for (int ii = 0; ii < sending_threads_; ++ii)
		{
			results_futures[ii] = std::async(std::bind(&TransferTest::do_sending, this, ii));
		}
		for (auto& future : results_futures)
		{
			if (future.valid())
			{
				auto thisresult = future.get();
				result.first += thisresult.first;
				result.second += thisresult.second;
			}
		}
	}
	else
	{
		result = do_receiving();
	}
	auto duration = std::chrono::duration_cast<artdaq::TimeUtils::seconds>(std::chrono::steady_clock::now() - start_time_).count();
	TLOG(TLVL_INFO) << (my_rank < senders_ ? "Sent " : "Received ") << result.first << " bytes in " << duration << " seconds ( " << formatBytes(result.first / duration) << "/s )." << std::endl;
	TLOG(TLVL_INFO) << "Rate of " << (my_rank < senders_ ? "sending" : "receiving") << ": " << formatBytes(result.first / result.second) << "/s." << std::endl;
	metricMan->do_stop();
	metricMan->shutdown();
	TLOG(11) << "runTest DONE";
	return 0;
}

std::pair<size_t, double> artdaq::TransferTest::do_sending(int index)
{
	TLOG(7) << "do_sending entered RawFragmentHeader::num_words()=" << artdaq::detail::RawFragmentHeader::num_words();

	size_t totalSize = 0;
	double totalTime = 0;
	artdaq::DataSenderManager sender(ps_);

	unsigned data_size_wrds = (fragment_size_ / sizeof(artdaq::RawDataType)) - artdaq::detail::RawFragmentHeader::num_words();
	artdaq::Fragment frag(data_size_wrds);

	if (validate_mode_)
	{
		artdaq::RawDataType gen_seed = 0;

		std::generate_n(frag.dataBegin(), data_size_wrds, [&]() { return ++gen_seed; });
		for (size_t ii = 0; ii < frag.dataSize(); ++ii)
		{
			if (*(frag.dataBegin() + ii) != ii + 1)
			{
				TLOG(TLVL_ERROR) << "Data corruption detected! (" << (*(frag.dataBegin() + ii)) << " != " << (ii + 1) << ") Aborting!";
				exit(1);
			}
		}
	}

	int metric_send_interval = sends_each_sender_ / 1000 > 1 ? sends_each_sender_ / 1000 : 1;
	auto init_time_metric = 0.0;
	auto send_time_metric = 0.0;
	auto after_time_metric = 0.0;
	auto send_size_metric = 0.0;
	auto error_count = 0;

	for (int ii = 0; ii < sends_each_sender_; ++ii)
	{
		auto loop_start = std::chrono::steady_clock::now();
		TLOG(7) << "sender rank " << my_rank << " #" << ii << " resized bytes=" << frag.sizeBytes();
		totalSize += frag.sizeBytes();

		//unsigned sndDatSz = data_size_wrds;
		frag.setSequenceID(ii * sending_threads_ + index);
		frag.setFragmentID(my_rank);
		frag.setSystemType(artdaq::Fragment::DataFragmentType);
		/*
				artdaq::Fragment::iterator it = frag.dataBegin();
				*it = my_rank;
				*++it = ii;
				*++it = sndDatSz;*/

		auto send_start = std::chrono::steady_clock::now();
		TLOG(TLVL_DEBUG) << "Sender " << my_rank << " sending fragment " << ii;
		auto stspair = sender.sendFragment(std::move(frag));
		auto after_send = std::chrono::steady_clock::now();
		TLOG(TLVL_TRACE) << "Sender " << my_rank << " sent fragment " << ii;
		//usleep( (data_size_wrds*sizeof(artdaq::RawDataType))/233 );

		if (stspair.second != artdaq::TransferInterface::CopyStatus::kSuccess)
		{
			error_count++;
			if (error_count >= error_count_max_)
			{
				TLOG(TLVL_ERROR) << "Too many errors sending fragments! Aborting... (sent=" << ii << "/" << sends_each_sender_ << ")";
				exit(sends_each_sender_ - ii);
			}
		}

		frag = artdaq::Fragment(data_size_wrds); // replace/renew
		if (validate_mode_)
		{
			artdaq::RawDataType gen_seed = ii + 1;

			std::generate_n(frag.dataBegin(), data_size_wrds, [&]() { return ++gen_seed; });
			for (size_t jj = 0; jj < frag.dataSize(); ++jj)
			{
				if (*(frag.dataBegin() + jj) != (ii + 1) + jj + 1)
				{
					TLOG(TLVL_ERROR) << "Input Data corruption detected! (" << *(frag.dataBegin() + jj) << " != " << ii + jj + 2 << " at position " << ii << ") Aborting!";
					exit(1);
				}
			}
		}
		TLOG(9) << "sender rank " << my_rank << " frag replaced";

		auto total_send_time = std::chrono::duration_cast<artdaq::TimeUtils::seconds>(after_send - send_start).count();
		totalTime += total_send_time;
		send_time_metric += total_send_time;
		send_size_metric += data_size_wrds * sizeof(artdaq::RawDataType);
		after_time_metric += std::chrono::duration_cast<artdaq::TimeUtils::seconds>(std::chrono::steady_clock::now() - after_send).count();
		init_time_metric += std::chrono::duration_cast<artdaq::TimeUtils::seconds>(send_start - loop_start).count();

		if (metricMan && ii % metric_send_interval == 0)
		{
			metricMan->sendMetric("send_init_time", init_time_metric, "seconds", 3, MetricMode::Accumulate);
			metricMan->sendMetric("total_send_time", send_time_metric, "seconds", 3, MetricMode::Accumulate);
			metricMan->sendMetric("after_send_time", after_time_metric, "seconds", 3, MetricMode::Accumulate);
			metricMan->sendMetric("send_rate", send_size_metric / send_time_metric, "B/s", 3, MetricMode::Average);
			init_time_metric = 0.0;
			send_time_metric = 0.0;
			after_time_metric = 0.0;
			send_size_metric = 0.0;
		}
		usleep(0); // Yield execution
	}

	return std::make_pair(totalSize, totalTime);
} // do_sending

std::pair<size_t, double> artdaq::TransferTest::do_receiving()
{
	TLOG(7) << "do_receiving entered";

	artdaq::FragmentReceiverManager receiver(ps_);
	receiver.start_threads();
	int counter = receives_each_receiver_;
	size_t totalSize = 0;
	double totalTime = 0;
	bool first = true;
	bool nonblocking_mode = ps_.get<bool>("nonblocking_sends", false);
	std::atomic<int> activeSenders(senders_ * sending_threads_);
	auto end_loop = std::chrono::steady_clock::now();

	auto recv_size_metric = 0.0;
	auto recv_time_metric = 0.0;
	auto input_wait_metric = 0.0;
	auto init_wait_metric = 0.0;
	int metric_send_interval = receives_each_receiver_ / 1000 > 1 ? receives_each_receiver_ : 1;

	// Only abort when there are no senders if were's > 90% done
	while ((activeSenders > 0 || (counter > receives_each_receiver_ / 10 && !nonblocking_mode)) && counter > 0)
	{
		auto start_loop = std::chrono::steady_clock::now();
		TLOG(7) << "do_receiving: Counter is " << counter << ", calling recvFragment (activeSenders=" << activeSenders << ")";
		int senderSlot = artdaq::TransferInterface::RECV_TIMEOUT;
		auto before_receive = std::chrono::steady_clock::now();

		auto ignoreFragPtr = receiver.recvFragment(senderSlot);
		auto after_receive = std::chrono::steady_clock::now();
		init_wait_metric += std::chrono::duration_cast<artdaq::TimeUtils::seconds>(before_receive - start_loop).count();
		size_t thisSize = 0;
		if (senderSlot >= artdaq::TransferInterface::RECV_SUCCESS && ignoreFragPtr)
		{
			if (ignoreFragPtr->type() == artdaq::Fragment::EndOfDataFragmentType)
			{
				TLOG(TLVL_INFO) << "Receiver " << my_rank << " received EndOfData Fragment from Sender " << senderSlot;
				activeSenders--;
				TLOG(TLVL_DEBUG) << "Active Senders is now " << activeSenders;
			}
			else if (ignoreFragPtr->type() != artdaq::Fragment::DataFragmentType)
			{
				TLOG(TLVL_WARNING) << "Receiver " << my_rank << " received Fragment with System type " << artdaq::detail::RawFragmentHeader::SystemTypeToString(ignoreFragPtr->type()) << " (Unexpected!)";
			}
			else
			{
				if (first)
				{
					start_time_ = std::chrono::steady_clock::now();
					first = false;
				}
				counter--;
				TLOG(TLVL_INFO) << "Receiver " << my_rank << " received fragment " << receives_each_receiver_ - counter
					<< " with seqID " << ignoreFragPtr->sequenceID() << " from Sender " << senderSlot << " (Expecting " << counter << " more)";
				thisSize = ignoreFragPtr->size() * sizeof(artdaq::RawDataType);
				totalSize += thisSize;
				if (validate_mode_)
				{
					for (size_t ii = 0; ii < ignoreFragPtr->dataSize(); ++ii)
					{
						if (*(ignoreFragPtr->dataBegin() + ii) != ignoreFragPtr->sequenceID() + ii + 1)
						{
							TLOG(TLVL_ERROR) << "Output Data corruption detected! (" << *(ignoreFragPtr->dataBegin() + ii) << " != " << (ignoreFragPtr->sequenceID() + ii + 1) << " at position " << ii << ") Aborting!";
							exit(1);
						}
					}
				}
			}
			input_wait_metric += std::chrono::duration_cast<artdaq::TimeUtils::seconds>(after_receive - end_loop).count();
		}
		else if (senderSlot == artdaq::TransferInterface::DATA_END)
		{
			TLOG(TLVL_ERROR) << "Receiver " << my_rank << " detected fatal protocol error! Reducing active sender count by one!" << std::endl;
			activeSenders--;
			TLOG(TLVL_DEBUG) << "Active Senders is now " << activeSenders;
		}
		TLOG(7) << "do_receiving: Recv Loop end, counter is " << counter;


		auto total_recv_time = std::chrono::duration_cast<artdaq::TimeUtils::seconds>(after_receive - before_receive).count();
		recv_time_metric += total_recv_time;
		totalTime += total_recv_time;
		recv_size_metric += thisSize;

		if (metricMan && counter % metric_send_interval == 0)
		{
			metricMan->sendMetric("input_wait", input_wait_metric, "seconds", 3, MetricMode::Accumulate);
			metricMan->sendMetric("recv_init_time", init_wait_metric, "seconds", 3, MetricMode::Accumulate);
			metricMan->sendMetric("total_recv_time", recv_time_metric, "seconds", 3, MetricMode::Accumulate);
			metricMan->sendMetric("recv_rate", recv_size_metric / recv_time_metric, "B/s", 3, MetricMode::Average);

			input_wait_metric = 0.0;
			init_wait_metric = 0.0;
			recv_time_metric = 0.0;
			recv_size_metric = 0.0;
		}
		end_loop = std::chrono::steady_clock::now();
	}

	if (counter != 0 && !nonblocking_mode)
	{
		TLOG(TLVL_ERROR) << "Did not receive all expected Fragments! Missing " << counter << " Fragments!";
		exit(counter);
	}

	return std::make_pair(totalSize, totalTime);
}
