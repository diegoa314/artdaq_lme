#define TRACE_NAME "CommandableFragmentGenerator_t"

#define BOOST_TEST_MODULE CommandableFragmentGenerator_t
#include <boost/test/auto_unit_test.hpp>

#include "artdaq-core/Data/Fragment.hh"
#include "artdaq-core/Data/ContainerFragment.hh"
#include "artdaq/Application/CommandableFragmentGenerator.hh"
#include "artdaq/DAQrate/RequestSender.hh"

#define MULTICAST_MODE 0

namespace artdaqtest
{
	class CommandableFragmentGeneratorTest;
}

/**
 * \brief CommandableFragmentGenerator derived class for testing
 */
class artdaqtest::CommandableFragmentGeneratorTest :
	public artdaq::CommandableFragmentGenerator
{
public:
	/**
	 * \brief CommandableFragmentGeneratorTest Constructor
	 */
	explicit CommandableFragmentGeneratorTest(const fhicl::ParameterSet& ps);

	virtual ~CommandableFragmentGeneratorTest() { joinThreads(); };

protected:
	/**
	 * \brief Generate data and return it to CommandableFragmentGenerator
	 * \param frags FragmentPtrs list that new Fragments should be added to
	 * \return True if data was generated
	 *
	 * CommandableFragmentGeneratorTest merely default-constructs Fragments, emplacing them on the frags list.
	 */
	bool getNext_(artdaq::FragmentPtrs& frags) override;

	/**
	 * \brief Returns whether the hwFail flag has not been set
	 * \return If hwFail has been set, false, otherwise true
	 */
	bool checkHWStatus_() override { return !hwFail_.load(); }

	/**
	 * \brief Perform start actions. No-Op
	 */
	void start() override;

	/**
	 * \brief Perform immediate stop actions. No-Op
	 */
	void stopNoMutex() override;

	/**
	* \brief Perform stop actions. No-Op
	*/
	void stop() override;

	/**
	 * \brief Perform pause actions. No-Op
	 */
	void pause() override;

	/**
	 * \brief Perform resume actions. No-Op
	 */
	void resume() override;

public:
	/**
	 * \brief Have getNext_ generate count fragments
	 * \param count Number of fragments to generate
	 */
	void setFireCount(size_t count) { fireCount_ = count; }

	/**
	 * \brief Set the hwFail flag
	 */
	void setHwFail() { hwFail_ = true; }

	/// <summary>
	/// Wait for all fragments generated to be read by the CommandableFragmentGenerator
	/// </summary>
	void waitForFrags() { 
		auto start_time = std::chrono::steady_clock::now();
		while (fireCount_ > 0) { usleep(1000); } 
		TLOG(TLVL_INFO) << "Waited " << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count() << " us for events to be picked up by CFG" ;
	}
private:
	std::atomic<size_t> fireCount_;
	std::atomic<bool> hwFail_;
	artdaq::Fragment::timestamp_t ts_;
	std::atomic<bool> hw_stop_;
};

artdaqtest::CommandableFragmentGeneratorTest::CommandableFragmentGeneratorTest(const fhicl::ParameterSet& ps)
	: CommandableFragmentGenerator(ps)
	, fireCount_(1)
	, hwFail_(false)
	, ts_(0)
	, hw_stop_(false)
{
	metricMan->initialize(ps);
	metricMan->do_start();
}

bool
artdaqtest::CommandableFragmentGeneratorTest::getNext_(artdaq::FragmentPtrs& frags)
{
	while (fireCount_ > 0)
	{
		frags.emplace_back(new artdaq::Fragment(ev_counter(), fragment_id(), artdaq::Fragment::FirstUserFragmentType, ++ts_));
		fireCount_--;
	}

	return !hw_stop_;
}

void
artdaqtest::CommandableFragmentGeneratorTest::start() { hw_stop_ = false; }

void
artdaqtest::CommandableFragmentGeneratorTest::stopNoMutex() {}

void
artdaqtest::CommandableFragmentGeneratorTest::stop() { hw_stop_ = true; }

void
artdaqtest::CommandableFragmentGeneratorTest::pause() {}

void
artdaqtest::CommandableFragmentGeneratorTest::resume() {}

BOOST_AUTO_TEST_SUITE(CommandableFragmentGenerator_t)

