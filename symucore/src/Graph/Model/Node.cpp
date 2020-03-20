#include "Node.h"

#include "AbstractPenalty.h"
#include "Utils/Point.h"
#include "Graph.h"

using namespace SymuCore;

Node::Node() : m_pParent(NULL), m_strName(), m_eNodeType(NT_Undefined), m_pCoordinates(NULL), m_bIsSimulatorInterface(false)
{

}

Node::Node(const std::string &strName, NodeType eNodeType, const Point *ptCoordinates)
	: m_pParent(NULL), m_bIsSimulatorInterface(false)
{
    m_strName = strName;
    m_eNodeType = eNodeType;
    if (ptCoordinates != NULL)
    {
        m_pCoordinates = new Point(*ptCoordinates);
    }
    else
    {
        m_pCoordinates = NULL;
    }
}

Node::~Node()
{
    for (std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::iterator iter = m_mapPenalties.begin(); iter != m_mapPenalties.end(); ++iter)
    {
        for (std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDown = iter->second.begin(); iterDown != iter->second.end(); ++iterDown)
        {
            delete iterDown->second;
        }
    }

    if (m_pCoordinates)
    {
        delete m_pCoordinates;
    }
}

Graph * Node::getParent() const
{
    return m_pParent;
}

void Node::setStrName(const std::string &strName)
{
    m_strName = strName;
}

std::string Node::getStrName() const
{
    return m_strName;
}

void Node::setNodeType(const NodeType &eNodeType)
{
    m_eNodeType = eNodeType;
}

NodeType Node::getNodeType() const
{
    return m_eNodeType;

}

void Node::setCoordinates(Point* ptCoordinate)
{
    if (m_pCoordinates)
    {
        delete m_pCoordinates;
        m_pCoordinates = NULL;
    }
    if (ptCoordinate)
    {
        m_pCoordinates = new Point(*ptCoordinate);
    }
}

void Node::setParent(Graph * pGraph)
{
    m_pParent = pGraph;
}

Point* Node::getCoordinates() const
{
    return m_pCoordinates;
}

const std::vector<Link*>& Node::getUpstreamLinks() const
{
    return m_listUpstreamLinks;
}

const std::vector<Link*>& Node::getDownstreamLinks() const
{
    return m_listDownstreamLinks;
}

const std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern > & Node::getMapPenalties() const
{
    return m_mapPenalties;
}

std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern > & Node::getMapPenalties()
{
    return m_mapPenalties;
}

void Node::addUpstreamLink(Link * pUpstreamLink)
{
    m_listUpstreamLinks.push_back(pUpstreamLink);
}

void Node::addDownstreamLink(Link * pDownstreamLink)
{
    m_listDownstreamLinks.push_back(pDownstreamLink);
}

Cost * Node::getPenaltyCost(int iSimulationInstance, Pattern* upstreamPattern, Pattern* downstreamPattern, double t, SubPopulation* pSubPopulation)
{
    std::map<Pattern*, std::map<Pattern*, AbstractPenalty*, ComparePattern>, ComparePattern>::iterator iter = m_mapPenalties.find(upstreamPattern);
    if (iter != m_mapPenalties.end())
    {
        std::map<Pattern*, AbstractPenalty*, ComparePattern>::iterator iterDown = iter->second.find(downstreamPattern);
        if (iterDown != iter->second.end())
        {
            return iterDown->second->getPenaltyCost(iSimulationInstance, t, pSubPopulation);
        }
    }
    
    return NULL;
}

bool Node::isSimulatorsInterface() const
{
	return m_bIsSimulatorInterface;
}

void Node::setIsSimulatorsInterface()
{
	m_bIsSimulatorInterface = true;
}
