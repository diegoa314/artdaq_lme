#ifndef TIMEOUT_HH
#define TIMEOUT_HH
/*  This file (Timeout.h) was created by Ron Rechenmacher <ron@fnal.gov> on
	Sep 28, 2009. "TERMS AND CONDITIONS" governing this file are in the README
	or COPYING file. If you do not have such a file, one can be obtained by
	contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
	$RCSfile: Timeout.h,v $
	rev="$Revision: 1.9 $$Date: 2016/10/12 07:11:55 $";
	*/
#include <time.h>               // struct timespec
#include <list>
#include <functional>			// std::function
#include <string>
#include <mutex>
#include <vector>
#include <map>
#include <unordered_map>

/**
 * \brief The Timeout class performs registered actions at specified intervals
 */
class Timeout
{
public:
	/**
	 * \brief Specification for a Timeout function
	 */
	struct timeoutspec
	{
#if 0
		timeoutspec();
		timeoutspec( const timeoutspec & other );
		Timeout::timeoutspec & operator=( const Timeout::timeoutspec & other );
#endif
		std::string desc; ///< Description of the Timeout function
		void* tag; ///< could be file descriptor (fd)
		std::function<void()> function; ///< Function to execute at Timeout
		uint64_t tmo_tod_us; ///< When the function should be executed (gettimeofday, microseconds)
		uint64_t period_us; ///< 0 if not periodic
		int missed_periods; ///< Number of periods that passed while the function was executing
		int check; ///< Check the timeoutspec
	};

	/**
	 * \brief Construct a Timeout object
	 * \param max_tmos Maximum number of registered Timeout functions
	 */
	explicit Timeout(int max_tmos = 100);

	/**
	 * \brief Add a periodic timeout to the Timeout container
	 * \param desc Description of the periodic timeout
	 * \param tag Tag (fd or other) to apply to timeout
	 * \param function Function to execute at timeout
	 * \param period_us Period for timeouts
	 * \param start_us When to start (defaults to 0)
	 * 
	 * maybe need to return a timeout id??   
	 */
	void add_periodic(const char* desc, void* tag, std::function<void()>& function
					  , uint64_t period_us
					  , uint64_t start_us = 0);

	/**
	* \brief Add a periodic timeout to the Timeout container
	* \param desc Description of the periodic timeout
	* \param tag Tag (fd or other) to apply to timeout
	* \param function Function to execute at timeout
	* \param rel_ms Timeout in rem_ms milliseconds
	*
	* maybe need to return a timeout id??
	*/
	void add_periodic(const char* desc, void* tag, std::function<void()>& function
					  , int rel_ms);

	/**
	* \brief Add a periodic timeout to the Timeout container
	* \param desc Description of the periodic timeout
	* \param period_us Period for timeouts
	* \param start_us When to start (defaults to 0)
	*
	* maybe need to return a timeout id??
	*/
	void add_periodic(const char* desc
					  , uint64_t period_us
					  , uint64_t start_us = 0);
 
	/**
	* \brief Add a periodic timeout to the Timeout container
	* \param desc Description of the periodic timeout
	* \param tag Tag (fd or other) to apply to timeout
	* \param function Function to execute at timeout
	* \param rel_ms Timeout in rem_ms milliseconds
	*
	* maybe need to return a timeout id??
	*/
	void add_relative(const char* desc, void* tag, std::function<void()>& function
					  , int rel_ms);

	/**
	* \brief Add a periodic timeout to the Timeout container
	* \param desc Description of the periodic timeout
	* \param rel_ms Timeout in rem_ms milliseconds
	*
	* maybe need to return a timeout id??
	*/
	void add_relative(std::string desc, int rel_ms);

	/**
	 * \brief Add a timeout with the given parameters
	 * \param desc Description of new timeout
	 * \param period_us Period that the timeout should execute with
	 * \param start_us When to start the timeout (default 0)
	 */
	void copy_in_timeout(const char* desc, uint64_t period_us, uint64_t start_us = 0);

	// maybe need to be able to cancel by id,and/or desc and/or Client/funptr
	/**
	 * \brief Cancel the timeout having the given description and tag
	 * \param tag Tag of the cancelled timeout
	 * \param desc Description of the cancelled timeout
	 * \return Whether a timeout was found and cancelled
	 */
	bool cancel_timeout(void* tag, std::string desc);

	/**
	 * \brief Get a timeout that has expired
	 * \param[out] desc Description of timeout that expired
	 * \param[out] tag Tag of timeout that expired
	 * \param[out] function Function of timeout that expired
	 * \param[out] tmo_tod_us When the timeout expired
	 * \return -1 if no timeouts expired
	 */
	int get_next_expired_timeout(std::string& desc, void** tag, std::function<void()>& function
								 , uint64_t* tmo_tod_us);

	/**
	 * \brief Get the amount to wait for the next timeout to occur
	 * \param[out] delay_us Microseconds until next timeout
	 */
	void get_next_timeout_delay(int64_t* delay_us);

	/**
	 * \brief Get the amount to wait for the next timeout to occur
	 * \return Milliseconds until next timeout
	 */
	int get_next_timeout_msdly();

	/**
	 * \brief Run a consistency check on all confiugured timeouts
	 * \return True if all timeouts pass check
	 */
	bool is_consistent();

	/**
	 * \brief TRACE all active timeouts
	 */
	void list_active_time();

private:

	std::mutex lock_mutex_;
	std::vector<timeoutspec> tmospecs_;
	std::list<size_t> free_; // list of tmospecs indexes
	std::multimap<uint64_t, size_t> active_time_;
	std::unordered_multimap<std::string, size_t> active_desc_;

	void timeoutlist_init();

	int tmo_is_before_ts(timeoutspec& tmo
						 , const timespec& ts);

	int get_clear_next_expired_timeout(timeoutspec& tmo
									   , uint64_t tod_now_us);

	void copy_in_timeout(timeoutspec& tmo);
};

#endif // TIMEOUT_HH