BOOST_AUTO_TEST_CASE(Simple)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "Simple test case BEGIN" ;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	artdaqtest::CommandableFragmentGeneratorTest testGen(ps);
	artdaq::FragmentPtrs fps;
	auto sts = testGen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	TLOG(TLVL_INFO) << "Simple test case END" ;
}

BOOST_AUTO_TEST_CASE(IgnoreRequests)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "IgnoreRequests test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 1;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.29");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 0);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<std::string>("request_mode", "ignored");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);
	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);
	t.AddRequest(53, 35);

	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "IgnoreRequests test case END" ;
}

BOOST_AUTO_TEST_CASE(SingleMode)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "SingleMode test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.30");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 0);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<std::string>("request_mode", "single");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);
	t.AddRequest(1, 1);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);
	gen.waitForFrags();
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 1);

	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	auto type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 2);
	fps.clear();

	t.AddRequest(2, 5);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 5);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);
	fps.clear();

	gen.setFireCount(2);
	gen.waitForFrags();
	t.AddRequest(4, 7);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 5);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 2);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	auto ts = artdaq::Fragment::InvalidTimestamp;
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), ts);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 3);
	auto emptyType = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), emptyType);
	fps.pop_front();
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 7);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 4);
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	fps.clear();

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "SingleMode test case END" ;
}

BOOST_AUTO_TEST_CASE(BufferMode)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "BufferMode test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.31");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 0);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<std::string>("request_mode", "buffer");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);
	t.AddRequest(1, 1);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 1);


	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	auto type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	BOOST_REQUIRE_GE(fps.front()->sizeBytes(), 2 * sizeof(artdaq::detail::RawFragmentHeader) + sizeof(artdaq::ContainerFragment::Metadata));
	auto cf = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 2);
	fps.clear();

	t.AddRequest(2, 5);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 5);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf2 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf2.block_count(), 0);
	BOOST_REQUIRE_EQUAL(cf2.missing_data(), false);
	type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf2.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);
	fps.clear();

	gen.setFireCount(2);
	gen.waitForFrags();
	t.AddRequest(4, 7);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 2);

	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	auto ts = artdaq::Fragment::InvalidTimestamp;
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), ts);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 3);
	auto emptyType = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), emptyType);
	BOOST_REQUIRE_EQUAL(fps.front()->size(), artdaq::detail::RawFragmentHeader::num_words());
	fps.pop_front();
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 7);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 4);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf3 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf3.block_count(), 2);
	BOOST_REQUIRE_EQUAL(cf3.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf3.fragment_type(), type);
	fps.clear();
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 5);


	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();


	TLOG(TLVL_INFO) << "BufferMode test case END" ;
}

BOOST_AUTO_TEST_CASE(CircularBufferMode)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "CircularBufferMode test case BEGIN";
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.31");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 0);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("circular_buffer_mode", true);
	ps.put<int>("data_buffer_depth_fragments", 3);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<std::string>("request_mode", "buffer");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);
	t.AddRequest(1, 1);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 1);


	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	auto type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	BOOST_REQUIRE_GE(fps.front()->sizeBytes(), 2 * sizeof(artdaq::detail::RawFragmentHeader) + sizeof(artdaq::ContainerFragment::Metadata));
	auto cf = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 2);
	fps.clear();

	t.AddRequest(2, 5);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 5);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf2 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf2.block_count(), 0);
	BOOST_REQUIRE_EQUAL(cf2.missing_data(), false);
	type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf2.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);
	fps.clear();

	gen.setFireCount(3);
	gen.waitForFrags();
	t.AddRequest(4, 7);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 2);

	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	auto ts = artdaq::Fragment::InvalidTimestamp;
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), ts);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 3);
	auto emptyType = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), emptyType);
	BOOST_REQUIRE_EQUAL(fps.front()->size(), artdaq::detail::RawFragmentHeader::num_words());
	fps.pop_front();
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 7);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 4);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf3 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf3.block_count(), 3);
	BOOST_REQUIRE_EQUAL(cf3.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf3.fragment_type(), type);
	fps.clear();
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 5);



	gen.setFireCount(5);
	gen.waitForFrags();
	t.AddRequest(5, 8);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);

	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 8);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 5);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 3);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(cf4.at(0)->timestamp(), 7);
	BOOST_REQUIRE_EQUAL(cf4.at(1)->timestamp(), 8);
	BOOST_REQUIRE_EQUAL(cf4.at(2)->timestamp(), 9);
	fps.clear();
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 6);


	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();


	TLOG(TLVL_INFO) << "CircularBufferMode test case END";
}

