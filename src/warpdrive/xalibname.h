/* File:			xalibname.h
 *
 * Description:		See "xalibname.cc"
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *                      Modifications to this file by Dremio Corporation, (C) 2020-2022.
 *
 */

#ifndef __XALIBNAME_H__
#define __XALIBNAME_H__

#ifdef	__cplusplus
extern "C" {
#endif
#ifdef	WIN32
#ifdef	_HANDLE_ENLIST_IN_DTC_

BOOL	IsWow64(void);
const char *GetXaLibName(void);
const char *GetXaLibPath(void);

#endif /* _HANDLE_ENLIST_IN_DTC_ */
#endif /* WIN32 */

#ifdef	__cplusplus
}
#endif
#endif /* __XALIBNAME_H__ */
