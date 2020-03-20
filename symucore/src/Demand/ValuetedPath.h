#pragma once

#ifndef SYMUCORE_VALUETEDPATH_H
#define SYMUCORE_VALUETEDPATH_H

#include "SymuCoreExports.h"
#include "Graph/Algorithms/ShortestPathsComputer.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class Path;

class SYMUCORE_DLL_DEF ValuetedPath {

public:
    ValuetedPath();
    ValuetedPath(const Path& pPath, double dTotalCost = 0.0, double dTotalTime = 0.0);
    ValuetedPath(ShortestPathsComputer::TimedNode* tnPathNode, bool hasToReverse = false);

    ~ValuetedPath();

    void setTime(double dbTime);

    void setCost(double dbCost);

    void setIsPredefined(bool bPredefined);

    void setStrName(const std::string & strName);

    void setPredefinedJunctionNode(Node * pJunction);

    void setPenalizedCost(double dbPenalizedCost);

    void setCommonalityFactor(double dbCommonalityFactor);

    bool operator<(const ValuetedPath& otherPath) const;

    bool operator==(const ValuetedPath &otherPath) const;

    double GetCost() const;

    double GetTime() const;

    Path& GetPath();

    const Path &GetPath() const;

    bool IsPredefined() const;

    const std::string & getStrName() const;

    Node * getPredefinedJunctionNode() const;

    double getPenalizedCost() const;

    double getCommonalityFactor() const;

protected:
    Path                        m_Path; // affective path
    double                      m_dTotalCost; //Total cost when arrived at destination Node
    double                      m_dTotalTime; //time when arrived at destination Node

    double                      m_dbPenalizedCost; //Penalised path cost
    double                      m_dbCommonalityFactor; //Commonality factor

    bool                        m_bIsPredefined; // tells if the path is predefined or computed
    std::string                 m_strName; // Name of the predefined path
    Node*                       m_pPredefinedJunctionNode; // Junction node in case of a predefined path
};
}

#pragma warning(pop)

#endif // SYMUCORE_VALUETEDPATH_H
