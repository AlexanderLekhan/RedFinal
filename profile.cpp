#include "profile.h"

std::unordered_map<std::string, std::chrono::steady_clock::duration> Profiler::m_blockDuration;