BOOST_AUTO_TEST_CASE(WindowMode_Function)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_Function test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 0);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put<size_t>("missing_request_window_timeout_us", 500000);
	ps.put<size_t>("window_close_timeout_us", 500000);
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);
	t.AddRequest(1, 1);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 1);

	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 2);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	auto type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	BOOST_REQUIRE_GE(fps.front()->sizeBytes(), 2 * sizeof(artdaq::detail::RawFragmentHeader) + sizeof(artdaq::ContainerFragment::Metadata));
	auto cf = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf.fragment_type(), type);
	fps.clear();

	// No data for request
	t.AddRequest(2, 2);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 2);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	gen.setFireCount(1);
	gen.waitForFrags();
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 2);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf2 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf2.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf2.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf2.fragment_type(), type);
	fps.clear();

	// Request Timeout
	t.AddRequest(4, 3);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);


	usleep(1500000);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);

	// Also, missing request timeout
	auto list = gen.getOutOfOrderWindowList();
	BOOST_REQUIRE_EQUAL(list.size(), 1);
	BOOST_REQUIRE_EQUAL(list.begin()->first, 4);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);

	usleep(1500000);


	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 3);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 4);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf3 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf3.block_count(), 0);
	BOOST_REQUIRE_EQUAL(cf3.missing_data(), true);
	type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf3.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 3);
	fps.clear();

	// Data-taking has passed request
	gen.setFireCount(12);
	gen.waitForFrags();
	t.AddRequest(5, 4);
	list = gen.getOutOfOrderWindowList(); // Out-of-order list is only updated in getNext calls
	BOOST_REQUIRE_EQUAL(list.size(), 1);
	sts = gen.getNext(fps);
	list = gen.getOutOfOrderWindowList(); // Out-of-order list is only updated in getNext calls
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 6);
	BOOST_REQUIRE_EQUAL(list.size(), 0);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 4);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 5);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 0);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), true);
	type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);
	fps.clear();


	// Out-of-order windows
	t.AddRequest(7, 13);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 13);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 7);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf5 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf5.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf5.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf5.fragment_type(), type);
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 6);
	fps.clear();

	list = gen.getOutOfOrderWindowList();
	BOOST_REQUIRE_EQUAL(list.size(), 1);
	BOOST_REQUIRE_EQUAL(list.begin()->first, 7);

	t.AddRequest(6, 12);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 12);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 6);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf6 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf6.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf6.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf6.fragment_type(), type);
	fps.clear();
	BOOST_REQUIRE_EQUAL(gen.ev_counter(), 8);

	list = gen.getOutOfOrderWindowList();
	BOOST_REQUIRE_EQUAL(list.size(), 0);

	usleep(1500000);

	gen.StopCmd(0xFFFFFFFF, 1);
	TLOG(TLVL_INFO) << "WindowMode_Function test case END" ;
	gen.joinThreads();

}

