#include "Destination.h"

#include "Utils/Point.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Pattern.h"

#include <stddef.h>

using namespace SymuCore;

Destination::Destination()
{
    m_pSelfNode = NULL;
    m_pPattern = NULL;
}

Destination::Destination(std::string strNodeName)
{
    m_strNodeName = strNodeName;
    m_pSelfNode = NULL;
    m_pPattern = NULL;
}

Destination::Destination(Node *pSelfNode)
{
    m_pSelfNode = pSelfNode;
    m_strNodeName = pSelfNode->getStrName();
    m_pPattern = NULL;
}

Destination::~Destination()
{

}

const std::string & Destination::getStrNodeName() const
{
    return m_strNodeName;
}

void Destination::setStrNodeName(const std::string &strNodeName)
{
    m_strNodeName = strNodeName;
}

Node *Destination::getNode() const
{
    return m_pSelfNode;
}

void Destination::setSelfNode(Node *pSelfNode)
{
    m_pSelfNode = pSelfNode;
}

void Destination::setPatternAsDestination(Pattern * pDestinationPattern)
{
    m_pPattern = pDestinationPattern;
    m_pSelfNode = pDestinationPattern->getLink()->getDownstreamNode();
}

Pattern * Destination::getPattern() const
{
    return m_pPattern;
}

void Destination::computeCoordinates()
{
    int nbPoints = 0;
    Point pt;

    for (size_t iUpstreamLink = 0; iUpstreamLink < m_pSelfNode->getUpstreamLinks().size(); iUpstreamLink++)
    {
        Point * pUpstreamCoords = m_pSelfNode->getUpstreamLinks()[iUpstreamLink]->getUpstreamNode()->getCoordinates();
        if (pUpstreamCoords)
        {
            pt.setX(pt.getX() + pUpstreamCoords->getX());
            pt.setY(pt.getY() + pUpstreamCoords->getY());
            nbPoints++;
        }
    }

    if (nbPoints > 0)
    {
        pt.setX(pt.getX() / nbPoints);
        pt.setY(pt.getY() / nbPoints);
        m_pSelfNode->setCoordinates(&pt);
    }
    else
    {
        m_pSelfNode->setCoordinates(NULL);
    }
}
