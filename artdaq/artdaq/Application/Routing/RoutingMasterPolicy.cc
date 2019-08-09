#define TRACE_NAME "RoutingMasterPolicy"

#include "artdaq/Application/Routing/RoutingMasterPolicy.hh"
#include "fhiclcpp/ParameterSet.h"

artdaq::RoutingMasterPolicy::RoutingMasterPolicy(fhicl::ParameterSet ps)
	: next_sequence_id_(1)
	, tokens_()
	, max_token_count_(0)
{
	auto receiver_ranks = ps.get<std::vector<int>>("receiver_ranks");
	receiver_ranks_.insert(receiver_ranks.begin(), receiver_ranks.end());
}

void artdaq::RoutingMasterPolicy::AddReceiverToken(int rank, unsigned new_slots_free)
{
	if (!receiver_ranks_.count(rank)) return;
	TLOG(10) << "AddReceiverToken BEGIN" ;
	std::unique_lock<std::mutex> lk(tokens_mutex_);
	if (new_slots_free == 1) 
	{
		tokens_.push_back(rank);
	}
	else 
	{
		// Randomly distribute multitokens through the token list
		// Only used at start run time, so we can take the performance hit
		for (unsigned i = 0; i < new_slots_free; ++i)
		{
			auto it = tokens_.begin();
			if(tokens_.size()) std::advance(it, rand() % tokens_.size());
			tokens_.insert(it, rank);
		}
	}
	if (tokens_.size() > max_token_count_) max_token_count_ = tokens_.size();
	TLOG(10) << "AddReceiverToken END" ;
}

std::unique_ptr<std::deque<int>> artdaq::RoutingMasterPolicy::getTokensSnapshot()
{
	TLOG(10) << "getTokensSnapshot BEGIN" ;
	std::unique_lock<std::mutex> lk(tokens_mutex_);
	auto out = std::make_unique<std::deque<int>>(tokens_);
	tokens_.clear();
	TLOG(10) << "getTokensSnapshot END" ;
	return out;
}

void artdaq::RoutingMasterPolicy::addUnusedTokens(std::unique_ptr<std::deque<int>> tokens)
{
	std::unique_lock<std::mutex> lk(tokens_mutex_);
	for (auto token = tokens.get()->rbegin(); token != tokens.get()->rend(); ++token)
	{
		tokens_.push_front(*token);
	}
	if (tokens_.size() > max_token_count_) max_token_count_ = tokens_.size();
}
