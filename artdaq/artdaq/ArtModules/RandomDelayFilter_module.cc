////////////////////////////////////////////////////////////////////////
// Class:       RandomDelayFilter
// Module Type: filter
// File:        RandomDelayFilter_module.cc
//
// Generated at Fri Mar  4 10:45:30 2016 by Eric Flumerfelt using artmod
// from cetpkgsupport v1_09_00.
////////////////////////////////////////////////////////////////////////

#define TRACE_NAME "RandomDelayFilter"

#include "art/Framework/Core/EDFilter.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"

#include "artdaq/DAQdata/Globals.hh"

#include <memory>
#include <random>
#include <chrono>

namespace artdaq
{
	class RandomDelayFilter;
}

/**
 * \brief A filter which delays for a random amount of time, then
 * drops a random fraction of events. Used to simulate the delays
 * and efficiency of real filters.
 * 
 * Multiple RandomDelayFilters in series can simulate the effect of multiple layers of filtering
 */
class artdaq::RandomDelayFilter : public art::EDFilter
{
public:
	/**
	 * \brief RandomDelayFilter Constructor
	 * \param p ParameterSet used to confgure RandomDelayFilter
	 * 
	 * RandomDelayFilter accepts the following Parametrs:
	 * "minimum_delay_ms" (Default: 0): The minimum amount of time to delay, in ms
	 * "maximum_delay_ms" (Default: 1000): The maximum amount of time to delay, in ms
	 * "mean_delay_ms" (Default: 500): If using a non-uniform distribution for delay times, the mean of the distribution, in ms (This value will be used for Fixed delays)
	 * "sigma_delay_ms" (Default: 100): If using a Normal distribution for delay times, the sigma of the distribution, in ms
	 * "pass_filter_percentage" (Default: 100): The fraction of events which will pass the filter
     * "distribution_type" (Default: "Uniform"): The distribution to sample for delays (Uniform, Normal, Exponential, Fixed)
	 * "cpu_load_ratio" (Default: 0.5): The fraction of the delay time which should be active (spinning) versis passive (sleeping)
	 * "random_seed" (Default: time(0)): The seed for the ditribution
	 */
	explicit RandomDelayFilter(fhicl::ParameterSet const& p);

	/**
	 * \brief Copy Constructor is deleted
	 */
	RandomDelayFilter(RandomDelayFilter const&) = delete;

	/**
	 * \brief Move Constructor is deleted
	 */
	RandomDelayFilter(RandomDelayFilter&&) = delete;

	/**
	 * \brief Copy Assignment operator is deleted
	 * \return RandomDelayFilter copy
	 */
	RandomDelayFilter& operator=(RandomDelayFilter const&) = delete;

	/**
	 * \brief Move Assignment operator is deleted
	 * \return RandomDelayFilter instance
	 */
	RandomDelayFilter& operator=(RandomDelayFilter&&) = delete;

	/**
	 * \brief Filter is a required override of art::EDFilter, and is called for each event
	 * \param e The art::Event to filter
	 * \return Whether the event passes the filter
	 * 
	 * This function is where RandomDelayFilter performs its work, using the delay distribution
	 * to pick a delay time, spinning and/or sleeping for that amount of time, then picking
	 * an integer from the pass distribution to determine if the event should pass or not.
	 */
	bool filter(art::Event& e) override;

private:
	// Random Delay Parameters (min/max for uniform, mean/sigma for normal)
	double min_ms_;
	double max_ms_;
	double mean_ms_;
	double sigma_ms_;

	int pass_factor_;
	double load_factor_;

	// Random Engine Setup
	std::mt19937 engine_;
	std::unique_ptr<std::uniform_real_distribution<double>> uniform_distn_;
	std::unique_ptr<std::normal_distribution<double>> normal_distn_;
	std::unique_ptr<std::exponential_distribution<double>> exponential_distn_;
	std::unique_ptr<std::uniform_int_distribution<int>> pass_distn_;

	enum class DistType {
		Fixed,
		Uniform,
		Normal,
		Exponential
	};
	DistType distribution_type_;
};


