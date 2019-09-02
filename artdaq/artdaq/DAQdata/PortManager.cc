#include "artdaq/DAQdata/PortManager.hh"
#define TRACE_NAME "PortManager"
#include "artdaq/DAQdata/Globals.hh"
#include "artdaq/DAQdata/TCPConnect.hh"
#include <sstream>

artdaq::PortManager::PortManager()
	: base_configured_(false)
	, multicasts_configured_(false)
	, routing_tokens_configured_(false)
	, routing_acks_configured_(false)
	, xmlrpc_configured_(false)
	, tcpsocket_configured_(false)
	, request_port_configured_(false)
	, request_pattern_configured_(false)
	, routing_table_port_configured_(false)
	, routing_table_pattern_configured_(false)
	, multicast_transfer_port_configued_(false)
	, multicast_transfer_pattern_configured_(false)
	, base_port_(DEFAULT_BASE)
	, ports_per_partition_(DEFAULT_PORTS_PER_PARTITION)
	, multicast_interface_address_()
	, multicast_group_offset_(DEFAULT_MULTICAST_GROUP_OFFSET)
	, routing_token_offset_(DEFAULT_ROUTING_TOKEN_OFFSET)
	, routing_ack_offset_(DEFAULT_ROUTING_TABLE_ACK_OFFSET)
	, xmlrpc_offset_(DEFAULT_XMLRPC_OFFSET)
	, tcp_socket_offset_(DEFAULT_TCPSOCKET_OFFSET)
	, request_message_port_(DEFAULT_REQUEST_PORT)
	, routing_table_port_(DEFAULT_ROUTING_TABLE_PORT)
	, multicast_transfer_offset_(1024)
	, request_message_group_pattern_("227.128.PPP.SSS")
	, routing_table_group_pattern_("227.129.PPP.SSS")
	, multicast_transfer_group_pattern_("227.130.14.PPP")
{
}

