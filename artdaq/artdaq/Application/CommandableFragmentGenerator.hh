#ifndef artdaq_Application_CommandableFragmentGenerator_hh
#define artdaq_Application_CommandableFragmentGenerator_hh

// Socket Includes
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <chrono>
#include <array>
#include <list>

#include "fhiclcpp/fwd.h"
#include "fhiclcpp/ParameterSet.h"

#include "artdaq/DAQdata/Globals.hh"
#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Generators/FragmentGenerator.hh"
#include "artdaq-utilities/Plugins/MetricManager.hh"
#include "artdaq/DAQrate/RequestReceiver.hh"

namespace artdaq
{
	/**
	 * \brief The RequestMode enumeration contains the possible ways which CommandableFragmentGenerator responds to data requests.
	 */
	enum class RequestMode
	{
		Single,
		Buffer,
		Window,
		Ignored
	};

	/**
	 * \brief CommandableFragmentGenerator is a FragmentGenerator-derived
	 * abstract class that defines the interface for a FragmentGenerator
	 * designed as a state machine with start, stop, etc., transition
	 * commands.
	 *
	 * Users of classes derived from
	 * CommandableFragmentGenerator will call these transitions via the
	 * publically defined StartCmd(), StopCmd(), etc.; these public
	 * functions contain functionality considered properly universal to
	 * all CommandableFragmentGenerator-derived classes, including calls
	 * to private virtual functions meant to be overridden in derived
	 * classes. The same applies to this class's implementation of the
	 * FragmentGenerator::getNext() pure virtual function, which is
	 * declared final (i.e., non-overridable in derived classes) and which
	 * itself calls a pure virtual getNext_() function to be implemented
	 * in derived classes.
	 *
	 * State-machine related interface functions will be called only from a
	 * single thread. getNext() will be called only from a single
	 * thread. The thread from which state-machine interfaces functions are
	 * called may be a different thread from the one that calls getNext().
	 *
	 * John F., 3/24/14
	 *
	 * After some discussion with Kurt, CommandableFragmentGenerator has
	 * been updated such that it now contains a member vector
	 * fragment_ids_ ; if "fragment_id" is set in the FHiCL document
	 * controlling a class derived from CommandableFragmentGenerator,
	 * fragment_ids_ will be booked as a length-1 vector, and the value in
	 * this vector will be returned by fragment_id(). fragment_id() will
	 * throw an exception if the length of the vector isn't 1. If
	 * "fragment_ids" is set in the FHiCL document, then fragment_ids_ is
	 * filled with the values in the list which "fragment_ids" refers to,
	 * otherwise it is set to the empty vector (this is what should happen
	 * if the user sets the "fragment_id" variable in the FHiCL document,
	 * otherwise exceptions will end up thrown due to the logical
	 * conflict). If neither "fragment_id" nor "fragment_ids" is set in
	 * the FHiCL document, writers of classes derived from this one will
	 * be expected to override the virtual fragmentIDs() function with
	 * their own code (the CompositeDriver class is an example of this)
	 */
	class CommandableFragmentGenerator : public FragmentGenerator
	{
	public:
		/// <summary>
		/// Configuration of the CommandableFragmentGenerator. May be used for parameter validation
		/// </summary>
		struct Config
		{
			/// "generator" (REQUIRED) Name of the CommandableFragmentGenerator plugin to load
			fhicl::Atom<std::string> generator_type                  { fhicl::Name{"generator"                        }, fhicl::Comment{"Name of the CommandableFragmentGenerator plugin to load"} };
			/// "request_window_offset" (Default: 0) : Request messages contain a timestamp.For Window request mode, start the window this far before the timestamp in the request
			fhicl::Atom<Fragment::timestamp_t> request_window_offset { fhicl::Name{"request_window_offset"            }, fhicl::Comment{"Request messages contain a timestamp. For Window request mode, start the window this far before the timestamp in the request"}, 0 };
			/// "request_window_width" (Default: 0) : For Window request mode, the window will be timestamp - offset to timestamp - offset + width
			fhicl::Atom<Fragment::timestamp_t> request_window_width  { fhicl::Name{"request_window_width"             }, fhicl::Comment{"For Window request mode, the window will be timestamp - offset to timestamp - offset + width"}, 0 };
			/// "stale_request_timeout" (Default: -1) : How long should request messages be retained
			fhicl::Atom<Fragment::timestamp_t> stale_request_timeout { fhicl::Name{"stale_request_timeout"            }, fhicl::Comment{"How long should request messages be retained"}, 0xFFFFFFFF };
			/// "expected_fragment_type" (Default: 231, EmptyFragmentType) : The type of Fragments this CFG will be generating. "Empty" will auto - detect type based on Fragments generated.
			fhicl::Atom<Fragment::type_t> expected_fragment_type     { fhicl::Name{"expected_fragment_type"           }, fhicl::Comment{"The type of Fragments this CFG will be generating. \"Empty\" will auto-detect type based on Fragments generated."}, Fragment::type_t(Fragment::EmptyFragmentType) };
			/// "request_windows_are_unique" (Default: true) : Whether Fragments should be removed from the buffer when matched to a request window
			fhicl::Atom<bool> request_windows_are_unique             { fhicl::Name{"request_windows_are_unique"       }, fhicl::Comment{"Whether Fragments should be removed from the buffer when matched to a request window"}, true };
			/// "missing_request_window_timeout_us" (Default: 5000000) : How long to track missing requests in the "out-of-order Windows" list
			fhicl::Atom<size_t> missing_request_window_timeout_us    { fhicl::Name{"missing_request_window_timeout_us"}, fhicl::Comment{"How long to track missing requests in the \"out - of - order Windows\" list"}, 5000000 };
			/// "window_close_timeout_us" (Default: 2000000) : How long to wait for the end of the data buffer to pass the end of a request window(measured from the time the request was received)
			fhicl::Atom<size_t> window_close_timeout_us              { fhicl::Name{"window_close_timeout_us"          }, fhicl::Comment{"How long to wait for the end of the data buffer to pass the end of a request window (measured from the time the request was received)"}, 2000000 };
			/// "separate_data_thread" (Default: false) : Whether data collection should proceed on its own thread.Required for all data request processing
			fhicl::Atom<bool> separate_data_thread                   { fhicl::Name{"separate_data_thread"             }, fhicl::Comment{"Whether data collection should proceed on its own thread. Required for all data request processing"}, false };
			/// "circular_buffer_mode" (Default: false) : Whether the data buffer should be treated as a circular buffer on the input side (i.e. old fragments are automatically discarded when the buffer is full to always call getNext_).
			fhicl::Atom<bool> circular_buffer_mode                   { fhicl::Name{"circular_buffer_mode"             }, fhicl::Comment{"Whether the data buffer should be treated as a circular buffer on the input side (i.e. old fragments are automatically discarded when the buffer is full to always call getNext_)."}, false };
			/// "sleep_on_no_data_us" (Default: 0 (no sleep)) : How long to sleep after calling getNext_ if no data is returned
			fhicl::Atom<size_t> sleep_on_no_data_us                  { fhicl::Name{"sleep_on_no_data_us"              }, fhicl::Comment{"How long to sleep after calling getNext_ if no data is returned"}, 0 };
			/// "data_buffer_depth_fragments" (Default: 1000) : How many Fragments to store in the buffer
			fhicl::Atom<int> data_buffer_depth_fragments             { fhicl::Name{"data_buffer_depth_fragments"      }, fhicl::Comment{"How many Fragments to store in the buffer"}, 1000 };
			/// "data_buffer_depth_mb" (Default: 1000) : The maximum size of the data buffer in MB
			fhicl::Atom<size_t> data_buffer_depth_mb                 { fhicl::Name{"data_buffer_depth_mb"             }, fhicl::Comment{"The maximum size of the data buffer in MB"}, 1000 };
			/// "separate_monitoring_thread" (Default: false) : Whether a thread that calls the checkHWStatus_ method should be created
			fhicl::Atom<bool> separate_monitoring_thread             { fhicl::Name{"separate_monitoring_thread"       }, fhicl::Comment{"Whether a thread that calls the checkHWStatus_ method should be created"}, false };
			/// "hardware_poll_interval_us" (Default: 0) : If a separate monitoring thread is used, how often should it call checkHWStatus_
			fhicl::Atom<int64_t> hardware_poll_interval_us           { fhicl::Name{"hardware_poll_interval_us"        }, fhicl::Comment{"If a separate monitoring thread is used, how often should it call checkHWStatus_"}, 0 };
			/// "board_id" (REQUIRED) : The identification number for this CommandableFragmentGenerator
			fhicl::Atom<int> board_id                                { fhicl::Name{"board_id"                         }, fhicl::Comment{"The identification number for this CommandableFragmentGenerator"} };
			/// "fragment_ids" (Default: empty vector) : A list of Fragment IDs created by this CommandableFragmentGenerator
			/// Note that only one of fragment_ids and fragment_id should be specified in the configuration
			fhicl::Sequence<Fragment::fragment_id_t> fragment_ids    { fhicl::Name("fragment_ids"                     ), fhicl::Comment("A list of Fragment IDs created by this CommandableFragmentGenerator") };
			/// "fragment_id" (Default: -99) : The Fragment ID created by this CommandableFragmentGenerator
			/// Note that only one of fragment_ids and fragment_id should be specified in the configuration
			fhicl::Atom<int> fragment_id                             { fhicl::Name{"fragment_id"                      }, fhicl::Comment{"The Fragment ID created by this CommandableFragmentGenerator"}, -99 };
			/// "sleep_on_stop_us" (Default: 0) : How long to sleep before returning when stop transition is called
			fhicl::Atom<int> sleep_on_stop_us                        { fhicl::Name{"sleep_on_stop_us"                 }, fhicl::Comment{"How long to sleep before returning when stop transition is called"}, 0 };
			/// <summary>
			/// "request_mode" (Deafult: Ignored) : The mode by which the CommandableFragmentGenerator will process reqeusts
			/// Ignored : Request messages are ignored.This is a "push" CommandableFragmentGenerator
			/// Single : The CommandableFragmentGenerator responds to each request with the latest Fragment it has received
			/// Buffer : The CommandableFragmentGenerator responds to each request with all Fragments it has received since the last request
			/// Window : The CommandableFragmentGenerator searches its data buffer for all Fragments whose timestamp falls within the request window
			/// </summary>
			fhicl::Atom<std::string> request_mode                    { fhicl::Name{"request_mode"                     }, fhicl::Comment{"The mode by which the CommandableFragmentGenerator will process reqeusts"}, "ignored" };
			fhicl::TableFragment<artdaq::RequestReceiver::Config> receiverConfig; ///< Configuration for the Request Receiver. See artdaq::RequestReceiver::Config
		};
		using Parameters = fhicl::WrappedTable<Config>;

