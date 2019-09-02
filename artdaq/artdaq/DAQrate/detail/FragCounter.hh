#ifndef artdaq_DAQrate_detail_FragCounter_hh
#define artdaq_DAQrate_detail_FragCounter_hh

#include <unordered_map>
#include <atomic>
#include <limits>
#include <mutex>

namespace artdaq
{
	namespace detail
	{
		class FragCounter;
	}
}

/**
 * \brief Keep track of the count of Fragments received from a set of sources
 */
class artdaq::detail::FragCounter
{
public:
	/**
	 * \brief Default Constructor
	 */
	explicit FragCounter();

	/**
	 * \brief Increment the given slot by one
	 * \param slot Slot to increment
	 */
	void incSlot(size_t slot);

	/**
	 * \brief Increment the given slot by the given amount
	 * \param slot Slot to increment
	 * \param inc Amount to increment
	 */
	void incSlot(size_t slot, size_t inc);

	/**
	 * \brief Set the given slot to the given value
	 * \param slot Slot to set
	 * \param val Value to set
	 */
	void setSlot(size_t slot, size_t val);

	/**
	 * \brief Get the number of slots this FragCounter instance is tracking
	 * \return The number of slots in this FragCounter instance
	 */
	size_t nSlots() const;

	/**
	 * \brief Get the total number of Fragments received
	 * \return The total number of Fragments received
	 */
	size_t count() const;

	/**
	 * \brief Get the current count for the requested slot
	 * \param slot Slot to get count for
	 * \return The current count for the requested slot
	 */
	size_t slotCount(size_t slot) const;

	/**
	 * \brief Get the minimum slot count
	 * \return The minimum slot count
	 */
	size_t minCount() const;

	/**
	* \brief Get the current count for the requested slot
	* \param slot Slot to get count for
	* \return The current count for the requested slot
	*/
	size_t operator[](size_t slot) const { return slotCount(slot); }

private:
	mutable std::mutex receipts_mutex_;
	std::unordered_map<size_t, std::atomic<size_t>> receipts_;
};

inline
artdaq::detail::FragCounter::
FragCounter()
	: receipts_() {}

inline
void
artdaq::detail::FragCounter::
incSlot(size_t slot)
{
	incSlot(slot, 1);
}

inline
void
artdaq::detail::FragCounter::
incSlot(size_t slot, size_t inc)
{
	std::unique_lock<std::mutex> lk(receipts_mutex_);
	receipts_[slot].fetch_add(inc);
}

inline
void
artdaq::detail::FragCounter::
setSlot(size_t slot, size_t val)
{
	std::unique_lock<std::mutex> lk(receipts_mutex_);
	receipts_[slot] = val;
}

inline
size_t
artdaq::detail::FragCounter::
nSlots() const
{
	std::unique_lock<std::mutex> lk(receipts_mutex_);
	return receipts_.size();
}

inline
size_t
artdaq::detail::FragCounter::
count() const
{
	std::unique_lock<std::mutex> lk(receipts_mutex_);
	size_t acc = 0;
	for (auto& it : receipts_)
	{
		acc += it.second;
	}
	return acc;
}

inline
size_t
artdaq::detail::FragCounter::
slotCount(size_t slot) const
{
	std::unique_lock<std::mutex> lk(receipts_mutex_);
	return receipts_.count(slot) ? receipts_.at(slot).load() : 0;
}

inline
size_t
artdaq::detail::FragCounter::
minCount() const
{
	std::unique_lock<std::mutex> lk(receipts_mutex_);
	size_t min = std::numeric_limits<size_t>::max();
	for (auto& it : receipts_)
	{
		if (it.second < min) min = it.second;
	}
	return min;
}

#endif /* artdaq_DAQrate_detail_FragCounter_hh */
