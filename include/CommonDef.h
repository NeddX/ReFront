#ifndef CMM_COMMON_DEFINITIONS
#define CMM_COMMON_DEFINITIONS

#include <cstdint>
#include <cstddef>

namespace cmm {
    using usize       = std::size_t;
    using intptr      = std::intptr_t;
    using uintptr     = std::uintptr_t;
    using i8          = std::int8_t;
    using i16         = std::int16_t;
    using i32         = std::int32_t;
    using i64         = std::int64_t;
    using u8          = std::uint8_t;
    using u16         = std::uint16_t;
    using u32         = std::uint32_t;
    using u64         = std::uint64_t;
    using f32         = float;
    using f64         = double;
    using f128        = long double;
} // namespace cmm

#endif // CMM_COMMON_DEFINITIONS