		/**
		 * \brief CommandableFragmentGenerator default constructor
		 *
		 * This constructor default-initializes all parameters
		 */
		CommandableFragmentGenerator();

		/**
		 * \brief CommandableFragmentGenerator Constructor
		 * \param ps ParameterSet used to configure CommandableFragmentGenerator. See artdaq::CommandableFragmentGenerator::Config.
		 */
		explicit CommandableFragmentGenerator(const fhicl::ParameterSet& ps);

		/**
		 * \brief CommandableFragmentGenerator Destructor
		 *
		 * Joins all threads before returning
		 */
		virtual ~CommandableFragmentGenerator();


		/**
		 * \brief Join any data-taking threads. Should be called when destructing CommandableFragmentGenerator
		 *
		 * Join any data-taking threads. Should be called when destructing CommandableFragmentGenerator
		 * Sets flags so that threads stop operations.
		 */
		void joinThreads();

		/**
		 * \brief getNext calls either applyRequests or getNext_ to get any data that is ready to be sent to the EventBuilders
		 * \param output FragmentPtrs object containing Fragments ready for transmission
		 * \return Whether getNext completed without exceptions
		 */
		bool getNext(FragmentPtrs& output) override final;


		/// <summary>
		/// Create fragments using data buffer for request mode Ignored.
		/// Precondition: dataBufferMutex_ and request_mutex_ are locked
		/// </summary>
		/// <param name="frags">Ouput fragments</param>
		void applyRequestsIgnoredMode(artdaq::FragmentPtrs& frags);

