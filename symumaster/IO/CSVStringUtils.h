#pragma once

#ifndef _SYMUMASTER_CSVSTRINGUTILS_
#define _SYMUMASTER_CSVSTRINGUTILS_

#include <boost/shared_ptr.hpp>
#include <xercesc/util/XMLString.hpp>

#include <vector>

namespace SymuCore {
    class Path;
    class Origin;
    class Destination;
}

namespace SymuMaster {

    class CSVStringUtils {

    public:

        static std::vector<std::string> split(std::string &str, char delimiter);

        static bool verifyPath(std::vector<std::string> &strPath, SymuCore::Origin* pOrigin, SymuCore::Destination* pDestination, SymuCore::Path& newPath);
    };

}

#endif // _SYMUMASTER_CSVSTRINGUTILS_
