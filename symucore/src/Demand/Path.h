#pragma once

#ifndef SYMUCORE_PATH_H
#define SYMUCORE_PATH_H

#include "SymuCoreExports.h"

#include "Graph/Model/Pattern.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Node.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class Pattern;
    class Node;
    class Link;
    class Cost;
    class SubPopulation;

class SYMUCORE_DLL_DEF Path {

public:

    Path();
    Path(const std::vector<Pattern*>& listPattern);
    virtual ~Path();

    //getters
    const std::vector<Pattern*>& GetlistPattern() const;
    std::vector<Pattern*>& GetlistPattern();
    void SetListPattern(const std::vector<Pattern *> &listPattern);

    Cost GetPathCost(int iSimulationInstance, double dbStrartingTime, SubPopulation *pSubPopulation, bool shouldStartFromEnd) const;

    bool operator==(const Path & otherPath) const;
    bool operator<(const Path & otherPath) const;

    bool empty() const;

    friend std::ostream& operator<< (std::ostream& stream, const Path& path)
    {
        for(size_t i = 0; i < path.GetlistPattern().size()-1; i++)
        {
            stream << path.GetlistPattern()[i]->toString() << "\\";
            stream << path.GetlistPattern()[i]->getLink()->getDownstreamNode()->getStrName() << "\\";
        }
        stream << path.GetlistPattern()[path.GetlistPattern().size()-1]->toString();

        return stream;
    }

private:

    std::vector<Pattern*>		m_listPattern; //list of pattern used from orign node to destination node
};
}

#pragma warning(pop)

#endif // SYMUCORE_PATH_H
