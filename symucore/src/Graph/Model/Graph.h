#pragma once

#ifndef SYMUCORE_GRAPH_H
#define SYMUCORE_GRAPH_H

#include "SymuCoreExports.h"

#include "Utils/SymuCoreConstants.h"

#include <vector>
#include <string>
#include <set>

#pragma warning(push)
#pragma warning(disable : 4251)

namespace SymuCore {

class MultiLayersGraph;
class Link;
class Node;
class VehicleType;
class PublicTransportLine;
class Point;
class MacroType;
class Origin;
class Destination;
class Pattern;

class SYMUCORE_DLL_DEF Graph {

public:

    Graph();
    Graph(ServiceType eServiceType);
    Graph(MultiLayersGraph * pParent, ServiceType eServiceType);
    virtual ~Graph();

    Node * CreateAndAddNode(const std::string & name, NodeType nodeType, const Point * pCoordinates);

    Link * CreateAndAddLink(Node * pUpstreamNode, Node * pDownstreamNode);

    Destination * GetOrCreateDestination(const std::string & strNodeName);
	Destination * getDestination(const std::string & strNodeName);
	bool hasDestinationOrUpstreamInterface(const std::string & strNodeName);
    void CreateLinkToDestination(Node *pNode);
    void RemoveDestination(Destination * pDestination);

    Origin * GetOrCreateOrigin(const std::string & strNodeName);
	Origin * getOrigin(const std::string & strNodeName);
	bool hasOriginOrDownstreamInterface(const std::string & strNodeName);
    void CreateLinkToOrigin(Node *pNode);
    void RemoveOrigin(Origin * pOrigin);

    void AddMacroType(MacroType * pMacroType);
    void AddPublicTransportLine(PublicTransportLine * pPublicTransportLine);

    const std::vector<Link *>& GetListLinks() const;
    const std::vector<Node *>& GetListNodes() const;
    const std::vector<Destination *>& GetListDestination() const;
    const std::vector<Origin *> &GetListOrigin() const;
    const std::vector<MacroType *> &GetListMacroTypes() const;
    std::vector<MacroType *> &GetListMacroTypes();
    const std::vector<PublicTransportLine *>& GetListTransportLines() const;
    ServiceType GetServiceType() const;
    void GetListServiceType(std::vector<ServiceType> &listServiceType);
	const std::set<std::string> & GetListUpstreamInterfaces() const;
	const std::set<std::string> & GetListDownstreamInterfaces() const;

    void SetListDestination(const std::vector<Destination *> & destinations);
    void SetListOrigin(const std::vector<Origin *> & origins);
    void SetListMacroTypes(const std::vector<MacroType*> & macroTypes);
	void SetListUpstreamInterfaces(const std::set<std::string> & upstreamInterfaces);
	void SetListDownstreamInterfaces(const std::set<std::string> & downstreamInterfaces);

    void setParent(MultiLayersGraph * pParent);
    MultiLayersGraph* getParent();

    virtual bool hasChild(Node * pNode) const;

	Node * GetNode(const std::string & strNodeName) const;
	Link * GetLink(const std::string & strNodeUpstream, const std::string & strNodeDownstream) const;

	void AddUpstreamInterface(const std::string & upInterfaceName);
	void AddDownstreamInterface(const std::string & downInterfaceName);


protected:

    MultiLayersGraph*                       m_pParent; // Parent graph if any

    std::vector<Link*>                      m_listLinks; //list of all the links for this graph all his sub-graph
    std::vector<Node*>                      m_listNodes; //list of all the nodes for this graph all his sub-graph
    std::vector<Destination*>               m_listDestination; //list of all the links for this graph all his sub-graph
    std::vector<Origin*>                    m_listOrigin; //list of all the nodes for this graph all his sub-graph
    ServiceType                             m_eServiceType; //Define the service proposed for this layer
    std::vector<MacroType*>                 m_listMacroTypes; //list of all the macro types for this graph all his sub-graph
    std::vector<PublicTransportLine*>       m_listTransportLines; //list of all public transport lines for this graph all his sub-graph

	std::set<std::string>					m_listUpstreamInterfaces; // list of all upstream interfaces.
	std::set<std::string>					m_listDownstreamInterfaces; // list of all upstream interfaces.

    bool                                    m_bOwnODsAndMacroTypes;

};
}

#pragma warning(pop)

#endif // SYMUCORE_GRAPH_H
