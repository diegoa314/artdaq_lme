//  This file (Timer.cxx) was created by Ron Rechenmacher <ron@fnal.gov> on
//  Sep 28, 2009. "TERMS AND CONDITIONS" governing this file are in the README
//  or COPYING file. If you do not have such a file, one can be obtained by
//  contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
//  $RCSfile: Timeout.cxx,v $
//  rev="$Revision: 1.13 $$Date: 2016/10/12 21:00:13 $";
/*
	g++ -Wall -g -std=c++0x -c Timeout.cxx
OR
	g++ -Wall -g -std=c++0x -shared -fPIC -o Timeout.so Timeout.cxx -lrt
*/

#define TRACE_NAME "Timeout"

#include <stdio.h>				// printf
#include <sys/time.h>           /* struct timeval */
#include <assert.h>				/* assert */
#include <string.h>				/* strcmp */
#include <stdlib.h>             // exit
#include <list>
using std::list;
#include "artdaq/TransferPlugins/detail/Timeout.hh"
#include "artdaq/DAQdata/Globals.hh"				// TRACE
#include "artdaq-core/Utilities/TimeUtils.hh"

// public:

#if 0
Timeout::timeoutspec::timeoutspec()
	: desc(), tag(), function()
	, ts(), period()
	, missed_periods(), check()
{
	TLOG(18) << "Timeout::timeoutspec ctor this=" << this;
}
Timeout::timeoutspec::timeoutspec(const timeoutspec & other)
	: desc(other.desc), tag(other.tag), function(other.function)
	, ts(other.ts), period(other.period)
	, missed_periods(other.missed_periods), check(other.check)
{
	TLOG(18) << "Timeout::timeoutspec copy ctor";
}
Timeout::timeoutspec & Timeout::timeoutspec::operator=(const Timeout::timeoutspec & other)
{
	TLOG(18) << "Timeout::timeoutspec copy assignment (operator= other.desc=" << other.desc << ")";
	desc = other.desc;
	tag = other.tag;
	function = other.function;
	ts = other.ts;
	period = other.period;
	missed_periods = other.missed_periods;
	check = other.check;
	return *this;
}
#endif


Timeout::Timeout(int max_tmos)
	: tmospecs_(max_tmos)
{
	TLOG(16) << "Timeout ctor";
	timeoutlist_init();
}


void
Timeout::add_periodic(const char* desc, void* tag, std::function<void()>& function
	, uint64_t period_us
	, uint64_t start_us)
{
	timeoutspec tmo;
	tmo.desc = desc;
	tmo.tag = tag;
	tmo.function = function;
	tmo.tmo_tod_us = start_us ? start_us : artdaq::TimeUtils::gettimeofday_us() + period_us;
	tmo.period_us = period_us;
	tmo.check = tmo.missed_periods = 0;
	copy_in_timeout(tmo);
} // add_periodic

void
Timeout::add_periodic(const char* desc, void* tag, std::function<void()>& function
	, int rel_ms)
{
	timeoutspec tmo;
	tmo.desc = desc;
	tmo.tag = tag;
	tmo.function = function;
	tmo.period_us = rel_ms * 1000;
	tmo.tmo_tod_us = artdaq::TimeUtils::gettimeofday_us() + tmo.period_us;
	tmo.check = tmo.missed_periods = 0;
	copy_in_timeout(tmo);
} // add_periodic

void
Timeout::add_periodic(const char* desc
	, uint64_t period_us
	, uint64_t start_us)
{
	TLOG(19) << "add_periodic - desc=" << desc << " period_us=" << period_us << " start_us=" << start_us;
	timeoutspec tmo;
	tmo.desc = desc;
	tmo.tag = 0;
	tmo.function = 0;
	tmo.tmo_tod_us = start_us;
	tmo.period_us = period_us;
	tmo.missed_periods = tmo.check = 0;
	copy_in_timeout(tmo);
} // add_periodic

void
Timeout::add_relative(const char* desc, void* tag, std::function<void()>& function
	, int rel_ms)
{
	timeoutspec tmo;
	tmo.desc = desc;
	tmo.tag = tag;
	tmo.function = function;
	tmo.tmo_tod_us = artdaq::TimeUtils::gettimeofday_us() + rel_ms * 1000;
	tmo.period_us = 0;
	tmo.missed_periods = tmo.check = 0;
	copy_in_timeout(tmo);
} // add_periodic

