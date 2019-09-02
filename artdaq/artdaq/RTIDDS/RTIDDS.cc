#define TRACE_NAME "RTIDDS"

#include "artdaq/RTIDDS/RTIDDS.hh"
#include "artdaq/DAQdata/Globals.hh"

#include <boost/tokenizer.hpp>

#include <iostream>


artdaq::RTIDDS::RTIDDS(std::string name, IOType iotype, std::string max_size) :
																			  name_(name)
																			  , iotype_(iotype)
																			  , max_size_(max_size)
{
	DDS_ReturnCode_t retcode = DDS_RETCODE_ERROR;
	DDS_DomainParticipantQos participant_qos;

	retcode = DDSDomainParticipantFactory::get_instance()->get_default_participant_qos(participant_qos);

	if (retcode != DDS_RETCODE_OK)
	{
		TLOG(TLVL_WARNING) << name_ << ": Problem obtaining default participant QoS, retcode was " << retcode ;
	}

	retcode = DDSPropertyQosPolicyHelper::add_property(
		participant_qos.property, "dds.builtin_type.octets.max_size",
		max_size_.c_str(),
		DDS_BOOLEAN_FALSE);

	if (retcode != DDS_RETCODE_OK)
	{
		TLOG(TLVL_WARNING) << name_ << ": Problem setting dds.builtin_type.octets.max_size, retcode was " << retcode ;
	}

	participant_.reset(DDSDomainParticipantFactory::get_instance()->
		create_participant(
			0, // Domain ID                                                
			participant_qos,
			nullptr, // Listener                                              
			DDS_STATUS_MASK_NONE)
	);

	topic_octets_ = participant_->create_topic(
		"artdaq fragments", // Topic name                                
		DDSOctetsTypeSupport::get_type_name(), // Type name                                     
		DDS_TOPIC_QOS_DEFAULT, // Topic QoS                                     
		nullptr, // Listener                                   
		DDS_STATUS_MASK_NONE);

	if (participant_ == nullptr || topic_octets_ == nullptr)
	{
		TLOG(TLVL_WARNING) << name_ << ": Problem setting up the RTI-DDS participant and/or topic" ;
	}

	// JCF, 9/16/15                                                                                                                    

	// Following effort to increase the max DDS buffer size from its                                                                   
	// default of 2048 bytes is cribbed from section 3.2.7 of the Core                                                                 
	// Utilities user manual, "Managing Memory for Built-in Types"                                                                     


	DDS_DataWriterQos writer_qos;

	retcode = participant_->get_default_datawriter_qos(writer_qos);

	if (retcode != DDS_RETCODE_OK)
	{
		TLOG(TLVL_WARNING) << name_ << ": Problem obtaining default datawriter QoS, retcode was " << retcode ;
	}

	retcode = DDSPropertyQosPolicyHelper::add_property(
		writer_qos.property, "dds.builtin_type.octets.alloc_size",
		max_size_.c_str(),
		DDS_BOOLEAN_FALSE);

	if (retcode != DDS_RETCODE_OK)
	{
		TLOG(TLVL_WARNING) << name_ << ": Problem setting dds.builtin_type.octets.alloc_size, retcode was " << retcode ;
	}


	if (iotype_ == IOType::writer)
	{
		octets_writer_ = DDSOctetsDataWriter::narrow(participant_->create_datawriter(
				topic_octets_,
				writer_qos,
				nullptr, // Listener   
				DDS_STATUS_MASK_NONE)
		);

		if (octets_writer_ == nullptr)
		{
			TLOG(TLVL_WARNING) << name_ << ": Problem setting up the RTI-DDS writer objects" ;
		}
	}
	else
	{
		octets_reader_ = participant_->create_datareader(
			topic_octets_,
			DDS_DATAREADER_QOS_DEFAULT, // QoS                                           
			&octets_listener_, // Listener                             
			DDS_DATA_AVAILABLE_STATUS);

		if (octets_reader_ == nullptr)
		{
			TLOG(TLVL_WARNING) << name_ << ": Problem setting up the RTI-DDS reader objects" ;
		}
	}
}

void artdaq::RTIDDS::transfer_fragment_reliable_mode_via_DDS_(artdaq::Fragment&& fragment) { transfer_fragment_min_blocking_mode_via_DDS_(fragment); }

