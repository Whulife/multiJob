/********************************************************************
 *
 * License:  See top level LICENSE.txt file.
 *
 * Author: Ken Melero
 * 
 * Description: Common file for global constants.
 *
 **************************************************************************
 * $Id: ossimConstants.h 22476 2013-11-07 16:08:32Z dburken $
 */
#ifndef ossimConstants_HEADER
#define ossimConstants_HEADER 1

#ifdef __win32__
/**
 * WARNINGS SECTION:
 */
#ifdef _MSC_VER /* Quiet a bunch of MSVC warnings... */
#  pragma warning(disable:4786) /* visual c6.0 compiler */
#  pragma warning(disable:4251)/* for std:: member variable to have dll interface */
#  pragma warning(disable:4275) /* for std:: base class to have dll interface */
#  pragma warning(disable:4800) /* int forcing value to bool */
#  pragma warning(disable:4244) /* conversion, possible loss of data */
#endif
   
/**
 * DLL IMPORT/EXORT SECTION
 */
#define  OSSIMMAKINGDLL

#define OSSIMEXPORT __declspec(dllexport)
#define OSSIMIMPORT __declspec(dllimport)
#ifdef OSSIMMAKINGDLL
	#define OSSIMDLLEXPORT OSSIMEXPORT
	#define OSSIM_DLL       OSSIMEXPORT
	#define OSSIMDLLEXPORT_DATA(type) OSSIMEXPORT type
	#define OSSIM_DLL_DATA(type) OSSIMEXPORT type
	#define OSSIMDLLEXPORT_CTORFN
#else
	#define OSSIMDLLEXPORT OSSIMIMPORT
	#define OSSIM_DLL      OSSIMIMPORT
	#define OSSIMDLLEXPORT_DATA(type) OSSIMIMPORT type
	#define OSSIM_DLL_DATA(type) OSSIMIMPORT type
	#define OSSIMDLLEXPORT_CTORFN
#endif

#elif __linux__
#define OSSIM_DLL
#endif//windows
   
#endif /* #ifndef ossimConstants_HEADER */
