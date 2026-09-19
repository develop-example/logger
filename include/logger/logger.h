#pragma once

namespace logger
{

inline constexpr int kVersionMajor = 0;
inline constexpr int kVersionMinor = 1;
inline constexpr int kVersionPatch = 0;

// Stage 0 exposes only the library health/version entry point. Logging APIs
// are added incrementally in the following stages.
const char* version() noexcept;

}  // namespace logger