artdaq::RandomDelayFilter::RandomDelayFilter(fhicl::ParameterSet const& p)
	: min_ms_(p.get<double>("minimum_delay_ms", 0))
	, max_ms_(p.get<double>("maximum_delay_ms", 1000))
	, mean_ms_(p.get<double>("mean_delay_ms", 500))
	, sigma_ms_(p.get<double>("sigma_delay_ms", 100))
	, pass_factor_(p.get<int>("pass_filter_percentage", 100))
	, load_factor_(p.get<double>("cpu_load_ratio", 0.5))
	, engine_(p.get<int64_t>("random_seed", time(0)))
	, pass_distn_(new std::uniform_int_distribution<int>(0, 100))
{
	// Set limits on parameters
	if (pass_factor_ > 100) pass_factor_ = 100;
	if (pass_factor_ < 0) pass_factor_ = 0;
	if (load_factor_ < 0.0) load_factor_ = 0.0;
	if (load_factor_ > 1.0) load_factor_ = 1.0;

	if (min_ms_ < 0) min_ms_ = 0;
	if (min_ms_ > max_ms_) max_ms_ = min_ms_;
	if (mean_ms_ < 0) mean_ms_ = 0;
	if (sigma_ms_ < 0) sigma_ms_ = 0;

	auto type = p.get<std::string>("distribution_type", "Uniform");
	assert(type.size() >= 1);
	switch (type[0])
	{
	case 'n':
	case 'N':
	  TLOG(TLVL_INFO) << "Generating delay times using Normal distribution with mean " << mean_ms_ << " ms, std. dev. " << sigma_ms_ << " ms, min " << min_ms_ << " ms and max " << max_ms_ << "ms.";
		distribution_type_ = DistType::Normal;
		if(mean_ms_ < min_ms_) { 
		  TLOG(TLVL_WARNING) << "Mean is smaller than min, setting to min";
		  mean_ms_ = min_ms_;
		}
		mean_ms_ -= min_ms_; // When we sample the distribution, we offset by min_ms_
		normal_distn_.reset(new std::normal_distribution<double>(mean_ms_, sigma_ms_));
		break;
	case 'e':
	case 'E':
	  TLOG(TLVL_INFO) << "Generating delay times using Exponential distribution with mean " << mean_ms_ << " ms, min " << min_ms_ << " ms and max " << max_ms_ << " ms.";
		distribution_type_ = DistType::Exponential;
		if(mean_ms_ < min_ms_) { 
		  TLOG(TLVL_WARNING) << "Mean is smaller than min, setting to min";
		  mean_ms_ = min_ms_;
		}
		mean_ms_ -= min_ms_; // When we sample the distribution, we offset by min_ms_
		if (mean_ms_ == 0) mean_ms_ = 1;
		exponential_distn_.reset(new std::exponential_distribution<double>(1/mean_ms_));
		break;
	case 'U':
	case 'u':
		TLOG(TLVL_INFO) << "Generating delay times using Uniform distribution with min " << min_ms_ << " ms and max " << max_ms_ << " ms.";
		distribution_type_ = DistType::Uniform;
		uniform_distn_.reset(new std::uniform_real_distribution<double>(min_ms_, max_ms_));
		break;
	case 'f':
	case 'F':
	default:
		TLOG(TLVL_INFO) << "Delay time set to " << min_ms_ << " ms.";
		distribution_type_ = DistType::Fixed;
		break;
	}
}

bool artdaq::RandomDelayFilter::filter(art::Event& e)
{
	double delay = min_ms_;
	do {
	switch (distribution_type_)
	{
	case DistType::Normal:
		delay += (*normal_distn_)(engine_);
		break;
	case DistType::Exponential:
		delay += (*exponential_distn_)(engine_);
		break;
	case DistType::Uniform:
		delay = (*uniform_distn_)(engine_);
		break;
	case DistType::Fixed:
		break;
	}
	} while(delay > max_ms_ && delay < min_ms_);
	TLOG(TLVL_DEBUG) << "Simulating processing of event " << e.event() << " by delaying " << delay << "ms." ;

	usleep(1000 * (1 - load_factor_) * delay);

	auto i = 0;
	auto now = std::chrono::steady_clock::now();
	while (TimeUtils::GetElapsedTimeMilliseconds(now) < static_cast<size_t>(delay * load_factor_))
	{
		i = i + 1 % std::numeric_limits<int>::max();
	}

	return (*pass_distn_)(engine_) < pass_factor_;
}

DEFINE_ART_MODULE(artdaq::RandomDelayFilter)
