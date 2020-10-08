
#include "Simulation/Assignment/AssignmentModel.h"
#include <map>

template <class T>
class Container {
public:
    Container() {}
    Container(std::map<SymuCore::Population*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > > >& mapData) : m_mapData(mapData){
        m_mapByPopulation = m_mapData.begin()->first;
        m_mapByOrigin = m_mapByPopulation.begin()->first;
    }
    ~Container(){}

    std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > >* GetData(SymuCore::Population* pPopulation){
        if(m_pPrevPopulation && m_pPrevPopulation == pPopulation)
            return &m_mapByPopulation;

        std::map<SymuCore::Population*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > > >::iterator itFind = m_mapData.find(pPopulation);
        if(itFind == m_mapData.end()){
            return NULL;
        }else{
            m_mapByPopulation = itFind->second;
        }
        m_pPrevPopulation = pPopulation;
        return &m_mapByPopulation;
    }
    std::map<SymuCore::Destination*, T >* GetData(SymuCore::Population* pPopulation, SymuCore::Origin* pOrigin){
        if(m_pPrevPopulation && m_pPrevOrigin && m_pPrevOrigin == pOrigin && m_pPrevPopulation == pPopulation)
            return &m_mapByOrigin;

        GetData(pPopulation);
        std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > >::iterator itFind = m_mapByPopulation.find(pOrigin);
        if(itFind == m_mapByPopulation.end()){
            return NULL;
        }else{
            m_mapByOrigin = itFind->second;
        }
        m_pPrevOrigin = pOrigin;
        return &m_mapByOrigin;
    }
    T* GetData(SymuCore::Population* pPopulation, SymuCore::Origin* pOrigin,  SymuCore::Destination* pDestination){
        GetData(pPopulation, pOrigin);
        std::map<SymuCore::Destination*, T >::iterator itFind = m_mapByOrigin.find(pDestination);
        if(itFind == m_mapByOrigin.end()){
            //CPQ - TODO - pas sur que se soit la meilleur solution a apporter dans le cas ou la map ne contient pas le résulat (ce qui n'est pas censé arrivé pour notre modèle d'assignement)
            return NULL;
        }else{
            return &itFind->second;
        }
    }
    std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > >& GetOrCreateData(SymuCore::Population* pPopulation){
        if(m_pPrevPopulation && m_pPrevPopulation == pPopulation)
            return m_mapByPopulation;

        m_pPrevPopulation = pPopulation;
        m_mapByPopulation = m_mapData[pPopulation];
        return m_mapByPopulation;
    }
    std::map<SymuCore::Destination*, T >& GetOrCreateData(SymuCore::Population* pPopulation, SymuCore::Origin* pOrigin){
        if(m_pPrevPopulation && m_pPrevOrigin && m_pPrevOrigin == pOrigin && m_pPrevPopulation == pPopulation)
            return m_mapByOrigin;

        m_pPrevOrigin = pOrigin;
        m_mapByOrigin = GetOrCreateData(pPopulation)[pOrigin];
        return m_mapByOrigin;
    }
    T& GetOrCreateData(SymuCore::Population* pPopulation, SymuCore::Origin* pOrigin,  SymuCore::Destination* pDestination){
        return GetOrCreateData(pPopulation, pOrigin)[pDestination];
    }
    void clear(){
        m_mapData.clear();
        m_pPrevPopulation = NULL;
        m_pPrevOrigin = NULL;
    }
    typename std::map<SymuCore::Population*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > > >::iterator begin(){return m_mapData.begin();}
    typename std::map<SymuCore::Population*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > > >::iterator end(){return m_mapData.end();}

private:
    std::map<SymuCore::Population*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > > > m_mapData;

    //Use for fast acces
    std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, T > > m_mapByPopulation;
    std::map<SymuCore::Destination*, T > m_mapByOrigin;
    SymuCore::Population* m_pPrevPopulation;
    SymuCore::Origin* m_pPrevOrigin;

};
