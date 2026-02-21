#pragma once
//Credits to The Cherno (Hazel Engine)
//https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Debug/Instrumentor.h


#include "Log.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <fstream>
#include <iomanip>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Syndra {

	using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

	struct ProfileResult
	{
		std::string Name;

		FloatingPointMicroseconds Start;
		std::chrono::microseconds ElapsedTime;
		std::thread::id ThreadID;
	};

	struct CpuProfileNode
	{
		std::string Name;
		double TotalTimeMs = 0.0;
		double SelfTimeMs = 0.0;
		uint32_t CallCount = 0;
		std::vector<CpuProfileNode> Children;
	};

	struct CpuFrameProfile
	{
		CpuProfileNode Root;
		double FrameTimeMs = 0.0;
		uint64_t FrameIndex = 0;
		bool Valid = false;
	};

	struct InstrumentationSession
	{
		std::string Name;
	};

	class Instrumentor
	{
	public:
		Instrumentor(const Instrumentor&) = delete;
		Instrumentor(Instrumentor&&) = delete;

		void BeginSession(const std::string& name, const std::string& filepath = "results.json")
		{
			std::lock_guard lock(m_Mutex);
			ResetCpuFrameState();
			m_LatestCpuFrame = {};
			m_CpuFrameHistory.clear();
			m_CpuFrameCounter = 0;

			if (m_CurrentSession)
			{
				// If there is already a current session, then close it before beginning new one.
				// Subsequent profiling output meant for the original session will end up in the
				// newly opened session instead.  That's better than having badly formatted
				// profiling output.
				if (Syndra::Log::GetCoreLogger()) // Edge case: BeginSession() might be before Log::Init()
				{
					SN_CORE_ERROR("Instrumentor::BeginSession('{0}') when session '{1}' already open.", name, m_CurrentSession->Name);
				}
				InternalEndSession();
			}
			m_OutputStream.open(filepath);

			if (m_OutputStream.is_open())
			{
				m_CurrentSession = new InstrumentationSession({ name });
				WriteHeader();
			}
			else
			{
				if (Syndra::Log::GetCoreLogger()) // Edge case: BeginSession() might be before Log::Init()
				{
					SN_CORE_ERROR("Instrumentor could not open results file '{0}'.", filepath);
				}
			}
		}

		void EndSession()
		{
			std::lock_guard lock(m_Mutex);
			InternalEndSession();
		}

		void WriteProfile(const ProfileResult& result)
		{
			std::stringstream json;

			json << std::setprecision(3) << std::fixed;
			json << ",{";
			json << "\"cat\":\"function\",";
			json << "\"dur\":" << (result.ElapsedTime.count()) << ',';
			json << "\"name\":\"" << result.Name << "\",";
			json << "\"ph\":\"X\",";
			json << "\"pid\":0,";
			json << "\"tid\":" << result.ThreadID << ",";
			json << "\"ts\":" << result.Start.count();
			json << "}";

			std::lock_guard lock(m_Mutex);
			if (m_CurrentSession)
			{
				m_OutputStream << json.str();
				m_OutputStream.flush();
			}
		}

		void BeginCpuScope(const char* name, std::thread::id threadID)
		{
			if (name == nullptr || threadID != m_MainThreadID)
				return;

			std::lock_guard lock(m_Mutex);

			if (std::strcmp(name, "Frame") == 0 && m_CpuScopeStack.empty())
				ResetCpuFrameState();

			CpuProfileNodeAccum* parent = m_CpuScopeStack.empty() ? &m_CpuRoot : m_CpuScopeStack.back();
			CpuProfileNodeAccum* child = GetOrCreateChildNode(parent, name);
			if (child != nullptr)
				m_CpuScopeStack.push_back(child);
		}

		void EndCpuScope(const char* name, std::chrono::microseconds elapsedTime, std::thread::id threadID)
		{
			if (name == nullptr || threadID != m_MainThreadID)
				return;

			std::lock_guard lock(m_Mutex);
			if (m_CpuScopeStack.empty())
				return;

			CpuProfileNodeAccum* node = m_CpuScopeStack.back();
			m_CpuScopeStack.pop_back();

			const double elapsedMs = static_cast<double>(elapsedTime.count()) * 0.001;
			node->TotalTimeMs += elapsedMs;
			node->SelfTimeMs += elapsedMs;
			++node->CallCount;

			if (!m_CpuScopeStack.empty())
				m_CpuScopeStack.back()->SelfTimeMs -= elapsedMs;

			if (std::strcmp(name, "Frame") == 0 && m_CpuScopeStack.empty())
			{
				m_LatestCpuFrame = {};
				m_LatestCpuFrame.Root = MakeCpuSnapshot(*node);
				m_LatestCpuFrame.FrameTimeMs = node->TotalTimeMs;
				m_LatestCpuFrame.FrameIndex = ++m_CpuFrameCounter;
				m_LatestCpuFrame.Valid = true;

				m_CpuFrameHistory.push_back(m_LatestCpuFrame);
				while (m_CpuFrameHistory.size() > kMaxCpuFrameHistory)
					m_CpuFrameHistory.pop_front();
			}
		}

		CpuFrameProfile GetLatestCpuFrameProfile()
		{
			std::lock_guard lock(m_Mutex);
			return m_LatestCpuFrame;
		}

		CpuFrameProfile GetAveragedCpuFrameProfile(size_t frameCount)
		{
			std::lock_guard lock(m_Mutex);

			if (m_CpuFrameHistory.empty())
				return {};

			const size_t availableFrameCount = m_CpuFrameHistory.size();
			const size_t sampleCount = std::min(
				(frameCount == 0) ? availableFrameCount : frameCount,
				availableFrameCount);

			if (sampleCount == 0)
				return {};

			const size_t firstSampleIndex = availableFrameCount - sampleCount;
			const CpuFrameProfile& referenceFrame = m_CpuFrameHistory.back();
			if (!referenceFrame.Valid)
				return {};

			std::unordered_map<std::string, CpuAggregateNode> aggregatedNodes;
			aggregatedNodes.reserve(256);

			double totalFrameTimeMs = 0.0;
			for (size_t i = firstSampleIndex; i < availableFrameCount; ++i)
			{
				const CpuFrameProfile& frame = m_CpuFrameHistory[i];
				if (!frame.Valid || frame.Root.Name.empty())
					continue;

				totalFrameTimeMs += frame.FrameTimeMs;
				AccumulateCpuNode(frame.Root, "", aggregatedNodes);
			}

			const std::string rootPath = referenceFrame.Root.Name;
			auto rootIt = aggregatedNodes.find(rootPath);
			if (rootIt == aggregatedNodes.end())
				return {};

			CpuFrameProfile averagedFrame{};
			averagedFrame.Root = BuildAveragedCpuTree(rootPath, aggregatedNodes, static_cast<double>(sampleCount));
			averagedFrame.FrameTimeMs = totalFrameTimeMs / static_cast<double>(sampleCount);
			averagedFrame.FrameIndex = referenceFrame.FrameIndex;
			averagedFrame.Valid = true;
			return averagedFrame;
		}

		static Instrumentor& Get()
		{
			static Instrumentor instance;
			return instance;
		}
	private:
		struct CpuAggregateNode
		{
			std::string Name;
			std::string ParentPath;
			double TotalTimeMs = 0.0;
			double SelfTimeMs = 0.0;
			double CallCount = 0.0;
		};

		struct CpuProfileNodeAccum
		{
			std::string Name;
			double TotalTimeMs = 0.0;
			double SelfTimeMs = 0.0;
			uint32_t CallCount = 0;
			std::vector<std::unique_ptr<CpuProfileNodeAccum>> Children;
			std::unordered_map<std::string, size_t> ChildIndexByName;
		};

		static void AccumulateCpuNode(
			const CpuProfileNode& node,
			const std::string& parentPath,
			std::unordered_map<std::string, CpuAggregateNode>& aggregatedNodes)
		{
			if (node.Name.empty())
				return;

			const std::string currentPath = parentPath.empty() ? node.Name : parentPath + "/" + node.Name;

			CpuAggregateNode& aggregate = aggregatedNodes[currentPath];
			if (aggregate.Name.empty())
			{
				aggregate.Name = node.Name;
				aggregate.ParentPath = parentPath;
			}

			aggregate.TotalTimeMs += node.TotalTimeMs;
			aggregate.SelfTimeMs += node.SelfTimeMs;
			aggregate.CallCount += static_cast<double>(node.CallCount);

			for (const CpuProfileNode& child : node.Children)
				AccumulateCpuNode(child, currentPath, aggregatedNodes);
		}

		static CpuProfileNode BuildAveragedCpuTree(
			const std::string& nodePath,
			const std::unordered_map<std::string, CpuAggregateNode>& aggregatedNodes,
			double divisor)
		{
			auto aggregateIt = aggregatedNodes.find(nodePath);
			if (aggregateIt == aggregatedNodes.end() || divisor <= 0.0)
				return {};

			const CpuAggregateNode& aggregate = aggregateIt->second;
			CpuProfileNode averagedNode{};
			averagedNode.Name = aggregate.Name;
			averagedNode.TotalTimeMs = aggregate.TotalTimeMs / divisor;
			averagedNode.SelfTimeMs = std::max(0.0, aggregate.SelfTimeMs / divisor);
			averagedNode.CallCount = static_cast<uint32_t>(std::round(std::max(0.0, aggregate.CallCount / divisor)));

			for (const auto& [childPath, childAggregate] : aggregatedNodes)
			{
				if (childAggregate.ParentPath != nodePath)
					continue;

				CpuProfileNode childNode = BuildAveragedCpuTree(childPath, aggregatedNodes, divisor);
				if (childNode.Name.empty())
					continue;
				averagedNode.Children.push_back(std::move(childNode));
			}

			std::sort(averagedNode.Children.begin(), averagedNode.Children.end(), [](const CpuProfileNode& a, const CpuProfileNode& b) {
				return a.TotalTimeMs > b.TotalTimeMs;
			});

			return averagedNode;
		}

		CpuProfileNodeAccum* GetOrCreateChildNode(CpuProfileNodeAccum* parent, const char* name)
		{
			if (parent == nullptr || name == nullptr)
				return nullptr;

			auto it = parent->ChildIndexByName.find(name);
			if (it != parent->ChildIndexByName.end())
				return parent->Children[it->second].get();

			CpuProfileNodeAccum childNode{};
			childNode.Name = name;
			parent->Children.push_back(std::make_unique<CpuProfileNodeAccum>(std::move(childNode)));
			const size_t childIndex = parent->Children.size() - 1;
			parent->ChildIndexByName[parent->Children[childIndex]->Name] = childIndex;
			return parent->Children[childIndex].get();
		}

		void ResetCpuFrameState()
		{
			m_CpuScopeStack.clear();
			m_CpuRoot.TotalTimeMs = 0.0;
			m_CpuRoot.SelfTimeMs = 0.0;
			m_CpuRoot.CallCount = 0;
			m_CpuRoot.Children.clear();
			m_CpuRoot.ChildIndexByName.clear();
		}

		static CpuProfileNode MakeCpuSnapshot(const CpuProfileNodeAccum& node)
		{
			CpuProfileNode snapshot{};
			snapshot.Name = node.Name;
			snapshot.TotalTimeMs = node.TotalTimeMs;
			snapshot.SelfTimeMs = std::max(0.0, node.SelfTimeMs);
			snapshot.CallCount = node.CallCount;
			snapshot.Children.reserve(node.Children.size());

			for (const auto& child : node.Children)
			{
				if (!child || child->CallCount == 0)
					continue;

				snapshot.Children.push_back(MakeCpuSnapshot(*child));
			}

			std::sort(snapshot.Children.begin(), snapshot.Children.end(), [](const CpuProfileNode& a, const CpuProfileNode& b) {
				return a.TotalTimeMs > b.TotalTimeMs;
			});

			return snapshot;
		}

		Instrumentor()
			: m_CurrentSession(nullptr), m_MainThreadID(std::this_thread::get_id())
		{
			m_CpuRoot.Name = "Root";
		}

		~Instrumentor()
		{
			EndSession();
		}

		void WriteHeader()
		{
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[{}";
			m_OutputStream.flush();
		}

		void WriteFooter()
		{
			m_OutputStream << "]}";
			m_OutputStream.flush();
		}

		// Note: you must already own lock on m_Mutex before
		// calling InternalEndSession()
		void InternalEndSession()
		{
			if (m_CurrentSession)
			{
				WriteFooter();
				m_OutputStream.close();
				delete m_CurrentSession;
				m_CurrentSession = nullptr;
			}
		}
	private:
		static constexpr size_t kMaxCpuFrameHistory = 240;
		std::mutex m_Mutex;
		InstrumentationSession* m_CurrentSession;
		std::ofstream m_OutputStream;
		std::thread::id m_MainThreadID;
		CpuProfileNodeAccum m_CpuRoot;
		std::vector<CpuProfileNodeAccum*> m_CpuScopeStack;
		CpuFrameProfile m_LatestCpuFrame;
		std::deque<CpuFrameProfile> m_CpuFrameHistory;
		uint64_t m_CpuFrameCounter = 0;
	};

	class InstrumentationTimer
	{
	public:
		InstrumentationTimer(const char* name)
			: m_Name(name), m_Stopped(false)
		{
			m_StartTimepoint = std::chrono::steady_clock::now();
			Instrumentor::Get().BeginCpuScope(m_Name, std::this_thread::get_id());
		}

		~InstrumentationTimer()
		{
			if (!m_Stopped)
				Stop();
		}

		void Stop()
		{
			auto endTimepoint = std::chrono::steady_clock::now();
			auto highResStart = FloatingPointMicroseconds{ m_StartTimepoint.time_since_epoch() };
			auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

			Instrumentor::Get().EndCpuScope(m_Name, elapsedTime, std::this_thread::get_id());
			Instrumentor::Get().WriteProfile({ m_Name, highResStart, elapsedTime, std::this_thread::get_id() });

			m_Stopped = true;
		}
	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
		bool m_Stopped;
	};

	namespace InstrumentorUtils {

		template <size_t N>
		struct ChangeResult
		{
			char Data[N];
		};

		template <size_t N, size_t K>
		constexpr auto CleanupOutputString(const char(&expr)[N], const char(&remove)[K])
		{
			ChangeResult<N> result = {};

			size_t srcIndex = 0;
			size_t dstIndex = 0;
			while (srcIndex < N)
			{
				size_t matchIndex = 0;
				while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 && expr[srcIndex + matchIndex] == remove[matchIndex])
					matchIndex++;
				if (matchIndex == K - 1)
					srcIndex += matchIndex;
				result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
				srcIndex++;
			}
			return result;
		}
	}
}
#if SN_DEBUG
	#define SN_PROFILE 1
