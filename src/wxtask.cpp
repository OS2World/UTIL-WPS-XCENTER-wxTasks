/***************************************************************************

    Copyright (C) 1997-2005 John Martin Alfredsson,
                            Dmitry A.Steklenev,
                            Ulrich M”ller,
                            Holger Veit,
                            and others.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as contained in
    the file COPYING in the installation directory.

    The full source code is available.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

#define  PROG_VERSION   "0.3.0"

#define  INCL_DOSMODULEMGR
#define  INCL_DOSERRORS

#define  INCL_WINWINDOWMGR
#define  INCL_WINFRAMEMGR
#define  INCL_WINDIALOGS
#define  INCL_WININPUT
#define  INCL_WINSWITCHLIST
#define  INCL_WINRECTANGLES
#define  INCL_WINPOINTERS
#define  INCL_WINSYS
#define  INCL_WINLISTBOXES
#define  INCL_WINENTRYFIELDS

#define  INCL_GPILOGCOLORTABLE

#define  INCL_DOSDEVIOCTL
#define  INCL_DOSDEVICES
#define  INCL_DOSPROCESS

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_GPI

#include <os2.h>

// C library headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <time.h>

#include "center.h"   // public XCenter interfaces
#include "wxtask.h"   // private header file

char szDebug[512];    // for debugging

/********************************************************************
 * Private definitions
 ********************************************************************/

HMODULE   hmodMe     = NULLHANDLE;  // DLL module handle
BOOL      bType      = FALSE;       // Show switchlist or task list
HWND      hwndMenu   = NULLHANDLE;  // global handle to popup menu

HPOINTER  ico_tasks  = NULLHANDLE;
HPOINTER  ico_detach = NULLHANDLE;
HPOINTER  ico_dos    = NULLHANDLE;
HPOINTER  ico_os2fs  = NULLHANDLE;
HPOINTER  ico_os2vio = NULLHANDLE;
HPOINTER  ico_pm     = NULLHANDLE;

/********************************************************************
 * XCenter widget class definition
 ********************************************************************/

/*
 *      This contains the name of the PM window class and
 *      the XCENTERWIDGETCLASS definition(s) for the widget
 *      class(es) in this DLL.
 *
 *      The address of this structure (or an array of these
 *      structures, if there were several widget classes in
 *      this plugin) is returned by the "init" export
 *      (WgtInitModule).
 */

#define WNDCLASS_WIDGET_TASKS "XWPCenterTasks"

static XCENTERWIDGETCLASS G_WidgetClasses[]
   = {
       WNDCLASS_WIDGET_TASKS,      // PM window class name
       0,                          // additional flag, not used here
       "TasksWidget",              // internal widget class name
       "Tasks",                    // widget class name displayed to user
       WGTF_UNIQUEPERXCENTER,      // widget class flags
       NULL                        // no settings dialog
     };

/********************************************************************
 * Function imports from XFLDR.DLL
 ********************************************************************/

/*
 *      To reduce the size of the widget DLL, it can
 *      be compiled with the VAC subsystem libraries.
 *      In addition, instead of linking frequently
 *      used helpers against the DLL again, you can
 *      import them from XFLDR.DLL, whose module handle
 *      is given to you in the INITMODULE export.
 *
 *      Note that importing functions from XFLDR.DLL
 *      is _not_ a requirement. We can't do this in
 *      this minimal sample anyway without having access
 *      to the full XWorkplace source code.
 *
 *      If you want to know how you can import the useful
 *      functions from XFLDR.DLL to use them in your widget
 *      plugin, again, see src\widgets in the XWorkplace sources.
 *      The actual imports would then be made by WgtInitModule.
 */

/********************************************************************
 * Private widget instance data
 ********************************************************************/

// None presently. The samples in src\widgets in the XWorkplace
// sources cleanly separate setup string data from other widget
// instance data to allow for easier manipulation with settings
// dialogs. We have skipped this for the minimal sample.

/********************************************************************
 * Widget setup management
 ********************************************************************/

// None presently. See above.

/********************************************************************
 * Widget settings dialog
 ********************************************************************/

// None currently. To see how a setup dialog can be done,
// see the "window list" widget in the XWorkplace sources
// (src\widgets\w_winlist.c).

/********************************************************************
 * Callbacks stored in XCENTERWIDGETCLASS
 ********************************************************************/

// If you implement a settings dialog, you must write a
// "show settings dlg" function and store its function pointer
// in XCENTERWIDGETCLASS.

/********************************************************************
 * PM window class implementation
 ********************************************************************/

/*
 *      This code has the actual PM window class.
 *
 */

