#ifndef PROFILE_H
#define PROFILE_H
#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>

//===========================================================================//
// Simple duration logger
//---------------------------------------------------------------------------//
class LogDuration
{
public:
    explicit LogDuration(const std::string& msg = "")
        : message(msg),
          start(std::chrono::steady_clock::now())
    {
    }

    ~LogDuration()
    {
        auto finish = std::chrono::steady_clock::now();
        auto dur = finish - start;
        std::cerr << message << ": "
           << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count()
           << " ms" << std::endl;
    }

private:
    std::string message;
    std::chrono::steady_clock::time_point start;
};

//---------------------------------------------------------------------------//
#define UNIQ_ID_IMPL(lineno) _a_local_var_##lineno
#define UNIQ_ID(lineno) UNIQ_ID_IMPL(lineno)

#define LOG_DURATION(message) \
  LogDuration UNIQ_ID(__LINE__){message};

//===========================================================================//
// Simple profiler
//---------------------------------------------------------------------------//
class Profiler
{
public:
    static void addBlockDuration(const std::string& blockName,
                                 std::chrono::steady_clock::duration duration)
    {
        auto foundBlockDur = m_blockDuration.find(blockName);

        if (foundBlockDur != m_blockDuration.end())
        {
            foundBlockDur->second += duration;
        }
        else
        {
            m_blockDuration.insert({blockName, duration});
        }
    }

    static void printAllDurations(std::ostream& os)
    {
        for (auto it : m_blockDuration)
        {
            os << it.first << ": "
               << std::chrono::duration_cast<std::chrono::milliseconds>(it.second).count()
               << " ms" << std::endl;
        }
    }
private:
    static std::unordered_map<std::string, std::chrono::steady_clock::duration> m_blockDuration;
};

//===========================================================================//
// Accumulates duration of a block
//---------------------------------------------------------------------------//
class DurationAccumulator
{
public:
    DurationAccumulator(const std::string& func,
                        const std::string& msg = "")
        : m_name(func),
          m_start(std::chrono::steady_clock::now())
    {
        if (!msg.empty())
        {
            m_name += '_';
            m_name += msg;
        }
    }

    ~DurationAccumulator()
    {
        Profiler::addBlockDuration(m_name, std::chrono::steady_clock::now() - m_start);
    }

private:
    std::string m_name;
    std::chrono::steady_clock::time_point m_start;
};

//---------------------------------------------------------------------------//
#define DUR_ACCUM(message) \
  DurationAccumulator UNIQ_ID(__LINE__){__func__, message};

#endif // PROFILE_H
