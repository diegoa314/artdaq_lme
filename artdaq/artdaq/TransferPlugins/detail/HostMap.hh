#ifndef ARTDAQ_TRANSFERPLUGINS_DETAIL_HOSTMAP_HH
#define ARTDAQ_TRANSFERPLUGINS_DETAIL_HOSTMAP_HH

#include <string>
#include <vector>
#include <map>
#include "fhiclcpp/ParameterSet.h"
#include "artdaq/TransferPlugins/TransferInterface.hh"
#include "fhiclcpp/types/Sequence.h"
#include "fhiclcpp/types/Table.h"
#include "fhiclcpp/types/Atom.h"

namespace artdaq {

	struct HostMap
	{
		/// <summary>
		/// Entries in the host_map should have these parameters. May be used for parameter validation
		/// </summary>
		struct HostConfig
		{
			/// "rank": Rank index
			fhicl::Atom<int> rank{ fhicl::Name{"rank"}, fhicl::Comment{"Rank index"} };
			/// "host": Hostname for artdaq application with this rank
			fhicl::Atom<std::string> host{ fhicl::Name{"host"}, fhicl::Comment{"Hostname for artdaq application with this rank"} };
		};
		/// <summary>
		/// Template for the host_map configuration parameter.
		/// </summary>
		struct Config
		{
			/// <summary>
			/// List of artdaq applications by rank and location. See artdaq::HostMap::HostConfig
			/// </summary>
			fhicl::Sequence<fhicl::Table<HostConfig>> host_map{ fhicl::Name("host_map"), fhicl::Comment("List of artdaq applications by rank and location") };
		};
	};

	typedef std::map<int, std::string> hostMap_t; ///< The host_map is a map associating ranks with artdaq::DestinationInfo objects

	/// <summary>
	/// Create a list of HostMap::HostConfig ParameterSets from a hostMap_t map
	/// </summary>
	/// <param name="input">Input hostMap_t</param>
	/// <returns>std::vector containing HostMap::HostConfig ParameterSets</returns>
	inline std::vector<fhicl::ParameterSet> MakeHostMapPset(std::map<int, std::string> input)
	{
		std::vector<fhicl::ParameterSet> output;
		for (auto& rank : input)
		{
			fhicl::ParameterSet rank_output;
			rank_output.put<int>("rank", rank.first);
			rank_output.put<std::string>("host", rank.second);
			output.push_back(rank_output);
		}
		return output;
	}

	/// <summary>
	/// Make a hostMap_t from a HostMap::Config ParameterSet
	/// </summary>
	/// <param name="pset">fhicl::ParameterSet containing a HostMap::Config</param>
	/// <param name="masterPortOffset">Port offset to apply to all entries (Default 0)</param>
	/// <param name="map">Input map for consistency checking (Default: hostMap_t())</param>
	/// <returns>hostMap_t object</returns>
	inline hostMap_t MakeHostMap(fhicl::ParameterSet pset, hostMap_t map = hostMap_t())
	{
		if (pset.has_key("host_map")) {
			auto hosts = pset.get<std::vector<fhicl::ParameterSet>>("host_map");
			for (auto& ps : hosts)
			{
				auto rank = ps.get<int>("rank", TransferInterface::RECV_TIMEOUT);
				auto hostname = ps.get<std::string>("host", "localhost");

				if (map.count(rank) && (map[rank] != hostname ))
				{
					TLOG(TLVL_ERROR) << "Inconsistent host maps supplied! Check configuration! There may be TCPSocket-related failures!";
				}
				map[rank] = hostname;
			}
		}
		return map;
	}
}

#endif