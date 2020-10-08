#include "CSVStringUtils.h"
#include "Demand/Origin.h"
#include "Demand/Destination.h"
#include "Demand/Path.h"
#include "Graph/Model/Node.h"
#include "Graph/Model/Link.h"
#include "Graph/Model/Pattern.h"

#include <boost/log/trivial.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/convenience.hpp>

#include <limits>

using namespace SymuMaster;
using namespace SymuCore;
using namespace std;


std::vector<std::string> CSVStringUtils::split(std::string& str, char delimiter) {
  std::vector<std::string> internal;
  std::stringstream ss(str); // Turn the string into a stream.
  std::string tok;

  while(std::getline(ss, tok, delimiter)) {
      // to deal with empty tokens at the end of lines on Linux with a Windows generated CSV file
      
      if (!(tok.size() == 1 && tok.front() == '\r'))
      {
      	  // to remove problems due to different codes for end of line with Windows, Mac or Linux 
          tok.erase(std::remove(tok.begin(), tok.end(), '\r'), tok.end());
          tok.erase(std::remove(tok.begin(), tok.end(), '\n'), tok.end());
      			
          internal.push_back(tok);
      }
  }

  return internal;
}


bool CSVStringUtils::verifyPath(vector<string> &strPath, Origin *pOrigin, Destination *pDestination, Path &newPath)
{
    bool bOk = true;
    Node* currentNode = NULL;
    Pattern* currentPattern = NULL;

    const std::string firstPathNode = strPath.size() > 1 ? strPath[1] : pDestination->getStrNodeName();
    for(auto pUpstreamLink: pOrigin->getNode()->getDownstreamLinks())
    {
        Node* firstNode = pUpstreamLink->getDownstreamNode();
        for(auto pDownstreamLink : firstNode->getDownstreamLinks())
        {
            if (pDownstreamLink->getDownstreamNode()->getStrName() == firstPathNode)
            {
                newPath.GetlistPattern().push_back(pUpstreamLink->getListPatterns()[0]); //this link always should have only 1 NullPattern
                currentNode = pDownstreamLink->getDownstreamNode();
                for(auto pPattern : pDownstreamLink->getListPatterns())
                {
					//BOOST_LOG_TRIVIAL(error) << "l60 " << strPath[0] << " " << pPattern->toString();
                    if(pPattern->toString() == strPath[0])
                    {
                        currentPattern = pPattern;
                        newPath.GetlistPattern().push_back(pPattern);
                        break;
                    }
                }//end listPattern
                break;
            }
        }//end listDownstreamLink
        if(!newPath.empty())
            break;
    }//end listUpstreamLink

    if(!currentNode || !currentPattern)
    {
        BOOST_LOG_TRIVIAL(error) << "Predefined path from """ << pOrigin->getStrNodeName() << """ to """ << pDestination->getStrNodeName() << """ is not correct (4) !";
        bOk = false;
        return bOk;
    }

    if (strPath.size() > 1)
    {
        currentPattern = NULL;
        for(size_t iPath=3; iPath < strPath.size(); iPath += 2)
        {
            for(auto pDownstreamLink : currentNode->getDownstreamLinks())
            {
                if(pDownstreamLink->getDownstreamNode()->getStrName() == strPath[iPath])
                {
                    currentNode = pDownstreamLink->getDownstreamNode();
                    for(auto pPattern : pDownstreamLink->getListPatterns())
                    {
                        if(pPattern->toString() == strPath[iPath-1])
                        {
                            currentPattern = pPattern;
                            newPath.GetlistPattern().push_back(pPattern);
                            break;
                        }
                    }//end listPattern
                    break;
                }
            }//end listDownstreamLink

            if(!currentPattern)
            {
                BOOST_LOG_TRIVIAL(error) << "Predefined path from """ << pOrigin->getStrNodeName() << """ to """ << pDestination->getStrNodeName() << """ is not correct (5) !";
                bOk = false;
                break;
            }else {
                currentPattern = NULL;
            }

        }//end list strPath

    
        currentPattern = NULL;
        for (auto pDownstreamLink : currentNode->getDownstreamLinks())
        {
            if (pDownstreamLink->getDownstreamNode()->getStrName() == pDestination->getStrNodeName())
            {
                currentNode = pDownstreamLink->getDownstreamNode();
                Pattern* pPattern;
                
                for (size_t iPattern = 0; iPattern < pDownstreamLink->getListPatterns().size(); iPattern++)
                {
                    pPattern = pDownstreamLink->getListPatterns()[iPattern];
                    
                   
                    if (pPattern->toString() == strPath[strPath.size() - 1])
                    {
                        currentPattern = pPattern;
                        newPath.GetlistPattern().push_back(pPattern);
                        break;
                    }
                }//end listPattern
                break;
            }
        }//end listDownstreamLink

        if (currentNode == NULL || currentPattern == NULL) 
        {
        	
            BOOST_LOG_TRIVIAL(error) << "Predefined path from """ << pOrigin->getStrNodeName() << """ to """ << pDestination->getStrNodeName() << """ is not correct (6) !";
			string sPath;
			for (size_t i = 0; i < strPath.size(); i ++)
				sPath = sPath + strPath[i] + " ";
			BOOST_LOG_TRIVIAL(error) << sPath;
        	  
            bOk = false;
            return bOk;
        }
    }

    if (currentNode->getStrName() != pDestination->getStrNodeName()) // last pattern doesn't lead to destination
    {
        BOOST_LOG_TRIVIAL(error) << "Predefined path from """ << pOrigin->getStrNodeName() << """ to """ << pDestination->getStrNodeName() << """ is not correct (7) !";
        bOk = false;
        return bOk;
    }

    for(auto pUpstreamLink : pDestination->getNode()->getUpstreamLinks())
    {
        if(pUpstreamLink->getUpstreamNode() == currentNode)
        {
            newPath.GetlistPattern().push_back(pUpstreamLink->getListPatterns()[0]); //this link always should have only 1 NullPattern
            break;
        }
    }

    return bOk;
}
