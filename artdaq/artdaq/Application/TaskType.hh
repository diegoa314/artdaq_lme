#ifndef artdaq_Application_TaskType_hh
#define artdaq_Application_TaskType_hh

#include "artdaq/DAQdata/Globals.hh"

/**
 * \brief The artdaq namespace
 */
namespace artdaq
{
	/**
	 * \brief The artdaq::detail namespace contains internal implementation details for some classes
	 */
	namespace detail
	{
		/**
		 * \brief The types of applications in artdaq
		 */
		enum TaskType : int
		{
			BoardReaderTask = 1,
			EventBuilderTask = 2,
			DataLoggerTask = 3,
			DispatcherTask = 4,
			RoutingMasterTask = 5,
			UnknownTask
		};

		/// <summary>
		/// Convert a string to a TaskType (Used by artdaq.cc)
		/// </summary>
		/// <param name="task">Name of the task</param>
		/// <returns>Corresponding TaskType or TaskType::UnknownTask if no match</returns>
		TaskType StringToTaskType(std::string const& task)
		{
			if (task.size() < 1) return TaskType::UnknownTask;
			if (task[0] == 'b' || task[0] == 'B') return TaskType::BoardReaderTask;
			if (task[0] == 'e' || task[0] == 'E') return TaskType::EventBuilderTask;
			if (task[0] == 'r' || task[0] == 'R') return TaskType::RoutingMasterTask;
			if (task[0] == 'd' || task[0] == 'D') {
				if (task.size() < 2) return TaskType::UnknownTask;
				if (task[1] == 'a' || task[1] == 'A') return TaskType::DataLoggerTask;
				if (task[1] == 'i' || task[1] == 'I') return TaskType::DispatcherTask;
			}

			return TaskType::UnknownTask;
		}

		/// <summary>
		/// Convert an integer to the corresponding TaskType
		/// </summary>
		/// <param name="task">Enumeration identifier of Task</param>
		/// <returns>Corresponding TaskType or TaskType::UnknownTask if no match</returns>
		TaskType IntToTaskType(int const& task)
		{
			if (task > 0 && task <= 5)
				return static_cast<TaskType>(task);

			return TaskType::UnknownTask;
		}

		/// <summary>
		/// Convert a TaskType to string representation
		/// </summary>
		/// <param name="task">TaskType to convert</param>
		/// <returns>String represenation of Task name</returns>
		std::string TaskTypeToString(TaskType const& task)
		{
			switch (task)
			{
			case(BoardReaderTask):
				return "BoardReader";
			case(EventBuilderTask):
				return "EventBuilder";
			case(DataLoggerTask):
				return "DataLogger";
			case(DispatcherTask):
				return "Dispatcher";
			case(RoutingMasterTask):
				return "RoutingMaster";
			default:
				break;
			}
			return "Unknown";
		}


		inline TraceStreamer& operator<<(TraceStreamer& x, TaskType r)
		{
			return x << TaskTypeToString(r);
		}
	}

	using detail::TaskType; // Require C++2011 scoping, hide C++03 scoping.
}

#endif /* artdaq_Application_TaskType_hh */
