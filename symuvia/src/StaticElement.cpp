#include "stdafx.h"
#include "StaticElement.h"

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

StaticElement::StaticElement()
{

} 

StaticElement::StaticElement(Point ptPos) : m_ptPos(ptPos)
{

}

StaticElement::~StaticElement()
{

}

const Point& StaticElement::GetPos() const
{
    return m_ptPos;
}

template void StaticElement::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void StaticElement::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void StaticElement::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_ptPos);
}
