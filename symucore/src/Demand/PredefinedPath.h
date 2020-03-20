#pragma once

#ifndef SYMUCORE_PREDEFINED_PATH_H
#define SYMUCORE_PREDEFINED_PATH_H

#include "Path.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

    class Pattern;
    class Node;

class SYMUCORE_DLL_DEF PredefinedPath {

public:

    PredefinedPath();
    PredefinedPath(const std::vector<Pattern*>& listPattern, Node * pJunction, const std::string & name, double dbCoeff);
    virtual ~PredefinedPath();

    double getCoeff() const;
    const Path & getPath() const;
    const std::string & getStrName() const;
    Node * getJunction() const;

private:

    Path m_Path; // predefined path
    Node * m_pJunction; // junction for the predefined path if the origin touches the destination on one node
    std::string m_strName; // name of the predefined path
    double m_dbCoeff;// assignment coefficient of the predefined path
};
}

#pragma warning(pop)

#endif // SYMUCORE_PREDEFINED_PATH_H
