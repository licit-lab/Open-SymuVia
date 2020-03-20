#ifndef _MOTIF_H__
#define _MOTIF_H__

#include <string>

namespace boost {
    namespace serialization {
        class access;
    }
}

//! Classe définissant un motif de déplacement
class CMotif
{

public:
    //! Default constructor
    CMotif(void);
    //! Constructor
    CMotif(const std::string & strID);
    //! Destructor
    virtual ~CMotif(void);


public:

    const std::string & GetID();

    void setDescription(const std::string & strDesc);

private:

    std::string m_strID;

    std::string m_strDescription;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // _MOTIF_H__