#else
	#define SN_PROFILE 0
#endif

#if SN_PROFILE
// Resolve which function signature macro will be used. Note that this only
// is resolved when the (pre)compiler starts, so the syntax highlighting
// could mark the wrong one in your editor!
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define SN_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define SN_FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define SN_FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define SN_FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define SN_FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define SN_FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define SN_FUNC_SIG __func__
#else
#define SN_FUNC_SIG "SN_FUNC_SIG unknown!"
#endif

#define SN_PROFILE_BEGIN_SESSION(name, filepath) ::Syndra::Instrumentor::Get().BeginSession(name, filepath)
#define SN_PROFILE_END_SESSION() ::Syndra::Instrumentor::Get().EndSession()
#define SN_PROFILE_SCOPE_LINE2(name, line) constexpr auto fixedName##line = ::Syndra::InstrumentorUtils::CleanupOutputString(name, "__cdecl ");\
											   ::Syndra::InstrumentationTimer timer##line(fixedName##line.Data)
#define SN_PROFILE_SCOPE_LINE(name, line) SN_PROFILE_SCOPE_LINE2(name, line)
#define SN_PROFILE_SCOPE(name) SN_PROFILE_SCOPE_LINE(name, __LINE__)
#define SN_PROFILE_FUNCTION() SN_PROFILE_SCOPE(SN_FUNC_SIG)
#else
#define SN_PROFILE_BEGIN_SESSION(name, filepath)
#define SN_PROFILE_END_SESSION()
#define SN_PROFILE_SCOPE(name)
#define SN_PROFILE_FUNCTION()
#endif