/*
 *@@ MwgtControl:
 *      implementation for WM_CONTROL in fnwpWxTaskWidget.
 *
 *      The XCenter communicates with widgets thru
 *      WM_CONTROL messages. At the very least, the
 *      widget should respond to XN_QUERYSIZE because
 *      otherwise it will be given some dumb default
 *      size.
 *
 *@@added V0.9.7 (2000-12-14) [umoeller]
 */

BOOL
WgtControl( PXCENTERWIDGET pWidget, MPARAM mp1, MPARAM mp2 )
{
  BOOL   brc = FALSE;
  USHORT usID = SHORT1FROMMP(mp1),
         usNotifyCode = SHORT2FROMMP(mp1);

  // is this from the XCenter client?
  if( usID == ID_XCENTER_CLIENT )
  {
    // yes:
    switch( usNotifyCode )
    {
      /*
       * XN_QUERYSIZE:
       *      XCenter wants to know our size.
       */

      case XN_QUERYSIZE:
      {
        PSIZEL pszl = (PSIZEL)mp2;

        pszl->cx = WinQuerySysValue( HWND_DESKTOP, SV_CXICON ) / 2 + 6;
        pszl->cy = WinQuerySysValue( HWND_DESKTOP, SV_CYICON ) / 2 + 6;
        brc = TRUE;
        break;
      }
    }
  }

  return brc;
}

/*
 *@@ WgtPaint:
 *      implementation for WM_PAINT in fnwpWxTaskWidget.
 *
 */

VOID
WgtPaint( HWND hwnd, PXCENTERWIDGET pWidget )
{
  ULONG cx_icon = WinQuerySysValue( HWND_DESKTOP, SV_CXICON ) / 2;
  ULONG cy_icon = WinQuerySysValue( HWND_DESKTOP, SV_CYICON ) / 2;

  HPS hps = WinBeginPaint( hwnd, NULLHANDLE, NULL );

  if( hps )
  {
    RECTL rclWin;
    int   x, y;

    // Get windows size
    WinQueryWindowRect( hwnd, &rclWin );

    // Fill background
    WinFillRect( hps, &rclWin, CLR_PALEGRAY );

    // Center icon in window
    y = rclWin.yTop - rclWin.yBottom;
    x = rclWin.xRight - rclWin.xLeft;

    y = ( y - cy_icon ) / 2;
    x = ( x - cx_icon ) / 2;

    // Draw icon in window
    WinDrawPointer( hps, x, y, ico_tasks, DP_MINI );
    WinEndPaint( hps );
  }
}

/*
 *@@ fnwpWxTaskWidget:
 *      window procedure for the winlist widget class.
 *
 *      There are a few rules which widget window procs
 *      must follow. See ctrDefWidgetProc in src\shared\center.c
 *      for details.
 *
 *      Other than that, this is a regular window procedure
 *      which follows the basic rules for a PM window class.
 */

