#ifndef ARTDAQ_DAQDATA_PORTMANAGER_HH
#define ARTDAQ_DAQDATA_PORTMANAGER_HH

#include <string>
#include <fhiclcpp/fwd.h>
#include "fhiclcpp/types/Atom.h"
#include "fhiclcpp/types/ConfigurationTable.h"
#include <netinet/in.h>

#define DEFAULT_BASE 10000
#define DEFAULT_PORTS_PER_PARTITION 1000
#define DEFAULT_ROUTING_TOKEN_OFFSET 10
#define DEFAULT_ROUTING_TABLE_ACK_OFFSET 30
#define DEFAULT_XMLRPC_OFFSET 100
#define DEFAULT_TCPSOCKET_OFFSET 500
#define DEFAULT_REQUEST_PORT 3001
#define DEFAULT_ROUTING_TABLE_PORT 3001
#define DEFAULT_MULTICAST_GROUP_OFFSET 128

namespace artdaq {
	class PortManager {
	public:
		/// <summary>
		/// Configuration of PortManager. May be used for parameter validation
		/// </summary>
		struct Config
		{
			/// "artdaq_base_port" (Default: 10000): Base port for all artdaq partitions. Should be the same across all running systems. Overridden by environment variable ARTDAQ_BASE_PORT.
			fhicl::Atom<int> artdaq_base_port{ fhicl::Name{ "artdaq_base_port" }, fhicl::Comment{ "Base port for all artdaq partitions. Should be the same across all running systems. Overridden by environment variable ARTDAQ_BASE_PORT." }, DEFAULT_BASE };
			/// "ports_per_partition" (Default: 1000): Number of ports to reserve for each partition. Should be the same across all running systems. Overridden by environment variable ARTDAQ_PORTS_PER_PARTITION.
			fhicl::Atom<int> ports_per_partition{ fhicl::Name{ "ports_per_partition" }, fhicl::Comment{"Number of ports to reserve for each partition. Should be the same across all running systems. Overridden by environment variable ARTDAQ_PORTS_PER_PARTITION." },DEFAULT_PORTS_PER_PARTITION };
			/// "multicast_output_interface" (Default: ""): Name of the interface to be used for all multicasts. Has precedence over "multicast_output_network". OPTIONAL
			fhicl::Atom<std::string> multicast_output_interface{ fhicl::Name{ "multicast_output_interface" }, fhicl::Comment{"Name of the interface to be used for all multicasts. Has precedence over \"multicast_output_network\". OPTIONAL"},"" };
			/// "multicast_output_network" (Default: "0.0.0.0"): Address in network to be used for all multicasts. OPTIONAL
			fhicl::Atom<std::string> multicast_output_network{ fhicl::Name{ "multicast_output_network" }, fhicl::Comment{"Address in network to be used for all multicasts. OPTIONAL"},"0.0.0.0" };
			/// "multicast_group_offset" (Default: 128): Number to add to last byte of multicast groups, to avoid problematic 0s.
			fhicl::Atom<int> multicast_group_offset{ fhicl::Name{ "multicast_group_offset" }, fhicl::Comment{"Number to add to last byte of multicast groups, to avoid problematic 0s."},DEFAULT_MULTICAST_GROUP_OFFSET };
			/// "routing_token_port_offset" (Default: 10): Offset from partition base port for routing token ports
			fhicl::Atom<int> routing_token_port_offset{ fhicl::Name{ "routing_token_port_offset" }, fhicl::Comment{"Offset from partition base port for routing token ports"},DEFAULT_ROUTING_TOKEN_OFFSET };
			/// "routing_table_ack_port_offset" (Default: 30): Offset from partition base port for routing table ack ports
			fhicl::Atom<int> routing_table_ack_port_offset{ fhicl::Name{ "routing_table_ack_port_offset" }, fhicl::Comment{"Offset from partition base port for routing table ack ports"},DEFAULT_ROUTING_TABLE_ACK_OFFSET };
			/// "xmlrpc_port_offset" (Default: 100): Offset from partition base port for XMLRPC ports
			fhicl::Atom<int> xmlrpc_port_offset{ fhicl::Name{ "xmlrpc_port_offset" }, fhicl::Comment{"Offset from partition base port for XMLRPC ports"},DEFAULT_XMLRPC_OFFSET };
			/// "tcp_socket_port_offset" (Default: 500): Offset from partition base port for TCP Socket ports
			fhicl::Atom<int> tcp_socket_port_offset{ fhicl::Name{ "tcp_socket_port_offset" }, fhicl::Comment{"Offset from partition base port for TCP Socket ports"},DEFAULT_TCPSOCKET_OFFSET };
			/// "request_port" (Default: 3001): Port to use for request messages (multicast)
			fhicl::Atom<int> request_port{ fhicl::Name{ "request_port" }, fhicl::Comment{"Port to use for request messages (multicast)"},DEFAULT_REQUEST_PORT };
			/// "request_pattern" (Default: "227.128.PPP.SSS"): Pattern to use to generate request multicast group. PPP => Partition number, SSS => Subsystem ID (default 0)
			fhicl::Atom<std::string> request_pattern{ fhicl::Name{ "request_pattern" }, fhicl::Comment{"Pattern to use to generate request multicast group. PPP => Partition number, SSS => Subsystem ID (default 0)"},"227.128.PPP.SSS" };
			/// "routing_table_port" (Default: 3001): Port to use for routing tables (multicast)
			fhicl::Atom<int> routing_table_port{ fhicl::Name{ "routing_table_port" }, fhicl::Comment{"Port to use for routing tables (multicast)"},DEFAULT_ROUTING_TABLE_PORT };
			/// "routing_table_pattern" (Default: "227.129.PPP.SSS"): Pattern to use to generate routing table multicast group. PPP => Partition number, SSS => Subsystem ID (default 0).
			fhicl::Atom<std::string> routing_table_pattern{ fhicl::Name{ "routing_table_pattern" }, fhicl::Comment{"Pattern to use to generate routing table multicast group. PPP => Partition number, SSS => Subsystem ID (default 0)."},"227.129.PPP.SSS" };
			/// "multicast_transfer_port_offset" (Default: 1024): Offset to use for MulticastTransfer ports (port = offset + rank)
			fhicl::Atom<int> multicast_transfer_port_offset{ fhicl::Name{ "multicast_transfer_port_offset" }, fhicl::Comment{"Offset to use for MulticastTransfer ports (port = offset + rank)"},1024 };
			/// "multicast_transfer_pattern" (Default: "227.130.14.PPP"): Pattern to use to generate Multicast Transfer group address. PPP => Partition Number, SSS => Subsystem ID (default 0), RRR => Rank
			fhicl::Atom<std::string> multicast_transfer_pattern{ fhicl::Name{ "multicast_transfer_pattern" }, fhicl::Comment{"Pattern to use to generate Multicast Transfer group address. PPP => Partition Number, SSS => Subsystem ID (default 0), RRR => Rank"},"227.130.14.PPP" };

		};
		using Parameters = fhicl::WrappedTable<Config>;

