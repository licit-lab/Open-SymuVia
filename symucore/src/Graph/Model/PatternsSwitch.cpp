#include "PatternsSwitch.h"

#include <stddef.h>

using namespace SymuCore;

PatternsSwitch::PatternsSwitch() : m_pUpstreamPattern(NULL), m_pDownstreamPattern(NULL)
{

}

PatternsSwitch::PatternsSwitch(Pattern *pUpstreamPattern, Pattern *pDownstreamPattern)
{
    m_pUpstreamPattern = pUpstreamPattern;
    m_pDownstreamPattern = pDownstreamPattern;
}

PatternsSwitch::~PatternsSwitch()
{

}

Pattern *PatternsSwitch::getUpstreamPattern() const
{
    return m_pUpstreamPattern;
}

void PatternsSwitch::setUpstreamPattern(Pattern *pUpstreamPattern)
{
    m_pUpstreamPattern = pUpstreamPattern;
}

Pattern *PatternsSwitch::getDownstreamPattern() const
{
    return m_pDownstreamPattern;
}

void PatternsSwitch::setDownstreamPattern(Pattern *pDownstreamPattern)
{
    m_pDownstreamPattern = pDownstreamPattern;
}