MRESULT EXPENTRY
fnwpWxTaskWidget( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  MRESULT mrc = 0;
  // get widget data from QWL_USER (stored there by WM_CREATE)
  PXCENTERWIDGET pWidget = (PXCENTERWIDGET)WinQueryWindowPtr( hwnd, QWL_USER );
  // this ptr is valid after WM_CREATE

  switch( msg )
  {
    /*
     * WM_CREATE:
     *      as with all widgets, we receive a pointer to the
     *      XCENTERWIDGET in mp1, which was created for us.
     *
     *      The first thing the widget MUST do on WM_CREATE
     *      is to store the XCENTERWIDGET pointer (from mp1)
     *      in the QWL_USER window word by calling:
     *
     *          WinSetWindowPtr(hwnd, QWL_USER, mp1);
     *
     *      We could use XCENTERWIDGET.pUser for allocating
     *      another private memory block for our own stuff,
     *      for example to be able to store fonts and colors.
     *      We ain't doing this in the minimal sample.
     */

    case WM_CREATE:
      WinSetWindowPtr(hwnd, QWL_USER, mp1);
      pWidget = (PXCENTERWIDGET)mp1;
      if(( !pWidget ) || ( !pWidget->pfnwpDefWidgetProc )) {
        // shouldn't happen... stop window creation!!
        mrc = (MPARAM)TRUE;
      }

      hwndMenu = WinCreateWindow( hwnd, WC_MENU, 0, 0, 0, 0, 0, 0,
                                  hwnd, HWND_TOP, ID_MENU_TASKS, 0, 0 );

      ico_tasks  = WinLoadPointer( HWND_DESKTOP, hmodMe, ID_ICON_TASKS  );
      ico_detach = WinLoadPointer( HWND_DESKTOP, hmodMe, ID_ICON_DETACH );
      ico_dos    = WinLoadPointer( HWND_DESKTOP, hmodMe, ID_ICON_DOS    );
      ico_os2fs  = WinLoadPointer( HWND_DESKTOP, hmodMe, ID_ICON_OS2FS  );
      ico_os2vio = WinLoadPointer( HWND_DESKTOP, hmodMe, ID_ICON_OS2VIO );
      ico_pm     = WinLoadPointer( HWND_DESKTOP, hmodMe, ID_ICON_PM     );
      break;

    /*
     * WM_BUTTON1CLICK:
     *      clicked on the window
     */

    case WM_BUTTON1CLICK:
    {
      mrc = (MPARAM)WgtControl( pWidget, mp1, mp2 );

      RECTL  rcl;
      POINTL ptl;
      SWP    swp;

      // Check if Ctrl is pressed
      if( WinGetKeyState( HWND_DESKTOP, VK_CTRL ) & 0x8000 ) {
        bType = TRUE;
      } else {
        bType = FALSE;
      }

      // Fill it with items
      FillMenu( hwndMenu );

      // Place popup according to xCenter position
      if( pWidget->pGlobals->ulPosition == XCENTER_BOTTOM )
      {
        WinQueryWindowRect( hwnd, &rcl );
        ptl.y = rcl.yTop + 1;
        ptl.x = rcl.xLeft;
        WinMapWindowPoints( HWND_DESKTOP, hwnd, &ptl, 0 );
      }
      else
      {
        WinQueryWindowRect( hwnd, &rcl );
        ptl.y = rcl.yBottom;
        ptl.x = rcl.xLeft;
        WinMapWindowPoints( HWND_DESKTOP, hwnd, &ptl, 0 );
        memset( &swp, 0, sizeof(SWP));
        swp.fl = SWP_SIZE;
        WinSendMsg( hwndMenu, WM_ADJUSTWINDOWPOS, MPFROMP(&swp), 0 );
        ptl.y -= swp.cy;
      }

      // Show menu
      WinPopupMenu( hwnd, hwnd, hwndMenu, ptl.x, ptl.y, 0,
                    PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_HCONSTRAIN | PU_VCONSTRAIN );
      break;
    }

    case WM_COMMAND:
    {
      char      szPid[1024];
      MENUITEM  mi;
      SHORT     id;
      TASKDATA* data;

      // Get menu id and data
      id = SHORT1FROMMP(mp1);
      WinSendMsg( hwndMenu, MM_QUERYITEM, MPFROM2SHORT( id, FALSE ), MPFROMP( &mi ));
      data = (TASKDATA*)mi.hItem;

      // Is it "our" item ??
      if( id >= ID_ITEM_FIRST && id < 10000 )
      {
        if( bType )
        {
          // Ctrl was pressed on icon, kill pid
          if( WinGetKeyState( HWND_DESKTOP, VK_CTRL ) & 0x8000 )
          {
            // Ctrl was pressed on menu item, use external kill
            sprintf( szPid, "Kill this process (%d) using fastio$ ?", data->pid );

            if( WinMessageBox( HWND_DESKTOP, hwnd, szPid, "xCenter", 0,
                               MB_ICONQUESTION | MB_YESNO    |
                               MB_APPLMODAL    | MB_MOVEABLE | MB_DEFBUTTON2 ) == MBID_YES )
            {
              DosKillFastIo( data->pid );
            }
          } else {
            sprintf( szPid, "Are you sure that you want to kill this process (%d)?", data->pid );

            if( WinMessageBox( HWND_DESKTOP, hwnd, szPid, "xCenter", 0,
                               MB_ICONQUESTION | MB_YESNO    |
                               MB_APPLMODAL    | MB_MOVEABLE | MB_DEFBUTTON2) == MBID_YES )
            {
              DosKillProcess( DKP_PROCESS, data->pid );
            }
          }
        } else {
          // No Ctrl, its a switch to task
          SwitchTo( data );
        }
      } else {
        // Its Ulrichs menu, forward
        mrc = pWidget->pfnwpDefWidgetProc( hwnd, msg, mp1, mp2 );
      }

      break;
    }

    /*
     * WM_CONTROL:
     *      process notifications/queries from the XCenter.
     */

    case WM_CONTROL:
      mrc = (MPARAM)WgtControl( pWidget, mp1, mp2 );
      break;

    /*
     * WM_PAINT:
     *      well, paint the widget.
     */

    case WM_PAINT:
      WgtPaint( hwnd, pWidget );
      break;

    /*
     * WM_PRESPARAMCHANGED:
     *      A well-behaved widget would intercept
     *      this and store fonts and colors.
     */

    case WM_PRESPARAMCHANGED:
      break;

    case WM_MEASUREITEM:
      return MeasureMenuItem((POWNERITEM)mp2 );

    case WM_DRAWITEM:
      return DrawMenuItem((POWNERITEM)mp2 );

    /*
     * WM_DESTROY:
     *      clean up. This _must_ be passed on to
     *      ctrDefWidgetProc.
     */

    case WM_DESTROY:
      // If we had any user data allocated in WM_CREATE
      // or elsewhere, we'd clean this up here.
      // We _MUST_ pass this on, or the default widget proc
      // cannot clean up.
      WinDestroyWindow ( hwndMenu   );
      WinDestroyPointer( ico_tasks  );
      WinDestroyPointer( ico_detach );
      WinDestroyPointer( ico_dos    );
      WinDestroyPointer( ico_os2fs  );
      WinDestroyPointer( ico_os2vio );
      WinDestroyPointer( ico_pm     );

      mrc = pWidget->pfnwpDefWidgetProc( hwnd, msg, mp1, mp2 );
      break;

    default:
      mrc = pWidget->pfnwpDefWidgetProc( hwnd, msg, mp1, mp2 );
  }

  return (mrc);
}