// 1. Both start and end before any data in buffer  "RequestBeforeBuffer"
// 2. Start before buffer, end in buffer            "RequestStartsBeforeBuffer"
// 3. Start befoer buffer, end after buffer         "RequestOutsideBuffer"
// 4. Start and end in buffer                       "RequestInBuffer"
// 5. Start in buffer, end after buffer             "RequestEndsAfterBuffer"
// 6. Start and end after buffer                    "RequestAfterBuffer"
BOOST_AUTO_TEST_CASE(WindowMode_RequestBeforeBuffer)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_RequestBeforeBuffer test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 3);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	int sts;
	artdaq::Fragment::type_t type;

	// 1. Both start and end before any data in buffer
	//  -- Should return ContainerFragment with MissingData bit set and zero Fragments
	gen.setFireCount(9); // Buffer start is at ts 6, end at 10
	gen.waitForFrags();
	t.AddRequest(1, 1); // Requesting data from ts 1 to 3

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 0);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), true);
	type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);

	gen.StopCmd(0xFFFFFFFF, 1);
	TLOG(TLVL_INFO) << "WindowMode_RequestBeforeBuffer test case END" ;

}
BOOST_AUTO_TEST_CASE(WindowMode_RequestStartsBeforeBuffer)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_RequestStartsBeforeBuffer test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 3);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	int sts;
	artdaq::Fragment::type_t type;

	gen.waitForFrags();
	gen.setFireCount(9); // Buffer contains 6 to 10
	gen.waitForFrags();

	// 2. Start before buffer, end in buffer
	//  -- Should return ContainerFragment with MissingData bit set and one or more Fragments
	t.AddRequest(1, 4); // Requesting data from ts 4 to 6

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 4);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), true);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "WindowMode_RequestStartsBeforeBuffer test case END" ;

}
BOOST_AUTO_TEST_CASE(WindowMode_RequestOutsideBuffer)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_RequestOutsideBuffer test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 4);
	ps.put<size_t>("window_close_timeout_us", 500000);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	int sts;
	artdaq::Fragment::type_t type;

	gen.waitForFrags();
	gen.setFireCount(9); // Buffer contains 6 to 10
	gen.waitForFrags();

	// 3. Start before buffer, end after buffer
	//  -- Should not return until buffer passes end or timeout (check both cases), MissingData bit set

	t.AddRequest(1, 6); // Requesting data from ts 6 to 9, buffer will contain 10
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 6);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf.block_count(), 4);
	BOOST_REQUIRE_EQUAL(cf.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf.fragment_type(), type);
	fps.clear();

	t.AddRequest(2, 9); // Requesting data from ts 9 to 12
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	gen.setFireCount(3); // Buffer start is at ts 10, end at 13
	gen.waitForFrags();

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 9);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf2 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf2.block_count(), 3);
	BOOST_REQUIRE_EQUAL(cf2.missing_data(), true);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf2.fragment_type(), type);
	fps.clear();



	t.AddRequest(3, 12); // Requesting data from ts 11 to 14

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	usleep(550000);

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 12);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 3);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), true);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "WindowMode_RequestOutsideBuffer test case END" ;

}
BOOST_AUTO_TEST_CASE(WindowMode_RequestInBuffer)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_RequestInBuffer test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 3);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	int sts;
	artdaq::Fragment::type_t type;
	gen.waitForFrags();

	// 4. Start and end in buffer
	//  -- Should return ContainerFragment with one or more Fragments
	gen.setFireCount(5); // Buffer start is at ts 2, end at 6
	gen.waitForFrags();
	t.AddRequest(1, 3); // Requesting data from ts 3 to 5

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 3);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 3);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);

	gen.StopCmd(0xFFFFFFFF, 1);
	TLOG(TLVL_INFO) << "WindowMode_RequestInBuffer test case END" ;

}
BOOST_AUTO_TEST_CASE(WindowMode_RequestEndsAfterBuffer)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_RequestEndsAfterBuffer test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 3);
	ps.put<size_t>("window_close_timeout_us", 500000);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	int sts;
	artdaq::Fragment::type_t type;
	gen.waitForFrags();
	gen.setFireCount(5); // Buffer contains 2 to 6
	gen.waitForFrags();

		// 5. Start in buffer, end after buffer
		//  -- Should not return until buffer passes end or timeout (check both cases). MissingData bit set if timeout
	t.AddRequest(1, 5); // Requesting data from ts 5 to 7
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	gen.setFireCount(2); // Buffer contains 4 to 8
	gen.waitForFrags();
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 5);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf.block_count(), 3);
	BOOST_REQUIRE_EQUAL(cf.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf.fragment_type(), type);
	fps.clear();

	t.AddRequest(2, 8); // Requesting data from ts 8 to 10

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	usleep(550000);

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 8);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 1);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), true);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "WindowMode_RequestEndsAfterBuffer test case END" ;

}
BOOST_AUTO_TEST_CASE(WindowMode_RequestAfterBuffer)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "WindowMode_RequestAfterBuffer test case BEGIN" ;
	const int REQUEST_PORT = (seedAndRandom() % (32768 - 1024)) + 1024;
	const int DELAY_TIME = 100;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<int>("request_port", REQUEST_PORT);
#if MULTICAST_MODE
	ps.put<std::string>("request_address", "227.18.12.32");
#else
	ps.put<std::string>("request_address", "localhost");
