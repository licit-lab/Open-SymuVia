#pragma once

#ifndef SYMUCORE_DESTINATION_H
#define SYMUCORE_DESTINATION_H

#include "Graph/Model/Node.h"

#include "SymuCoreExports.h"

#include <string>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Node;

class SYMUCORE_DLL_DEF Destination {

public:

    Destination();
    Destination(std::string strNodeName);
    Destination(Node* pSelfNode);
    virtual ~Destination();

    //getters
    Node *getNode() const;
    const std::string & getStrNodeName() const;
    Pattern * getPattern() const;

    //setters
    void setSelfNode(Node *pSelfNode);
    void setStrNodeName(const std::string &strNodeName);
    void setPatternAsDestination(Pattern * pDestinationPattern);

    void computeCoordinates();



private:
    Node*                          m_pSelfNode; //Node that represent the destination
    std::string                    m_strNodeName; // Name of the Node

    Pattern*                       m_pPattern; // Pattern as the destination if the destination is not a node.
};
}

#pragma warning(pop)

#endif // SYMUCORE_DESTINATION_H