		/// <summary>
		/// Create fragments using data buffer for request mode Single.
		/// Precondition: dataBufferMutex_ and request_mutex_ are locked
		/// </summary>
		/// <param name="frags">Ouput fragments</param>
		void applyRequestsSingleMode(artdaq::FragmentPtrs& frags);

		/// <summary>
		/// Create fragments using data buffer for request mode Buffer.
		/// Precondition: dataBufferMutex_ and request_mutex_ are locked
		/// </summary>
		/// <param name="frags">Ouput fragments</param>
		void applyRequestsBufferMode(artdaq::FragmentPtrs& frags);

		/// <summary>
		/// Create fragments using data buffer for request mode Window.
		/// Precondition: dataBufferMutex_ and request_mutex_ are locked
		/// </summary>
		/// <param name="frags">Ouput fragments</param>
		void applyRequestsWindowMode(artdaq::FragmentPtrs& frags);

		/**
		 * \brief See if any requests have been received, and add the corresponding data Fragment objects to the output list
		 * \param[out] output list of FragmentPtr objects ready for transmission
		 * \return True if not stopped
		 */
		bool applyRequests(FragmentPtrs& output);

		/**
		 * \brief Send an EmptyFragmentType Fragment
		 * \param[out] frags Output list to append EmptyFragmentType to
		 * \param sequenceId Sequence ID of Empty Fragment
		 * \param desc Message to log with reasoning for sending Empty Fragment
		 * \return True if no exceptions
		 */
		bool sendEmptyFragment(FragmentPtrs& frags, size_t sequenceId, std::string desc);

