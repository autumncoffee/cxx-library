#include <string>
#include <random>
#include <string.h>

namespace NAC {
    namespace NHTTPUtils {
        static inline std::string Boundary() {
            static const char* Chars("-_1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
            static const size_t CharsSize(strlen(Chars));
            static const size_t OutSize(40);

            std::string out;
            out.reserve(OutSize);

            thread_local static std::random_device rd;
            thread_local static std::mt19937 g(rd());
            thread_local static std::uniform_int_distribution<size_t> dis(0, (CharsSize - 1));

            for (size_t i = 0; i < OutSize; ++i) {
                out += Chars[dis(g)];
            }

            return out;
        }
    }
}
