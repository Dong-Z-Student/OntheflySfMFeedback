#pragma once

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

class TimerProfiler
{
public:
	using Clock = std::chrono::steady_clock;

	struct TimerEntry
	{
		bool running = false;
		Clock::time_point startTime{};
		std::uint64_t totalMicroseconds = 0;
		std::uint64_t callCount = 0;
	};

public:
	void Start(const std::string& name)
	{
		auto& entry = m_timers[name];
		if (entry.running)
		{
			return;
		}

		entry.running = true;
		entry.startTime = Clock::now();
	}

	void Stop(const std::string& name)
	{
		auto it = m_timers.find(name);
		if (it == m_timers.end())
		{
			return;
		}

		TimerEntry& entry = it->second;
		if (!entry.running)
		{
			return;
		}

		const auto endTime = Clock::now();
		const auto elapsedUs =
			std::chrono::duration_cast<std::chrono::microseconds>(endTime - entry.startTime).count();

		entry.totalMicroseconds += static_cast<std::uint64_t>(elapsedUs);
		entry.callCount += 1;
		entry.running = false;
	}

	void Reset()
	{
		m_timers.clear();
	}

	void Print() const
	{
		std::cout << "================ TimerProfiler ================\n";
		for (const auto& kv : m_timers)
		{
			const std::string& name = kv.first;
			const TimerEntry& entry = kv.second;

			const double totalMs = static_cast<double>(entry.totalMicroseconds) / 1000.0;
			const double avgMs = (entry.callCount > 0)
				? totalMs / static_cast<double>(entry.callCount)
				: 0.0;

			std::cout
				<< std::left << std::setw(24) << name
				<< " total: " << std::setw(10) << totalMs << " ms"
				<< " count: " << std::setw(8) << entry.callCount
				<< " avg: " << avgMs << " ms";

			if (entry.running)
			{
				std::cout << "  [RUNNING]";
			}

			std::cout << "\n";
		}
		std::cout << "===============================================\n";
	}

private:
	std::unordered_map<std::string, TimerEntry> m_timers;
};