		/**
		 * \brief This function is for Buffered and Single request modes, as they can only respond to one data request at a time
		 * If the request message seqID > ev_counter, simply send empties until they're equal
		 * \param[out] frags Output list to append EmptyFragmentType to
		 * \param requests List of requests to process
		 */
		void sendEmptyFragments(FragmentPtrs& frags, std::map<Fragment::sequence_id_t, Fragment::timestamp_t>& requests);

		/**
		 * \brief Check the windows_sent_ooo_ map for sequence IDs that may be removed
		 * \param seq Sequence ID of current window
		 */
		void checkOutOfOrderWindows(Fragment::sequence_id_t seq);

		/**
		 * \brief Access the windows_sent_ooo_ map
		 * \return windows_sent_ooo_ map
		 */
		std::map<Fragment::sequence_id_t, std::chrono::steady_clock::time_point> getOutOfOrderWindowList() const
		{
			return windows_sent_ooo_;
		}

		/**
		 * \brief Function that launches the data thread (getDataLoop())
		 */
		void startDataThread();

		/**
		 * \brief Function that launches the monitoring thread (getMonitoringDataLoop())
		 */
		void startMonitoringThread();

		/**
		 * \brief When separate_data_thread is set to true, this loop repeatedly calls getNext_ and adds returned Fragment
		 * objects to the data buffer, blocking when the data buffer is full.
		 */
		void getDataLoop();

		/**
		 * \brief Wait for the data buffer to drain (dataBufferIsTooLarge returns false), periodically reporting status.
		 * \return True if wait ended without something else disrupting the run
		 */
		bool waitForDataBufferReady();

		/**
		 * \brief Test the configured constraints on the data buffer
		 * \return Whether the data buffer is full
		 */
		bool dataBufferIsTooLarge();

		/**
		 * \brief Calculate the size of the dataBuffer and report appropriate metrics
		 */
		void getDataBufferStats();

		/**
		 * \brief Perform data buffer pruning operations. If the RequestMode is Single, removes all but the latest Fragment from the data buffer.
		 * In Window and Buffer RequestModes, this function discards the oldest Fragment objects until the data buffer is below its size constraints,
		 * then also checks for stale Fragments, based on the timestamp of the most recent Fragment.
		 */
		void checkDataBuffer();

		/**
		 * \brief This function regularly calls checkHWStatus_(), and sets the isHardwareOK flag accordingly.
		 */
		void getMonitoringDataLoop();

		/**
		 * \brief Get the list of Fragment IDs handled by this CommandableFragmentGenerator
		 * \return A std::vector<Fragment::fragment_id_t> containing the Fragment IDs handled by this CommandableFragmentGenerator
		 */
		std::vector<Fragment::fragment_id_t> fragmentIDs() override
		{
			return fragment_ids_;
		}

		/**
		* \brief Get the current value of the event counter
		* \return The current value of the event counter
		*/
		size_t ev_counter() const { return ev_counter_.load(); }

