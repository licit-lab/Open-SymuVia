#include "MultiLayersGraph.h"

#include "Node.h"

using namespace SymuCore;

MultiLayersGraph::MultiLayersGraph() : Graph(ST_Undefined) // Usually a multiLayers graph doesn't specify any service
{

}

MultiLayersGraph::~MultiLayersGraph()
{
    for (size_t iLayer = 0; iLayer < m_listLayers.size(); iLayer++)
    {
        delete m_listLayers[iLayer];
    }
}

Graph * MultiLayersGraph::CreateAndAddLayer(ServiceType eServiceType)
{
    Graph * pNewLayer = new Graph(this, eServiceType);
    m_listLayers.push_back(pNewLayer);
    return pNewLayer;
}

void MultiLayersGraph::AddLayer(Graph* pGraph)
{
    pGraph->setParent(this);
    m_listLayers.push_back(pGraph);
}

const std::vector<Graph *>& MultiLayersGraph::GetListLayers() const
{
    return m_listLayers;
}

bool MultiLayersGraph::hasChild(Node * pNode) const
{
    if (Graph::hasChild(pNode))
    {
        return true;
    }
    else
    {
        for (size_t i = 0; i < m_listLayers.size(); i++)
        {
            if (pNode->getParent() == m_listLayers[i])
            {
                return true;
            }
        }
        return false;
    }
}



