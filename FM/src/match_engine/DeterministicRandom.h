#pragma once

#include<cstdint>

inline std::uint64_t matchEngineMix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

inline double matchEngineDeterministicUnitInterval(std::uint64_t seed) {
    constexpr double denominator = static_cast<double>(1ULL << 53);
    return static_cast<double>(matchEngineMix64(seed) >> 11) / denominator;
}