		/// <summary>
		/// Get the current request mode of the CommandableFragmentGenerator
		/// </summary>
		/// <returns>Current RequestMode of the CFG</returns>
		RequestMode request_mode() const { return mode_; }

		//
		// State-machine related interface below.
		//

		/**
		 * \brief Start the CommandableFragmentGenerator
		 * \param run Run ID of the new run
		 * \param timeout Timeout for transition
		 * \param timestamp Timestamp of transition
		 *
		 * After a call to 'StartCmd', all Fragments returned by getNext()
		 * will be marked as part of a Run with the given run number, and
		 * with subrun number 1. Calling StartCmd also resets the event
		 * number to 1.  After a call to StartCmd(), and until a call to
		 * StopCmd, getNext() -- and hence the virtual function it calls,
		 * getNext_() -- should return true as long as datataking is meant
		 * to take place, even if a particular call returns no fragments.
		 */
		void StartCmd(int run, uint64_t timeout, uint64_t timestamp);

		/**
		 * \brief Stop the CommandableFragmentGenerator
		 * \param timeout Timeout for transition
		 * \param timestamp Timestamp of transition
		 *
		 * After a call to StopCmd(), getNext() will eventually return
		 * false. This may not happen for several calls, if the
		 * implementation has data to be 'drained' from the system.
		 */
		void StopCmd(uint64_t timeout, uint64_t timestamp);

		/**
		 * \brief Pause the CommandableFragmentGenerator
		 * \param timeout Timeout for transition
		 * \param timestamp Timestamp of transition
		 *
		 * A call to PauseCmd() is advisory. It is an indication that the
		 * BoardReader should stop the incoming flow of data, if it can do
		 * so.
		 */
		void PauseCmd(uint64_t timeout, uint64_t timestamp);

		/**
		 * \brief Resume the CommandableFragmentGenerator
		 * \param timeout Timeout for transition
		 * \param timestamp Timestamp of transition
		 *
		 * After a call to ResumeCmd(), the next Fragments returned from
		 * getNext() will be part of a new SubRun.
		 */
		void ResumeCmd(uint64_t timeout, uint64_t timestamp);

		/**
		 * \brief Get a report about a user-specified run-time quantity
		 * \param which Which quantity to report
		 * \return The report about the specified quantity
		 *
		 * CommandableFragmentGenerator only implements "latest_exception",
		 * a report on the last exception received. However, child classes
		 * can override the reportSpecific function to provide additional
		 * reports.
		 */
		std::string ReportCmd(std::string const& which = "");

		/**
		 * \brief Get the name used when reporting metrics
		 * \return The name used when reporting metrics
		 */
		virtual std::string metricsReportingInstanceName() const
		{
			return instance_name_for_metrics_;
		}

		// The following functions are not yet implemented, and their
		// signatures may be subject to change.

		// John F., 12/6/13 -- do we want Reset and Shutdown commands?
		// Kurt B., 15-Feb-2014. For the moment, I suspect that we don't
		// want a Shutdown command. FragmentGenerator instances are 
		// Constructed at Initialization time, and they are destructed
		// at Shutdown time. So, any shutdown operations that need to be
		// done should be put in the FragmentGenerator child class
		// destructors. If we find that want shutdown (or initialization)
		// operations that are different from destruction (construction),
		// then we'll have to add InitCmd and ShutdownCmd methods.

		//    virtual void ResetCmd() final {}
		//    virtual void ShutdownCmd() final {}

		/**
		 * \brief Get the current value of the exception flag
		 * \return The current value of the exception flag
		 */
		bool exception() const { return exception_.load(); }


		/**
		* \brief The meta-command is used for implementing user-specific commands in a CommandableFragmentGenerator
		* \param command Name of the command to run
		* \param arg Argument(s) for command
		* \return true if command succeeded or if command not supported
		*/
		virtual bool metaCommand(std::string const& command, std::string const& arg);

	protected:

		// John F., 12/6/13 -- need to figure out which of these getter
		// functions should be promoted to "public"

		// John F., 1/21/15 -- after more than a year, there hasn't been a
		// single complaint that a CommandableFragmentGenerator-derived
		// class hasn't allowed its users to access these quantities, so
		// they're probably fine as is