void artdaq::PortManager::UpdateConfiguration(fhicl::ParameterSet const& ps)
{
	if (ps.has_key("artdaq_base_port"))
	{
		auto newVal = ps.get<int>("artdaq_base_port");
		if (base_port_ != DEFAULT_BASE && base_port_ != newVal)
		{
			TLOG(TLVL_WARNING) << "Base port has changed! This may lead to misconfiguration and non-functional systems!";
		}
		base_port_ = newVal;
	}
	if (ps.has_key("ports_per_partition"))
	{
		auto newVal = ps.get<int>("ports_per_partition");
		if (ports_per_partition_ != DEFAULT_PORTS_PER_PARTITION && ports_per_partition_ != newVal)
		{
			TLOG(TLVL_WARNING) << "Ports per Partition has changed! This may lead to misconfiguration and non-functional systems!";
		}
		ports_per_partition_ = newVal;
	}

	auto bp = getenv("ARTDAQ_BASE_PORT"); //Environment overrides configuration
	if (bp != nullptr)
	{
		try
		{
			auto bp_s = std::string(bp);
			auto bp_tmp = std::stoi(bp_s, 0, 0);
			if (bp_tmp < 1024 || bp_tmp > 32000)
			{
				TLOG(TLVL_ERROR) << "Base port specified in ARTDAQ_BASE_PORT is invalid! Ignoring...";
			}
			else
			{
				base_port_ = bp_tmp;
			}
		}
		catch (std::invalid_argument) {}
		catch (std::out_of_range) {}
	}

	auto ppp = getenv("ARTDAQ_PORTS_PER_PARTITION"); //Environment overrides configuration
	if (ppp != nullptr)
	{
		try
		{
			auto ppp_s = std::string(ppp);
			auto ppp_tmp = std::stoi(ppp_s, 0, 0);
			if (ppp_tmp < 0 || ppp_tmp > 32000)
			{
				TLOG(TLVL_ERROR) << "Ports per partition specified in ARTDAQ_PORTS_PER_PARTITION is invalid! Ignoring...";
			}
			else
			{
				ports_per_partition_ = ppp_tmp;
			}
		}
		catch (std::invalid_argument) {}
		catch (std::out_of_range) {}
	}

	if (!base_configured_ && (base_port_ != DEFAULT_BASE || ports_per_partition_ != DEFAULT_PORTS_PER_PARTITION))
	{
		base_configured_ = true;
		auto max_partitions = (65535 - base_port_) / ports_per_partition_;
		TLOG(TLVL_INFO) << "Based on configuration, there can be " << max_partitions << " partitions of " << ports_per_partition_ << " ports each, starting at port " << base_port_;
		if (GetPartitionNumber() > max_partitions)
		{
			TLOG(TLVL_ERROR) << "Currently-configured partition number is greater than the allowed number! The system WILL NOT WORK!";
			exit(22);
		}
	}

	in_addr tmp_addr;
	bool multicast_configured = false;
	if (ps.has_key("multicast_output_interface"))
	{
		multicast_configured = GetIPOfInterface(ps.get<std::string>("multicast_output_interface"), tmp_addr) == 0;
	}
	else if (ps.has_key("multicast_output_network"))
	{
		multicast_configured = GetInterfaceForNetwork(ps.get<std::string>("multicast_output_network").c_str(), tmp_addr) == 0;
	}
	if (multicast_configured && multicasts_configured_ && tmp_addr.s_addr != multicast_interface_address_.s_addr)
	{
		TLOG(TLVL_WARNING) << "Multicast output address has changed! This may lead to misconfiguration and non-functional systems!";
	}
	else if (multicast_configured)
	{
		multicasts_configured_ = true;
		multicast_interface_address_ = tmp_addr;
	}

	if (ps.has_key("multicast_group_offset"))
	{
		auto newVal = ps.get<int>("multicast_group_offset");
		if (multicast_group_offset_ != DEFAULT_MULTICAST_GROUP_OFFSET && multicast_group_offset_ != newVal)
		{
			TLOG(TLVL_WARNING) << "Multicast group offset (added to last part of group IP address) has changed! This may lead to misconfiguration and non-functional systems!";
		}
		multicast_group_offset_ = newVal;
	}

	if (ps.has_key("routing_token_port_offset"))
	{
		auto newVal = ps.get<int>("routing_token_port_offset");
		if (routing_tokens_configured_ && newVal != routing_token_offset_)
		{
			TLOG(TLVL_WARNING) << "Routing Token Port Offset has changed! This may lead to misconfiguration and non-functional systems!";
		}

		routing_tokens_configured_ = true;
		routing_token_offset_ = newVal;
	}

	if (ps.has_key("routing_table_ack_port_offset"))
	{
		auto newVal = ps.get<int>("routing_table_ack_port_offset");
		if (routing_acks_configured_ && newVal != routing_ack_offset_)
		{
			TLOG(TLVL_WARNING) << "Routing Table Acknowledgement Port Offset has changed! This may lead to misconfiguration and non-functional systems!";
		}

		routing_acks_configured_ = true;
		routing_ack_offset_ = newVal;
	}

	if (ps.has_key("xmlrpc_port_offset"))
	{

		auto newVal = ps.get<int>("xmlrpc_port_offset");
		if (xmlrpc_configured_ && newVal != xmlrpc_offset_)
		{
			TLOG(TLVL_WARNING) << "XMLRPC Port Offset has changed! This may lead to misconfiguration and non-functional systems!";
		}

		xmlrpc_configured_ = true;
		xmlrpc_offset_ = newVal;
	}

	if (ps.has_key("tcp_socket_port_offset"))
	{
		auto newVal = ps.get<int>("tcp_socket_port_offset");
		if (tcpsocket_configured_ && newVal != tcp_socket_offset_)
		{
			TLOG(TLVL_WARNING) << "TCPSocketTransfer Port Offset has changed! This may lead to misconfiguration and non-functional systems!";
		}

		tcpsocket_configured_ = true;
		tcp_socket_offset_ = newVal;
	}

	if (ps.has_key("request_port"))
	{
		auto newVal = ps.get<int>("request_port");
		if (request_port_configured_ && newVal != request_message_port_)
		{
			TLOG(TLVL_WARNING) << "Request Message Port has changed! This may lead to misconfiguration and non-functional systems!";
		}

		request_port_configured_ = true;
		request_message_port_ = newVal;

	}

	if (ps.has_key("request_pattern"))
	{
		auto newVal = ps.get<std::string>("request_pattern");
		if (request_pattern_configured_ && newVal != request_message_group_pattern_)
		{
			TLOG(TLVL_WARNING) << "Request Message Multicast Group Pattern has changed! This may lead to misconfiguration and non-functional systems!";
		}

		request_pattern_configured_ = true;
		request_message_group_pattern_ = newVal;

	}

	if (ps.has_key("routing_table_port"))
	{
		auto newVal = ps.get<int>("routing_table_port");
		if (routing_table_port_configured_ && newVal != routing_table_port_)
		{
			TLOG(TLVL_WARNING) << "Routing Table Port has changed! This may lead to misconfiguration and non-functional systems!";
		}

		routing_table_port_configured_ = true;
		routing_table_port_ = newVal;

	}

	if (ps.has_key("routing_table_pattern"))
	{
		auto newVal = ps.get<std::string>("routing_table_pattern");
		if (routing_table_pattern_configured_ && newVal != routing_table_group_pattern_)
		{
			TLOG(TLVL_WARNING) << "Routing Table Multicast Group Pattern has changed! This may lead to misconfiguration and non-functional systems!";
		}

		routing_table_pattern_configured_ = true;
		routing_table_group_pattern_ = newVal;

	}

	if (ps.has_key("multicast_transfer_port_offset"))
	{
		auto newVal = ps.get<int>("multicast_transfer_port_offset");
		if (multicast_transfer_port_configued_ && newVal != multicast_transfer_offset_)
		{
			TLOG(TLVL_WARNING) << "Multicast Transfer Port Offset has changed! This may lead to misconfiguration and non-functional systems!";
		}

		multicast_transfer_port_configued_ = true;
		multicast_transfer_offset_ = newVal;

	}

	if (ps.has_key("multicast_transfer_pattern"))
	{
		auto newVal = ps.get<std::string>("multicast_transfer_pattern");
		if (multicast_transfer_pattern_configured_ && newVal != multicast_transfer_group_pattern_)
		{
			TLOG(TLVL_WARNING) << "Multicast Transfer Multicast Group Pattern has changed! This may lead to misconfiguration and non-functional systems!";
		}

		multicast_transfer_pattern_configured_ = true;
		multicast_transfer_group_pattern_ = newVal;

	}
}