void artdaq::RTIDDS::transfer_fragment_min_blocking_mode_via_DDS_(artdaq::Fragment const& fragment)
{
	// check if a fragment of this type has already been copied
	size_t fragmentType = fragment.type();

	if (octets_writer_ == NULL) { return; }

	// JCF, 9/15/15

	// Perform the same checks Kurt implemented in the AggregatorCore's
	// copyFragmentToSharedMemory_() function

	size_t max_fragment_size = boost::lexical_cast<size_t>(max_size_) -
							   detail::RawFragmentHeader::num_words() * sizeof(RawDataType);

	if (fragment.type() != artdaq::Fragment::InvalidFragmentType &&
		fragment.sizeBytes() < max_fragment_size)
	{
		if (fragment.type() == artdaq::Fragment::InitFragmentType)
		{
			usleep(500000);
		}

		DDS_ReturnCode_t retcode = octets_writer_->write(reinterpret_cast<unsigned char*>(fragment.headerBeginBytes()),
														 fragment.sizeBytes(),
														 DDS_HANDLE_NIL);

		if (retcode != DDS_RETCODE_OK)
		{
			TLOG(TLVL_WARNING) << name_ << ": Problem writing octets (bytes), retcode was " << retcode ;

			TLOG(TLVL_WARNING) << name_ << ": Fragment failed for DDS! "
				<< "fragment address and size = "
				<< static_cast<void*>(fragment.headerBeginBytes()) << " " << static_cast<int>(fragment.sizeBytes()) << " "
				<< "sequence ID, fragment ID, and type = "
				<< fragment.sequenceID() << " "
				<< fragment.fragmentID() << " "
				<< ((int) fragment.type()) ;
		}
	}
	else
	{
		TLOG(TLVL_WARNING) << name_ << ": Fragment invalid for DDS! "
			<< "fragment address and size = "
			<< static_cast<void*>(fragment.headerBeginBytes()) << " " << static_cast<int>(fragment.sizeBytes()) << " "
			<< "sequence ID, fragment ID, and type = "
			<< fragment.sequenceID() << " "
			<< fragment.fragmentID() << " "
			<< ((int) fragment.type()) ;
	}
}

void artdaq::RTIDDS::OctetsListener::on_data_available(DDSDataReader* reader)
{
	DDSOctetsDataReader* octets_reader = NULL;
	DDS_SampleInfo info;
	DDS_ReturnCode_t retcode;

	TLOG(TLVL_DEBUG) << name_ << ": In OctetsListener::on_data_available" ;


	octets_reader = DDSOctetsDataReader::narrow(reader);
	if (octets_reader == nullptr)
	{
		TLOG(TLVL_ERROR) << name_ << ": Error: Very unexpected - DDSOctetsDataReader::narrow failed" ;
		return;
	}

	// Loop until there are messages available in the queue 


	for (;;)
	{
		retcode = octets_reader->take_next_sample(
			dds_octets_,
			info);
		if (retcode == DDS_RETCODE_NO_DATA)
		{
			// No more samples 
			break;
		}
		else if (retcode != DDS_RETCODE_OK)
		{
			TLOG(TLVL_WARNING) << "Unable to take data from data reader, error "
				<< retcode ;
			return;
		}
		if (info.valid_data)
		{
			// JCF, 9/17/15

			// We want to make sure the dds_octets_ doesn't get popped from
			// by receiveFragmentFromDDS before we display its contents

			std::lock_guard<std::mutex> lock(queue_mutex_);

			dds_octets_queue_.push(dds_octets_);
		}
	}
}

// receiveFragmentFromDDS returns true (and fills fragment passed to
// it) in there's no timeout; if there is, returns false

bool artdaq::RTIDDS::OctetsListener::receiveFragmentFromDDS(artdaq::Fragment& fragment,
															const size_t receiveTimeout)
{
	int loopCount = 0;
	size_t sleepTime = 1000; // microseconds
	size_t nloops = receiveTimeout / sleepTime;

	while (dds_octets_queue_.empty() && loopCount < nloops)
	{
		usleep(sleepTime);
		++loopCount;
	}

	// JCF, 9/17/15

	// Make sure the on_data_available() callback function doesn't
	// modify dds_octets_queue_ while this section is running

	std::lock_guard<std::mutex> lock(queue_mutex_);

	if (!dds_octets_queue_.empty())
	{
		fragment.resizeBytes(dds_octets_queue_.front().length);
		memcpy(fragment.headerAddress(), dds_octets_queue_.front().value, dds_octets_queue_.front().length);

		dds_octets_queue_.pop();

		TLOG(TLVL_DEBUG) << name_
			<< ": Received fragment from DDS, type ="
			<< ((int)fragment.type()) << ", sequenceID = "
			<< fragment.sequenceID() << ", size in bytes = "
			<< fragment.sizeBytes()
			;

		return true;
	}

	return false;
}