/********************************************************************
 * Exported procedures
 ********************************************************************/

/*
 *@@ WgtInitModule:
 *      required export with ordinal 1, which must tell
 *      the XCenter how many widgets this DLL provides,
 *      and give the XCenter an array of XCENTERWIDGETCLASS
 *      structures describing the widgets.
 *
 *      With this call, you are given the module handle of
 *      XFLDR.DLL. For convenience, and if you have the full
 *      XWorkplace source code, you could resolve imports
 *      for some useful functions which are exported thru
 *      src\shared\xwp.def. We don't do this here.
 *
 *      This function must also register the PM window classes
 *      which are specified in the XCENTERWIDGETCLASS array
 *      entries. For this, you are given a HAB which you
 *      should pass to WinRegisterClass. For the window
 *      class style (4th param to WinRegisterClass),
 *      you should specify
 *
 +          CS_PARENTCLIP | CS_SIZEREDRAW | CS_SYNCPAINT
 *
 *      Your widget window _will_ be resized by the XCenter,
 *      even if you're not planning it to be.
 *
 *      This function only gets called _once_ when the widget
 *      DLL has been successfully loaded by the XCenter. If
 *      there are several instances of a widget running (in
 *      the same or in several XCenters), this function does
 *      not get called again. However, since the XCenter unloads
 *      the widget DLLs again if they are no longer referenced
 *      by any XCenter, this might get called again when the
 *      DLL is re-loaded.
 *
 *      There will ever be only one load occurence of the DLL.
 *      The XCenter manages sharing the DLL between several
 *      XCenters. As a result, it doesn't matter if the DLL
 *      has INITINSTANCE etc. set or not.
 *
 *      If this returns 0, this is considered an error, and the
 *      DLL will be unloaded again immediately.
 *
 *      If this returns any value > 0, *ppaClasses must be
 *      set to a static array (best placed in the DLL's
 *      global data) of XCENTERWIDGETCLASS structures,
 *      which must have as many entries as the return value.
 */

ULONG __export EXPENTRY
WgtInitModule( HAB hab,             // XCenter's anchor block
               HMODULE hmodPlugin,  // module handle of the widget DLL
               HMODULE hmodXFLDR,   // XFLDR.DLL module handle
               PXCENTERWIDGETCLASS *ppaClasses,
               PSZ pszErrorMsg )    // if 0 is returned, 500 bytes of error msg
{
  ULONG ulrc = 0;

  // Save the DLL hmod in a global variable
  // not really needed but I'm lazy
  hmodMe = hmodPlugin;

  // register our PM window class
  if( !WinRegisterClass( hab,
                         WNDCLASS_WIDGET_TASKS,
                         fnwpWxTaskWidget,
                         CS_PARENTCLIP | CS_SIZEREDRAW | CS_SYNCPAINT,
                         sizeof(PVOID))) // extra memory to reserve for QWL_USER
  {
    // error registering class: report error then
    strcpy( pszErrorMsg, "WinRegisterClass failed." );
  } else {
    // no error:
    // return widget classes array
    *ppaClasses = G_WidgetClasses;

    // return no. of classes in this DLL (one here):
    ulrc = sizeof(G_WidgetClasses) / sizeof(G_WidgetClasses[0]);
  }

  return ulrc;
}