int artdaq::PortManager::GetRoutingTokenPort(int subsystemID)
{
	if (!routing_tokens_configured_)
	{
		TLOG(TLVL_INFO) << "Using default port range for Routing Tokens";
		routing_tokens_configured_ = true;
	}

	return base_port_ + routing_token_offset_ + (GetPartitionNumber() * ports_per_partition_) + subsystemID;
}

int artdaq::PortManager::GetRoutingAckPort(int subsystemID)
{
	if (!routing_acks_configured_)
	{
		TLOG(TLVL_INFO) << "Using default port range for Routing Table Acknowledgements";
		routing_acks_configured_ = true;
	}
	return base_port_ + routing_ack_offset_ + (GetPartitionNumber() * ports_per_partition_) + subsystemID;
}

int artdaq::PortManager::GetXMLRPCPort(int rank)
{
	if (!xmlrpc_configured_)
	{
		TLOG(TLVL_INFO) << "Using default port range for XMLRPC";
		xmlrpc_configured_ = true;
	}
	return base_port_ + xmlrpc_offset_ + rank + (GetPartitionNumber() * ports_per_partition_);
}

int artdaq::PortManager::GetTCPSocketTransferPort(int rank)
{
	if (!tcpsocket_configured_)
	{
		TLOG(TLVL_INFO) << "Using default port range for TCPSocket Transfer";
		tcpsocket_configured_ = true;
	}
	return base_port_ + tcp_socket_offset_ + (GetPartitionNumber() * ports_per_partition_) + rank;
}

