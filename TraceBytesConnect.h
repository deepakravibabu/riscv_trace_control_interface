# pragma once
#include <cstdint>
#include <vector>

using TraceBytes = std::vector<std::uint8_t>;

class TraceBytesConnect {
    public:
        virtual ~TraceBytesConnect() = default;
        virtual void pushBytes(const std::uint8_t* data, std::size_t n) = 0;

        // Convenience overload
        void pushBytes(const TraceBytes& b) { pushBytes(b.data(), b.size()); }
};