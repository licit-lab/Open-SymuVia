#include "Origin.h"

#include "Utils/Point.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Pattern.h"

#include <stddef.h>
#include <cassert>

using namespace SymuCore;

Origin::Origin()
{
    m_pSelfNode = NULL;
    m_pPattern = NULL;
}

Origin::Origin(const std::string & strNodeName)
{
    m_strNodeName = strNodeName;
    m_pSelfNode = NULL;
    m_pPattern = NULL;
}

Origin::~Origin()
{
}

const std::string & Origin::getStrNodeName() const
{
    return m_strNodeName;
}

Node *Origin::getNode() const
{
    return m_pSelfNode;
}

void Origin::setSelfNode(Node *pSelfNode)
{
    m_pSelfNode = pSelfNode;
}

void Origin::setPatternAsOrigin(Pattern * pPattern)
{
    m_pSelfNode = pPattern->getLink()->getUpstreamNode();
    m_pPattern = pPattern;
}

Pattern * Origin::getPattern() const
{
    return m_pPattern;
}

void Origin::computeCoordinates()
{
    int nbPoints = 0;
    Point pt;

    for (size_t iDownstreamLink = 0; iDownstreamLink < m_pSelfNode->getDownstreamLinks().size(); iDownstreamLink++)
    {
        Point * pDownstreamCoords = m_pSelfNode->getDownstreamLinks()[iDownstreamLink]->getDownstreamNode()->getCoordinates();
        if (pDownstreamCoords)
        {
            pt.setX(pt.getX() + pDownstreamCoords->getX());
            pt.setY(pt.getY() + pDownstreamCoords->getY());
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