/*
 *@@ WgtUnInitModule:
 *      optional export with ordinal 2, which can clean
 *      up global widget class data.
 *
 *      This gets called by the XCenter right before
 *      a widget DLL gets unloaded. Note that this
 *      gets called even if the "init module" export
 *      returned 0 (meaning an error) and the DLL
 *      gets unloaded right away.
 */

VOID __export EXPENTRY
WgtUnInitModule( void )
{}

/*
 *@@ WgtQueryVersion:
 *      this new export with ordinal 3 can return the
 *      XWorkplace version number which is required
 *      for this widget to run. For example, if this
 *      returns 0.9.10, this widget will not run on
 *      earlier XWorkplace versions.
 *
 *      NOTE: This export was mainly added because the
 *      prototype for the "Init" export was changed
 *      with V0.9.9. If this returns 0.9.9, it is
 *      assumed that the INIT export understands
 *      the new FNWGTINITMODULE_099 format (see center.h).
 *
 *@@added V0.9.9 (2001-02-06) [umoeller]
 */

VOID __export EXPENTRY
WgtQueryVersion( PULONG pulMajor,
                 PULONG pulMinor,
                 PULONG pulRevision )
{
  // report 0.9.12
  *pulMajor = 0;
  *pulMinor = 9;
  *pulRevision = 12;
}

/*
 *@@ CleanString:
 *      this cleans specified string from useless characters.
 */

void
CleanString( char* p )
{
  for( ; *p; ++p ) {
    if( *p == '\n' || *p == '\r' ) {
      *p =  ' ';
    }
  }
}

/*
 *@@ MeasureMenuItem:
 *      this calculates the height and width for an
 *      specified menu item.
 */

MRESULT
MeasureMenuItem( POWNERITEM poi )
{
  TASKDATA* data = (TASKDATA*)poi->hItem;
  ULONG cx_icon = WinQuerySysValue( HWND_DESKTOP, SV_CXICON );
  ULONG cy_icon = WinQuerySysValue( HWND_DESKTOP, SV_CYICON );

  RECTL rect = { 0, 0, 32000, 32000 };

  WinDrawText( poi->hps, -1, data->szTitle, &rect, 0, 0, DT_QUERYEXTENT | DT_LEFT | DT_VCENTER );

  poi->rclItem.xLeft   = 0;
  poi->rclItem.xRight  = max( cx_icon/2 + 4, rect.xRight - rect.xLeft   ) + 10;
  poi->rclItem.yBottom = 0;
  poi->rclItem.yTop    = max( cy_icon/2 + 2, rect.yTop   - rect.yBottom ) + 1;

  return MRFROMLONG( poi->rclItem.yTop );
}

/*
 *@@ DrawMenuItem:
 *      this is called each time an item is to be drawn.
 */

MRESULT
DrawMenuItem( POWNERITEM poi )
{
  TASKDATA* data = (TASKDATA*)poi->hItem;
  BOOL   select  = (poi->fsAttribute & MIA_HILITED) ? TRUE : FALSE;
  ULONG  cx_icon = WinQuerySysValue( HWND_DESKTOP, SV_CXICON );
  ULONG  cy_icon = WinQuerySysValue( HWND_DESKTOP, SV_CYICON );
  POINTL pos;
  RECTL  rect = poi->rclItem;

  WinFillRect( poi->hps, &rect, select ? SYSCLR_MENUHILITEBGND : SYSCLR_MENU );

  rect.xLeft += 2;
  rect.yBottom += ( rect.yTop - rect.yBottom - cy_icon/2 ) / 2;
  if( data->hIcon != NULLHANDLE ) {
    WinDrawPointer( poi->hps, rect.xLeft + 1, rect.yBottom + 1, data->hIcon, DP_MINI );
  }

  rect.xLeft += cx_icon/2 + 5;
  rect.yBottom = poi->rclItem.yBottom;
  WinDrawText( poi->hps, -1, data->szTitle, &rect,
                    select ? SYSCLR_MENUHILITE : SYSCLR_MENUTEXT, 0,
                    DT_LEFT | DT_VCENTER );

  // Well well well... And now ;) we are going to fix A great-Warp4-Menu-Bug
  // Make to redraw erased parts of menu's window border
  // Define something like #ifdef WARP_3 if you are running OS/2 Warp 3.x

  // vertical "light" line
  pos.x = poi->rclItem.xLeft;
  pos.y = 1;
  GpiSetColor( poi->hps, SYSCLR_BUTTONLIGHT );
  GpiMove( poi->hps, &pos );
  pos.y = poi->rclItem.yTop;
  GpiLine( poi->hps, &pos );

  // horizontal "dark" line
  pos.x = 1;
  pos.y = 1;
  GpiSetColor( poi->hps, SYSCLR_BUTTONDARK );
  GpiMove( poi->hps, &pos );
  pos.x = poi->rclItem.xRight;
  GpiLine( poi->hps, &pos );

  poi->fsAttributeOld = (poi->fsAttribute &= ~MIA_HILITED);
  return MRFROMLONG( TRUE );
}