void
Timeout::add_relative(std::string desc
	, int rel_ms)
{
	timeoutspec tmo;
	tmo.desc = desc.c_str();
	tmo.tag = 0;
	tmo.function = 0;
	tmo.period_us = 0;
	tmo.tmo_tod_us = artdaq::TimeUtils::gettimeofday_us() + rel_ms * 1000;
	tmo.missed_periods = tmo.check = 0;
	copy_in_timeout(tmo);
} // add_periodic

int // tmo_tod_us is an output
Timeout::get_next_expired_timeout(std::string& desc, void** tag, std::function<void()>& function
	, uint64_t* tmo_tod_us)
{
	int skipped = 0;
	timeoutspec tmo;
	TLOG(15) << "get_next_expired_timeout b4 get_clear_next_expired_timeout";
	skipped = get_clear_next_expired_timeout(tmo, artdaq::TimeUtils::gettimeofday_us());
	if (skipped == -1)
	{
		TLOG(18) << "get_next_expired_timeout - get_clear_next_expired_timeout returned false";
		desc = std::string(""); // 2 ways to check for none timed out
	}
	else
	{
		desc = tmo.desc;
		*tag = tmo.tag;
		function = tmo.function;
		*tmo_tod_us = tmo.tmo_tod_us;
	}
	return (skipped);
} // get_next_expired_timeout

void
Timeout::get_next_timeout_delay(int64_t* delay_us)
{
	std::unique_lock<std::mutex> ulock(lock_mutex_);
	size_t active_time_size = active_time_.size();
	if (active_time_size == 0)
	{
		TLOG(17) << "get_next_timeout_delay active_.size() == 0";
		*delay_us = -1; // usually means a very very long time
	}
	else
	{
		TLOG(17) << "get_next_timeout_delay active_.size() != 0: " << active_time_size;
		uint64_t tod_us = artdaq::TimeUtils::gettimeofday_us();
		timeoutspec* tmo = &tmospecs_[(*(active_time_.begin())).second];
		*delay_us = tmo->tmo_tod_us - tod_us;
		if (*delay_us < 0)
			*delay_us = 0;
	}
} // get_next_timeout_delay

int
Timeout::get_next_timeout_msdly()
{
	int64_t delay_us;
	int tmo;
	get_next_timeout_delay(&delay_us);
	if (delay_us == -1)
	{
		tmo = -1;
	}
	else
	{ // NOTE THE + 1 b/c of integer division and/or system HZ resolution
		tmo = delay_us / 1000;
	}
	return (tmo);
} // get_next_timeout_msdly


bool
Timeout::is_consistent()
{
	std::map<uint64_t, size_t>::iterator itactive;
	std::list<size_t>::iterator itfree;
	for (unsigned ii = 0; ii < tmospecs_.size(); ++ii)
		tmospecs_[ii].check = 1;
	for (itactive = active_time_.begin(); itactive != active_time_.end(); ++itactive)
		tmospecs_[(*itactive).second].check--;
	for (itfree = free_.begin(); itfree != free_.end(); ++itfree)
		tmospecs_[*itfree].check--;
	for (unsigned ii = 0; ii < tmospecs_.size(); ++ii)
		if (tmospecs_[ii].check != 0) return false;
	return (true);
}

void
Timeout::timeoutlist_init()
{
	size_t list_sz = tmospecs_.size();
	for (size_t ii = 0; ii < list_sz; ++ii)
	{ //bzero( &tmospecs_[list_sz], sizeof(tmospecs_[0]) );
		free_.push_front(ii);
	}
}


