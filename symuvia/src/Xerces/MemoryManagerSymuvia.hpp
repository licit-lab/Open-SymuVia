#pragma once
#if !defined(XERCESC_INCLUDE_GUARD_MemoryManagerSymuvia_HPP)
#define XERCESC_INCLUDE_GUARD_MemoryManagerSymuvia_HPP

/*
 * $Id: MemoryManagerSymuvia.hpp 673975 2008-07-04 09:23:56Z borisk $
 */

#include <xercesc/framework/MemoryManager.hpp>
#include <map>

XERCES_CPP_NAMESPACE_USE

/**
  * Configurable memory manager
  *
  */

class MemoryManagerSymuvia : public MemoryManager
{
public:

    /** @name Constructor */
    //@{

    /**
      * Default constructor
      */
    MemoryManagerSymuvia()
    {
    }
    //@}

    /** @name Destructor */
    //@{

    /**
      * Default destructor
      */
    virtual ~MemoryManagerSymuvia()
    {
		m_list.clear();
    }
    //@}


    /**
      * This method is called to obtain the memory manager that should be
      * used to allocate memory used in exceptions.
      *
      * @return A pointer to the memory manager
      */
    virtual MemoryManager* getExceptionMemoryManager();


    /** @name The virtual methods in MemoryManager */
    //@{

    /**
      * This method allocates requested memory.
      *
      * @param size The requested memory size
      *
      * @return A pointer to the allocated memory
      */
    virtual void* allocate(XMLSize_t size);

    /**
      * This method deallocates memory
      *
      * @param p The pointer to the allocated memory to be deleted
      */
    virtual void deallocate(void* p);

    //@}

private:
	std::map<void*,void*> m_list;
	std::map<void*,void*> m_listerr;

private:
    // -----------------------------------------------------------------------
    //  Unimplemented constructors and operators
    // -----------------------------------------------------------------------
    MemoryManagerSymuvia(const MemoryManagerSymuvia&);
    MemoryManagerSymuvia& operator=(const MemoryManagerSymuvia&);

};


#endif
