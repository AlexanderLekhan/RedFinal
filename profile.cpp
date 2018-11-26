#include "profile.h"

std::map<std::string, std::chrono::steady_clock::duration> Profiler::m_blockDuration;
