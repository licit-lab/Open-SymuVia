#pragma once

#ifndef SYMUCORE_ORIGIN_H
#define SYMUCORE_ORIGIN_H

#include <string>
#include <vector>
#include "Graph/Model/Node.h"

#include "SymuCoreExports.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class Node;

class SYMUCORE_DLL_DEF Origin{

public:

    Origin();
    Origin(const std::string & strNodeName);
    virtual ~Origin();

    //getters
    Node *getNode() const;
    const std::string & getStrNodeName() const;
    Pattern * getPattern() const;

    //setters
    void setSelfNode(Node *pSelfNode);
    void setPatternAsOrigin(Pattern * pPattern);

    void computeCoordinates();


private:
    Node*                          m_pSelfNode; //Node tha represent the Origin
    std::string                    m_strNodeName; // Name of the Node
    Pattern*                       m_pPattern; // Target Pattern for the origin if the origin is a pattern and not a node
};
}

#pragma warning(pop)

#endif // SYMUCORE_ORIGIN_H
