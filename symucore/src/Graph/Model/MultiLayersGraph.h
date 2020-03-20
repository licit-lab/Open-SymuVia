#pragma once

#ifndef SYMUCORE_MULTILAYERSGRAPH_H
#define SYMUCORE_MULTILAYERSGRAPH_H

#include "Graph.h"
#include "SymuCoreExports.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class SYMUCORE_DLL_DEF MultiLayersGraph : public Graph {

public:

    MultiLayersGraph();
    virtual ~MultiLayersGraph();

    Graph * CreateAndAddLayer(ServiceType eServiceType);
    void AddLayer(Graph * pGraph);

    const std::vector<Graph *>& GetListLayers() const;

    virtual bool hasChild(Node * pNode) const;

private:

    std::vector<Graph* > m_listLayers;

};
}

#pragma warning(pop)

#endif // SYMUCORE_MULTILAYERSGRAPH_H