int Timeout::get_clear_next_expired_timeout(timeoutspec& tmo
	, uint64_t tod_now_us)
{
	int skipped = 0;
	if (active_time_.size() == 0)
	{
		TLOG(17) << "get_clear_next_expired_timeout - nothing to get/clear!";
		return (false);
	}
	else
	{
		std::unique_lock<std::mutex> ulock(lock_mutex_);
		std::multimap<uint64_t, size_t>::iterator itfront = active_time_.begin();
		size_t idx = (*itfront).second;
		if (tmospecs_[idx].tmo_tod_us < tod_now_us)
		{
			tmo = tmospecs_[idx];
			TLOG(17) << "get_clear_next_expired_timeout - clearing tag=" << tmo.tag << " desc=" << tmo.desc << " period=" << tmo.period_us << " idx=" << idx;

			active_time_.erase(itfront);
			// now, be effecient -- if periodic, add back at new time, else
			// find/erase active_desc_ with same idx and free
			if (tmo.period_us)
			{
				// PERIODIC
				int64_t delta_us;
				uint64_t period_us = tmo.period_us;
				delta_us = tod_now_us - tmo.tmo_tod_us;
				skipped = delta_us / period_us;
				assert(skipped >= 0);
				tmo.missed_periods += skipped;

				/* now fast forward over skipped */
				period_us += period_us * skipped;
				tmospecs_[idx].tmo_tod_us += period_us;
				active_time_.insert(std::pair<uint64_t, size_t>(tmospecs_[idx].tmo_tod_us, idx));
				TLOG(18) << "get_clear_next_expired_timeout - periodic timeout desc=" << tmo.desc
					<< " period_us=" << period_us << " delta_us=" << delta_us
					<< " skipped=" << skipped << " next tmo at:" << tmospecs_[idx].tmo_tod_us;
			}
			else
			{
				// find active_desc_ with same idx
				std::unordered_multimap<std::string, size_t>::iterator i2;
				i2 = active_desc_.equal_range(tmospecs_[idx].desc).first;
				while (1)
				{ // see also in cancel_timeout below
					if (i2->second == idx)
						break;
					++i2;
				}
				active_desc_.erase(i2);
				free_.push_front(idx);
			}
		}
		else
		{
			TLOG(17) << "get_clear_next_expired_timeout - front " << tmospecs_[idx].tmo_tod_us << " NOT before ts_now " << tod_now_us << " - not clearing!";
			return (-1);
		}
	}
	return true;
} // get_clear_next_expired_timeout

// this doesn't do anything (function undefined)
void Timeout::copy_in_timeout(const char* desc, uint64_t period_us, uint64_t start_us)
{
	TLOG(18) << "copy_in_timeout desc=" + std::string(desc);
	timeoutspec tos;
	tos.desc = desc;
	tos.tag = NULL;
	tos.function = 0;
	tos.period_us = period_us;
	tos.tmo_tod_us = start_us;
	tos.missed_periods = tos.check = 0;
	copy_in_timeout(tos);
}

void
Timeout::copy_in_timeout(timeoutspec& tmo)
{
	// check for at least one empty entry
	assert(free_.size());

	// get/fill-in free tmospec
	std::unique_lock<std::mutex> ulock(lock_mutex_);
	size_t idx = free_.front();
	free_.pop_front();
	tmospecs_[idx] = tmo;
	TLOG(20) << "copy_in_timeout timeoutspec desc=" + tmo.desc;
	active_time_.insert(std::pair<uint64_t, size_t>(tmo.tmo_tod_us, idx));
	active_desc_.insert(std::pair<std::string, size_t>(tmo.desc, idx));
}

bool
Timeout::cancel_timeout(void* tag
	, std::string desc)
{
	bool retsts = false;
	std::unordered_multimap<std::string, size_t>::iterator ii, ee;
	std::unique_lock<std::mutex> ulock(lock_mutex_);
	auto pairOfIters = active_desc_.equal_range(desc);
	ii = pairOfIters.first;
	ee = pairOfIters.second;
	for (; ii != ee && ii->first.compare(desc) == 0; ++ii)
	{
		size_t idx = ii->second;
		if (tmospecs_[idx].tag == tag)
		{
			// found a match
			retsts = true;
			uint64_t tmo_tod_us = tmospecs_[idx].tmo_tod_us;

			// now make sure to find the active_time_ with the same idx
			std::multimap<uint64_t, size_t>::iterator i2;
			i2 = active_time_.equal_range(tmo_tod_us).first;
			while (1)
			{ // see also in get_clear_next_expired_timeout above
				if (i2->second == idx)
					break;
				++i2;
			}

			active_desc_.erase(ii);
			active_time_.erase(i2);
			free_.push_front(idx);
			break;
		}
	}
	TLOG(22) << "cancel_timeout returning " << retsts;
	return retsts;
} // cancel_timeout

void
Timeout::list_active_time()
{
	std::map<uint64_t, size_t>::iterator ii = active_time_.begin(), ee = active_time_.end();
	for (; ii != ee; ++ii)
	{
		TLOG(TLVL_DEBUG) << "list_active_time " << (*ii).first << " desc=" << tmospecs_[(*ii).second].desc;
	}
}
