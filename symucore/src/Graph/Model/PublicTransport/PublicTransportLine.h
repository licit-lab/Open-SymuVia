#pragma once

#ifndef SYMUCORE_PUBLICTRANSPORTLINE_H
#define SYMUCORE_PUBLICTRANSPORTLINE_H

#include "SymuCoreExports.h"
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF PublicTransportLine {

public:

    PublicTransportLine();
    PublicTransportLine(const std::string& strName);
    virtual ~PublicTransportLine();

    //getters
    const std::string & getStrName() const;

    //setters
    void setStrName(const std::string &strName);

private:

    std::string             m_strName; //name of the line

};
}

#pragma warning(pop)

#endif // SYMUCORE_PUBLICTRANSPORTLINE_H