#endif
	ps.put<artdaq::Fragment::timestamp_t>("request_window_offset", 0);
	ps.put<artdaq::Fragment::timestamp_t>("request_window_width", 3);
	ps.put<size_t>("window_close_timeout_us", 500000);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 0);
	ps.put<size_t>("data_buffer_depth_fragments", 5);
	ps.put<std::string>("request_mode", "window");
	ps.put("request_delay_ms", DELAY_TIME);
	ps.put("send_requests", true);

	artdaq::RequestSender t(ps);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	int sts;
	artdaq::Fragment::type_t type;
	gen.waitForFrags();

	// 6. Start and end after buffer
	//  -- Should not return until buffer passes end or timeout (check both cases). MissingData bit set if timeout
	gen.setFireCount(9); // Buffer start is 6, end at 10
	gen.waitForFrags();
	t.AddRequest(1, 11); // Requesting data from ts 11 to 13

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);
	gen.setFireCount(1); // Buffer start is 7, end at 11
	gen.waitForFrags();
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	gen.setFireCount(3); // Buffer start is 10, end at 14
	gen.waitForFrags();
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 11);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf.block_count(), 3);
	BOOST_REQUIRE_EQUAL(cf.missing_data(), false);
	type = artdaq::Fragment::FirstUserFragmentType;
	BOOST_REQUIRE_EQUAL(cf.fragment_type(), type);
	fps.clear();

	t.AddRequest(2, 16); // Requesting data from ts 15 to 17
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	usleep(550000);

	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 16);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 2);
	type = artdaq::Fragment::ContainerFragmentType;
	BOOST_REQUIRE_EQUAL(fps.front()->type(), type);
	auto cf4 = artdaq::ContainerFragment(*fps.front());
	BOOST_REQUIRE_EQUAL(cf4.block_count(), 0);
	BOOST_REQUIRE_EQUAL(cf4.missing_data(), true);
	type = artdaq::Fragment::EmptyFragmentType;
	BOOST_REQUIRE_EQUAL(cf4.fragment_type(), type);

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "WindowMode_RequestAfterBuffer test case END" ;

}

BOOST_AUTO_TEST_CASE(HardwareFailure_NonThreaded)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "HardwareFailure_NonThreaded test case BEGIN" ;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<bool>("separate_data_thread", false);
	ps.put<bool>("separate_monitoring_thread", false);
	ps.put<int64_t>("hardware_poll_interval_us", 10);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);

	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	fps.clear();

	gen.setFireCount(1);
	gen.setHwFail();
	usleep(10000);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, false);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "HardwareFailure_NonThreaded test case END" ;
}

BOOST_AUTO_TEST_CASE(HardwareFailure_Threaded)
{
	artdaq::configureMessageFacility("CommandableFragmentGenerator_t");
	TLOG(TLVL_INFO) << "HardwareFailure_Threaded test case BEGIN" ;
	fhicl::ParameterSet ps;
	ps.put<int>("board_id", 1);
	ps.put<int>("fragment_id", 1);
	ps.put<bool>("separate_data_thread", true);
	ps.put<bool>("separate_monitoring_thread", true);
	ps.put<int64_t>("hardware_poll_interval_us", 750000);

	artdaqtest::CommandableFragmentGeneratorTest gen(ps);
	gen.StartCmd(1, 0xFFFFFFFF, 1);



	artdaq::FragmentPtrs fps;
	auto sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, true);
	BOOST_REQUIRE_EQUAL(fps.size(), 1u);
	BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 1);
	BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	fps.clear();

	gen.setHwFail();
	//gen.setFireCount(1);
	//sts = gen.getNext(fps);
	//BOOST_REQUIRE_EQUAL(sts, true);
	//BOOST_REQUIRE_EQUAL(fps.size(), 1);
	//BOOST_REQUIRE_EQUAL(fps.front()->fragmentID(), 1);
	//BOOST_REQUIRE_EQUAL(fps.front()->timestamp(), 2);
	//BOOST_REQUIRE_EQUAL(fps.front()->sequenceID(), 1);
	//fps.clear();

	sleep(1);

	gen.setFireCount(1);
	sts = gen.getNext(fps);
	BOOST_REQUIRE_EQUAL(sts, false);
	BOOST_REQUIRE_EQUAL(fps.size(), 0);

	gen.StopCmd(0xFFFFFFFF, 1);
	gen.joinThreads();
	TLOG(TLVL_INFO) << "HardwareFailure_Threaded test case END" ;
}

BOOST_AUTO_TEST_SUITE_END()
