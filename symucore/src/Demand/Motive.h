#pragma once

#ifndef SYMUCORE_Motive_H
#define SYMUCORE_Motive_H

#include "SymuCoreExports.h"
#include <string>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF Motive {

public:

    Motive();
    Motive(const std::string &strName);
    virtual ~Motive();

    //getters
    const std::string & GetStrName() const;

    //setters
    void SetStrName(const std::string &strName);

private:

    std::string                m_strName; //name of the motive

};
}

#pragma warning(pop)

#endif // SYMUCORE_Motive_H
