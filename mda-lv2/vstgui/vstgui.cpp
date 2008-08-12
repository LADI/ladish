/* ----------------------------------------------------------------------------
 * VSTGUI for X11/LV2/PNG
 * Author: Dave Robillard
 * Released under the revised BSD license, as below
 * ----------------------------------------------------------------------------
 *
 * Based on:
 * ----------------------------------------------------------------------------
 * VSTGUIL: Graphical User Interface Framework for VST plugins on LINUX
 * Version: 0.1, Date: 2007/01/21
 * Author: kRAkEn/gORe
 *
 * Which was based on:
 * ----------------------------------------------------------------------------
 * VSTGUI: Graphical User Interface Framework for VST plugins
 * Version 3.0       $Date: 2005/08/12 12:45:00 $
 * ----------------------------------------------------------------------------
 * VSTGUI LICENSE
 * 2004, Steinberg Media Technologies, All Rights Reserved
 * ----------------------------------------------------------------------------
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the Steinberg Media Technologies nor the names of
 *     its contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A  PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string.h>

#include "vstgui.h"
#include "audioeffectx.h"
#include "vstkeycode.h"

//-----------------------------------------------------------------------------
// Some defines
//-----------------------------------------------------------------------------
#ifdef DEBUG
    #define DBG(x)              std::cout << x << std::endl;
    #define assertfalse         assert (false);
#else
    #define DBG(x)
    #define assertfalse
#endif

#define USE_ALPHA_BLEND            0
#define USE_CLIPPING_DRAWRECT      1
#define NEW_UPDATE_MECHANISM       1


//-----------------------------------------------------------------------------
// our global display
Display* display = 0;


//-----------------------------------------------------------------------------
// AEffGUIEditor Implementation
//-----------------------------------------------------------------------------
#define kIdleRate    100 // host idle rate in ms
#define kIdleRate2    50
#define kIdleRateMin   4 // minimum time between 2 idles in ms

//-----------------------------------------------------------------------------
VstInt32 AEffGUIEditor::knobMode = kCircularMode;

//-----------------------------------------------------------------------------
AEffGUIEditor::AEffGUIEditor (AudioEffect* effect)
: AEffEditor (effect),
  lLastTicks (0),
  inIdleStuff (false),
  bundlePath(NULL),
  frame (0)
{
    rect.left = rect.top = rect.right = rect.bottom = 0;
    lLastTicks = getTicks ();

    effect->setEditor (this);
}
	
//-----------------------------------------------------------------------------
AEffGUIEditor::~AEffGUIEditor ()
{
}

//-----------------------------------------------------------------------------
void AEffGUIEditor::setParameter (VstInt32 index, float value)
{}

//-----------------------------------------------------------------------------
void AEffGUIEditor::beginEdit (VstInt32 index)
{
    ((AudioEffectX*)effect)->beginEdit (index);
}

//-----------------------------------------------------------------------------
void AEffGUIEditor::endEdit (VstInt32 index)
{
    ((AudioEffectX*)effect)->endEdit (index);
}

//-----------------------------------------------------------------------------
#if VST_2_1_EXTENSIONS
long AEffGUIEditor::onKeyDown (VstKeyCode& keyCode)
{
    return frame && frame->onKeyDown (keyCode) == 1 ? 1 : 0;
}

//-----------------------------------------------------------------------------
long AEffGUIEditor::onKeyUp (VstKeyCode& keyCode)
{
    return frame && frame->onKeyUp (keyCode) == 1 ? 1 : 0;
}

//-----------------------------------------------------------------------------
long AEffGUIEditor::setKnobMode (VstInt32 val)
{
    knobMode = val;
    return 1;
}

//-----------------------------------------------------------------------------
bool AEffGUIEditor::onWheel (float distance)
{
/*
    if (frame)
    {
        CDrawContext context (frame, NULL, systemWindow);
        CPoint where;
        context.getMouseLocation (where);
        return frame->onWheel (&context, where, distance);
    }
*/
    return false;
}
#endif

//-----------------------------------------------------------------------------
long AEffGUIEditor::getRect (ERect **ppErect)
{
    *ppErect = &rect;
    return 1;
}

//-----------------------------------------------------------------------------
void AEffGUIEditor::idle ()
{
    if (inIdleStuff)
        return;

    AEffEditor::idle ();
    if (frame)
        frame->idle ();
}

//-----------------------------------------------------------------------------
void AEffGUIEditor::wait (unsigned int ms)
{
    usleep (ms * 1000);
}

//-----------------------------------------------------------------------------
unsigned int AEffGUIEditor::getTicks ()
{
    return _getTicks ();
}

//-----------------------------------------------------------------------------
void AEffGUIEditor::doIdleStuff ()
{
    // get the current time
    unsigned int currentTicks = getTicks ();

    // YG TEST idle ();
    if (currentTicks < lLastTicks)
    {
        wait (kIdleRateMin);

        currentTicks += kIdleRateMin;
        if (currentTicks < lLastTicks - kIdleRate2)
            return;
    }

    idle ();

    // save the next time
    lLastTicks = currentTicks + kIdleRate;

    inIdleStuff = true;

    if (effect)
        effect->masterIdle ();

    inIdleStuff = false;
}

