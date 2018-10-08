#include "cookies.hpp"

namespace NAC {
    namespace NHTTP {
        std::string TSetCookieSpec::Dump() const {
            std::string out(Name + "=" + Value);

            if(!Expires.empty())
                out += "; Expires=" + Expires;

            if(!MaxAge.empty())
                out += "; Max-Age=" + MaxAge;

            if(!Domain.empty())
                out += "; Domain=" + Domain;

            if(!Path.empty())
                out += "; Path=" + Path;

            if(Secure)
                out += "; Secure";

            if(HttpOnly)
                out += "; HttpOnly";

            return out;
        }
    }
}
