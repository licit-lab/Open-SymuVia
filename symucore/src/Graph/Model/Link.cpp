#include "Link.h"

#include "Pattern.h"

using namespace SymuCore;


Link::Link() : m_pParent(NULL), m_pUpstreamNode(NULL), m_pDownstreamNode(NULL)
{

}

Link::Link(Node * pUpstreamNode, Node * pDownstreamNode)
: m_pParent(NULL)
{
    m_pUpstreamNode=pUpstreamNode;
    m_pDownstreamNode=pDownstreamNode;
}

Link::~Link()
{
    for (size_t i = 0; i < m_listPatterns.size(); i++)
    {
        delete m_listPatterns[i];
    }
}

Graph * Link::getParent() const
{
    return m_pParent;
}

Node* Link::getUpstreamNode() const
{
    return m_pUpstreamNode;
}

Node* Link::getDownstreamNode() const
{
    return m_pDownstreamNode;
}

const std::vector<Pattern*> & Link::getListPatterns() const
{
    return m_listPatterns;
}

std::vector<Pattern*> & Link::getListPatterns()
{
    return m_listPatterns;
}

//setters

void Link::setParent(Graph * pGraph)
{
    m_pParent = pGraph;
}

void Link::setUpstreamNode(Node *pUpstreamNode)
{
    m_pUpstreamNode = pUpstreamNode;
}

void Link::setDownstreamNode(Node *pDownstreamNode)
{
    m_pDownstreamNode = pDownstreamNode;
}