		PortManager();
		void UpdateConfiguration(fhicl::ParameterSet const& ps);

		int GetRoutingTokenPort(int subsystemID = 0);
		int GetRoutingAckPort(int subsystemID = 0);
		int GetXMLRPCPort(int rank);
		int GetTCPSocketTransferPort(int rank);
		int GetRequestMessagePort();
		std::string GetRequestMessageGroupAddress(int subsystemID = 0);
		int GetRoutingTablePort();
		std::string GetRoutingTableGroupAddress(int subsystemID = 0);
		int GetMulticastTransferPort(int rank);
		std::string GetMulticastTransferGroupAddress();

		in_addr GetMulticastOutputAddress(std::string interface_name = "", std::string interface_address = "");


	private:
		bool base_configured_;
		bool multicasts_configured_;

		bool routing_tokens_configured_;
		bool routing_acks_configured_;
		bool xmlrpc_configured_;
		bool tcpsocket_configured_;
		bool request_port_configured_;
		bool request_pattern_configured_;
		bool routing_table_port_configured_;
		bool routing_table_pattern_configured_;
		bool multicast_transfer_port_configued_;
		bool multicast_transfer_pattern_configured_;

		int base_port_;
		int ports_per_partition_;

		in_addr multicast_interface_address_;
		int multicast_group_offset_;

		int routing_token_offset_;
		int routing_ack_offset_;
		int xmlrpc_offset_;
		int tcp_socket_offset_;
		int request_message_port_;
		int routing_table_port_;
		int multicast_transfer_offset_;
		std::string request_message_group_pattern_;
		std::string routing_table_group_pattern_;
		std::string multicast_transfer_group_pattern_;


		std::string parse_pattern_(std::string pattern, int subsystemID = 0, int rank = 0);
	};
}

#endif //ARTDAQ_DAQDATA_PORTMANAGER_HH