int artdaq::PortManager::GetRequestMessagePort()
{
	if (!request_port_configured_)
	{
		TLOG(TLVL_INFO) << "Using default port for Request Messages";
		request_port_configured_ = true;
	}
	return request_message_port_;

}

std::string artdaq::PortManager::GetRequestMessageGroupAddress(int subsystemID)
{
	if (!request_pattern_configured_)
	{
		TLOG(TLVL_INFO) << "Using default address for Request Messages";
		request_pattern_configured_ = true;
	}

	return  parse_pattern_(request_message_group_pattern_, subsystemID);
}

int artdaq::PortManager::GetRoutingTablePort()
{
	if (!routing_table_port_configured_)
	{
		TLOG(TLVL_INFO) << "Using default port for Routing Tables";
		routing_table_port_configured_ = true;
	}

	return routing_table_port_;
}

std::string artdaq::PortManager::GetRoutingTableGroupAddress(int subsystemID)
{
	if (!routing_table_pattern_configured_)
	{
		TLOG(TLVL_INFO) << "Using default address for Routing Tables";
		routing_table_pattern_configured_ = true;
	}

	return parse_pattern_(routing_table_group_pattern_, subsystemID);
}

int artdaq::PortManager::GetMulticastTransferPort(int rank)
{
	if (!multicast_transfer_port_configued_)
	{
		TLOG(TLVL_INFO) << "Using default port for Multicast Transfer";
		multicast_transfer_port_configued_ = true;
	}

	return multicast_transfer_offset_ + rank;
}

std::string artdaq::PortManager::GetMulticastTransferGroupAddress()
{
	if (!multicast_transfer_pattern_configured_)
	{
		TLOG(TLVL_INFO) << "Using default address for Multicast Transfer";
		multicast_transfer_pattern_configured_ = true;
	}

	return parse_pattern_(multicast_transfer_group_pattern_);
}

in_addr artdaq::PortManager::GetMulticastOutputAddress(std::string interface_name, std::string interface_address)
{
	if (!multicasts_configured_)
	{
		if (interface_name == "" && interface_address == "")	TLOG(TLVL_INFO) << "Using default multicast output address (autodetected private interface)";
		if (interface_name != "")
		{
			GetIPOfInterface(interface_name, multicast_interface_address_);
		}
		else if (interface_address != "")
		{
			GetInterfaceForNetwork(interface_address.c_str(), multicast_interface_address_);
		}
		else
		{
			AutodetectPrivateInterface(multicast_interface_address_);
		}
		multicasts_configured_ = true;
	}
	return multicast_interface_address_;
}

std::string artdaq::PortManager::parse_pattern_(std::string pattern, int subsystemID, int rank)
{
	std::istringstream f(pattern);
	std::vector<int> address(4);
	std::string s;

	while (getline(f, s, '.'))
	{
		if (s == "PPP") {
			address.push_back(GetPartitionNumber());
		}
		else if (s == "SSS") {
			address.push_back(subsystemID);
		}
		else if (s == "RRR") {
			address.push_back(rank);
		}
		else
		{
			address.push_back(stoi(s));
		}
	}

	if (address.size() != 4)
	{
		TLOG(TLVL_ERROR) << "Invalid address pattern!";
		while (address.size() < 4) { address.push_back(0); }
	}
	address[3] += multicast_group_offset_;

	return std::to_string(address[0]) + "." + std::to_string(address[1]) + "." + std::to_string(address[2]) + "." + std::to_string(address[3]);
}
