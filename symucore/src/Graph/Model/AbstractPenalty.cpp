#include "AbstractPenalty.h"

using namespace SymuCore;

AbstractPenalty::AbstractPenalty()
{
    m_pNode = NULL;
}

AbstractPenalty::AbstractPenalty(Node* pParentNode, const PatternsSwitch & patternsSwitch)
{
    m_pNode = pParentNode;
    m_PatternsSwitch = patternsSwitch;
}

AbstractPenalty::~AbstractPenalty()
{

}

Node * AbstractPenalty::getNode() const
{
    return m_pNode;
}

const PatternsSwitch & AbstractPenalty::getPatternsSwitch() const
{
    return m_PatternsSwitch;
}
