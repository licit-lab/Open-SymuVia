#include "stdafx.h"
#include "MemoryManagerSymuvia.hpp"

#include <xercesc/util/OutOfMemoryException.hpp>

XERCES_CPP_NAMESPACE_USE

MemoryManager* MemoryManagerSymuvia::getExceptionMemoryManager()
{
  return this;
}

void* MemoryManagerSymuvia::allocate(XMLSize_t size)
{
    void* memptr;
    try {
        memptr = ::operator new(size);
    }
    catch(...) {
        throw OutOfMemoryException();
    }
    if (memptr != NULL) {

		std::map<void*,void*>::iterator ptr = m_list.find(memptr);
		if (ptr == m_list.end())
		{
			m_list[memptr] = memptr;
		}
		else
		{
			m_listerr[memptr] = memptr;
			int zz = 0;
			zz++;
		}
		return memptr;
    }
    throw OutOfMemoryException();
}

void MemoryManagerSymuvia::deallocate(void* p)
{
    if (p)
	{
		//std::deque<void*>::iterator ptr;
		//int i = 0;
		//int idx = -1;
		//for (ptr = m_list.begin(); (ptr != m_list.end()) && (idx < 0); ptr++)
		//{
		//	if (p == (*ptr))
		//	{
		//		idx = i;
		//	}
		//	i ++;
		//}
		std::map<void*,void*>::iterator ptr = m_list.find(p);
		//if (idx < 0)
		if (ptr == m_list.end())
		{
			return;
		}
		else
		{
			//m_list.erase(m_list.begin() + idx);
			m_list.erase(ptr);
		}
		if ((m_list.size() == 5084) && (m_listerr.size() == 49))
		{
			int zz = 0;
			zz++;
		}
        ::operator delete(p);
	}
}

