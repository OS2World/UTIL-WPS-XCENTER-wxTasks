//***************************************************************************
//
//    Copyright (C) 1997-2005 John Martin Alfredsson,
//                            Dmitry A.Steklenev,
//                            Ulrich M”ller,
//                            and others.
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as contained in
//    the file COPYING in the installation directory.
//
//    The full source code is available.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//***************************************************************************
// wxTask.h
//***************************************************************************

#ifndef __WXTASK_H
#define __WXTASK_H

#define ID_MENU_TASKS       101
#define ID_ICON_TASKS       500
#define ID_ICON_DETACH      501
#define ID_ICON_DOS         502
#define ID_ICON_OS2FS       503
#define ID_ICON_OS2VIO      504
#define ID_ICON_PM          505

#define ID_ITEM_FIRST      1000

struct TASKDATA {

  char*    szTitle;
  HWND     hWindow;
  HPOINTER hIcon;
  PID      pid;
};

SHORT winhInsertMenuItem( HWND hwndMenu, SHORT iPosition, SHORT sItemId,
                          const char* pcszItemTitle, SHORT afStyle,
                          SHORT afAttr, TASKDATA* data );

void    FillMenu( HWND hwndMenu );
char*   FileGetFileExt( char* szFullFile );
void    Log( char* szData );
APIRET  DosKillFastIo( PID pid );
void    SwitchTo( TASKDATA* data );
MRESULT MeasureMenuItem( POWNERITEM );
MRESULT DrawMenuItem( POWNERITEM );

#endif
