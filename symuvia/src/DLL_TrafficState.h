// author: Julien SOULA <julien.soula@cstb.fr>

// For Windows dll

#ifdef WIN32
#  ifdef STATIC_TRAFFICSTATE
#    	define TRAFFICSTATE_DLL_DEF
#  else
#   	if defined(TrafficState_EXPORTS) || defined(TRAFFICSTATE_EXPORTS)
#   		define TRAFFICSTATE_DLL_DEF __declspec(dllexport)
#  		else
#		    define TRAFFICSTATE_DLL_DEF __declspec(dllimport)
#   		ifdef _DEBUG
#    			pragma comment(lib,"TrafficStateD")
#   		else
#    			pragma comment(lib,"TrafficState")
#   		endif
#  		endif
# 	endif
#else
# 	define TRAFFICSTATE_DLL_DEF
#endif
