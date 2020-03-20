#pragma once

#ifndef SYMUCORE_LINK_H
#define SYMUCORE_LINK_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Node;
class Graph;
class Pattern;

class SYMUCORE_DLL_DEF Link {

public:

    Link();
    Link(Node* pUpstreamNode, Node* pDownstreamNode);
    virtual ~Link();

    //getters
    Graph * getParent() const;
    Node* getUpstreamNode() const;
    Node* getDownstreamNode() const;
	const std::vector<Pattern*> & getListPatterns() const;
    std::vector<Pattern*> & getListPatterns();

    //setters
    void setParent(Graph * pGraph);
    void setUpstreamNode(Node *pUpstreamNode);
    void setDownstreamNode(Node *pDownstreamNode);

private:

    Graph*                                                      m_pParent;

    Node*                                                       m_pUpstreamNode; //pointer to the upstream Node
    Node*                                                       m_pDownstreamNode; //pointer to the downstream Node
	std::vector<Pattern*>                                       m_listPatterns; //all the patterns for this link

};
}

#pragma warning(pop)

#endif // SYMUCORE_LINK_H