		/**
		 * \brief Get the current Run number
		 * \return The current Run number
		 */
		int run_number() const { return run_number_; }
		/**
		 * \brief Get the current Subrun number
		 * \return The current Subrun number
		 */
		int subrun_number() const { return subrun_number_; }
		/**
		 * \brief Timeout of last command
		 * \return Timeout of last command
		 */
		uint64_t timeout() const { return timeout_; }
		/**
		 * \brief Timestamp of last command
		 * \return Timestamp of last command
		 */
		uint64_t timestamp() const { return timestamp_; }

		/**
		 * \brief Get the current value of the should_stop flag
		 * \return The current value of the should_stop flag
		 */
		bool should_stop() const { return should_stop_.load(); }

		/**
		 * \brief Routine used by applyRequests to make sure that all outstanding requests have been fulfilled before returning
		 * \return The logical AND of should_stop, mode is not Ignored, and requests list size equal to 0
		 */
		bool check_stop();

		/**
		 * \brief Gets the current board_id
		 * \return The current board_id
		 */
		int board_id() const { return board_id_; }

		/**
		 * \brief Get the current Fragment ID, if there is only one
		 * \return The current fragment ID, if the re is only one
		 * \exception cet::exception if the Fragment IDs list has more than one member
		 */
		int fragment_id() const;

		/**
		 * \brief Increment the event counter, if the current RequestMode allows it
		 * \param step Amount to increment the event counter by
		 * \param force Force incrementing the event Counter
		 * \return The previous value of the event counter
		 */
		size_t ev_counter_inc(size_t step = 1, bool force = false); // returns the prev value

		/**
		 * \brief Control the exception flag
		 * \param exception Whether an excpetion has occurred
		 */
		void set_exception(bool exception) { exception_.store(exception); }

		/**
		 * \brief Sets the name for metrics reporting
		 * \param name The new name for metrics reporting
		 */
		void metricsReportingInstanceName(std::string const& name)
		{
			instance_name_for_metrics_ = name;
		}

		/**
		 * \brief Return the string representation of the current RequestMode
		 * \return The string representation of the current RequestMode
		 */
		std::string printMode_();

		// John F., 12/10/13 
		// Is there a better way to handle mutex_ than leaving it a protected variable?

		// John F., 1/21/15
		// Translation above is "should mutex_ be a private variable,
		// accessible via a getter function". Probably, but at this point
		// it's not worth breaking code by implementing this. 

		std::mutex mutex_; ///< Mutex used to ensure that multiple transition commands do not run at the same time

	private:
		// FHiCL-configurable variables. Note that the C++ variable names
		// are the FHiCL variable names with a "_" appended

		//Socket parameters
		std::unique_ptr<RequestReceiver> requestReceiver_;

		RequestMode mode_;
		Fragment::timestamp_t windowOffset_;
		Fragment::timestamp_t windowWidth_;
		Fragment::timestamp_t staleTimeout_;
		Fragment::type_t expectedType_;
		size_t maxFragmentCount_;
		bool uniqueWindows_;
		std::map<Fragment::sequence_id_t, std::chrono::steady_clock::time_point> windows_sent_ooo_;
		size_t missing_request_window_timeout_us_;
		size_t window_close_timeout_us_;

		bool useDataThread_;
		bool circularDataBufferMode_;
		size_t sleep_on_no_data_us_;
		std::atomic<bool> data_thread_running_;
		boost::thread dataThread_;

		std::condition_variable dataCondition_;
		std::atomic<int> dataBufferDepthFragments_;
		std::atomic<size_t> dataBufferDepthBytes_;
		int maxDataBufferDepthFragments_;
		size_t maxDataBufferDepthBytes_;

		bool useMonitoringThread_;
		boost::thread monitoringThread_;
		int64_t monitoringInterval_; // Microseconds
		std::chrono::steady_clock::time_point lastMonitoringCall_;
		std::atomic<bool> isHardwareOK_;

		FragmentPtrs dataBuffer_;
		FragmentPtrs newDataBuffer_;
		std::mutex dataBufferMutex_;

		std::vector<artdaq::Fragment::fragment_id_t> fragment_ids_;

		// In order to support the state-machine related behavior, all
		// CommandableFragmentGenerators must be able to remember a run number and a
		// subrun number.
		int run_number_, subrun_number_;

