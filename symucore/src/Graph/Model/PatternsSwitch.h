#pragma once

#ifndef SYMUCORE_PATTERNSSWITCH_H
#define SYMUCORE_PATTERNSSWITCH_H

#include "SymuCoreExports.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Pattern;

class SYMUCORE_DLL_DEF PatternsSwitch {

public:

    PatternsSwitch();
    PatternsSwitch(Pattern* pUpstreamPattern, Pattern* pDownstreamPattern);
    virtual ~PatternsSwitch();

    //getters
    Pattern *getUpstreamPattern() const;
    Pattern *getDownstreamPattern() const;

    //setters
    void setUpstreamPattern(Pattern *pUpstreamPattern);
    void setDownstreamPattern(Pattern *pDownstreamPattern);

private:
    Pattern*                m_pUpstreamPattern; //pointer to the upstream Pattern
    Pattern*                m_pDownstreamPattern; //pointer to the downstream Pattern
};
}

#pragma warning(pop)

#endif // SYMUCORE_PATTERNSSWITCH_H