/*
 *@@ winhInsertMenuItem:
 *      this inserts one one menu item into a given menu.
 *
 *      Returns the return value of the MM_INSERTITEM msg:
 *      --  MIT_MEMERROR:    space allocation for menu item failed
 *      --  MIT_ERROR:       other error
 *      --  other:           zero-based index of new item in menu.
 */


SHORT
winhInsertMenuItem( HWND hwndMenu,
                      // in:  menu to insert item into
                    SHORT iPosition,
                      // in:  zero-based index of where to insert or MIT_END
                    SHORT sItemId,
                      // in:  ID of new menu item
                    const char *pcszItemTitle,
                      // in:  title of new menu item
                    SHORT afStyle,
                      // in:  MIS_* style flags.
                      // Valid menu item styles are:
                      // --  MIS_SUBMENU
                      // --  MIS_SEPARATOR
                      // --  MIS_BITMAP: the display object is a bit map.
                      // --  MIS_TEXT: the display object is a text string.
                      // --  MIS_BUTTONSEPARATOR:
                      //          The item is a menu button. Any menu can have zero,
                      //          one, or two items of this type.  These are the last
                      //          items in a menu and are automatically displayed after
                      //          a separator bar. The user cannot move the cursor to
                      //          these items, but can select them with the pointing
                      //          device or with the appropriate key.
                      // --  MIS_BREAK: the item begins a new row or column.
                      // --  MIS_BREAKSEPARATOR:
                      //          Same as MIS_BREAK, except that it draws a separator
                      //          between rows or columns of a pull-down menu.
                      //          This style can only be used within a submenu.
                      // --  MIS_SYSCOMMAND:
                      //          menu posts a WM_SYSCOMMAND message rather than a
                      //          WM_COMMAND message.
                      // --  MIS_OWNERDRAW:
                      //          WM_DRAWITEM and WM_MEASUREITEM notification messages
                      //          are sent to the owner to draw the item or determine its size.
                      // --  MIS_HELP:
                      //          menu posts a WM_HELP message rather than a
                      //          WM_COMMAND message.
                      // --  MIS_STATIC
                      //          This type of item exists for information purposes only.
                      //          It cannot be selected with the pointing device or
                      //          keyboard.
                    SHORT afAttr,
                      // in:  MIA_* attribute flags
                      // Valid menu item attributes (afAttr) are:
                      // --  MIA_HILITED: if and only if, the item is selected.
                      // --  MIA_CHECKED: a check mark appears next to the item (submenu only).
                      // --  MIA_DISABLED: item is disabled and cannot be selected.
                      //         The item is drawn in a disabled state (gray).
                      // --  MIA_FRAMED: a frame is drawn around the item (top-level menu only).
                      // --  MIA_NODISMISS:
                      //          if the item is selected, the submenu remains down. A menu
                      //          with this attribute is not hidden until the  application
                      //          or user explicitly does so, for example by selecting either
                      //          another menu on the action bar or by pressing the escape key.
                    TASKDATA* data )
{
  MENUITEM mi;
  SHORT src = MIT_ERROR;

  mi.iPosition = iPosition;
  mi.afStyle = afStyle;
  mi.afAttribute = afAttr;
  mi.id = sItemId;
  mi.hwndSubMenu = 0;
  mi.hItem = (ULONG)data;

  src = SHORT1FROMMR( WinSendMsg( hwndMenu,
                                  MM_INSERTITEM,
                                  (MPARAM)&mi,
                                  (MPARAM)pcszItemTitle ));

  if( src != MIT_ERROR && src != MIT_MEMERROR )
  {
    // If it is necessary the list of tasks is formatted to
    // several columns.

    RECTL item_rect;
    RECTL desk_rect;

    if( WinSendMsg( hwndMenu, MM_QUERYITEMRECT,
                    MPFROM2SHORT( sItemId, FALSE ), MPFROMP(&item_rect))
        && WinQueryWindowRect( HWND_DESKTOP, &desk_rect ))
    {
      int items = ( 0.80 * ( desk_rect.yTop - desk_rect.yBottom )) /
                  ( item_rect.yTop - item_rect.yBottom );

      if( src % items == 0 )
      {
        mi.afStyle |= MIS_BREAKSEPARATOR;
        WinSendMsg( hwndMenu, MM_SETITEM, MPFROM2SHORT( 0, FALSE ),
                                          MPFROMP( &mi ));
      }
    }
  }

  return (src);
}

