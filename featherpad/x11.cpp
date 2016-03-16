/*
 * Copyright (C) Pedram Pourang (aka Tsu Jan) 2014 <tsujan2000@gmail.com>
 *
 * FeatherPad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FeatherPad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QString>
#include "x11.h"

#if defined Q_WS_X11 || defined Q_OS_LINUX
#include <X11/Xatom.h>
#include <QX11Info>
#endif

namespace FeatherPad {

/*************************************************************
 *** These are all X11 related functions FeatherPad uses   ***
 *** because Qt does not fetch enough information on X11. ***
 *************************************************************/

// Get the curent virtual desktop.
long fromDesktop()
{
    long res = -1;

#if defined Q_WS_X11 || defined Q_OS_LINUX
    Display  *disp = QX11Info::display(); // memory leak with XOpenDisplay(NULL)?
    if (!disp) return res;

    Atom actual_type;
    int actual_format;
    long unsigned nitems;
    long unsigned bytes;
    long *data = NULL;
    int status;

    /* QX11Info::appRootWindow() or even RootWindow (disp, 0)
       could be used instead of XDefaultRootWindow (disp) */
    status = XGetWindowProperty (disp, XDefaultRootWindow (disp),
                                 XInternAtom (disp, "_NET_CURRENT_DESKTOP", True),
                                 0, (~0L), False, AnyPropertyType,
                                 &actual_type, &actual_format, &nitems, &bytes,
                                 (unsigned char**)&data);
    if (status != Success) return res;

    if (data)
    {
        res = *data;
        XFree (data);
    }
#endif

    return res;
}
/*************************/
// Get the list of all X11 windows in current stacking order (from bottommost to topmost).
#if defined Q_WS_X11 || defined Q_OS_LINUX
QList<Window> listXWins (Window w)
{
    QList<Window> windows;

    Window root;
    Window parent;
    Window *children;
    unsigned int childrenCount;

    if (XQueryTree (QX11Info::display(), w, &root, &parent, &children, &childrenCount))
    {
        for (unsigned int i = 0; i < childrenCount; ++i)
        {
            windows << children[i];
            windows << listXWins (children[i]);
        }
        XFree (children);
    }

    return windows;
}
#endif
/*************************/
// Get the desktop of a window.
long onWhichDesktop (Window w)
{
    long res = -1;

#if defined Q_WS_X11 || defined Q_OS_LINUX
    Display *disp = QX11Info::display();
    if (!disp) return res;

    Atom wm_desktop = XInternAtom (disp, "_NET_WM_DESKTOP", False);
    Atom type_ret;
    int fmt_ret;
    unsigned long nitems_ret;
    unsigned long bytes_after_ret;

    long *desktop = NULL;

    int status = XGetWindowProperty (disp, w,
                                     wm_desktop,
                                     0, 1, False, XA_CARDINAL,
                                     &type_ret, &fmt_ret, &nitems_ret, &bytes_after_ret,
                                     (unsigned char**)&desktop);
    if (status != Success) return res;
    if (desktop)
    {
        res = (long)desktop[0];
        XFree (desktop);
    }
#endif

    return res;
}
/*************************/
// The following two functions are adapted from x11tools.cpp,
// belonging to kadu (https://github.com/vogel/kadu).
// They were needed because isMinimized() may not detect
// the shaded state with all WMs.

bool isWindowShaded (Window window)
{
#if defined Q_WS_X11 || defined Q_OS_LINUX
    Display *disp = QX11Info::display();
    if (!disp) return false;

    Atom property = XInternAtom (disp, "_NET_WM_STATE", False);
    if (property == None) return false;
    Atom atom = XInternAtom (disp, "_NET_WM_STATE_SHADED", False);
    if (atom == None) return false;

    Atom *atoms = NULL;
    Atom realtype;
    int realformat;
    unsigned long nitems, left;
    int result = XGetWindowProperty (disp, window,
                                     property,
                                     0L, 8192L, False, XA_ATOM,
                                     &realtype, &realformat, &nitems, &left,
                                     (unsigned char**)&atoms);
    if (result != Success || realtype != XA_ATOM)
        return false;
    for (unsigned long i = 0; i < nitems; i++)
    {
        if (atoms[i] == atom)
        {
            XFree (atoms);
            return true;
        }
    }
    XFree (atoms);
#endif
    return false;
}
/*************************/
void unshadeWindow (Window window)
{
#if defined Q_WS_X11 || defined Q_OS_LINUX
    Display *disp = QX11Info::display();
    if (!disp) return;

    Atom atomtype = XInternAtom (disp, "_NET_WM_STATE", False);
    Atom atommessage = XInternAtom (disp, "_NET_WM_STATE_SHADED", False);
    XEvent xev;
    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.serial = 0;
    xev.xclient.send_event = True;
    xev.xclient.window = window;
    xev.xclient.message_type = atomtype;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 0; // unshade
    xev.xclient.data.l[1] = atommessage;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;
    XSendEvent (disp, DefaultRootWindow (disp),
                False,
                SubstructureRedirectMask | SubstructureNotifyMask,
                &xev);
    XFlush (disp);
#endif
}

}
