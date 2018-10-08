#pragma once

#include <string>

namespace NAC {
    namespace NHTTP {
        struct TSetCookieSpec {
            std::string Name;
            std::string Value;
            std::string Expires;
            std::string MaxAge;
            std::string Domain;
            std::string Path;
            bool Secure = false;
            bool HttpOnly = false;

            std::string Dump() const;
        };
    }
}