void AEffGUIEditor::update ()
{
    if (frame)
        frame->invalidate (CRect (0, 0, rect.right, rect.bottom));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//---For Debugging------------------------
#if DEBUG
long gNbCBitmap = 0;
long gNbCView = 0;
long gNbCDrawContext = 0;
long gNbCOffscreenContext = 0;
long gBitmapAllocation = 0;
long gNbDC = 0;
#include <stdarg.h>
void DebugPrint (const char *format, ...);
void DebugPrint (const char *format, ...)
{
    char string[300];
    va_list marker;
    va_start (marker, format);
    vsprintf (string, format, marker);
    if (!strcmp(string, ""))
        strcpy (string, "Empty string\n");
    fprintf (stderr, string);
}
#endif
//---End For Debugging------------------------

#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define XDRAWPARAM display, (Window)pWindow, (GC) pSystemContext
#define XWINPARAM  display, (Window)pFrame->getWindow()
#define XGCPARAM   display, (GC) pSystemContext

// #define XDRAWPARAM display, (Window)pWindow, (GC)pSystemContext
// #define XWINPARAM  display, (Window)pWindow
// #define XGCPARAM   display, (GC)pSystemContext

// init the static variable about font
bool gFontInit = false;
XFontStruct *gFontStructs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

struct SFontTable { const char* name; const char* string; };

static SFontTable gFontTable[] = {
  {"SystemFont",          "-*-fixed-*-*-*--12-*-*-*-*-*-*-*"}, // kSystemFont
  {"NormalFontVeryBig",   "-*-fixed-*-*-*--18-*-*-*-*-*-*-*"}, // kNormalFontVeryBig
  {"NormalFontBig",       "-*-fixed-*-*-*--18-*-*-*-*-*-*-*"}, // kNormalFontBig
  {"NormalFont",          "-*-fixed-*-*-*--12-*-*-*-*-*-*-*"}, // kNormalFont
  {"NormalFontSmall",     "-*-courier-*-*-*--10-*-*-*-*-*-*-*"}, // kNormalFontSmall
  {"NormalFontSmaller",   "-*-courier-*-*-*--9-*-*-*-*-*-*-*"}, // kNormalFontSmaller
  {"NormalFontVerySmall", "-*-courier-*-*-*--8-*-*-*-*-*-*-*"}, // kNormalFontVerySmall
  {"SymbolFont",          "-*-fixed-*-*-*--12-*-*-*-*-*-*-*"}  // kSymbolFont
};

long gStandardFontSize[] = { 12, 18, 18, 12, 10, 9, 8, 13 };

// declaration of different local functions
static long convertPoint2Angle (CPoint &pm, CPoint &pt);

// stuff for color
long CDrawContext::nbNewColor = 0;

//-----------------------------------------------------------------------------
bool CRect::pointInside (const CPoint& where) const
{
    return where.h >= left && where.h < right && where.v >= top && where.v < bottom;
}

//-----------------------------------------------------------------------------
bool CRect::isEmpty () const
{
    if (right <= left)
        return true;
    if (bottom <= top)
        return true;
    return false;
}

//-----------------------------------------------------------------------------
void CRect::bound (const CRect& rect)
{
    if (left < rect.left)
        left = rect.left;
    if (top < rect.top)
        top = rect.top;
    if (right > rect.right)
        right = rect.right;
    if (bottom > rect.bottom)
        bottom = rect.bottom;
    if (bottom < top)
        bottom = top;
    if (right < left)
        right = left;
}

namespace VSTGUI {

CColor kTransparentCColor = {255, 255, 255,   0};
CColor kBlackCColor       = {  0,   0,   0, 255};
CColor kWhiteCColor       = {255, 255, 255, 255};
CColor kGreyCColor        = {127, 127, 127, 255};
CColor kRedCColor         = {255,   0,   0, 255};
CColor kGreenCColor       = {  0, 255,   0, 255};
CColor kBlueCColor        = {  0,   0, 255, 255};
CColor kYellowCColor      = {255, 255,   0, 255};
CColor kMagentaCColor     = {255,   0, 255, 255};
CColor kCyanCColor        = {  0, 255, 255, 255};

#define kDragDelay 0

//-----------------------------------------------------------------------------
// CDrawContext Implementation
//-----------------------------------------------------------------------------
/**
 * CDrawContext constructor.
 * @param inFrame the parent CFrame
 * @param inSystemContext the platform system context, can be NULL
 * @param inWindow the platform window object
 */
CDrawContext::CDrawContext (CFrame *inFrame, void *inSystemContext, void *inWindow)
: pSystemContext (inSystemContext)
, pWindow (inWindow)
, pFrame (inFrame)
, fontSize (-1)
, fontStyle (0)
, fontId (kNumStandardFonts)
, frameWidth (0)
, lineStyle (kLineOnOffDash)
, drawMode (kAntialias)
{
#if DEBUG
    gNbCDrawContext++;
#endif

    // initialize values
    if (pFrame)
        pFrame->getViewSize (clipRect);
    else
        clipRect (0, 0, 1000, 1000);

    const CColor notInitalized = {0, 0, 0, 0};
    frameColor = notInitalized;
    fillColor  = notInitalized;
    fontColor  = notInitalized;

    // offsets use by offscreen
    offset (0, 0);
    offsetScreen (0, 0);

    // set the current font
    if (pSystemContext)
    {
        // set the default values
        setFont (kNormalFont);
        setFrameColor (kWhiteCColor);
        setLineStyle (kLineSolid);
        setLineWidth (1);
        setFont (kSystemFont);
        setDrawMode (kCopyMode);
    }
}

//-----------------------------------------------------------------------------
CDrawContext::~CDrawContext ()
{
#if DEBUG
    gNbCDrawContext--;
#endif
}

//-----------------------------------------------------------------------------
void CDrawContext::setLineStyle (CLineStyle style)
{
    if (lineStyle == style)
        return;

    lineStyle = style;

    long line_width;
    long line_style;
    if (frameWidth == 1)
        line_width = 0;
    else
        line_width = frameWidth;

    switch (lineStyle)
    {
    case kLineOnOffDash:
        line_style = LineOnOffDash;
        break;
    default:
        line_style = LineSolid;
        break;
    }

    XSetLineAttributes (XGCPARAM, line_width, line_style, CapNotLast, JoinRound);
}

//-----------------------------------------------------------------------------
void CDrawContext::setLineWidth (CCoord width)
{
    if (frameWidth == width)
        return;

    frameWidth = width;

    setLineStyle (lineStyle);
}

//-----------------------------------------------------------------------------
void CDrawContext::setDrawMode (CDrawMode mode)
{
    if (drawMode == mode)
        return;

    drawMode = mode;

    long iMode = 0;
    switch (drawMode)
    {
    case kXorMode :
        iMode = GXinvert;
        break;
    case kOrMode :
        iMode = GXor;
        break;
    default:
        iMode = GXcopy;
    }

    ((XGCValues*)pSystemContext)->function = iMode;

    XChangeGC (XGCPARAM, GCFunction, (XGCValues*)pSystemContext);
}

//------------------------------------------------------------------------------
void CDrawContext::setClipRect (const CRect &clip)
{
    CRect _clip (clip);
    _clip.offset (offset.h, offset.v);

    if (clipRect == _clip)
        return;

    clipRect = _clip;

    XRectangle r;
    r.x = 0;
    r.y = 0;
    r.width  = clipRect.right - clipRect.left + 1;
    r.height = clipRect.bottom - clipRect.top + 1;

    XSetClipRectangles (XGCPARAM, clipRect.left, clipRect.top, &r, 1, Unsorted);
}

//------------------------------------------------------------------------------
void CDrawContext::resetClipRect ()
{
    CRect newClip;
    if (pFrame)
        pFrame->getViewSize (newClip);
    else
        newClip (0, 0, 1000, 1000);

    setClipRect (newClip);

    clipRect = newClip;
}

//-----------------------------------------------------------------------------
void CDrawContext::moveTo (const CPoint &_point)
{
    CPoint point (_point);
    point.offset (offset.h, offset.v);

    penLoc = point;
}

//-----------------------------------------------------------------------------
void CDrawContext::lineTo (const CPoint& _point)
{
    CPoint point (_point);
    point.offset (offset.h, offset.v);

    CPoint start (penLoc);
    CPoint end (point);
    if (start.h == end.h)
    {
        if (start.v < -5)
            start.v = -5;
        else if (start.v > 10000)
            start.v = 10000;

        if (end.v < -5)
            end.v = -5;
        else if (end.v > 10000)
            end.v = 10000;
    }
    if (start.v == end.v)
    {
        if (start.h < -5)
            start.h = -5;
        else if (start.h > 10000)
            start.h = 10000;

        if (end.h < -5)
            end.h = -5;
        else if (end.h > 10000)
            end.h = 10000;
    }

    XDrawLine (XDRAWPARAM, start.h, start.v, end.h, end.v);

    // keep trace of the new position
    penLoc = point;
}

//-----------------------------------------------------------------------------
void CDrawContext::drawLines (const CPoint* points, const long& numLines)
{
    // default implementation, when no platform optimized code is implemented
    for (long i = 0; i < numLines * 2; i+=2)
    {
        moveTo (points[i]);
        lineTo (points[i+1]);
    }
}

//-----------------------------------------------------------------------------
void CDrawContext::drawPolygon (const CPoint *pPoints, long numberOfPoints, const CDrawStyle drawStyle)
{
    if (drawStyle == kDrawFilled || drawStyle == kDrawFilledAndStroked)
        fillPolygon (pPoints, numberOfPoints);
    if (drawStyle == kDrawStroked || drawStyle == kDrawFilledAndStroked)
        polyLine (pPoints, numberOfPoints);
}

//-----------------------------------------------------------------------------
void CDrawContext::polyLine (const CPoint *pPoints, long numberOfPoints)
{
    XPoint* pt = (XPoint*)malloc (numberOfPoints * sizeof (XPoint));
    if (!pt)
        return;
    for (long i = 0; i < numberOfPoints; i++)
    {
        pt[i].x = (short)pPoints[i].h + offset.h;
        pt[i].y = (short)pPoints[i].v + offset.v;
    }

    XDrawLines (XDRAWPARAM, pt, numberOfPoints, CoordModeOrigin);

    free (pt);
}

//-----------------------------------------------------------------------------
void CDrawContext::fillPolygon (const CPoint *pPoints, long numberOfPoints)
{
    // convert the points
    XPoint* pt = (XPoint*)malloc (numberOfPoints * sizeof (XPoint));
    if (!pt)
        return;
    for (long i = 0; i < numberOfPoints; i++)
    {
        pt[i].x = (short)pPoints[i].h + offset.h;
        pt[i].y = (short)pPoints[i].v + offset.v;
    }

    XFillPolygon (XDRAWPARAM, pt, numberOfPoints, Convex, CoordModeOrigin);

    free (pt);
}

//-----------------------------------------------------------------------------
void CDrawContext::drawRect (const CRect &_rect, const CDrawStyle drawStyle)
{
    CRect rect (_rect);
    rect.offset (offset.h, offset.v);

    XDrawRectangle (XDRAWPARAM, rect.left, rect.top, rect.width (), rect.height ());
}

//-----------------------------------------------------------------------------
void CDrawContext::fillRect (const CRect &_rect)
{
    CRect rect (_rect);
    rect.offset (offset.h, offset.v);

    // Don't draw boundary
    XFillRectangle (XDRAWPARAM, rect.left + 1, rect.top + 1, rect.width () - 1, rect.height () - 1);
}

//-----------------------------------------------------------------------------
void CDrawContext::drawEllipse (const CRect &_rect, const CDrawStyle drawStyle)
{
    CPoint point (_rect.left + (_rect.right - _rect.left) / 2, _rect.top);
    drawArc (_rect, point, point);
}

//-----------------------------------------------------------------------------
void CDrawContext::fillEllipse (const CRect &_rect)
{
    CRect rect (_rect);
    rect.offset (offset.h, offset.v);

    // Don't draw boundary
    CPoint point (_rect.left + ((_rect.right - _rect.left) / 2), _rect.top);
    fillArc (_rect, point, point);
}

//-----------------------------------------------------------------------------
void CDrawContext::drawPoint (const CPoint &_point, CColor color)
{
    CPoint point (_point);

    CColor oldframecolor = frameColor;
    setFrameColor (color);

    XDrawPoint (XDRAWPARAM, point.h, point.v);

    setFrameColor (oldframecolor);
}

//-----------------------------------------------------------------------------
CColor CDrawContext::getPoint (const CPoint& _point)
{
//    CPoint point (_point);
//    point.offset (offset.h, offset.v);

    assertfalse // not implemented !

    CColor color = kBlackCColor;
    return color;
}

//-----------------------------------------------------------------------------
void CDrawContext::floodFill (const CPoint& _start)
{
//    CPoint start (_start);
//    start.offset (offset.h, offset.v);

    assertfalse // not implemented !
}

//-----------------------------------------------------------------------------
void CDrawContext::drawArc (const CRect &_rect, const float _startAngle, const float _endAngle, const CDrawStyle drawStyle) // in degree
{
    CRect rect (_rect);
    rect.offset (offset.h, offset.v);

    XDrawArc (XDRAWPARAM, rect.left, rect.top, rect.width (), rect.height (),
              (int) _startAngle * 64, (int) _endAngle * 64);
}

//-----------------------------------------------------------------------------
void CDrawContext::drawArc (const CRect &_rect, const CPoint &_point1, const CPoint &_point2)
{
    CRect rect (_rect);
    rect.offset (offset.h, offset.v);
    CPoint point1 (_point1);
    point1.offset (offset.h, offset.v);
    CPoint point2 (_point2);
    point2.offset (offset.h, offset.v);

    int    angle1, angle2;
    if ((point1.v == point2.v) && (point1.h == point2.h))
    {
        angle1 = 0;
        angle2 = 23040; // 360 * 64
    }
    else
    {
        CPoint pm ((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
        angle1 = convertPoint2Angle (pm, point1);
        angle2 = convertPoint2Angle (pm, point2) - angle1;
        if (angle2 < 0)
            angle2 += 23040; // 360 * 64
    }

    XDrawArc (XDRAWPARAM, rect.left, rect.top, rect.width (), rect.height (),
              angle1, angle2);
}

//-----------------------------------------------------------------------------
void CDrawContext::fillArc (const CRect &_rect, const CPoint &_point1, const CPoint &_point2)
{
    CRect rect (_rect);
    rect.offset (offset.h, offset.v);
    CPoint point1 (_point1);
    point1.offset (offset.h, offset.v);
    CPoint point2 (_point2);
    point2.offset (offset.h, offset.v);

    // Don't draw boundary
    int    angle1, angle2;
    if ((point1.v == point2.v) && (point1.h == point2.h))
    {
        angle1 = 0;
        angle2 = 23040; // 360 * 64
    }
    else
    {
        CPoint pm ((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
        angle1 = convertPoint2Angle (pm, point1);
        angle2 = convertPoint2Angle (pm, point2);
    }

    XFillArc (XDRAWPARAM, rect.left, rect.top, rect.width (), rect.height (),
              angle1, angle2);
}

//-----------------------------------------------------------------------------
void CDrawContext::setFontColor (const CColor color)
{
    fontColor = color;

    setFrameColor (fontColor);
}

//-----------------------------------------------------------------------------
void CDrawContext::setFrameColor (const CColor color)
{
    if (frameColor == color)
        return;

    frameColor = color;

    XSetForeground (XGCPARAM, getIndexColor (frameColor));
}

//-----------------------------------------------------------------------------
void CDrawContext::setFillColor (const CColor color)
{
    if (fillColor == color)
        return;

    fillColor = color;

    // set the background for the text
    //XSetBackground (XGCPARAM, getIndexColor (fillColor));
    XSetBackground (display, pFrame->getGC(), getIndexColor (fillColor));

    // set the foreground for the fill
    setFrameColor (fillColor);
}

//-----------------------------------------------------------------------------
void CDrawContext::setFont (CFont fontID, const long size, long style)
{
    if (fontID < 0 || fontID >= kNumStandardFonts)
        fontID = kSystemFont;

    if (fontId == fontID && fontSize == (size != 0 ? size : gStandardFontSize[fontID]) && fontStyle == style)
        return;

    fontStyle = style;
    fontId = fontID;
    if (size != 0)
        fontSize = size;
    else
        fontSize = gStandardFontSize[fontID];

	if (gFontStructs[fontID] != NULL)
	    XSetFont (display, pFrame->getGC(), gFontStructs[fontID]->fid);
	else
		fprintf(stderr, "ERROR: Font not defined\n");

    // keep trace of the current font
    pFontInfoStruct = gFontStructs[fontID];

#ifdef DEBUG
//    if (!pFontInfoStruct) assert (false); // fonts have invalid pointers !
#endif
}

//------------------------------------------------------------------------------
CCoord CDrawContext::getStringWidth (const char *pStr)
{
    CCoord result = 0;

    result = (long) XTextWidth (pFontInfoStruct, pStr, strlen (pStr));

    return result;
}

//-----------------------------------------------------------------------------
void CDrawContext::drawString (const char *string, const CRect &_rect,
                               const short opaque, const CHoriTxtAlign hAlign)
{
    if (!string)
        return;

    CRect rect (_rect);
    rect.offset (offset.h, offset.v);

    int width;
    int fontHeight = pFontInfoStruct->ascent + pFontInfoStruct->descent;
    int xPos;
    int yPos;
    int rectHeight = rect.height ();

    if (rectHeight >= fontHeight)
        yPos = rect.bottom - (rectHeight - fontHeight) / 2;
    else
        yPos = rect.bottom;
    yPos -= pFontInfoStruct->descent;

    switch (hAlign)
    {
    case kCenterText:
        width = XTextWidth (pFontInfoStruct, string, strlen (string));
        xPos = (rect.right + rect.left - width) / 2;
        break;

    case kRightText:
        width = XTextWidth (pFontInfoStruct, string, strlen (string));
        xPos = rect.right - width;
        break;

    default: // left adjust
        xPos = rect.left + 1;
    }

    if (opaque)
        XDrawImageString (XDRAWPARAM, xPos, yPos, string, strlen (string));
    else
        XDrawString (XDRAWPARAM, xPos, yPos, string, strlen (string));
}

//-----------------------------------------------------------------------------
long CDrawContext::getMouseButtons ()
{
    long buttons = 0;

    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int mask;

    XQueryPointer (XWINPARAM,
                   &root,
                   &child,
                   &rootX,
                   &rootY,
                   &childX,
                   &childY,
                   &mask);

    if (mask & Button1Mask)
        buttons |= kLButton;
    if (mask & Button2Mask)
        buttons |= kMButton;
    if (mask & Button3Mask)
        buttons |= kRButton;

    if (mask & ShiftMask)
        buttons |= kShift;
    if (mask & ControlMask)
        buttons |= kControl;
    if (mask & Mod1Mask)
        buttons |= kAlt;

    return buttons;
}

//-----------------------------------------------------------------------------
void CDrawContext::getMouseLocation (CPoint &point)
{
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int mask;

    XQueryPointer (XWINPARAM,
                   &root,
                   &child,
                   &rootX,
                   &rootY,
                   &childX,
                   &childY,
                   &mask);

    point (childX, childY);

    point.offset (-offsetScreen.h, -offsetScreen.v);
}

//-----------------------------------------------------------------------------
bool CDrawContext::waitDoubleClick ()
{
    bool doubleClick = false;

    long currentTime = _getTicks ();
    long clickTime = currentTime + 200; // XtGetMultiClickTime (display);

    XEvent e;
    while (currentTime < clickTime)
    {
        if (XCheckTypedEvent (display, ButtonPress, &e))
        {
            doubleClick = true;
            break;
        }

        currentTime = _getTicks ();
    }

    return doubleClick;
}

//-----------------------------------------------------------------------------
bool CDrawContext::waitDrag ()
{
    if (!pFrame)
        return false;

    CPoint mouseLoc;
    getMouseLocation (mouseLoc);
    CRect observe (mouseLoc.h - 2, mouseLoc.v - 2, mouseLoc.h + 2, mouseLoc.v + 2);

    long currentTime = pFrame->getTicks ();
    bool wasOutside = false;

    while (((getMouseButtons () & ~(kMButton|kRButton)) & kLButton) != 0)
    {
        pFrame->doIdleStuff ();
        if (!wasOutside)
        {
            getMouseLocation (mouseLoc);
            if (!observe.pointInside (mouseLoc))
            {
                if (kDragDelay <= 0)
                    return true;
                wasOutside = true;
            }
        }

        if (wasOutside && (pFrame->getTicks () - currentTime > kDragDelay))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void CDrawContext::forget ()
{
    CReferenceCounter::forget ();
}

//-----------------------------------------------------------------------------
long CDrawContext::getIndexColor (CColor color)
{
    XColor xcol;
    xcol.red = color.red << 8;
    xcol.green = color.green << 8;
    xcol.blue = color.blue << 8;
    xcol.flags = (DoRed | DoGreen | DoBlue);
    XAllocColor (display, XDefaultColormap (display, 0), &xcol);
    return xcol.pixel;
}

//-----------------------------------------------------------------------------
Colormap CDrawContext::getColormap ()
{
    if (pFrame)
        return pFrame->getColormap ();
    else
        return None;
}

//-----------------------------------------------------------------------------
Visual* CDrawContext::getVisual ()
{
    if (pFrame)
        return pFrame->getVisual ();
    else
        return None;
}

//-----------------------------------------------------------------------------
unsigned int CDrawContext::getDepth ()
{
    if (pFrame)
        return pFrame->getDepth ();
    else
        return None;
}


//-----------------------------------------------------------------------------
// COffscreenContext Implementation
//-----------------------------------------------------------------------------
COffscreenContext::COffscreenContext (CDrawContext *pContext, CBitmap *pBitmapBg, bool drawInBitmap)
: CDrawContext (pContext->pFrame, NULL, NULL)
, pBitmap (0)
, pBitmapBg (pBitmapBg)
, height (20)
, width (20)
{
    std::cout << "COffscreenContext::COffscreenContext with bitmap" << std::endl;

    if (pBitmapBg)
    {
        height = pBitmapBg->getHeight ();
        width  = pBitmapBg->getWidth ();

        clipRect (0, 0, width, height);
    }

#if DEBUG
    gNbCOffscreenContext++;
    gBitmapAllocation += (long)height * (long)width;
#endif

    bDestroyPixmap = false;

//    Window root = RootWindow (display, DefaultScreen (display));

     // if no bitmap handle => create one
    if (! pWindow)
    {
        pWindow = (void*) XCreatePixmap (display, pFrame->getWindow(), width, height, pFrame->getDepth ());
        bDestroyPixmap = true;
    }

    // set the current font
    if (pSystemContext)
        setFont (kNormalFont);

    if (!drawInBitmap)
    {
        // draw bitmap to Offscreen
        CRect r (0, 0, width, height);
        if (pBitmapBg)
            pBitmapBg->draw (this, r);
        else
        {
            setFillColor (kBlackCColor);
            fillRect (r);
        }
    }
}

//-----------------------------------------------------------------------------
COffscreenContext::COffscreenContext (CFrame *pFrame, long width, long height, const CColor backgroundColor)
: CDrawContext (pFrame, NULL, NULL)
, pBitmap (0)
, pBitmapBg (0)
, height (height)
, width (width)
, backgroundColor (backgroundColor)
{
    std::cout << "COffscreenContext::COffscreenContext without bitmap" << std::endl;

    clipRect (0, 0, width, height);

#if DEBUG
    gNbCOffscreenContext++;
    gBitmapAllocation += height * width;
#endif

    bDestroyPixmap = true;

    pWindow = (void*) XCreatePixmap (display,
                                     pFrame->getWindow(),
                                     width,
                                     height,
                                     pFrame->getDepth ());

/*
    // clear the pixmap
    XGCValues values;
    values.foreground = getIndexColor (backgroundColor);
    GC gc = XCreateGC (display, (Window)pWindow, GCForeground, &values);
    XFillRectangle (display, (Window)pWindow, gc, 0, 0, width, height);
    XFreeGC (display, gc);
*/

    XGCValues values;
    values.foreground = getIndexColor (backgroundColor);
    pSystemContext = XCreateGC (display, (Window)pWindow, GCForeground, &values);
    XFillRectangle (display, (Window)pWindow, (GC) pSystemContext, 0, 0, width, height);

    // set the current font
    if (pSystemContext)
        setFont (kNormalFont);
}

//-----------------------------------------------------------------------------
COffscreenContext::~COffscreenContext ()
{
#if DEBUG
    gNbCOffscreenContext--;
    gBitmapAllocation -= (long)height * (long)width;
#endif

    if (pBitmap)
        pBitmap->forget ();

    if (bDestroyPixmap && pWindow)
        XFreePixmap (display, (Pixmap)pWindow);

    if (pSystemContext)
        XFreeGC (display, (GC) pSystemContext);
}

//-----------------------------------------------------------------------------
void COffscreenContext::copyTo (CDrawContext* pContext, CRect& srcRect, CPoint destOffset)
{
    std::cout << "COffscreenContext::copyTo" << std::endl;

    XCopyArea (display,
               (Drawable) pContext->pWindow,
               (Drawable) pWindow,
               (GC) pSystemContext,
               srcRect.left,
               srcRect.top,
               srcRect.width (),
               srcRect.height (),
               destOffset.h,
               destOffset.v);
}

//-----------------------------------------------------------------------------
void COffscreenContext::copyFrom (CDrawContext *pContext, CRect destRect, CPoint srcOffset)
{
    //std::cout << "COffscreenContext::copyFrom ";

    XCopyArea (display,
               (Drawable) pContext->pWindow,
               (Drawable) pWindow,
               (GC) pSystemContext,
               srcOffset.h,
               srcOffset.v,
               destRect.width(),
               destRect.height(),
               destRect.left,
               destRect.top);

    pContext->setFrameColor (kRedCColor);
    pContext->drawRect (destRect);
}


//-----------------------------------------------------------------------------
class CAttributeListEntry
{
public:
    CAttributeListEntry (long size, CViewAttributeID id)
    : nextEntry (0)
    , pointer (0)
    , sizeOfPointer (size)
    , id (id)
    {
        pointer = malloc (size);
    }

    ~CAttributeListEntry ()
    {
        if (pointer)
            free (pointer);
    }

    CViewAttributeID getID () const { return id; }
    long getSize () const { return sizeOfPointer; }
    void* getPointer () const { return pointer; }
    CAttributeListEntry* getNext () const { return nextEntry; }

    void setNext (CAttributeListEntry* entry) { nextEntry = entry; }

protected:
    CAttributeListEntry () : nextEntry (0), pointer (0), sizeOfPointer (0), id (0) {}

    CAttributeListEntry* nextEntry;
    void* pointer;
    long sizeOfPointer;
    CViewAttributeID id;
};

//-----------------------------------------------------------------------------
const char* kMsgCheckIfViewContainer    = "kMsgCheckIfViewContainer";

//-----------------------------------------------------------------------------
// CView
//-----------------------------------------------------------------------------
/*! @class CView
base class of all view objects
*/
//-----------------------------------------------------------------------------
CView::CView (const CRect& size)
: size (size)
, mouseableArea (size)
, pParentFrame (0)
, pParentView (0)
, bDirty (false)
, bMouseEnabled (true)
, bTransparencyEnabled (false)
, bWantsFocus (false)
, bVisible (true)
, pBackground (0)
, pAttributeList (0)
{
#if DEBUG
    gNbCView++;
#endif
}

//-----------------------------------------------------------------------------
CView::~CView ()
{
    if (pBackground)
        pBackground->forget ();

    if (pAttributeList)
    {
        CAttributeListEntry* entry = pAttributeList;
        while (entry)
        {
            CAttributeListEntry* nextEntry = entry->getNext ();
            delete entry;
            entry = nextEntry;
        }
    }
#if DEBUG
    gNbCView--;
#endif
}

//-----------------------------------------------------------------------------
void CView::getMouseLocation (CDrawContext* context, CPoint &point)
{
    if (context)
    {
        if (pParentView && pParentView->notify (this, kMsgCheckIfViewContainer) == kMessageNotified)
        {
            CCoord save[4];
            ((CViewContainer*)pParentView)->modifyDrawContext (save, context);
            pParentView->getMouseLocation (context, point);
            ((CViewContainer*)pParentView)->restoreDrawContext (context, save);
        }
        else
            context->getMouseLocation (point);
    }
}

//-----------------------------------------------------------------------------
CPoint& CView::frameToLocal (CPoint& point) const
{
    if (pParentView && pParentView->isTypeOf ("CViewContainer"))
        return pParentView->frameToLocal (point);
    return point;
}

//-----------------------------------------------------------------------------
CPoint& CView::localToFrame (CPoint& point) const
{
    if (pParentView && pParentView->isTypeOf ("CViewContainer"))
        return pParentView->localToFrame (point);
    return point;
}

//-----------------------------------------------------------------------------
void CView::redraw ()
{
    if (pParentFrame)
        pParentFrame->draw (this);
}

//-----------------------------------------------------------------------------
void CView::redrawRect (CDrawContext* context, const CRect& rect)
{
    if (pParentView)
        pParentView->redrawRect (context, rect);
    else if (pParentFrame)
        pParentFrame->drawRect (context, rect);
}

//-----------------------------------------------------------------------------
void CView::draw (CDrawContext *pContext)
{
    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size);
        else
            pBackground->draw (pContext, size);
    }
    setDirty (false);
}

//-----------------------------------------------------------------------------
void CView::mouse (CDrawContext *pContext, CPoint &where, long buttons)
{}

//-----------------------------------------------------------------------------
bool CView::onWheel (CDrawContext *pContext, const CPoint &where, float distance)
{
    return false;
}

//------------------------------------------------------------------------
bool CView::onWheel (CDrawContext *pContext, const CPoint &where, const CMouseWheelAxis axis, float distance)
{
    return onWheel (pContext, where, distance);
}

//------------------------------------------------------------------------
void CView::update (CDrawContext *pContext)
{
    if (isDirty ())
    {
#if NEW_UPDATE_MECHANISM
        if (pContext)
            redrawRect (pContext, size);
        else
            redraw ();
#else
    #if USE_ALPHA_BLEND
        if (pContext)
        {
            if (bTransparencyEnabled)
                getFrame ()->drawRect (pContext, size);
            else
                draw (pContext);
        }
    #else
        if (pContext)
            draw (pContext);
    #endif
        else
            redraw ();
#endif
        setDirty (false);
    }
}

//------------------------------------------------------------------------------
long CView::onKeyDown (VstKeyCode& keyCode)
{
    return -1;
}

//------------------------------------------------------------------------------
long CView::onKeyUp (VstKeyCode& keyCode)
{
    return -1;
}

//------------------------------------------------------------------------------
long CView::notify (CView* sender, const char* message)
{
    return kMessageUnknown;
}

//------------------------------------------------------------------------------
void CView::looseFocus (CDrawContext *pContext)
{}

//------------------------------------------------------------------------------
void CView::takeFocus (CDrawContext *pContext)
{}

//------------------------------------------------------------------------------
void CView::setViewSize (CRect &rect)
{
    size = rect;
    setDirty ();
}

//-----------------------------------------------------------------------------
void *CView::getEditor () const
{
    return pParentFrame ? pParentFrame->getEditor () : 0;
}

//-----------------------------------------------------------------------------
void CView::setBackground (CBitmap *background)
{
    if (pBackground)
        pBackground->forget ();
    pBackground = background;
    if (pBackground)
        pBackground->remember ();
    setDirty (true);
}

//-----------------------------------------------------------------------------
const CViewAttributeID kCViewAttributeReferencePointer = (CViewAttributeID) "cvrp";

//-----------------------------------------------------------------------------
/**
 * @param id the ID of the Attribute
 * @param outSize on return the size of the attribute
 */
bool CView::getAttributeSize (const CViewAttributeID id, long& outSize) const
{
    if (pAttributeList)
    {
        CAttributeListEntry* entry = pAttributeList;
        while (entry)
        {
            if (entry->getID () == id)
                break;
            entry = entry->getNext ();
        }
        if (entry)
        {
            outSize = entry->getSize ();
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
/**
 * @param id the ID of the Attribute
 * @param inSize the size of the outData pointer
 * @param outData a pointer where to copy the attribute data
 * @param outSize the size in bytes which was copied into outData
 */
bool CView::getAttribute (const CViewAttributeID id, const long inSize, void* outData, long& outSize) const
{
    if (pAttributeList)
    {
        CAttributeListEntry* entry = pAttributeList;
        while (entry)
        {
            if (entry->getID () == id)
                break;
            entry = entry->getNext ();
        }
        if (entry && inSize >= entry->getSize ())
        {
            outSize = entry->getSize ();
            memcpy (outData, entry->getPointer (), outSize);
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
/**
 * copies data into the attribute. If it does not exist, creates a new attribute.
 * @param id the ID of the Attribute
 * @param inSize the size of the outData pointer
 * @param inData a pointer to the data
 */
bool CView::setAttribute (const CViewAttributeID id, const long inSize, void* inData)
{
    CAttributeListEntry* lastEntry = 0;
    if (pAttributeList)
    {
        CAttributeListEntry* entry = pAttributeList;
        while (entry)
        {
            if (entry->getID () == id)
                break;
            if (entry->getNext () == 0)
                lastEntry = entry;
            entry = entry->getNext ();
        }
        if (entry)
        {
            if (entry->getSize () >= inSize)
            {
                memcpy (entry->getPointer (), inData, inSize);
                return true;
            }
            else
                return false;
        }
    }

    // create a new attribute
    CAttributeListEntry* newEntry = new CAttributeListEntry (inSize, id);
    memcpy (newEntry->getPointer (), inData, inSize);
    if (lastEntry)
        lastEntry->setNext (newEntry);
    else if (!pAttributeList)
        pAttributeList = newEntry;
    else
    {
        delete newEntry;
        return false;
    }
    return true;
}

#if DEBUG
//-----------------------------------------------------------------------------
void CView::dumpInfo ()
{
    CRect viewRect = getViewSize (viewRect);
    DebugPrint ("left:%4d, top:%4d, width:%4d, height:%4d ", viewRect.left, viewRect.top, viewRect.getWidth (), viewRect.getHeight ());
    if (getMouseEnabled ())
        DebugPrint ("(Mouse Enabled) ");
    if (getTransparency ())
        DebugPrint ("(Transparent) ");
    CRect mouseRect = getMouseableArea (mouseRect);
    if (mouseRect != viewRect)
        DebugPrint (" (Mouseable Area: left:%4d, top:%4d, width:%4d, height:%4d ", mouseRect.left, mouseRect.top, mouseRect.getWidth (), mouseRect.getHeight ());
}
#endif

#define FOREACHSUBVIEW for (CCView *pSv = pFirstView; pSv; pSv = pSv->pNext) {CView *pV = pSv->pView;
#define FOREACHSUBVIEW_REVERSE(reverse) for (CCView *pSv = reverse ? pLastView : pFirstView; pSv; pSv = reverse ? pSv->pPrevious : pSv->pNext) {CView *pV = pSv->pView;
#define ENDFOR }

//-----------------------------------------------------------------------------
// CFrame Implementation
//-----------------------------------------------------------------------------
/*! @class CFrame
It creates a platform dependend view object.
On classic Mac OS it just draws into the provided window.
On Mac OS X it is a ControlRef.
On Windows it's a WS_CHILD Window.
*/
CFrame::CFrame (const CRect &inSize, void *inSystemWindow, void *inEditor)
: CViewContainer (inSize, 0, 0)
, pEditor (inEditor)
, pModalView (0)
, pFocusView (0)
, bFirstDraw (true)
, bDropActive (false)
, bUpdatesDisabled (false)
, pSystemWindow ((Window)inSystemWindow)
, pFrameContext (0)
, bAddedWindow (false)
, defaultCursor (0)
{
    setOpenFlag (true);

    pParentFrame = this;

    depth    = 0;
    pVisual  = 0;
    window   = 0;
	gc       = 0;
	
	if (display == NULL)
		display = XOpenDisplay(NULL);

	initFrame ((void*)pSystemWindow);
	backBuffer = XdbeAllocateBackBufferName(display, (Window)window, XdbeUndefined);

	XGCValues values;
	values.foreground = 1;
	gc = XCreateGC (display, (Window)backBuffer, GCForeground, &values);

    pFrameContext = new CDrawContext (this, gc, (void*)backBuffer);
}

//-----------------------------------------------------------------------------
CFrame::~CFrame ()
{
    if (pModalView)
        removeView (pModalView, false);

    setCursor (kCursorDefault);

    setDropActive (false);

    if (pFrameContext)
        pFrameContext->forget ();

    if (getOpenFlag ())
        close ();

    if (window)
        XDestroyWindow (display, (Window) window);
    window = 0;

    // remove callbacks to avoid undesirable update
    if (gc)
        freeGc ();
}

//-----------------------------------------------------------------------------
bool CFrame::open ()
{
    if (! window)
        return false;

    XMapRaised (display, window);

    return getOpenFlag ();
}

//-----------------------------------------------------------------------------
bool CFrame::close ()
{
    if (!window || !getOpenFlag () || !pSystemWindow)
        return false;

    XUnmapWindow (display, window);

    return true;
}

//-----------------------------------------------------------------------------
bool xerror;
int errorHandler (Display *dp, XErrorEvent *e)
{
    xerror = true;
	return 0;
}
int getProperty (Window handle, Atom atom)
{
    int result = 0, userSize;
    unsigned long bytes, userCount;
    unsigned char *data;
    Atom userType;
    xerror = false;
    XErrorHandler olderrorhandler = XSetErrorHandler (errorHandler);
    XGetWindowProperty (display, handle, atom, 0, 1, false, AnyPropertyType,
                        &userType,  &userSize, &userCount, &bytes, &data);
    if (xerror == false && userCount == 1)
        result = *(int*)data;
    XSetErrorHandler (olderrorhandler);
    return result;
}

void eventProc (XEvent* ev)
{
    CFrame* frame = (CFrame*) getProperty (ev->xany.window, XInternAtom (display, "_this", false));
    if (frame == 0)
        return;

    switch (ev->type)
    {
        case ButtonPress:
        {
            CPoint where (ev->xbutton.x, ev->xbutton.y);
            frame->mouse (frame->createDrawContext(), where);
            break;
        }

        case MotionNotify:
        {
            CPoint where (ev->xbutton.x, ev->xbutton.y);
            frame->mouse (frame->createDrawContext(), where);
            break;
        }

        case ButtonRelease:
        {
            break;
        }

        case Expose:
        {
            CRect rc (ev->xexpose.x, ev->xexpose.y,
                      ev->xexpose.x + ev->xexpose.width, ev->xexpose.y + ev->xexpose.height);

            while (XCheckTypedWindowEvent (display, ev->xexpose.window, Expose, ev))
            {
                rc.left = std::min ((int) rc.left, ev->xexpose.x);
                rc.top = std::min ((int) rc.top, ev->xexpose.y);
                rc.right = std::max ((int) rc.right, ev->xexpose.x + ev->xexpose.width);
                rc.bottom = std::max ((int) rc.bottom, ev->xexpose.y + ev->xexpose.height);
            }

            frame->drawRect (0, rc);
            break;
        }

        case EnterNotify:
            break;

        case LeaveNotify:
        {
            XCrossingEvent *xevent = (XCrossingEvent*) ev;
            if (frame->getFocusView ())
            {
                if (xevent->x < 0 || xevent->x >= frame->getWidth ()
                    || xevent->y < 0 || xevent->y >= frame->getHeight ())
                {
                    // if button pressed => don't defocus
                    if (xevent->state & (Button1Mask | Button2Mask | Button3Mask))
                        break;

                    frame->getFocusView ()->looseFocus ();
                    frame->setFocusView (0);
                }
            }
            break;
        }

        case ConfigureNotify:
        {
            frame->draw (); // @XXX - keep out ?
            break;
        }
    }
}

//-----------------------------------------------------------------------------
bool CFrame::initFrame (void *systemWin)
{
    if (!systemWin)
        return false;

    // window attributes
    XSetWindowAttributes swa;
    swa.override_redirect = false;
    swa.background_pixmap = None;
    swa.colormap = 0;
    swa.event_mask =
        StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask | FocusChangeMask | PointerMotionMask |
        EnterWindowMask | LeaveWindowMask | PropertyChangeMask;

	// create child window of host supplied window
	window = XCreateWindow (display, (Window)systemWin,
                            0, 0, size.width(), size.height(),
                            0, CopyFromParent, InputOutput, (Visual*) CopyFromParent,
                            CWEventMask | CWOverrideRedirect | CWColormap | CWBackPixmap | CWEventMask, &swa);

    XGrabButton (display, AnyButton, AnyModifier, window, False,
                    ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask,
                    GrabModeAsync, GrabModeAsync, None, None);

#if 0
    // remove window caption/frame
    #define MWM_HINTS_DECORATIONS (1L << 1)
    #define PROP_MOTIF_WM_HINTS_ELEMENTS 5
    typedef struct
    {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long          inputMode;
        unsigned long status;
    }
    PropMotifWmHints;

    PropMotifWmHints motif_hints;
    motif_hints.flags = MWM_HINTS_DECORATIONS;
    motif_hints.decorations = 0;
    Atom prop = XInternAtom (display, "_MOTIF_WM_HINTS", True );
    XChangeProperty (display, window, prop, prop, 32, PropModeReplace,
                     (unsigned char *) &motif_hints, PROP_MOTIF_WM_HINTS_ELEMENTS);
#endif

    Atom atom;
    void* data = this;
    atom = XInternAtom (display, "_this", false);
    XChangeProperty (display, (Window)window, atom, atom, 32,
                     PropModeReplace, (unsigned char*)&data, 1);

    data = (void*)&eventProc;
    atom = XInternAtom (display, "_XEventProc", false);
    XChangeProperty (display, (Window)window, atom, atom, 32,
                     PropModeReplace, (unsigned char*)&data, 1);
	
    XGCValues values;
    values.foreground = 1;
    gc = XCreateGC (display, (Window)window, GCForeground, &values);
	
    // get the std colormap
    XWindowAttributes attr;
    XGetWindowAttributes (display, (Window) window, &attr);
    colormap = attr.colormap;
    pVisual  = attr.visual;
    depth    = attr.depth;
	
    // init and load the fonts
    if (! gFontInit) {
        for (long i = 0; i < kNumStandardFonts; i++) {
            DBG (gFontTable[i].string);
            gFontStructs[i] = XLoadQueryFont (display, gFontTable[i].string);
            assert (gFontStructs[i] != 0);
        }
        gFontInit = true;
    }

    setDropActive (true);
    bAddedWindow = true;

    //XReparentWindow (display, window, (Window) systemWin, 0, 0);
    //XMapRaised (display, window);

    return true;
}

//-----------------------------------------------------------------------------
bool CFrame::setDropActive (bool val)
{
    if (!bDropActive && !val)
        return true;

    bDropActive = val;
    return true;
}

//-----------------------------------------------------------------------------
void CFrame::freeGc ()
{
	XFreeGC (display, gc);
}

//-----------------------------------------------------------------------------
CDrawContext* CFrame::createDrawContext ()
{
    if (pFrameContext)
    {
        pFrameContext->remember ();
        return pFrameContext;
    }

    pFrameContext = new CDrawContext (this, gc, (void*)window);

    return pFrameContext;
}

//-----------------------------------------------------------------------------
void CFrame::draw (CDrawContext *pContext)
{
    if (bFirstDraw)
        bFirstDraw = false;

    bool localContext = false;
    if (! pContext)
    {
        localContext = true;
        pContext = createDrawContext ();
    }

    // draw the background and the children
    CViewContainer::draw (pContext);

    if (localContext)
        pContext->forget ();
}

//-----------------------------------------------------------------------------
void CFrame::drawRect (CDrawContext *pContext, const CRect& updateRect)
{
    if (bFirstDraw)
        bFirstDraw = false;

    bool localContext = false;
    if (! pContext)
    {
        localContext = true;
        pContext = createDrawContext ();
    }

#if USE_CLIPPING_DRAWRECT
    CRect oldClip;
    pContext->getClipRect (oldClip);
    CRect newClip (updateRect);
    newClip.bound (oldClip);
    pContext->setClipRect (newClip);
#endif

    // draw the background and the children
    if (updateRect.getWidth () > 0 && updateRect.getHeight () > 0)
        CViewContainer::drawRect (pContext, updateRect);

#if USE_CLIPPING_DRAWRECT
    pContext->setClipRect (oldClip);
#endif

    if (localContext)
        pContext->forget ();
}

//-----------------------------------------------------------------------------
void CFrame::draw (CView *pView)
{
    CView *pViewToDraw = 0;

        // Search it in the view list
    if (pView && isChild(pView))
        pViewToDraw = pView;

    CDrawContext *pContext = createDrawContext ();
    if (pContext)
    {
        if (pViewToDraw && pViewToDraw->isVisible())
            pViewToDraw->draw (pContext);
        else
            draw (pContext);

        pContext->forget ();
    }
}

//-----------------------------------------------------------------------------
void CFrame::mouse (CDrawContext *pContext, CPoint &where, long buttons)
{
/*
    XGrabPointer (display,
                  window,
                  False,
                  ButtonPressMask | ButtonReleaseMask |
                  EnterWindowMask | LeaveWindowMask |
                  PointerMotionMask | Button1MotionMask |
                  Button2MotionMask | Button3MotionMask |
                  Button4MotionMask | Button5MotionMask,
                  GrabModeAsync,
                  GrabModeAsync,
                  None,
                  None,
                  CurrentTime);
*/

    if (!pContext)
        pContext = pFrameContext;

    if (pFocusView)
        setFocusView (NULL);

    if (buttons == -1 && pContext)
        buttons = pContext->getMouseButtons ();

    if (pModalView && pModalView->isVisible())
    {
        if (pModalView->hitTest (where, buttons))
            pModalView->mouse (pContext, where, buttons);
    }
    else
    {
        CViewContainer::mouse (pContext, where, buttons);
    }

//    XUngrabPointer (display, CurrentTime);
}

//-----------------------------------------------------------------------------
long CFrame::onKeyDown (VstKeyCode& keyCode)
{
    long result = -1;

    if (pFocusView && pFocusView->isVisible())
        result = pFocusView->onKeyDown (keyCode);

    if (result == -1 && pModalView && pModalView->isVisible())
        result = pModalView->onKeyDown (keyCode);

    if (result == -1 && keyCode.virt == VKEY_TAB)
        result = advanceNextFocusView (pFocusView, (keyCode.modifier & MODIFIER_SHIFT) ? true : false) ? 1 : -1;

    return result;
}

//-----------------------------------------------------------------------------
long CFrame::onKeyUp (VstKeyCode& keyCode)
{
    long result = -1;

    if (pFocusView && pFocusView->isVisible())
        result = pFocusView->onKeyUp (keyCode);

    if (result == -1 && pModalView && pModalView->isVisible())
        result = pModalView->onKeyUp (keyCode);

    return result;
}

//------------------------------------------------------------------------
bool CFrame::onWheel (CDrawContext *pContext, const CPoint &where, const CMouseWheelAxis axis, float distance)
{
    bool result = false;

    CView *view = pModalView ? pModalView : getViewAt (where);
    if (view && view->isVisible())
    {
        bool localContext = false;
        if (!pContext)
        {
            localContext = true;
            pContext = createDrawContext ();
        }

        result = view->onWheel (pContext, where, axis, distance);

        if (localContext)
            pContext->forget ();
    }
    return result;
}

//-----------------------------------------------------------------------------
bool CFrame::onWheel (CDrawContext *pContext, const CPoint &where, float distance)
{
    return onWheel (pContext, where, kMouseWheelAxisY, distance);
}

//-----------------------------------------------------------------------------
void CFrame::update (CDrawContext *pContext)
{
    if (!getOpenFlag () || updatesDisabled ())
        return;

    CDrawContext* dc = pContext;

    if (bDirty)
    {
        draw (dc);
        setDirty (false);
    }
    else
    {
#if USE_CLIPPING_DRAWRECT
        CRect oldClipRect;
        dc->getClipRect (oldClipRect);
#endif
#if NEW_UPDATE_MECHANISM
        if (pModalView && pModalView->isVisible () && pModalView->isDirty ())
            pModalView->update (dc);
#endif
        FOREACHSUBVIEW
#if USE_CLIPPING_DRAWRECT
            CRect viewSize (pV->size);
            viewSize.bound (oldClipRect);
            dc->setClipRect (viewSize);
#endif
            pV->update (dc);
        ENDFOR
#if USE_CLIPPING_DRAWRECT
        dc->setClipRect (oldClipRect);
#endif
    }
}

//-----------------------------------------------------------------------------
void CFrame::idle ()
{
    if (!getOpenFlag ())
        return;

    // don't do an idle before a draw
    if (bFirstDraw)
        return;

    if (!isDirty ())
        return;
	
	XdbeBeginIdiom(display);
	XdbeSwapInfo swap_info;
	swap_info.swap_window = window;
	swap_info.swap_action = XdbeUndefined;
	XdbeSwapBuffers(display, &swap_info, 1);

    CDrawContext *pContext = createDrawContext ();

    update (pContext);

    pContext->forget ();
	
	XdbeEndIdiom(display);
}

//-----------------------------------------------------------------------------
void CFrame::doIdleStuff ()
{
    if (pEditor)
        ((AEffGUIEditor*)pEditor)->doIdleStuff ();
}

//-----------------------------------------------------------------------------
unsigned long CFrame::getTicks () const
{
    if (pEditor)
        return ((AEffGUIEditor*)pEditor)->getTicks ();
    return 0;
}

//-----------------------------------------------------------------------------
long CFrame::getKnobMode () const
{
    return AEffGUIEditor::getKnobMode ();
}

//-----------------------------------------------------------------------------
bool CFrame::setPosition (CCoord x, CCoord y)
{
    size.left = 0;
    size.top = 0;

    XWindowChanges attr;
    attr.x = x;
    attr.y = y;

    XConfigureWindow (display, window, CWX | CWY, &attr);

    return false;
}

//-----------------------------------------------------------------------------
bool CFrame::getPosition (CCoord &x, CCoord &y) const
{
    x = size.left;
    y = size.top;
    return true;
}

//-----------------------------------------------------------------------------
void CFrame::setViewSize (CRect& inRect)
{
    setSize (inRect.width (), inRect.height ());
}

//-----------------------------------------------------------------------------
bool CFrame::setSize (CCoord width, CCoord height)
{
    if ((width == size.width ()) && (height == size.height ()))
        return false;

    // set the new size
    size.right  = size.left + width;
    size.bottom = size.top  + height;

    XResizeWindow (display, window, size.width (), size.height ());

    CRect myViewSize (0, 0, size.width (), size.height ());
    CViewContainer::setViewSize (myViewSize);

    return true;
}

//-----------------------------------------------------------------------------
bool CFrame::getSize (CRect *pRect) const
{
//    if (!getOpenFlag ())
//        return false;

    pRect->left   = 0;
    pRect->top    = 0;
    pRect->right  = size.width () + pRect->left;
    pRect->bottom = size.height () + pRect->top;

    return true;
}

//-----------------------------------------------------------------------------
bool CFrame::getSize (CRect& outSize) const
{
    return getSize (&outSize);
}

//-----------------------------------------------------------------------------
long CFrame::setModalView (CView *pView)
{
    // There's already a modal view so we get out
    if (pView && pModalView)
        return 0;

    if (pModalView)
        removeView (pModalView, false);

    pModalView = pView;
    if (pModalView)
        addView (pModalView);

    return 1;
}

//-----------------------------------------------------------------------------
void CFrame::beginEdit (long index)
{
    if (pEditor)
        ((AEffGUIEditor*)pEditor)->beginEdit (index);
}

//-----------------------------------------------------------------------------
void CFrame::endEdit (long index)
{
    if (pEditor)
        ((AEffGUIEditor*)pEditor)->endEdit (index);
}

//-----------------------------------------------------------------------------
CView *CFrame::getCurrentView () const
{
    if (pModalView)
        return pModalView;

    return CViewContainer::getCurrentView ();
}

//-----------------------------------------------------------------------------
bool CFrame::getCurrentLocation (CPoint &where)
{
    // create a local context
    CDrawContext *pContext = createDrawContext ();
    if (pContext)
    {
    // get the current position
        pContext->getMouseLocation (where);
        pContext->forget ();
    }
    return true;
}

//-----------------------------------------------------------------------------
void CFrame::setCursor (CCursorType type)
{
}

//-----------------------------------------------------------------------------
void CFrame::setFocusView (CView *pView)
{
    if (pView && ! pView->isVisible())
        return;

    CView *pOldFocusView = pFocusView;
    pFocusView = pView;

    if (pFocusView && pFocusView->wantsFocus ())
        pFocusView->setDirty ();

    if (pOldFocusView)
    {
        pOldFocusView->looseFocus ();
        if (pOldFocusView->wantsFocus ())
            pOldFocusView->setDirty ();
    }
}

//-----------------------------------------------------------------------------
bool CFrame::advanceNextFocusView (CView* oldFocus, bool reverse)
{
    if (pModalView)
        return false; // currently not supported, but should be done sometime
    if (oldFocus == 0)
    {
        if (pFocusView == 0)
            return CViewContainer::advanceNextFocusView (0, reverse);
        oldFocus = pFocusView;
    }
    if (isChild (oldFocus))
    {
        if (CViewContainer::advanceNextFocusView (oldFocus, reverse))
            return true;
        else
        {
            setFocusView (NULL);
            return false;
        }
    }
    CView* parentView = oldFocus->getParentView ();
    if (parentView && parentView->isTypeOf ("CViewContainer"))
    {
        CView* tempOldFocus = oldFocus;
        CViewContainer* vc = (CViewContainer*)parentView;
        while (vc)
        {
            if (vc->advanceNextFocusView (tempOldFocus, reverse))
                return true;
            else
            {
                tempOldFocus = vc;
                if (vc->getParentView () && vc->getParentView ()->isTypeOf ("CViewContainer"))
                    vc = (CViewContainer*)vc->getParentView ();
                else
                    vc = 0;
            }
        }
    }
    return CViewContainer::advanceNextFocusView (oldFocus, reverse);
}

//-----------------------------------------------------------------------------
void CFrame::invalidate (const CRect &rect)
{
    CRect rectView;
    FOREACHSUBVIEW
    if (pV)
    {
        pV->getViewSize (rectView);
        if (rect.rectOverlap (rectView))
            pV->setDirty (true);
    }
    ENDFOR

    XClearArea (display,
                window,
                rect.left,
                rect.top,
                rect.right - rect.left,
                rect.bottom - rect.top,
                true);

    XSync (display, false);

/*
    XEvent ev;
    memset (&ev, 0, sizeof (XEvent));
    ev.xexpose.type = Expose;
    ev.xexpose.display = display;
    ev.xexpose.window = (Window) window;

    ev.xexpose.x = rect.left;
    ev.xexpose.y = rect.top;
    ev.xexpose.width = rect.right - rect.left;
    ev.xexpose.height = rect.bottom - rect.top;

    XSendEvent (display, (Window) window, False, 0L, &ev);
    XFlush (display);
*/
}

#if DEBUG
//-----------------------------------------------------------------------------
void CFrame::dumpHierarchy ()
{
    dumpInfo ();
    DebugPrint ("\n");
    CViewContainer::dumpHierarchy ();
}
#endif

//-----------------------------------------------------------------------------
// CCView Implementation
//-----------------------------------------------------------------------------
CCView::CCView (CView *pView)
 :  pView (pView), pNext (0), pPrevious (0)
{
    if (pView)
        pView->remember ();
}

//-----------------------------------------------------------------------------
CCView::~CCView ()
{
    if (pView)
        pView->forget ();
}

//-----------------------------------------------------------------------------
// CViewContainer Implementation
//-----------------------------------------------------------------------------
/**
 * CViewContainer constructor.
 * @param rect the size of the container
 * @param pParent the parent CFrame
 * @param pBackground the background bitmap, can be NULL
 */
CViewContainer::CViewContainer (const CRect &rect, CFrame *pParent, CBitmap *pBackground)
  : CView (rect),
    pFirstView (0),
    pLastView (0),
    mode (kNormalUpdate),
    pOffscreenContext (0),
    bDrawInOffscreen (false),
    currentDragView (0)
{
    pParentFrame = pParent;

    backgroundOffset (0, 0);

    setBackground (pBackground);
    backgroundColor = kBlackCColor;

#if NEW_UPDATE_MECHANISM
    mode = kOnlyDirtyUpdate;
#endif
}

//-----------------------------------------------------------------------------
CViewContainer::~CViewContainer ()
{
    // remove all views
    removeAll (true);

     if (pOffscreenContext)
        pOffscreenContext->forget ();
    pOffscreenContext = 0;
}

//-----------------------------------------------------------------------------
/**
 * @param rect the new size of the container
 */
void CViewContainer::setViewSize (CRect &rect)
{
    CView::setViewSize (rect);

    if (pOffscreenContext && bDrawInOffscreen)
    {
        pOffscreenContext->forget ();
        pOffscreenContext = new COffscreenContext (pParentFrame, (long)size.width (), (long)size.height (), kBlackCColor);
    }
}

//-----------------------------------------------------------------------------
/**
 * @param color the new background color of the container
 */
void CViewContainer::setBackgroundColor (CColor color)
{
    backgroundColor = color;
    setDirty (true);
}

//------------------------------------------------------------------------------
long CViewContainer::notify (CView* sender, const char* message)
{
    if (message == kMsgCheckIfViewContainer)
        return kMessageNotified;
    return kMessageUnknown;
}

//-----------------------------------------------------------------------------
/**
 * @param pView the view object to add to this container
 */
void CViewContainer::addView (CView *pView)
{
    if (!pView)
        return;

    CCView *pSv = new CCView (pView);

    pView->pParentFrame = pParentFrame;
    pView->pParentView = this;

    CCView *pV = pFirstView;
    if (!pV)
    {
        pLastView = pFirstView = pSv;
    }
    else
    {
        while (pV->pNext)
            pV = pV->pNext;
        pV->pNext = pSv;
        pSv->pPrevious = pV;
        pLastView = pSv;
    }
    pView->attached (this);
    pView->setDirty ();
}

//-----------------------------------------------------------------------------
/**
 * @param pView the view object to add to this container
 * @param mouseableArea the view area in where the view will get mouse events
 * @param mouseEnabled bool to set if view will get mouse events
 */
void CViewContainer::addView (CView *pView, CRect &mouseableArea, bool mouseEnabled)
{
    if (!pView)
        return;

    pView->setMouseEnabled (mouseEnabled);
    pView->setMouseableArea (mouseableArea);

    addView (pView);
}

//-----------------------------------------------------------------------------
/**
 * @param withForget bool to indicate if the view's reference counter should be decreased after removed from the container
 */
void CViewContainer::removeAll (const bool &withForget)
{
    CCView *pV = pFirstView;
    while (pV)
    {
        CCView *pNext = pV->pNext;
        if (pV->pView)
        {
            pV->pView->removed (this);
            if (withForget)
                pV->pView->forget ();
        }

        delete pV;

        pV = pNext;
    }
    pFirstView = 0;
    pLastView = 0;
}

//-----------------------------------------------------------------------------
/**
 * @param pView the view which should be removed from the container
 * @param withForget bool to indicate if the view's reference counter should be decreased after removed from the container
 */
void CViewContainer::removeView (CView *pView, const bool &withForget)
{
    if (pParentFrame && pParentFrame->getFocusView () == pView)
        pParentFrame->setFocusView (0);
    CCView *pV = pFirstView;
    while (pV)
    {
        if (pView == pV->pView)
        {
            CCView *pNext = pV->pNext;
            CCView *pPrevious = pV->pPrevious;
            if (pV->pView)
            {
                pV->pView->removed (this);
                if (withForget)
                    pV->pView->forget ();
            }
            delete pV;
            if (pPrevious)
            {
                pPrevious->pNext = pNext;
                if (pNext)
                    pNext->pPrevious = pPrevious;
                else
                    pLastView = pPrevious;
            }
            else
            {
                pFirstView = pNext;
                if (pNext)
                    pNext->pPrevious = 0;
                else
                    pLastView = 0;
            }
            break;
        }
        else
            pV = pV->pNext;
    }
}

//-----------------------------------------------------------------------------
/**
 * @param pView the view which should be checked if it is a child of this container
 */
bool CViewContainer::isChild (CView *pView) const
{
    bool found = false;

    CCView *pV = pFirstView;
    while (pV)
    {
        if (pView == pV->pView)
        {
            found = true;
            break;
        }
        pV = pV->pNext;
    }
    return found;
}

//-----------------------------------------------------------------------------
long CViewContainer::getNbViews () const
{
    long nb = 0;
    CCView *pV = pFirstView;
    while (pV)
    {
        pV = pV->pNext;
        nb++;
    }
    return nb;
}

//-----------------------------------------------------------------------------
/**
 * @param index the index of the view to return
 */
CView *CViewContainer::getView (long index) const
{
    long nb = 0;
    CCView *pV = pFirstView;
    while (pV)
    {
        if (nb == index)
            return pV->pView;
        pV = pV->pNext;
        nb++;
    }
    return 0;
}

//-----------------------------------------------------------------------------
/**
 * @param pContext the context which to use to draw this container and its subviews
 */
void CViewContainer::draw (CDrawContext *pContext)
{
    CDrawContext *pC;
    CCoord save[4];

    if (!pOffscreenContext && bDrawInOffscreen)
        pOffscreenContext = new COffscreenContext (pParentFrame, (long)size.width (), (long)size.height (), kBlackCColor);

#if USE_ALPHA_BLEND
    if (pOffscreenContext && bTransparencyEnabled)
        pOffscreenContext->copyTo (pContext, size);
#endif

    if (bDrawInOffscreen)
        pC = pOffscreenContext;
    else
    {
        pC = pContext;
        modifyDrawContext (save, pContext);
    }

    CRect r (0, 0, size.width (), size.height ());

#if USE_CLIPPING_DRAWRECT
    CRect oldClip;
    pContext->getClipRect (oldClip);
    CRect oldClip2 (oldClip);
    if (bDrawInOffscreen && getFrame () != this)
        oldClip.offset (-oldClip.left, -oldClip.top);

    CRect newClip (r);
    newClip.bound (oldClip);
    pC->setClipRect (newClip);
#endif

    // draw the background
    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pC, r, backgroundOffset);
        else
            pBackground->draw (pC, r, backgroundOffset);
    }
    else if (!bTransparencyEnabled)
    {
        pC->setFillColor (backgroundColor);
        pC->fillRect (r);
    }

    // draw each view
    FOREACHSUBVIEW
#if USE_CLIPPING_DRAWRECT
        CRect vSize (pV->size);
        vSize.bound (oldClip);
        pC->setClipRect (vSize);
#endif
        if (pV->isVisible())
            pV->draw (pC);
    ENDFOR

#if USE_CLIPPING_DRAWRECT
    pC->setClipRect (oldClip2);
#endif

    // transfert offscreen
    if (bDrawInOffscreen)
        ((COffscreenContext*)pC)->copyFrom (pContext, size);
    else
        restoreDrawContext (pContext, save);

    setDirty (false);
}

//-----------------------------------------------------------------------------
/**
 * @param pContext the context which to use to draw the background
 * @param _updateRect the area which to draw
 */
void CViewContainer::drawBackgroundRect (CDrawContext *pContext, CRect& _updateRect)
{
    if (pBackground)
    {
        CRect oldClip;
        pContext->getClipRect (oldClip);
        CRect newClip (_updateRect);
        newClip.bound (oldClip);
        pContext->setClipRect (newClip);
        CRect tr (0, 0, pBackground->getWidth (), pBackground->getHeight ());
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, tr, backgroundOffset);
        else
            pBackground->draw (pContext, tr, backgroundOffset);
        pContext->setClipRect (oldClip);
    }
    else if (!bTransparencyEnabled)
    {
        pContext->setFillColor (backgroundColor);
        pContext->fillRect (_updateRect);
    }
}

//-----------------------------------------------------------------------------
/**
 * @param pContext the context which to use to draw
 * @param _updateRect the area which to draw
 */
void CViewContainer::drawRect (CDrawContext *pContext, const CRect& _updateRect)
{
    CDrawContext *pC;
    CCoord save[4];

    if (!pOffscreenContext && bDrawInOffscreen)
        pOffscreenContext = new COffscreenContext (pParentFrame, (long)size.width (), (long)size.height (), kBlackCColor);

#if USE_ALPHA_BLEND
    if (pOffscreenContext && bTransparencyEnabled)
        pOffscreenContext->copyTo (pContext, size);
#endif

    if (bDrawInOffscreen)
        pC = pOffscreenContext;
    else
    {
        pC = pContext;
        modifyDrawContext (save, pContext);
    }

    CRect updateRect (_updateRect);
    updateRect.bound (size);

    CRect clientRect (updateRect);
    clientRect.offset (-size.left, -size.top);

#if USE_CLIPPING_DRAWRECT
    CRect oldClip;
    pContext->getClipRect (oldClip);
    CRect oldClip2 (oldClip);
    if (bDrawInOffscreen && getFrame () != this)
        oldClip.offset (-oldClip.left, -oldClip.top);

    CRect newClip (clientRect);
    newClip.bound (oldClip);
    pC->setClipRect (newClip);
#endif

    // draw the background
    drawBackgroundRect (pC, clientRect);

    // draw each view
    FOREACHSUBVIEW
        if (pV->isVisible() && pV->checkUpdate (clientRect))
        {
#if USE_CLIPPING_DRAWRECT
            CRect viewSize (pV->size);
            viewSize.bound (newClip);
            if (viewSize.getWidth () == 0 || viewSize.getHeight () == 0)
                continue;
            pC->setClipRect (viewSize);
#endif

            bool wasDirty = pV->isDirty ();
            pV->drawRect (pC, clientRect);

#if DEBUG_FOCUS_DRAWING
            if (getFrame ()->getFocusView() == pV && pV->wantsFocus ())
            {
                pC->setDrawMode (kCopyMode);
                pC->setFrameColor (kRedCColor);
                pC->drawRect (pV->size);
            }
#endif
            if (wasDirty && /* pV->size != viewSize &&*/ !isTypeOf ("CScrollContainer"))
            {
                pV->setDirty (true);
            }
        }
    ENDFOR

#if USE_CLIPPING_DRAWRECT
    pC->setClipRect (oldClip2);
#endif

    // transfer offscreen
    if (bDrawInOffscreen)
        ((COffscreenContext*)pC)->copyFrom (pContext, updateRect, CPoint (clientRect.left, clientRect.top));
    else
        restoreDrawContext (pContext, save);

#if EVENT_DRAW_FIX
    if (bDirty && newClip == size)
#endif
        setDirty (false);
}

//-----------------------------------------------------------------------------
/**
 * @param context the context which to use to redraw this container
 * @param rect the area which to redraw
 */
void CViewContainer::redrawRect (CDrawContext* context, const CRect& rect)
{
    CRect _rect (rect);
    _rect.offset (size.left, size.top);
    if (bTransparencyEnabled)
    {
        // as this is transparent, we call the parentview to redraw this area.
        if (pParentView)
            pParentView->redrawRect (context, _rect);
        else if (pParentFrame)
            pParentFrame->drawRect (context, _rect);
    }
    else
    {
        CCoord save[4];
        if (pParentView)
        {
            CPoint off;
            pParentView->localToFrame (off);
            // store
            save[0] = context->offsetScreen.h;
            save[1] = context->offsetScreen.v;
            save[2] = context->offset.h;
            save[3] = context->offset.v;

            context->offsetScreen.h += off.x;
            context->offsetScreen.v += off.y;
            context->offset.h += off.x;
            context->offset.v += off.y;

        	drawRect (context, _rect);
            
			// restore
            context->offsetScreen.h = save[0];
            context->offsetScreen.v = save[1];
            context->offset.h = save[2];
            context->offset.v = save[3];
        }
		else
		{
        	drawRect (context, _rect);
		}
    }
}

//-----------------------------------------------------------------------------
bool CViewContainer::hitTestSubViews (const CPoint& where, const long buttons)
{
    CPoint where2 (where);
    where2.offset (-size.left, -size.top);

    CCView *pSv = pLastView;
    while (pSv)
    {
        CView *pV = pSv->pView;
        if (pV && pV->getMouseEnabled () && pV->hitTest (where2, buttons))
            return true;
        pSv = pSv->pPrevious;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool CViewContainer::hitTest (const CPoint& where, const long buttons)
{
    //return hitTestSubViews (where); would change default behavior
    return CView::hitTest (where, buttons);
}

//-----------------------------------------------------------------------------
void CViewContainer::mouse (CDrawContext *pContext, CPoint &where, long buttons)
{
    // convert to relativ pos
    CPoint where2 (where);
    where2.offset (-size.left, -size.top);

    if (buttons == -1 && pContext)
        buttons = pContext->getMouseButtons ();

    CCView *pSv = pLastView;
    while (pSv)
    {
        CView *pV = pSv->pView;
        if (pV && pV->getMouseEnabled () && pV->hitTest (where2, buttons))
        {
            pV->mouse (pContext, where2, buttons);
            break;
        }
        pSv = pSv->pPrevious;
    }
}

//-----------------------------------------------------------------------------
long CViewContainer::onKeyDown (VstKeyCode& keyCode)
{
    long result = -1;

    CCView* pSv = pLastView;
    while (pSv)
    {
        long result = pSv->pView->onKeyDown (keyCode);
        if (result != -1)
            break;

        pSv = pSv->pPrevious;
    }

    return result;
}

//-----------------------------------------------------------------------------
long CViewContainer::onKeyUp (VstKeyCode& keyCode)
{
    long result = -1;

    CCView* pSv = pLastView;
    while (pSv)
    {
        long result = pSv->pView->onKeyUp (keyCode);
        if (result != -1)
            break;

        pSv = pSv->pPrevious;
    }

    return result;
}

//-----------------------------------------------------------------------------
bool CViewContainer::onWheel (CDrawContext *pContext, const CPoint &where, const CMouseWheelAxis axis, float distance)
{
    bool result = false;
    CView *view = getViewAt (where);
    if (view)
    {
        // convert to relativ pos
        CPoint where2 (where);
        where2.offset (-size.left, -size.top);

        CCoord save[4];
        modifyDrawContext (save, pContext);

        result = view->onWheel (pContext, where2, axis, distance);

        restoreDrawContext (pContext, save);
    }
    return result;
}

//-----------------------------------------------------------------------------
bool CViewContainer::onWheel (CDrawContext *pContext, const CPoint &where, float distance)
{
    return onWheel (pContext, where, kMouseWheelAxisY, distance);
}

//-----------------------------------------------------------------------------
bool CViewContainer::onDrop (CDrawContext* context, CDragContainer* drag, const CPoint& where)
{
    if (!pParentFrame)
        return false;

    bool result = false;

    CCoord save[4];
    modifyDrawContext (save, context);

    // convert to relativ pos
    CPoint where2 (where);
    where2.offset (-size.left, -size.top);

    CView* view = getViewAt (where);
    if (view != currentDragView)
    {
        if (currentDragView)
            currentDragView->onDragLeave (context, drag, where2);
        currentDragView = view;
    }
    if (currentDragView)
    {
        result = currentDragView->onDrop (context, drag, where2);
        currentDragView->onDragLeave (context, drag, where2);
    }
    currentDragView = 0;

    restoreDrawContext (context, save);

    return result;
}

//-----------------------------------------------------------------------------
void CViewContainer::onDragEnter (CDrawContext* context, CDragContainer* drag, const CPoint& where)
{
    if (!pParentFrame)
        return;

    CCoord save[4];
    modifyDrawContext (save, context);

    // convert to relativ pos
    CPoint where2 (where);
    where2.offset (-size.left, -size.top);

    if (currentDragView)
        currentDragView->onDragLeave (context, drag, where2);
    CView* view = getViewAt (where);
    currentDragView = view;
    if (view)
        view->onDragEnter (context, drag, where2);

    restoreDrawContext (context, save);
}

//-----------------------------------------------------------------------------
void CViewContainer::onDragLeave (CDrawContext* context, CDragContainer* drag, const CPoint& where)
{
    if (!pParentFrame)
        return;

    CCoord save[4];
    modifyDrawContext (save, context);

    // convert to relativ pos
    CPoint where2 (where);
    where2.offset (-size.left, -size.top);

    if (currentDragView)
        currentDragView->onDragLeave (context, drag, where2);
    currentDragView = 0;

    restoreDrawContext (context, save);
}

//-----------------------------------------------------------------------------
void CViewContainer::onDragMove (CDrawContext* context, CDragContainer* drag, const CPoint& where)
{
    if (!pParentFrame)
        return;

    CCoord save[4];
    modifyDrawContext (save, context);

    // convert to relativ pos
    CPoint where2 (where);
    where2.offset (-size.left, -size.top);

    CView* view = getViewAt (where);
    if (view != currentDragView)
    {
        if (currentDragView)
            currentDragView->onDragLeave (context, drag, where2);
        if (view)
            view->onDragEnter (context, drag, where2);
        currentDragView = view;
    }
    else if (currentDragView)
        currentDragView->onDragMove (context, drag, where2);

    restoreDrawContext (context, save);
}

//-----------------------------------------------------------------------------
void CViewContainer::update (CDrawContext *pContext)
{
    switch (mode)
    {
        //---Normal : redraw all...
        case kNormalUpdate:
            if (isDirty ())
            {
#if NEW_UPDATE_MECHANISM
                CRect ur (0, 0, size.width (), size.height ());
                redrawRect (pContext, ur);
#else
    #if USE_ALPHA_BLEND
                if (bTransparencyEnabled)
                {
                    CRect updateRect (size);
                    CPoint offset (0,0);
                    localToFrame (offset);
                    updateRect.offset (offset.x, offset.y);
                    getFrame ()->drawRect (pContext, updateRect);
                }
                else
    #endif
                draw (pContext);
    #endif
                setDirty (false);
            }
        break;

        //---Redraw only dirty controls-----
        case kOnlyDirtyUpdate:
        {
#if NEW_UPDATE_MECHANISM
            if (bDirty)
            {
                CRect ur (0, 0, size.width (), size.height ());
                redrawRect (pContext, ur);
            }
            else
            {
                CRect updateRect (size);
                updateRect.offset (-size.left, -size.top);
                FOREACHSUBVIEW
                    if (pV->isDirty () && pV->checkUpdate (updateRect))
                    {
                        if (pV->notify (this, kMsgCheckIfViewContainer))
                            pV->update (pContext);
                        else
                        {
                            CRect drawSize (pV->size);
                            drawSize.bound (updateRect);
                            pV->redrawRect (pContext, drawSize);
                        }
                    }
                ENDFOR
            }
#else
    #if USE_ALPHA_BLEND
            if (bTransparencyEnabled)
            {
                if (bDirty)
                {
                    CRect updateRect (size);
                    CPoint offset (0,0);
                    localToFrame (offset);
                    updateRect.offset (offset.x, offset.y);
                    getFrame ()->drawRect (pContext, updateRect);
                }
                else
                {
                    CRect updateRect (size);
                    updateRect.offset (-size.left, -size.top);
                    FOREACHSUBVIEW
                        if (pV->isDirty () && pV->checkUpdate (updateRect))
                        {
                            if (pV->notify (this, kMsgCheckIfViewContainer))
                            {
                                pV->update (pContext);
                            }
                            else
                            {
                                CPoint offset;
                                CRect viewSize (pV->size);
                                pV->localToFrame (offset);
                                viewSize.offset (offset.x, offset.y);
                                getFrame ()->drawRect (pContext, viewSize);
                            }
                        }
                    ENDFOR
                }
                setDirty (false);
                return;
            }
    #endif
            if (bDirty)
                draw (pContext);
            else if (bDrawInOffscreen && pOffscreenContext)
            {
                bool doCopy = false;
                if (isDirty ())
                    doCopy = true;

                FOREACHSUBVIEW
                    pV->update (pOffscreenContext);
                ENDFOR

                // transfert offscreen
                if (doCopy)
                    pOffscreenContext->copyFrom (pContext, size);
            }
            else
            {
                long save[4];
                modifyDrawContext (save, pContext);

                FOREACHSUBVIEW
                    if (pV->isDirty ())
                    {
                        long oldMode = 0;
                        CViewContainer* child = 0;
                        if (pV->notify (this, kMsgCheckIfViewContainer))
                        {
                            child = (CViewContainer*)pV;
                            oldMode = child->getMode ();
                            child->setMode (kNormalUpdate);
                        }
                        CRect viewSize (pV->size);
                        drawBackgroundRect (pContext, viewSize);
                        pV->update (pContext);
                        if (child)
                            child->setMode (oldMode);
                    }
                ENDFOR

                restoreDrawContext (pContext, save);
            }
#endif
            setDirty (false);
        break;
        }
    }
}

//-----------------------------------------------------------------------------
void CViewContainer::looseFocus (CDrawContext *pContext)
{
    FOREACHSUBVIEW
        pV->looseFocus (pContext);
    ENDFOR
}

//-----------------------------------------------------------------------------
void CViewContainer::takeFocus (CDrawContext *pContext)
{
    FOREACHSUBVIEW
        pV->takeFocus (pContext);
    ENDFOR
}

//-----------------------------------------------------------------------------
bool CViewContainer::advanceNextFocusView (CView* oldFocus, bool reverse)
{
    bool foundOld = false;
    FOREACHSUBVIEW_REVERSE(reverse)
        if (oldFocus && !foundOld)
        {
            if (oldFocus == pV)
            {
                foundOld = true;
                continue;
            }
        }
        else
        {
            if (pV->wantsFocus ())
            {
                getFrame ()->setFocusView (pV);
                return true;
            }
            else if (pV->isTypeOf ("CViewContainer"))
            {
                if (((CViewContainer*)pV)->advanceNextFocusView (0, reverse))
                    return true;
            }
        }
    ENDFOR
    return false;
}

//-----------------------------------------------------------------------------
bool CViewContainer::isDirty () const
{
    if (bDirty)
        return true;

    CRect viewSize (size);
    viewSize.offset (-size.left, -size.top);

    FOREACHSUBVIEW
        if (pV->isDirty ())
        {
            CRect r (pV->size);
            r.bound (viewSize);
            if (r.getWidth () > 0 && r.getHeight () > 0)
                return true;
        }
    ENDFOR
    return false;
}

//-----------------------------------------------------------------------------
CView *CViewContainer::getCurrentView () const
{
    if (!pParentFrame)
        return 0;

    // get the current position
    CPoint where;
    pParentFrame->getCurrentLocation (where);

    frameToLocal (where);

    CCView *pSv = pLastView;
    while (pSv)
    {
        CView *pV = pSv->pView;
        if (pV && where.isInside (pV->mouseableArea))
            return pV;
        pSv = pSv->pPrevious;
    }

    return 0;
}

//-----------------------------------------------------------------------------
CView *CViewContainer::getViewAt (const CPoint& p, bool deep) const
{
    if (!pParentFrame)
        return 0;

    CPoint where (p);

    // convert to relativ pos
    where.offset (-size.left, -size.top);

    CCView *pSv = pLastView;
    while (pSv)
    {
        CView *pV = pSv->pView;
        if (pV && where.isInside (pV->mouseableArea))
        {
            if (deep)
            {
                if (pV->isTypeOf ("CViewContainer"))
                    return ((CViewContainer*)pV)->getViewAt (where, deep);
            }
            return pV;
        }
        pSv = pSv->pPrevious;
    }

    return 0;
}

//-----------------------------------------------------------------------------
CPoint& CViewContainer::frameToLocal (CPoint& point) const
{
    point.offset (-size.left, -size.top);
    if (pParentView && pParentView->isTypeOf ("CViewContainer"))
        return pParentView->frameToLocal (point);
    return point;
}

//-----------------------------------------------------------------------------
CPoint& CViewContainer::localToFrame (CPoint& point) const
{
    point.offset (size.left, size.top);
    if (pParentView && pParentView->isTypeOf ("CViewContainer"))
        return pParentView->localToFrame (point);
    return point;
}

//-----------------------------------------------------------------------------
bool CViewContainer::removed (CView* parent)
{
     if (pOffscreenContext)
        pOffscreenContext->forget ();
    pOffscreenContext = 0;

    return true;
}

//-----------------------------------------------------------------------------
bool CViewContainer::attached (CView* view)
{
    // create offscreen bitmap
    if (!pOffscreenContext && bDrawInOffscreen)
        pOffscreenContext = new COffscreenContext (pParentFrame, (long)size.width (), (long)size.height (), kBlackCColor);

    return true;
}

//-----------------------------------------------------------------------------
void CViewContainer::useOffscreen (bool b)
{
    bDrawInOffscreen = b;

    if (!bDrawInOffscreen && pOffscreenContext)
    {
        pOffscreenContext->forget ();
        pOffscreenContext = 0;
    }
}

//-----------------------------------------------------------------------------
void CViewContainer::modifyDrawContext (CCoord save[4], CDrawContext* pContext)
{
    // store
    save[0] = pContext->offsetScreen.h;
    save[1] = pContext->offsetScreen.v;
    save[2] = pContext->offset.h;
    save[3] = pContext->offset.v;

    pContext->offsetScreen.h += size.left;
    pContext->offsetScreen.v += size.top;
    pContext->offset.h += size.left;
    pContext->offset.v += size.top;
}

//-----------------------------------------------------------------------------
void CViewContainer::restoreDrawContext (CDrawContext* pContext, CCoord save[4])
{
    // restore
    pContext->offsetScreen.h = save[0];
    pContext->offsetScreen.v = save[1];
    pContext->offset.h = save[2];
    pContext->offset.v = save[3];
}

#if DEBUG
static long _debugDumpLevel = 0;
//-----------------------------------------------------------------------------
void CViewContainer::dumpInfo ()
{
    static const char* modeString[] = { "Normal Update Mode", "Only Dirty Update Mode"};
    DebugPrint ("CViewContainer: Mode: %s, Offscreen:%s ", modeString[mode], bDrawInOffscreen ? "Yes" : "No");
    CView::dumpInfo ();
}

//-----------------------------------------------------------------------------
void CViewContainer::dumpHierarchy ()
{
    _debugDumpLevel++;
    FOREACHSUBVIEW
        for (long i = 0; i < _debugDumpLevel; i++)
            DebugPrint ("\t");
        pV->dumpInfo ();
        DebugPrint ("\n");
        if (pV->isTypeOf ("CViewContainer"))
            ((CViewContainer*)pV)->dumpHierarchy ();
    ENDFOR
    _debugDumpLevel--;
}

#endif


//-----------------------------------------------------------------------------
// CBitmap Implementation
//-----------------------------------------------------------------------------
/*! @class CBitmap
@section cbitmap_alphablend Alpha Blend and Transparency
With Version 3.0 of VSTGUI it is possible to use alpha blended bitmaps. This comes free on Mac OS X and with Windows you need to include libpng.
Per default PNG images will be rendered alpha blended. If you want to use a transparency color with PNG Bitmaps, you need to call setNoAlpha(true) on the bitmap and set the transparency color.
@section cbitmap_macos Classic Apple Mac OS
The Bitmaps are PICTs and stored inside the resource fork.
@section cbitmap_macosx Apple Mac OS X
The Bitmaps can be of type PNG, JPEG, PICT, BMP and are stored in the Resources folder of the plugin bundle.
They must be named bmp00100.png (or bmp00100.jpg, etc). The number is the resource id.
@section cbitmap_windows Microsoft Windows
The Bitmaps are .bmp files and must be included in the plug (usually using a .rc file).
It's also possible to use png as of version 3.0 if you define the macro USE_LIBPNG and include the libpng and zlib libraries/sources to your project.
*/
CBitmap::CBitmap (AEffGUIEditor& editor, const char* filename)
    : resourceID (resourceID), width (0), height (0), noAlpha (true)
{
#if DEBUG
    gNbCBitmap++;
#endif

    bool found = false;
    long  i = 0;
    long  ncolors, cpp;

    pHandle = 0;
    pMask  = 0;
    
	if (editor.getBundlePath() == NULL) {
		std::cerr << "ERROR: No bundle path set, unable to load images" << std::endl;
	} else {
		std::string path = std::string(editor.getBundlePath()) + filename;
		if (openPng(path.c_str())) // reads width, height
			closePng();
	}

    setTransparentColor (kTransparentCColor);

#if DEBUG
    gBitmapAllocation += (long)height * (long)width;
#endif
}

//-----------------------------------------------------------------------------
CBitmap::CBitmap (CFrame &frame, CCoord width, CCoord height)
    : width (width), height (height), noAlpha (true), pMask (0)
{
#if DEBUG
    gNbCBitmap++;
#endif

    pHandle = (void*) XCreatePixmap (display,
                                     frame.getBackBuffer (),
                                     width,
                                     height,
                                     frame.getDepth ());

    setTransparentColor (kTransparentCColor);
}

//-----------------------------------------------------------------------------
CBitmap::CBitmap ()
	: resourceID (0)
	, width (0)
	, height (0)
	, noAlpha (true)
{
    pMask = 0;
    pHandle = 0;
}

//-----------------------------------------------------------------------------
CBitmap::~CBitmap ()
{
    dispose ();
}

//-----------------------------------------------------------------------------
void CBitmap::dispose ()
{
#if DEBUG
    gNbCBitmap--;
    gBitmapAllocation -= (long)height * (long)width;
#endif

    if (pHandle)
        XFreePixmap (display, (Pixmap)pHandle);
    if (pMask)
        XFreePixmap (display, (Pixmap)pMask);

    pHandle = 0;
    pMask = 0;

    width = 0;
    height = 0;

}

//-----------------------------------------------------------------------------
void *CBitmap::getHandle () const
{
    return pHandle;
}

//-----------------------------------------------------------------------------
bool CBitmap::loadFromResource (long resourceID)
{
    dispose ();
	fprintf (stderr, "CBitmap::loadFromResource not implemented\n");
    return false;
}

//-----------------------------------------------------------------------------
bool CBitmap::loadFromPath (const void* platformPath)
{
	if (pHandle != NULL) {
		dispose ();
		closePng ();
	}

	bool success = openPng ((char*)platformPath);
	if (success)
		closePng ();

	return success;
}

//-----------------------------------------------------------------------------
bool CBitmap::isLoaded () const
{
    return (pHandle != NULL);
}

//-----------------------------------------------------------------------------
void CBitmap::draw (CDrawContext *pContext, CRect &rect, const CPoint &offset)
{
    if (!pHandle)
    {
        // the first time try to decode the pixmap
        pHandle = createPixmapFromPng (pContext);
        if (!pHandle)
            return;
    }

    CFrame* frame = pContext->pFrame;

    XCopyArea (display,
               (Drawable) pHandle,
               (Drawable) frame->getBackBuffer(),
               (GC) frame->getGC(),
               offset.h,
               offset.v,
               std::min (rect.width (), getWidth()),
               std::min (rect.height (), getHeight()),
               rect.left + pContext->offset.h,
               rect.top + pContext->offset.v);
}

//-----------------------------------------------------------------------------
void CBitmap::drawTransparent (CDrawContext *pContext, CRect &rect, const CPoint &offset)
{
    if (!pHandle)
    {
        // the first time try to decode the pixmap
        pHandle = createPixmapFromPng (pContext);
        if (!pHandle)
            return;
    }

    CFrame* frame = pContext->pFrame;

    if (pMask == 0)
    {
        // get image from the pixmap
        XImage* image = XGetImage (display, (Drawable)pHandle,
                                   0, 0, width, height, AllPlanes, ZPixmap);
        assert (image);

        // create the bitmap mask
        pMask = (void*) XCreatePixmap (display, (Drawable)frame->getWindow(),
                                       width, height, 1);
        assert (pMask);

        // create a associated GC
        XGCValues values;
        values.foreground = 1;
        GC gc = XCreateGC (display, (Drawable)pMask, GCForeground, &values);

        // clear the mask
        XFillRectangle (display, (Drawable)pMask, gc, 0, 0, width, height);

        // get the transparent color index
        int color = pContext->getIndexColor (transparentCColor);

        // inverse the color
        values.foreground = 0;
        XChangeGC (display, gc, GCForeground, &values);

        // compute the mask
        XPoint *points = new XPoint [height * width];
        int x, y, nbPoints = 0;
        switch (image->depth)
        {
        case 8:
            for (y = 0; y < height; y++)
            {
                char* src = image->data + (y * image->bytes_per_line);

                for (x = 0; x < width; x++)
                {
                    if (src[x] == color)
                    {
                        points[nbPoints].x = x;
                        points[nbPoints].y = y;
                        nbPoints++;
                    }
                }
            }
            break;

        case 24: {
            int bytesPerPixel = image->bits_per_pixel >> 3;
            char *lp = image->data;
            for (y = 0; y < height; y++)
            {
                char* cp = lp;
                for (x = 0; x < width; x++)
                {
                    if (*(int*)cp == color)
                    {
                        points[nbPoints].x = x;
                        points[nbPoints].y = y;
                        nbPoints++;
                    }
                    cp += bytesPerPixel;
                }
                lp += image->bytes_per_line;
            }
        } break;

        default :
            break;
        }

        XDrawPoints (display, (Drawable)pMask, gc,
                     points, nbPoints, CoordModeOrigin);

        // free
        XFreeGC (display, gc);
        delete []points;

        // delete
        XDestroyImage (image);
    }

    // set the new clipmask
    XGCValues value;
    value.clip_mask = (Pixmap)pMask;
    value.clip_x_origin = (rect.left + pContext->offset.h) - offset.h;
    value.clip_y_origin = (rect.top + pContext->offset.v) - offset.v;

    XChangeGC (display,
               (GC) /*frame->getGC(),*/ pContext->pSystemContext,
               GCClipMask | GCClipXOrigin | GCClipYOrigin,
               &value);

    XCopyArea (display,
               (Drawable) pHandle,
               (Drawable) frame->getBackBuffer(),
               (GC) frame->getGC(),
               offset.h,
               offset.v,
               rect.width (),
               rect.height (),
               rect.left + pContext->offset.h,
               rect.top + pContext->offset.v);

    // unset the clipmask
    XSetClipMask (display, (GC)frame->getGC(), None);
}

//-----------------------------------------------------------------------------
void CBitmap::drawAlphaBlend (CDrawContext *pContext, CRect &rect, const CPoint &offset, unsigned char alpha)
{
    std::cout << "CBitmap::drawAlphaBlend (not implemented!)" << std::endl;
}

//-----------------------------------------------------------------------------
void CBitmap::setTransparentColor (const CColor color)
{
    transparentCColor = color;
}

//-----------------------------------------------------------------------------
void CBitmap::setTransparencyMask (CDrawContext* pContext, const CPoint& offset)
{
    // todo: implement me!
}


//-----------------------------------------------------------------------------
bool CBitmap::openPng (const char* path)
{
	assert(path);

	FILE* fp = fopen(path, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open file %s\n", path);
		return false;
	}
	
	png_byte header[8];
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "File not recognized as a PNG image");
		return false;
	}

	pngRead = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!pngRead) {
		fprintf(stderr, "Unable to initialize libpng\n");
		return false;
	}
    
	pngInfo = png_create_info_struct(pngRead);
    if (!pngInfo) {
        png_destroy_read_struct(&pngRead, NULL, NULL);
        return false;
    }

	png_init_io(pngRead, fp);
	png_set_sig_bytes(pngRead, 8);
	
	png_read_info(pngRead, pngInfo);

	width = pngInfo->width;
	height = pngInfo->height;

	pngFp = fp;
	pngRead = pngRead;
	pngPath = path;

	return true;
}

//-----------------------------------------------------------------------------
bool CBitmap::closePng ()
{
	png_destroy_read_struct(&pngRead, &pngInfo, NULL);
	fclose(pngFp);
	pngFp = NULL;
	return true;
}

//-----------------------------------------------------------------------------
void* CBitmap::createPixmapFromPng (CDrawContext *pContext)
{
	if (!openPng(pngPath.c_str()))
        return NULL;

	assert(width > 0 && height > 0);
	
	png_byte** rows = (png_byte**)malloc(height * sizeof(png_byte*));
	for (int i = 0; i < height; ++i)
		rows[i] = (png_byte*)(malloc(pngInfo->width * sizeof(uint32_t)));
	
	png_read_image(pngRead, rows);

	CFrame* frame = pContext->pFrame;

	Pixmap p = XCreatePixmap(display, frame->getBackBuffer(),
			pngInfo->width, pngInfo->height, 24);
    
	XGCValues values;
    values.foreground = 0xFFFFFFFF;
    
	// Draw
	GC gc = XCreateGC (display, p, GCForeground, &values);
	for (unsigned y = 0; y < pngInfo->height; y++) {
		for (unsigned x = 0; x < pngInfo->width; ++x) {
			char r = rows[y][(x*3)];
			char g = rows[y][(x*3)+1];
			char b = rows[y][(x*3)+2];
			uint32_t color = (r << 16) + (g << 8) + b;
			XSetForeground(display, gc, color);
			XDrawPoint(display, p, gc, x, y);
		}
	}
    XFreeGC (display, gc);
	
	closePng();

	return (void*)p;
}

//-----------------------------------------------------------------------------
// CDragContainer Implementation
//-----------------------------------------------------------------------------
CDragContainer::CDragContainer (void* platformDrag)
: platformDrag (platformDrag)
, nbItems (0)
, iterator (0)
, lastItem (0)
{
}

//-----------------------------------------------------------------------------
CDragContainer::~CDragContainer ()
{
    if (lastItem)
    {
        free (lastItem);
        lastItem = 0;
    }
}

//-----------------------------------------------------------------------------
long CDragContainer::getType (long idx) const
{
    // not implemented
    return kUnknown;
}

//-----------------------------------------------------------------------------
void* CDragContainer::first (long& size, long& type)
{
    iterator = 0;
    return next (size, type);
}

//-----------------------------------------------------------------------------
void* CDragContainer::next (long& size, long& type)
{
    if (lastItem)
    {
        free (lastItem);
        lastItem = 0;
    }
    size = 0;
    type = kUnknown;

    // not implemented

    return NULL;
}

} // namespace VSTGUI

//-----------------------------------------------------------------------------
// return a degre value between [0, 360 * 64[
static long convertPoint2Angle (CPoint &pm, CPoint &pt)
{
    long angle;
    if (pt.h == pm.h)
    {
        if (pt.v < pm.v)
            angle = 5760;    // 90 * 64
        else
            angle = 17280; // 270 * 64
    }
    else if (pt.v == pm.v)
    {
        if (pt.h < pm.h)
            angle = 11520;    // 180 * 64
        else
            angle = 0;
    }
    else
    {
        // 3666.9299 = 180 * 64 / pi
        angle = (long)(3666.9298 * atan ((double)(pm.v - pt.v) / (double)(pt.h - pm.h)));

        if (pt.v < pm.v)
        {
            if (pt.h < pm.h)
                angle += 11520; // 180 * 64
        }
        else
        {
            if (pt.h < pm.h)
                angle += 11520; // 180 * 64
            else
                angle += 23040; // 360 * 64
        }
    }
    return angle;
}



#if !PLUGGUI
//-----------------------------------------------------------------------------
// CFileSelector Implementation
//-----------------------------------------------------------------------------
#define stringAnyType  "Any Type (*.*)"
#define stringAllTypes "All Types: ("
#define stringSelect   "Select"
#define stringCancel   "Cancel"
#define stringLookIn   "Look in"
#define kPathMax        1024

namespace VSTGUI {

//-----------------------------------------------------------------------------
CFileSelector::CFileSelector (AudioEffectX* effect)
: effect (effect), vstFileSelect (0)
{}

//-----------------------------------------------------------------------------
CFileSelector::~CFileSelector ()
{
    if (vstFileSelect)
    {
        if (effect && effect->canHostDo ("closeFileSelector"))
            effect->closeFileSelector (vstFileSelect);
        else
        {
            if (vstFileSelect->reserved == 1 && vstFileSelect->returnPath)
            {
                delete []vstFileSelect->returnPath;
                vstFileSelect->returnPath = 0;
                vstFileSelect->sizeReturnPath = 0;
            }
            if (vstFileSelect->returnMultiplePaths)
            {
                for (long i = 0; i < vstFileSelect->nbReturnPath; i++)
                {
                    delete []vstFileSelect->returnMultiplePaths[i];
                    vstFileSelect->returnMultiplePaths[i] = 0;
                }
                delete[] vstFileSelect->returnMultiplePaths;
                vstFileSelect->returnMultiplePaths = 0;
            }
        }
    }
}

//-----------------------------------------------------------------------------
long CFileSelector::run (VstFileSelect *vstFileSelect_)
{
    vstFileSelect = vstFileSelect_;
    vstFileSelect->nbReturnPath = 0;
    vstFileSelect->returnPath[0] = 0;

    if (effect
        && effect->canHostDo ("openFileSelector")
        && effect->canHostDo ("closeFileSelector"))
    {
        if (effect->openFileSelector (vstFileSelect))
            return vstFileSelect->nbReturnPath;
    }
    return 0;
}

} // namespace VSTGUI

//-----------------------------------------------------------------------------
#endif // !PLUGGUI
//-----------------------------------------------------------------------------

