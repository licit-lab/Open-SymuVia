#include "stdafx.h"
#include "C_ITSApplication.h"

#include "Logger.h"

C_ITSApplication::C_ITSApplication()
{

}

C_ITSApplication::C_ITSApplication(const std::string strID, const std::string strType) : m_strApplicationID(strID), m_strApplicationType(strType)
{

}

C_ITSApplication::~C_ITSApplication()
{

}

std::string C_ITSApplication::GetLabel() const
{
    return m_strApplicationID;
}

std::string C_ITSApplication::GetType() const
{
    return m_strApplicationType;
}

std::vector<std::string> C_ITSApplication::split(const std::string &str, char delimiter)
{
    std::vector<std::string> internal;
    std::stringstream ss(str); // Turn the string into a stream.
    std::string tok;

    while(std::getline(ss, tok, delimiter)) {
        // to deal with empty tokens at the end of lines on Linux with a Windows generated CSV file
        if (!(tok.size() == 1 && tok.front() == '\r'))
        {
            internal.push_back(tok);
        }
    }

    return internal;
}

bool C_ITSApplication::ReadAction(Reseau* pNetwork, const std::string& Action, std::string& Command, std::string& Value)
{
    std::vector<std::string> CommandAndValue = split(Action, ';');
    if(CommandAndValue.size() != 2)
    {
        *(pNetwork->GetLogger()) << Logger::Error << "Error while reading the message in " << m_strApplicationID << " : Wrong format for the Action " << Action << " !" << std::endl;
        return false;
    }

    Command = CommandAndValue[0];
    Value = CommandAndValue[1];

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void C_ITSApplication::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void C_ITSApplication::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void C_ITSApplication::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_strApplicationID);
    ar & BOOST_SERIALIZATION_NVP(m_strApplicationType);
}