/*
 *@@ FillMenu:
 */

void
FillMenu( HWND hwndMenu )
{
  char      szPidPath[1024];
  char      szBuffer[1024];
  char*     pBuf;
  QSPTRREC* pRecHead;
  QSPREC*   pApp;
  int       numItems, iCounter;
  PVOID     pBuffer;
  PSWBLOCK  pSB;
  TASKDATA* data;
  SHORT     id = ID_ITEM_FIRST;

  // Delete all current items
  numItems = LONGFROMMR( WinSendMsg( hwndMenu, MM_QUERYITEMCOUNT, 0, 0 ));

  while( numItems-- )
  {
    MENUITEM mi;
    SHORT    id = LONGFROMMR( WinSendMsg( hwndMenu,
                              MM_ITEMIDFROMPOSITION, MPFROMSHORT(0), 0 ));

    if( WinSendMsg( hwndMenu, MM_QUERYITEM,
                    MPFROM2SHORT( id, FALSE ), MPFROMP( &mi )))
    {
      if( mi.hItem )
      {
        data = (TASKDATA*)mi.hItem;

        free( data->szTitle );
        free( data );
      }
    }

    WinSendMsg( hwndMenu, MM_DELETEITEM,
                MPFROM2SHORT( id, FALSE ), 0 );
  }

  if( bType )
  {
    // Ctrl is pressed, get the processes in the system.
    pBuf = (char*)malloc( 0x8000 );

    // Get processes
    DosQuerySysState( QS_PROCESS, 0, 0, 0, (char*)pBuf, 0x8000 );

    // Point to first process
    pRecHead = (QSPTRREC*)pBuf;
    pApp = pRecHead->pProcRec;

    // While its a process record
    while( pApp->RecType == 1 )
    {
      // Get module name for process
      memset( szPidPath, 0, _MAX_PATH );
      DosQueryModuleName( pApp->hMte, 512, szPidPath );

      if( strlen( szPidPath ) == 0 )
      {
        // If no hit is the kernel
        if( pApp->type == 1 ) {
          sprintf( szPidPath, "VDM (%d)", pApp->pid );
        } else {
          strcpy ( szPidPath, "*SYSINIT" );
        }
      }
      else
      {
        // Else trim the path
        strcpy ( szBuffer, FileGetFileExt( szPidPath ));
        sprintf( szPidPath, "%s (%d)", szBuffer, pApp->pid );
      }

      // add to menu
      data = (TASKDATA*)malloc( sizeof( TASKDATA ));
      data->szTitle = strdup( szPidPath );
      data->hWindow = NULLHANDLE;
      data->pid = pApp->pid;

      switch( pApp->type ) {
        case  0: data->hIcon = ico_os2fs;  break;
        case  1: data->hIcon = ico_dos;    break;
        case  2: data->hIcon = ico_os2vio; break;
        case  3: data->hIcon = ico_pm;     break;
        case  4: data->hIcon = ico_detach; break;
        default: data->hIcon = NULLHANDLE; break;
      }

      winhInsertMenuItem( hwndMenu, MIT_END,
                          id++, szPidPath, MIS_OWNERDRAW, 0, data );
      // get next record
      pApp=(QSPREC *)((pApp->pThrdRec) + pApp->cTCB);
    }

    free(pBuf);
  }
  else
  {
    // Get number of items in switchlist
    numItems = WinQuerySwitchList( WinQueryAnchorBlock( hwndMenu ), NULL, 0 );

    // Get all items into buffer
    pBuffer = malloc(( numItems * sizeof(SWENTRY)) + sizeof(HSWITCH));

    WinQuerySwitchList( WinQueryAnchorBlock(hwndMenu),
                        (SWBLOCK*)pBuffer,
                        (numItems * sizeof(SWENTRY)) + sizeof(HSWITCH));

    pSB = (PSWBLOCK)(pBuffer);
    for( iCounter = 0; iCounter < numItems; iCounter++ )
    {
      // Should be JUMPABLE and VISIBLE to show in list
      if( pSB->aswentry[iCounter].swctl.uchVisibility == SWL_VISIBLE )
      {

        // Put in menu
        data = (TASKDATA*)malloc( sizeof( TASKDATA ));
        data->szTitle = strdup( pSB->aswentry[iCounter].swctl.szSwtitle );
        data->hWindow = pSB->aswentry[iCounter].swctl.hwnd;
        data->pid = pSB->aswentry[iCounter].swctl.idProcess;
        CleanString( data->szTitle );

        if( pSB->aswentry[iCounter].swctl.hwndIcon != NULLHANDLE ) {
          data->hIcon = pSB->aswentry[iCounter].swctl.hwndIcon;
        } else {
          data->hIcon = (HPOINTER)WinSendMsg( data->hWindow, WM_QUERYICON, 0, 0 );
        }

        winhInsertMenuItem( hwndMenu, MIT_END, id++,
                            pSB->aswentry[iCounter].swctl.szSwtitle,
                            MIS_OWNERDRAW, 0, data );
      }
    }
    free(pBuffer);
  }
}