		// JCF, 8/28/14

		// Provide a user-adjustable timeout for the start transition
		uint64_t timeout_;

		// JCF, 8/21/14

		// In response to a need to synchronize various components using
		// different fragment generators in an experiment, keep a record
		// of a timestamp (see Redmine Issue #6783 for more)

		uint64_t timestamp_;

		std::atomic<bool> should_stop_, exception_, force_stop_;
		std::string latest_exception_report_;
		std::atomic<size_t> ev_counter_;

		int board_id_;
		std::string instance_name_for_metrics_;

		// Depending on what sleep_on_stop_us_ is set to, this gives the
		// stopping thread the chance to gather the required lock

		int sleep_on_stop_us_;

	protected:

		/// <summary>
		/// Obtain the next group of Fragments, if any are available. Return false if readout cannot continue, 
		/// if we are 'stopped', or if we are not running in state-machine mode. 
		/// Note that getNext_() must return n of each fragmentID declared by fragmentIDs_().
		/// </summary>
		/// <param name="output">Reference to list of Fragment pointers to which additional Fragments should be added</param>
		/// <returns>True if readout should continue, false otherwise</returns>
		virtual bool getNext_(FragmentPtrs& output) = 0;


		/// <summary>
		/// Check any relavent hardware status registers. Return false if
		/// an error condition exists that should halt data-taking.
		/// This function should probably make MetricManager calls.
		/// </summary>
		/// <returns>False if a condition exists that should halt all data-taking, true otherwise</returns>
		virtual bool checkHWStatus_();

		//
		// State-machine related implementor interface below.
		//

		/// <summary>
		/// If a CommandableFragmentGenerator subclass is reading from a
		/// file, and start() is called, any run-, subrun-, and
		/// event-numbers in the data read from the file must be
		/// over-written by the specified run number, etc. After a call to
		/// StartCmd(), and until a call to StopCmd(), getNext_() is
		/// expected to return true as long as datataking is intended.
		/// 
		/// This is a pure virtual function, and must be overriden by Fragment Generator implementations
		/// </summary>
		virtual void start() = 0;

		/// <summary>
		/// On call to StopCmd, stopNoMutex() is called prior to StopCmd
		/// acquiring the mutex
		/// 
		/// This is a pure virtual function, and must be overriden by Fragment Generator implementations
		/// </summary>
		virtual void stopNoMutex() = 0;

		/// <summary>
		/// If a CommandableFragmentGenerator subclass is reading from a file, calling
		/// stop() should arrange that the next call to getNext_() returns
		/// false, rather than allowing getNext_() to read to the end of the
		/// file.
		/// 
		/// This is a pure virtual function, and must be overriden by Fragment Generator implementations
		/// </summary>
		virtual void stop() = 0;

		/// <summary>
		/// On call to PauseCmd, pauseNoMutex() is called prior to PauseCmd
		/// acquiring the mutex
		/// </summary>
		virtual void pauseNoMutex();

		/// <summary>
		/// If a CommandableFragmentGenerator subclass is reading from hardware, the
		/// implementation of pause() should tell the hardware to stop
		/// sending data.
		/// </summary>
		virtual void pause();

		/// <summary>
		/// The subrun number will be incremented *before* a call to resume.
		/// </summary>
		virtual void resume();

		/// <summary>
		/// Let's say that the contract with the report() functions is that they
		/// return a non-empty string if they have something useful to report,
		/// but if they don't know how to handle a given request, they simply
		/// return an empty string and the ReportCmd() takes care of saying
		/// "the xyz command is not currently supported".
		/// For backward compatibility, we keep the report function that takes
		/// no arguments and add one that takes a "which" argument. In the 
		/// ReportCmd function, we'll call the more specific one first.
		/// </summary>
		/// <returns>Default report from the FragmentGenerator</returns>
		virtual std::string report();

		/// <summary>
		/// Report the status of a specific quantity
		/// </summary>
		/// <param name="what">Name of the quantity to report</param>
		/// <returns>Value of requested quantity. Null string if what is unsupported.</returns>
		virtual std::string reportSpecific(std::string const& what);

	};
}

#endif /* artdaq_Application_CommandableFragmentGenerator_hh */