/*
 *@@ FileGetFileExt:
 *      Get filename + ext from full filename.
 */

char*
FileGetFileExt( char* szFullFile )
{
  static char szReturn[_MAX_EXT];
  char        szDrive[_MAX_DRIVE];
  char        szDir[_MAX_DIR];
  char        szName[_MAX_FNAME];
  char        szExt[_MAX_EXT];

  _splitpath( szFullFile,
              szDrive,
              szDir,
              szName,
              szExt
            );

  sprintf( szReturn, "%s%s", szName, szExt );
  return ( szReturn );
}

/*
 *@@ Log:
 *      Skickar information till log-fil.
 */

void
Log( char *szData )
{
  FILE*      hLogFile;
  time_t     ltime;
  struct tm* curtime;

  time( &ltime );
  curtime = localtime( &ltime );

  hLogFile = fopen( "debug.log", "a" );

  fprintf( hLogFile, "%.4d-%.2d-%.2d, %.2d:%.2d:%.2d\n\t%s\n",
           1900 + curtime->tm_year,
           curtime->tm_mon + 1,
           curtime->tm_mday,
           curtime->tm_hour,
           curtime->tm_min,
           curtime->tm_sec,
           szData
          );

  fclose( hLogFile );
}


/*
 *@@ DosKillFastIo:
 *      Kill process using fastio$ by Holger Veit.
 *
 *      Copyright (C) 1996 by Holger.Veit@gmd.de,
 *      This code may be freely used in own software.
 */

APIRET
DosKillFastIo( PID pid )
{
  APIRET rc;
  HFILE  hfd;
  ULONG  action, plen;


  if(( rc = DosOpen((PSZ)"/dev/fastio$", (PHFILE)&hfd,
                    (PULONG)&action, (ULONG)0, FILE_SYSTEM, FILE_OPEN,
                    OPEN_SHARE_DENYNONE | OPEN_FLAGS_NOINHERIT | OPEN_ACCESS_READONLY,
                    (ULONG)0)) != NO_ERROR )
  {
    return rc;
  }

  if(( rc = DosDevIOCtl( hfd, (ULONG)0x76, (ULONG)0x65,
                        (PULONG*)&pid, sizeof(USHORT), &plen, NULL, 0, NULL)) != 0 )
  {
    DosClose( hfd );
    return rc;
  }

  DosClose(hfd);
  return NO_ERROR;
}

/*
 *@@ SwitchTo:
 *      Mostly stolen from xCenter winlist widget.
 */

void SwitchTo( TASKDATA* data )
{
  SWP swp;

  if( WinQueryWindowPos( data->hWindow, &swp ))
  {
    if( swp.fl & ( SWP_HIDE | SWP_MINIMIZE ))
    {
      // window is hidden or minimized: restore and activate
      HSWITCH hsw = WinQuerySwitchHandle( data->hWindow, data->pid );

      if( hsw )
      {
        // first check if the thing is hidden
        if( swp.fl & SWP_HIDE ) {
            WinSetWindowPos( data->hWindow, 0, 0, 0, 0, 0, SWP_SHOW );
        }

        if( !WinSwitchToProgram( hsw ))
        {
          // OK, now we have the program active, but it's
          // probably in the background...
          WinSetWindowPos( data->hWindow, HWND_TOP, 0, 0, 0, 0,
                           SWP_SHOW | SWP_ACTIVATE | SWP_ZORDER );
        }
      }
    }
    else
    {
      // not minimized: see if it's active
      HWND hwndActive = WinQueryActiveWindow( HWND_DESKTOP );

      if( hwndActive != data->hWindow )
      {
        // not minimized, not active:
        WinSetActiveWindow( HWND_DESKTOP, data->hWindow );
      }
    }
  }
}


