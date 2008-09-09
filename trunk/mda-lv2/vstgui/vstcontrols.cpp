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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef __vstcontrols__
#include "vstcontrols.h"
#endif

#include "vstkeycode.h"

namespace VSTGUI {

// some external variables (vstgui.cpp)
extern long gStandardFontSize [];
extern const char *gStandardFontName [];

//------------------------------------------------------------------------
// CControl
//------------------------------------------------------------------------
/*! @class CControl
This object manages the tag identification and the value of a control object.

Note:
Since version 2.1, when an object uses the transparency for its background and draws on it (tranparency area)
or the transparency area changes during different draws (CMovieBitmap ,...), the background will be false (not updated),
you have to rewrite the draw function in order to redraw the background and then call the draw of the object.
*/
CControl::CControl (const CRect &size, CControlListener *listener, long tag,
                    CBitmap *pBackground)
:    CView (size),
    listener (listener), tag (tag), oldValue (1), defaultValue (0.5f),
    value (0), vmin (0), vmax (1.f), wheelInc (0.1f), lastTicks (-1)
{
    delta = 500;

    if (delta < 250)
        delta = 250;

    setTransparency (false);
    setMouseEnabled (true);
    backOffset (0 ,0);

    setBackground (pBackground);
}

//------------------------------------------------------------------------
CControl::~CControl ()
{
}

//------------------------------------------------------------------------
void CControl::beginEdit ()
{
    // begin of edit parameter
    getFrame ()->setFocusView(this);
    getFrame ()->beginEdit (tag);
}

//------------------------------------------------------------------------
void CControl::endEdit ()
{
    // end of edit parameter
    getFrame ()->endEdit (tag);
}

//------------------------------------------------------------------------
bool CControl::isDirty () const
{
    if (oldValue != value || CView::isDirty ())
        return true;
    return false;
}

//------------------------------------------------------------------------
void CControl::setDirty (const bool val)
{
    CView::setDirty (val);
    if (val)
    {
        if (value != -1.f)
            oldValue = -1.f;
        else
            oldValue = 0.f;
    }
    else
        oldValue = value;
}

//------------------------------------------------------------------------
void CControl::setBackOffset (CPoint &offset)
{
    backOffset = offset;
}

//-----------------------------------------------------------------------------
void CControl::copyBackOffset ()
{
    backOffset (size.left, size.top);
}

//------------------------------------------------------------------------
void CControl::bounceValue ()
{
    if (value > vmax)
        value = vmax;
    else if (value < vmin)
        value = vmin;
}

//-----------------------------------------------------------------------------
bool CControl::checkDefaultValue (CDrawContext *pContext, long button)
{
    if (button == (kControl|kLButton))
    {
        // begin of edit parameter
        beginEdit ();

        value = getDefaultValue ();
        if (isDirty () && listener)
            listener->valueChanged (pContext, this);

        // end of edit parameter
        endEdit ();
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool CControl::isDoubleClick ()
{
    long ticks = getFrame ()->getTicks ();
    if (lastTicks <= 0)
    {
        lastTicks = ticks;
        return false;
    }

    if (lastTicks + delta > ticks)
        lastTicks = 0;
    else
    {
        lastTicks = ticks;
        return false;
    }
    return true;
}

//------------------------------------------------------------------------
// COnOffButton
//------------------------------------------------------------------------
/*! @class COnOffButton
Define a button with 2 positions.
The pixmap includes the 2 subpixmaps (i.e the rectangle used for the display of this button is half-height of the pixmap).
When its value changes, the listener is called.
*/
COnOffButton::COnOffButton (const CRect &size, CControlListener *listener, long tag,
                            CBitmap *background, long style)
: CControl (size, listener, tag, background)
, style (style)
{}

//------------------------------------------------------------------------
COnOffButton::~COnOffButton ()
{}

//------------------------------------------------------------------------
void COnOffButton::draw (CDrawContext *pContext)
{
//    DBG ("COnOffButton::draw");

    CCoord off;

    if (value && pBackground)
        off = pBackground->getHeight () / 2;
    else
        off = 0;

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, CPoint (0, off));
        else
            pBackground->draw (pContext, size, CPoint (0, off));
    }
    setDirty (false);
}

//------------------------------------------------------------------------
void COnOffButton::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();
    if (!(button & kLButton))
        return;

    if (listener && (button & (kAlt | kShift | kControl | kApple)))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    value = ((long)value) ? 0.f : 1.f;

    if (listener && style == kPostListenerUpdate)
    {
        // begin of edit parameter
        beginEdit ();

        listener->valueChanged (pContext, this);

        // end of edit parameter
        endEdit ();
    }

    doIdleStuff ();

    if (listener && style == kPreListenerUpdate)
    {
        // begin of edit parameter
        beginEdit ();

        listener->valueChanged (pContext, this);

        // end of edit parameter
        endEdit ();
    }
}


//------------------------------------------------------------------------
// CKnob
//------------------------------------------------------------------------
/*! @class CKnob
Define a knob with a given background and foreground handle.
The handle describes a circle over the background (between -45deg and +225deg).
By clicking Alt+Left Mouse the default value is used.
By clicking Alt+Left Mouse the value changes with a vertical move (version 2.1)
*/
CKnob::CKnob (const CRect &size, CControlListener *listener, long tag,
              CBitmap *background, CBitmap *handle, const CPoint &offset)
:    CControl (size, listener, tag, background), offset (offset), pHandle (handle)
{
    if (pHandle)
    {
        pHandle->remember ();
        inset = (long)((float)pHandle->getWidth () / 2.f + 2.5f);
    }
    else
        inset = 3;

    colorShadowHandle = kGreyCColor;
    colorHandle = kWhiteCColor;
    radius = (float)(size.right - size.left) / 2.f;

    rangeAngle = 1.f;
    setStartAngle ((float)(5.f * kPI / 4.f));
    setRangeAngle ((float)(-3.f * kPI / 2.f));
    zoomFactor = 1.5f;

    setWantsFocus (true);
}

//------------------------------------------------------------------------
CKnob::~CKnob ()
{
    if (pHandle)
        pHandle->forget ();
}

//------------------------------------------------------------------------
void CKnob::draw (CDrawContext *pContext)
{
//    DBG ("CKnob::draw");

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, offset);
        else
            pBackground->draw (pContext, size, offset);
    }
    drawHandle (pContext);
    setDirty (false);
}

//------------------------------------------------------------------------
void CKnob::drawHandle (CDrawContext *pContext)
{
    CPoint where;
    valueToPoint (where);

    if (pHandle)
    {
        long width  = (long)pHandle->getWidth ();
        long height = (long)pHandle->getHeight ();
        where.offset (size.left - width / 2, size.top - height / 2);

        CRect handleSize (0, 0, width, height);
        handleSize.offset (where.h, where.v);
        pHandle->drawTransparent (pContext, handleSize);
    }
    else
    {
        CPoint origin (size.width () / 2, size.height () / 2);

        where.offset (size.left - 1, size.top);
        origin.offset (size.left - 1, size.top);
        pContext->setFrameColor (colorShadowHandle);
        pContext->moveTo (where);
        pContext->lineTo (origin);

        where.offset (1, -1);
        origin.offset (1, -1);
        pContext->setFrameColor (colorHandle);
        pContext->moveTo (where);
        pContext->lineTo (origin);
    }
}

//------------------------------------------------------------------------
void CKnob::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();
    if (!(button & kLButton))
        return;

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    // check if default value wanted
    if (checkDefaultValue (pContext, button))
        return;

    float old = oldValue;
    CPoint firstPoint;
    bool  modeLinear = false;
    float fEntryState = value;
    float middle = (vmax - vmin) * 0.5f;
    float range = 200.f;
    float coef = (vmax - vmin) / range;
    long  oldButton = button;

    long mode    = kCircularMode;
    long newMode = getFrame ()->getKnobMode ();
    if (kLinearMode == newMode)
    {
        if (!(button & kAlt))
            mode = newMode;
    }
    else if (button & kAlt)
        mode = kLinearMode;

    if (mode == kLinearMode && (button & kLButton))
    {
        if (button & kShift)
            range *= zoomFactor;
        firstPoint = where;
        modeLinear = true;
        coef = (vmax - vmin) / range;
    }
    else
    {
        CPoint where2 (where);
        where2.offset (-size.left, -size.top);
        old = valueFromPoint (where2);
    }

    CPoint oldWhere (-1, -1);

    // begin of edit parameter
    beginEdit ();
    do
    {
        button = pContext->getMouseButtons ();
        if (where != oldWhere)
        {
            oldWhere = where;
            if (modeLinear)
            {
                CCoord diff = (firstPoint.v - where.v) + (where.h - firstPoint.h);
                if (button != oldButton)
                {
                    range = 200.f;
                    if (button & kShift)
                        range *= zoomFactor;

                    float coef2 = (vmax - vmin) / range;
                    fEntryState += diff * (coef - coef2);
                    coef = coef2;
                    oldButton = button;
                }
                value = fEntryState + diff * coef;
                bounceValue ();
            }
            else
            {
                where.offset (-size.left, -size.top);
                value = valueFromPoint (where);
                if (old - value > middle)
                    value = vmax;
                else if (value - old > middle)
                    value = vmin;
                else
                    old = value;
            }
            if (isDirty () && listener)
                listener->valueChanged (pContext, this);
        }
        getMouseLocation (pContext, where);
        doIdleStuff ();

    } while (button & kLButton);

    // end of edit parameter
    endEdit ();
}

//------------------------------------------------------------------------
bool CKnob::onWheel (CDrawContext *pContext, const CPoint &where, float distance)
{
    if (!bMouseEnabled)
        return false;

    long buttons = pContext->getMouseButtons ();
    if (buttons & kShift)
        value += 0.1f * distance * wheelInc;
    else
        value += distance * wheelInc;
    bounceValue ();

    if (isDirty () && listener)
    {
        // begin of edit parameter
        beginEdit ();

        listener->valueChanged (pContext, this);

        // end of edit parameter
        endEdit ();
    }
    return true;
}

//------------------------------------------------------------------------
long CKnob::onKeyDown (VstKeyCode& keyCode)
{
    switch (keyCode.virt)
    {
    case VKEY_UP :
    case VKEY_RIGHT :
    case VKEY_DOWN :
    case VKEY_LEFT :
        {
            float distance = 1.f;
            if (keyCode.virt == VKEY_DOWN || keyCode.virt == VKEY_LEFT)
                distance = -distance;

            if (keyCode.modifier & MODIFIER_SHIFT)
                value += 0.1f * distance * wheelInc;
            else
                value += distance * wheelInc;
            bounceValue ();

            if (isDirty () && listener)
            {
                // begin of edit parameter
                beginEdit ();

                listener->valueChanged (0, this);

                // end of edit parameter
                endEdit ();
            }
        } return 1;
    }
    return -1;
}

//------------------------------------------------------------------------
void CKnob::setStartAngle (float val)
{
    startAngle = val;
    compute ();
}

//------------------------------------------------------------------------
void CKnob::setRangeAngle (float val)
{
    rangeAngle = val;
    compute ();
}

//------------------------------------------------------------------------
void CKnob::compute ()
{
    aCoef = (vmax - vmin) / rangeAngle;
    bCoef = vmin - aCoef * startAngle;
    halfAngle = ((float)k2PI - fabsf (rangeAngle)) * 0.5f;
    setDirty ();
}

//------------------------------------------------------------------------
void CKnob::valueToPoint (CPoint &point) const
{
    float alpha = (value - bCoef) / aCoef;
    point.h = (long)(radius + cosf (alpha) * (radius - inset) + 0.5f);
    point.v = (long)(radius - sinf (alpha) * (radius - inset) + 0.5f);
}

//------------------------------------------------------------------------
float CKnob::valueFromPoint (CPoint &point) const
{
    float v;
    float alpha = (float)atan2 (radius - point.v, point.h - radius);
    if (alpha < 0.f)
        alpha += (float)k2PI;

    float alpha2 = alpha - startAngle;
    if (rangeAngle < 0)
    {
        alpha2 -= rangeAngle;
        float alpha3 = alpha2;
        if (alpha3 < 0.f)
            alpha3 += (float)k2PI;
        else if (alpha3 > k2PI)
            alpha3 -= (float)k2PI;
        if (alpha3 > halfAngle - rangeAngle)
            v = vmax;
        else if (alpha3 > -rangeAngle)
            v = vmin;
        else
        {
            if (alpha2 > halfAngle - rangeAngle)
                alpha2 -= (float)k2PI;
            else if (alpha2 < -halfAngle)
                alpha2 += (float)k2PI;
            v = aCoef * alpha2 + vmax;
        }
    }
    else
    {
        float alpha3 = alpha2;
        if (alpha3 < 0.f)
            alpha3 += (float)k2PI;
        else if (alpha3 > k2PI)
            alpha3 -= (float)k2PI;
        if (alpha3 > rangeAngle + halfAngle)
            v = vmin;
        else if (alpha3 > rangeAngle)
            v = vmax;
        else
        {
            if (alpha2 > rangeAngle + halfAngle)
                alpha2 -= (float)k2PI;
            else if (alpha2 < -halfAngle)
                alpha2 += (float)k2PI;
            v = aCoef * alpha2 + vmin;
        }
    }

    return v;
}

//------------------------------------------------------------------------
void CKnob::setColorShadowHandle (CColor color)
{
    colorShadowHandle = color;
    setDirty ();
}

//------------------------------------------------------------------------
void CKnob::setColorHandle (CColor color)
{
    colorHandle = color;
    setDirty ();
}

//------------------------------------------------------------------------
void CKnob::setHandleBitmap (CBitmap *bitmap)
{
    if (pHandle)
    {
        pHandle->forget ();
        pHandle = 0;
    }

    if (bitmap)
    {
        pHandle = bitmap;
        pHandle->remember ();
        inset = (long)((float)pHandle->getWidth () / 2.f + 2.5f);
    }
}


//------------------------------------------------------------------------
// CParamDisplay
//------------------------------------------------------------------------
/*! @class CParamDisplay
Define a rectangle view where a text-value can be displayed with a given font and color.
The user can specify its convert function (from float to char) by default the string format is "%2.2f".
The text-value is centered in the given rect.
*/
CParamDisplay::CParamDisplay (const CRect &size, CBitmap *background, const long style)
:    CControl (size, 0, 0, background), stringConvert (0), stringConvert2 (0), string2FloatConvert (0),
    horiTxtAlign (kCenterText), style (style), bTextTransparencyEnabled (true)
{
    backOffset (0, 0);

    fontID      = kNormalFont;
    txtFace     = kNormalFace;
    fontColor   = kWhiteCColor;
    backColor   = kBlackCColor;
    frameColor  = kBlackCColor;
    shadowColor = kRedCColor;
    userData    = 0;
    if (style & kNoDrawStyle)
        setDirty (false);
}

//------------------------------------------------------------------------
CParamDisplay::~CParamDisplay ()
{}

//------------------------------------------------------------------------
void CParamDisplay::setStyle (long val)
{
    if (style != val)
    {
        style = val;
        setDirty ();
    }
}

//------------------------------------------------------------------------
void CParamDisplay::draw (CDrawContext *pContext)
{
//    DBG ("CParamDisplay::draw");

    char string[256];
    string[0] = 0;

    if (stringConvert2)
        stringConvert2 (value, string, userData);
    else if (stringConvert)
        stringConvert (value, string);
    else
        sprintf (string, "%2.2f", value);

    drawText (pContext, string);
}

//------------------------------------------------------------------------
void CParamDisplay::drawText (CDrawContext *pContext, char *string, CBitmap *newBack)
{
    setDirty (false);

    if (style & kNoDrawStyle)
        return;

    // draw the background
    if (newBack)
    {
        if (bTransparencyEnabled)
            newBack->drawTransparent (pContext, size, backOffset);
        else
            newBack->draw (pContext, size, backOffset);
    }
    else if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, backOffset);
        else
            pBackground->draw (pContext, size, backOffset);
    }
    else
    {
        if (!bTransparencyEnabled)
        {
            pContext->setFillColor (backColor);
            pContext->fillRect (size);

            if (!(style & (k3DIn|k3DOut|kNoFrame)))
            {
                pContext->setFrameColor (frameColor);
                pContext->drawRect (size);
            }
        }
    }
    // draw the frame for the 3D effect
    if (style & (k3DIn|k3DOut))
    {
        if (style & k3DIn)
            pContext->setFrameColor (backColor);
        else
            pContext->setFrameColor (frameColor);
        CPoint p;
        pContext->moveTo (p (size.left, size.bottom));
        pContext->lineTo (p (size.left, size.top));
        pContext->lineTo (p (size.right + 1, size.top));

        if (style & k3DIn)
            pContext->setFrameColor (frameColor);
        else
            pContext->setFrameColor (backColor);
        pContext->moveTo (p (size.right, size.top + 1));
        pContext->lineTo (p (size.right, size.bottom));
        pContext->lineTo (p (size.left, size.bottom));
    }

    if (!(style & kNoTextStyle) && string)
    {
        CRect oldClip;
        pContext->getClipRect (oldClip);
        CRect newClip (size);
        newClip.bound (oldClip);
        pContext->setClipRect (newClip);
        pContext->setFont (fontID, 0, txtFace);

        // draw darker text (as shadow)
        if (style & kShadowText)
        {
            CRect newSize (size);
            newSize.offset (1, 1);
            pContext->setFontColor (shadowColor);
            pContext->drawString (string, newSize, !bTextTransparencyEnabled, horiTxtAlign);
        }
        pContext->setFontColor (fontColor);
        pContext->drawString (string, size, !bTextTransparencyEnabled, horiTxtAlign);
        pContext->setClipRect (oldClip);
    }
}

//------------------------------------------------------------------------
void CParamDisplay::setFont (CFont newFontID)
{
    // to force the redraw
    if (fontID != newFontID)
        setDirty ();
    fontID = newFontID;
}

//------------------------------------------------------------------------
void CParamDisplay::setTxtFace (CTxtFace newTxtFace)
{
    // to force the redraw
    if (txtFace != newTxtFace)
        setDirty ();
    txtFace = newTxtFace;
}

//------------------------------------------------------------------------
void CParamDisplay::setFontColor (CColor color)
{
    // to force the redraw
    if (fontColor != color)
        setDirty ();
    fontColor = color;
}

//------------------------------------------------------------------------
void CParamDisplay::setBackColor (CColor color)
{
    // to force the redraw
    if (backColor != color)
        setDirty ();
    backColor = color;
}

//------------------------------------------------------------------------
void CParamDisplay::setFrameColor (CColor color)
{
    // to force the redraw
    if (frameColor != color)
        setDirty ();
    frameColor = color;
}

//------------------------------------------------------------------------
void CParamDisplay::setShadowColor (CColor color)
{
    // to force the redraw
    if (shadowColor != color)
        setDirty ();
    shadowColor = color;
}

//------------------------------------------------------------------------
void CParamDisplay::setHoriAlign (CHoriTxtAlign hAlign)
{
    // to force the redraw
    if (horiTxtAlign != hAlign)
        setDirty ();
    horiTxtAlign = hAlign;
}

//------------------------------------------------------------------------
void CParamDisplay::setStringConvert (void (*convert) (float value, char *string))
{
    stringConvert = convert;
}

//------------------------------------------------------------------------
void CParamDisplay::setStringConvert (void (*convert) (float value, char *string,
                                      void *userDta), void *userData_)
{
    stringConvert2 = convert;
    userData = userData_;
}

//------------------------------------------------------------------------
void CParamDisplay::setString2FloatConvert (void (*convert) (char *string, float &output))
{
    string2FloatConvert = convert;
}

//------------------------------------------------------------------------
// CTextLabel
//------------------------------------------------------------------------
/*! @class CTextLabel
*/
CTextLabel::CTextLabel (const CRect& size, const char* txt, CBitmap* background, const long style)
: CParamDisplay (size, background, style)
, text (0)
{
    setText (txt);
}

//------------------------------------------------------------------------
CTextLabel::~CTextLabel ()
{
    freeText ();
}

//------------------------------------------------------------------------
void CTextLabel::freeText ()
{
    if (text)
        free (text);
    text = 0;
}

//------------------------------------------------------------------------
void CTextLabel::setText (const char* txt)
{
    freeText ();
    if (txt)
    {
        text = (char*)malloc (strlen (txt)+1);
        strcpy (text, txt);
    }
}

//------------------------------------------------------------------------
const char* CTextLabel::getText () const
{
    return text;
}

//------------------------------------------------------------------------
void CTextLabel::draw (CDrawContext *pContext)
{
//    DBG ("CTextLabel::draw");

    drawText (pContext, text);
    setDirty (false);
}


//------------------------------------------------------------------------
// CTextEdit
//------------------------------------------------------------------------
/*! @class CTextEdit
Define a rectangle view where a text-value can be displayed and edited with a given font and color.
The user can specify its convert function (from char to char). The text-value is centered in the given rect.
A pixmap can be used as background.
*/
CTextEdit::CTextEdit (const CRect &size, CControlListener *listener, long tag,
    const char *txt, CBitmap *background, const long style)
:    CParamDisplay (size, background, style), platformFontColor (0), platformControl (0),
    platformFont (0), editConvert (0), editConvert2 (0)
{
    this->listener = listener;
    this->tag = tag;

    if (txt)
        strcpy (text, txt);
    else
        strcpy (text, "");
    setWantsFocus (true);
}

//------------------------------------------------------------------------
CTextEdit::~CTextEdit ()
{}

//------------------------------------------------------------------------
void CTextEdit::setText (char *txt)
{
    if (txt)
    {
        if (strcmp (text, txt))
        {
            strcpy (text, txt);

            // to force the redraw
            setDirty ();
        }
    }
    else
    {
        if (strcmp (text, ""))
        {
            strcpy (text, "");

            // to force the redraw
            setDirty ();
        }
    }
}

//------------------------------------------------------------------------
void CTextEdit::getText (char *txt) const
{
    if (txt)
        strcpy (txt, text);
}

//------------------------------------------------------------------------
void CTextEdit::draw (CDrawContext *pContext)
{
//    DBG ("CTextEdit::draw");

    if (platformControl)
    {
        setDirty (false);
        return;
    }

    char string[256];
    string[0] = 0;

    if (editConvert2)
        editConvert2 (text, string, userData);
    else if (editConvert)
        editConvert (text, string);
    // Allow to display strings through the stringConvert
    // callbacks inherited from CParamDisplay
    else if (stringConvert2)
    {
        string[0] = 0;
        stringConvert2 (value, string, userData);
        strcpy(text, string);
    }
    else if (stringConvert)
    {
        string[0] = 0;
        stringConvert (value, string);
        strcpy(text, string);
    }
    else
        sprintf (string, "%s", text);

    drawText (pContext, string);
    setDirty (false);
}

//------------------------------------------------------------------------
void CTextEdit::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (button & kLButton)
    {
        if (getFrame ()->getFocusView () != this)
        {
            if (style & kDoubleClickStyle)
                if (!isDoubleClick ())
                    return;

            beginEdit();
            takeFocus (pContext);
        }
    }
}

//------------------------------------------------------------------------
// #include <Xm/Text.h>
extern XFontStruct *gFontStructs[];

//------------------------------------------------------------------------
void CTextEdit::takeFocus (CDrawContext *pContext)
{
    bWasReturnPressed = false;

/*
    // we have to add the Text to the parent !!
    Dimension posX, posY;
    Widget widget = (Widget)(getFrame ()->getSystemWindow ());
    XtVaGetValues (widget, XmNx, &posX, XmNy, &posY, 0);

    Arg args[20];
    int n = 0;
    XtSetArg (args[n], XmNx, size.left + posX); n++;
    XtSetArg (args[n], XmNy, size.top + posY); n++;
    XtSetArg (args[n], XmNwidth, size.width () + 1); n++;
    XtSetArg (args[n], XmNheight, size.height () + 2); n++;

    XtSetArg (args[n], XmNvalue, text); n++;

    XtSetArg (args[n], XmNshadowType, XmSHADOW_IN); n++;
    XtSetArg (args[n], XmNshadowThickness, 0); n++;
    XtSetArg (args[n], XmNcursorPositionVisible, true); n++;

    XtSetArg (args[n], XmNmarginWidth, 0); n++;
    XtSetArg (args[n], XmNmarginHeight, 0); n++;
    XtSetArg (args[n], XmNresizeHeight, True); n++;
    XtSetArg (args[n], XmNborderWidth, 0); n++;
    XtSetArg (args[n], XmNeditMode, XmSINGLE_LINE_EDIT); n++;

    // get/set the current font
    XmFontList fl = 0;
    XFontStruct* fs = gFontStructs [fontID];
    if (fs)
    {
        XmFontListEntry entry = XmFontListEntryCreate (XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONT, fs);
        XmFontList fl = XmFontListAppendEntry (0, entry);
        XtSetArg (args[n], XmNfontList, fl); n++;
    }

    platformControl = XmCreateText (XtParent (widget), "Text", args, n);
    XtManageChild ((Widget)platformControl);
    if (fl)
        XmFontListFree (fl);
    XmTextSetSelection ((Widget)platformControl, 0, strlen (text), 0);
    XmTextSetHighlight ((Widget)platformControl, 0, strlen (text), XmHIGHLIGHT_SELECTED);
*/
}

//------------------------------------------------------------------------
void CTextEdit::looseFocus (CDrawContext *pContext)
{
    // Call this yet to avoid recursive call
    endEdit();
    if (getFrame ()->getFocusView () == this)
        getFrame ()->setFocusView (0);

    if (platformControl == 0)
        return;

/*
    char oldText[256];
    strcpy (oldText, text);

    char *pNewText = XmTextGetString ((Widget)platformControl);
    strcpy (text, pNewText);
    XtFree (pNewText);

    XtUnmanageChild ((Widget)platformControl);
    XtDestroyWidget ((Widget)platformControl);

    CPoint origOffset;
    bool resetContextOffset = false;
    if (!pContext)
    {
        // create a local context
        pContext = getFrame ()->createDrawContext ();
        if (getParentView ())
        {
            resetContextOffset = true;
            origOffset.x = pContext->offset.x;
            origOffset.y = pContext->offset.y;
            CView *view= getParentView ();
            CRect rect2;
            view->getViewSize (rect2);
            CPoint offset;
            view->localToFrame (offset);
            rect2.offset (offset.x, offset.y);
            pContext->offset.h = rect2.left;
            pContext->offset.v = rect2.top;
        }
    }
    else
        pContext->remember ();

    // update dependency
    bool change = false;
    if (strcmp (oldText, text))
    {
        change = true;
        if (listener)
            listener->valueChanged (pContext, this);
    }

    platformControl = 0;
    if (resetContextOffset)
    {
        pContext->offset.x = origOffset.x;
        pContext->offset.y = origOffset.y;
    }
    pContext->forget ();

    if (change)
        doIdleStuff ();

    CView* receiver = pParentView ? pParentView : pParentFrame;
    if (receiver)
        receiver->notify (this, "LooseFocus");
*/
}

//------------------------------------------------------------------------
void CTextEdit::setTextEditConvert (void (*convert) (char *input, char *string))
{
    editConvert = convert;
}

//------------------------------------------------------------------------
void CTextEdit::setTextEditConvert (void (*convert) (char *input, char *string,
                                      void *userDta), void *userData)
{
    editConvert2 = convert;
    this->userData = userData;
}

//------------------------------------------------------------------------
// COptionMenuScheme
//------------------------------------------------------------------------
/*! @class COptionMenuScheme
Used to define the appearance (font color, background color...) of a popup-menu.
To define the scheme of a menu, use the appropriate setScheme method (see COptionMenu).
@section coptionmenuscheme_new_in_3_0 New since 3.0
You can also use the global variable gOptionMenuScheme to use one scheme on all menus.
@section coptionmenuscheme_note Note
If you want to use it on Mac OS X, you must set the macro MAC_ENABLE_MENU_SCHEME (needs Mac OS X 10.3 or higher)
*/
COptionMenuScheme* gOptionMenuScheme = 0;

//------------------------------------------------------------------------
COptionMenuScheme::COptionMenuScheme ()
{
    backgroundColor = kGreyCColor;
    selectionColor = kBlueCColor;
    textColor = kBlackCColor;
    hiliteTextColor = kWhiteCColor;
    disableTextColor = kWhiteCColor;

    font = kNormalFontSmall;
}

//------------------------------------------------------------------------
COptionMenuScheme::~COptionMenuScheme ()
{
}

//------------------------------------------------------------------------
void COptionMenuScheme::getItemSize (const char* text, CDrawContext* pContext, CPoint& size)
{
    if (!strcmp (text, kMenuSeparator)) // separator
    {
        // was: size.h = size.v = 6;
        size.h = 6;
        size.v = 18;
        // separators must have same height, otherwise we have problems
        // in multi-column menus :(
    }
    else
    {
        pContext->setFont (font);
        size.h = pContext->getStringWidth (text) + 18;
        size.v = 18;
    }
}

//------------------------------------------------------------------------
void COptionMenuScheme::drawItemBack (CDrawContext* pContext, const CRect& rect, bool hilite)
{
    if (hilite)
        pContext->setFillColor (selectionColor);
    else
        pContext->setFillColor (backgroundColor);
    pContext->fillRect (rect);
}

//------------------------------------------------------------------------
void COptionMenuScheme::drawItem (const char* text, long itemId, long state, CDrawContext* pContext, const CRect& rect)
{
    bool hilite = (state & kSelected) != 0;

    drawItemBack (pContext, rect, hilite);

    if (!strcmp (text, kMenuSeparator))
    {
        CCoord y = rect.top + rect.height () / 2;

        const CColor bc = { 0, 0, 0, 150};
        const CColor wc = { 255, 255, 255, 150};

        pContext->setFrameColor (bc);
        pContext->moveTo (CPoint (rect.left + 2, y - 1));
        pContext->lineTo (CPoint (rect.right - 2, y - 1));
        pContext->setFrameColor (wc);
        pContext->moveTo (CPoint (rect.left + 2, y));
        pContext->lineTo (CPoint (rect.right - 2, y));
        return;
    }

    CRect r;
    if (state & kChecked)
    {
        r (6, 4, 14, 12);
        r.offset (rect.left, rect.top);
        if (hilite)
            pContext->setFillColor (hiliteTextColor);
        else
            pContext->setFillColor (textColor);
        pContext->fillEllipse (r);
    }

    r = rect;
    r.left += 18;
    pContext->setFont (font);
    if (state & kDisabled)
        pContext->setFontColor (disableTextColor);
    else
    {
        if (hilite)
            pContext->setFontColor (hiliteTextColor);
        else
            pContext->setFontColor (textColor);
    }

    // this needs to be done right, without changing the text pointer in anyway ;-)
    char *ptr = (char*)strstr (text, "\t");
    if (ptr)
    {
        char modifier[32];
        strcpy (modifier, ptr + 1);
        *ptr = 0;
        pContext->drawString (text, r, false, kLeftText);

        *ptr = '\t';
        r.left = r.right - 50;
        pContext->drawString (modifier, r, false, kLeftText);
    }
    else
        pContext->drawString (text, r, false, kLeftText);
}


//------------------------------------------------------------------------
// COptionMenu
//------------------------------------------------------------------------
/*! @class COptionMenu
Define a rectangle view where a text-value can be displayed with a given font and color.
The text-value is centered in the given rect.
A pixmap can be used as background, a second pixmap can be used when the option menu is popuped.
There are 2 styles with or without a shadowed text. When a mouse click occurs, a popup menu is displayed.
*/
COptionMenu::COptionMenu (const CRect &size, CControlListener *listener, long tag,
                          CBitmap *background, CBitmap *bgWhenClick, const long style)
:    CParamDisplay (size, background, style), bgWhenClick (bgWhenClick), nbItemsPerColumn (-1),
    prefixNumbers (0), scheme (0)
{
    this->listener = listener;
    this->tag = tag;

    nbEntries = 0;
    nbSubMenus = 0;
    currentIndex = -1;
    lastButton = kRButton;
    platformControl = 0;
    lastResult = -1;
    lastMenu = 0;

    if (bgWhenClick)
        bgWhenClick->remember ();

    nbSubMenuAllocated = nbAllocated = 0;

    check = 0;
    entry = 0;
    submenuEntry = 0;
}

//------------------------------------------------------------------------
COptionMenu::~COptionMenu ()
{
    removeAllEntry ();

    if (bgWhenClick)
        bgWhenClick->forget ();
}

//------------------------------------------------------------------------
void COptionMenu::setPrefixNumbers (long preCount)
{
    prefixNumbers = preCount;
}

//-----------------------------------------------------------------------------
bool COptionMenu::allocateSubMenu (long nb)
{
    long newAllocated = nbSubMenuAllocated + nb;

    if (submenuEntry)
        submenuEntry = (COptionMenu**)realloc (submenuEntry, newAllocated * sizeof (COptionMenu*));
    else
        submenuEntry = (COptionMenu**)malloc (newAllocated * sizeof (COptionMenu*));

    long i;
    for (i = nbSubMenuAllocated; i < newAllocated; i++)
        submenuEntry[i] = 0;

    nbSubMenuAllocated = newAllocated;

    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::allocateMenu (long nb)
{
    long newAllocated = nbAllocated + nb;

    if (check)
        check = (bool*)realloc (check, newAllocated * sizeof (bool));
    else
        check = (bool*)malloc (newAllocated * sizeof (bool));
    if (!check)
        return false;

    if (entry)
        entry = (char**)realloc (entry, newAllocated * sizeof (char*));
    else
        entry = (char**)malloc (newAllocated * sizeof (char*));
    if (!entry)
    {
        free (check);
        return false;
    }

    long i;
    for (i = nbAllocated; i < newAllocated; i++)
    {
        check[i] = false;
        entry[i] = 0;
    }

    nbAllocated = newAllocated;

    return true;
}

//------------------------------------------------------------------------
COptionMenu* COptionMenu::getSubMenu (long idx) const
{
    if (submenuEntry && idx < nbSubMenus)
        return submenuEntry[idx];
    return 0;
}

//------------------------------------------------------------------------
bool COptionMenu::addEntry (COptionMenu *subMenu, char *txt)
{
    if (nbEntries >= MAX_ENTRY || !subMenu || !txt)
        return false;

    if (nbEntries >= nbAllocated)
        if (!allocateMenu (32))
            return false;

    entry[nbEntries] = (char*)malloc (256);
    switch (prefixNumbers)
    {
        case 2:
            sprintf (entry[nbEntries], "-M%1d %s", (int)(nbEntries + 1), txt);
            break;

        case 3:
            sprintf (entry[nbEntries], "-M%02d %s", (int)(nbEntries + 1), txt);
            break;

        case 4:
            sprintf (entry[nbEntries], "-M%03d %s", (int)(nbEntries + 1), txt);
            break;

        default:
            sprintf (entry[nbEntries], "-M%s", txt);
    }


    if (nbSubMenus >= nbSubMenuAllocated)
        if (!allocateSubMenu (10))
            return false;

    submenuEntry[nbSubMenus++] = subMenu;
    subMenu->remember ();

    nbEntries++;

    if (currentIndex < 0)
        currentIndex = 0;

    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::addEntry (char *txt, long index)
{
    if (nbEntries >= MAX_ENTRY)
        return false;

    if (nbEntries >= nbAllocated)
        if (!allocateMenu (32))
            return false;

    entry[nbEntries] = (char*)malloc (256);

    long pos = nbEntries;

    // switch the entries for the insert
    if (index >= 0)
    {
        for (long i = nbEntries; i > index; i--)
            strcpy (entry[i], entry[i - 1]);
        if (index >= nbEntries)
            pos = nbEntries;
        else
            pos = index;
        if (currentIndex >= index)
            currentIndex++;
    }

    *entry[pos] = 0;
    if (txt)
    {
        switch (prefixNumbers)
        {
            case 2:
                sprintf (entry[pos], "%1d %s", (int)(index + 1), txt);
                break;

            case 3:
                sprintf (entry[pos], "%02d %s", (int)(index + 1), txt);
                break;

            case 4:
                sprintf (entry[pos], "%03d %s", (int)(index + 1), txt);
                break;

            default:
                strncpy (entry[pos], txt, 256);
        }
    }

    nbEntries++;

    if (currentIndex < 0)
        currentIndex = 0;

    return true;
}

//------------------------------------------------------------------------
long COptionMenu::getCurrent (char *txt, bool countSeparator) const
{
    if (currentIndex < 0)
        return -1;

    long result = 0;

    if (countSeparator)
    {
        if (txt)
            strcpy (txt, entry[currentIndex]);
        result = currentIndex;
    }
    else
    {
        for (long i = 0; i < currentIndex; i++)
        {
            if (strcmp (entry[i], kMenuSeparator) && strncmp (entry[i], kMenuTitle, 2))
                result++;
        }
        if (txt)
            strcpy (txt, entry[currentIndex]);
    }
    return result;
}

//------------------------------------------------------------------------
bool COptionMenu::setCurrent (long index, bool countSeparator)
{
    if (index < 0 || index >= nbEntries)
        return false;

    if (countSeparator)
    {
        if (!strcmp (entry[index], kMenuSeparator) && strncmp (entry[index], kMenuTitle, 2))
            return false;

        currentIndex = index;
    }
    else
    {
        long newCurrent = 0;
        long i = 0;
        while (i <= index && newCurrent < nbEntries)
        {
            if (strcmp (entry[newCurrent], kMenuSeparator) && strncmp (entry[newCurrent], kMenuTitle, 2))
                i++;
            newCurrent++;
        }
        currentIndex = newCurrent - 1;
    }
    if (style & (kMultipleCheckStyle & ~kCheckStyle))
        check[currentIndex] = !check[currentIndex];

    // to force the redraw
    setDirty ();

    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::getEntry (long index, char *txt) const
{
    if (index < 0 || index >= nbEntries)
        return false;

    if (txt)
        strcpy (txt, entry[index]);
    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::setEntry (long index, char *txt)
{
    if (index < 0 || index >= nbEntries)
        return false;

    if (txt)
        strcpy (entry[index], txt);
    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::removeEntry (long index)
{
    if (index < 0 || index >= nbEntries)
        return false;

    nbEntries--;

    // switch the entries
    for (long i = index; i < nbEntries; i++)
    {
        strcpy (entry[i], entry[i + 1]);
        check[i] = check [i + 1];
    }

    if (currentIndex >= index)
        currentIndex--;

    // delete the last one
    free (entry[nbEntries]);
    entry[nbEntries] = 0;
    check[nbEntries] = false;

    if (nbEntries == 0)
        currentIndex = -1;
    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::removeAllEntry ()
{
    long i;
    for (i = 0; i < nbEntries; i++)
    {
        free (entry[i]);
        entry[i] = 0;
        check[i] = false;
    }

    nbEntries = 0;
    currentIndex = -1;

    for (i = 0; i < nbSubMenus; i++)
    {
        submenuEntry[i]->forget ();
        submenuEntry[i] = 0;
    }
    nbSubMenus = 0;

    if (check)
        free (check);
    check = 0;
    if (entry)
        free (entry);
    entry = 0;
    if (submenuEntry)
        free (submenuEntry);
    submenuEntry = 0;
    nbAllocated = 0;
    nbSubMenuAllocated = 0;

    return true;
}

//------------------------------------------------------------------------
long COptionMenu::getIndex (char *txt) const
{
    if (!txt)
        return -1;

    // search entries
    for (long i = 0; i < nbEntries; i++)
        if (!strcmp (entry[i], txt))
            return i;

    // not found
    return -1;
}

//------------------------------------------------------------------------
bool COptionMenu::checkEntry (long index, bool state)
{
    if (index < 0 || index >= nbEntries)
        return false;

    check[index] = state;

    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::checkEntryAlone (long index)
{
    if (index < 0 || index >= nbEntries)
        return false;
    for (long i = 0; i < nbEntries; i++)
        check[i] = false;
    check[index] = true;

    return true;
}

//------------------------------------------------------------------------
bool COptionMenu::isCheckEntry (long index) const
{
    if (index < 0 || index >= nbEntries)
        return false;

    return check[index];
}

//------------------------------------------------------------------------
void COptionMenu::draw (CDrawContext *pContext)
{
//    DBG ("COptionMenu::draw");

    if (currentIndex >= 0 && nbEntries)
        drawText (pContext, entry[currentIndex] + prefixNumbers);
    else
        drawText (pContext, NULL);
}

//------------------------------------------------------------------------
void COptionMenu::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled || !getFrame () || !pContext)
        return;

    lastButton = (button != -1) ? button : pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (lastButton & (kLButton|kRButton|kApple))
    {
        if (bgWhenClick)
        {
            char string[256];
            if (currentIndex >= 0)
                sprintf (string, "%s", entry[currentIndex]);
            else
                string[0] = 0;

            drawText (pContext, string, bgWhenClick);
        }

        beginEdit();
        takeFocus (pContext);
    }
}

//------------------------------------------------------------------------
/*
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/SeparatoG.h>

static void _unmapCallback (Widget item, XtPointer clientData, XtPointer callData);
static void _activateCallback (Widget item, XtPointer clientData, XtPointer callData);

//------------------------------------------------------------------------
static void _unmapCallback (Widget item, XtPointer clientData, XtPointer callData)
{
    COptionMenu *optionMenu= (COptionMenu*)clientData;
    optionMenu->looseFocus ();
}

//------------------------------------------------------------------------
static void _activateCallback (Widget item, XtPointer clientData, XtPointer callData)
{
    COptionMenu *optionMenu= (COptionMenu*)clientData;
    optionMenu->setCurrentSelected ((void*)item);
}
*/

//------------------------------------------------------------------------
COptionMenu *COptionMenu::getLastItemMenu (long &idxInMenu) const
{
    idxInMenu = lastMenu ? (long)lastMenu->getValue (): -1;
    return lastMenu;
}

//------------------------------------------------------------------------
COptionMenu *COptionMenu::getItemMenu (long idx, long &idxInMenu, long &offsetIdx)
{
    COptionMenu *menu = 0;
    for (long i = 0; i < nbSubMenus; i++)
    {
        menu = submenuEntry[i]->getItemMenu (idx, idxInMenu, offsetIdx);
        if (menu)
            break;
    }
    return menu;
}

//------------------------------------------------------------------------
void COptionMenu::removeItems ()
{
    for (long i = 0; i < nbSubMenus; i++)
        submenuEntry[i]->removeItems ();
}

//------------------------------------------------------------------------
void *COptionMenu::appendItems (long &offsetIdx)
{
    //bool multipleCheck = style & (kMultipleCheckStyle & ~kCheckStyle);
	return NULL;
}

//------------------------------------------------------------------------
void COptionMenu::setValue (float val)
{
    if ((long)val < 0 || (long)val >= nbEntries)
        return;

    currentIndex = (long)val;
    if (style & (kMultipleCheckStyle & ~kCheckStyle))
        check[currentIndex] = !check[currentIndex];
    CParamDisplay::setValue (val);

    // to force the redraw
    setDirty ();
}

//------------------------------------------------------------------------
void COptionMenu::takeFocus (CDrawContext *pContext)
{
    if (!getFrame ())
        return;

/*

    bool multipleCheck = style & (kMultipleCheckStyle & ~kCheckStyle);
    lastResult = -1;
    lastMenu = 0;

    Arg args[10];
    int n = 0;

    // get the position of the pParent
    CRect rect;
    getFrame ()->getSize (&rect);

    if (pContext)
    {
        rect.left += pContext->offset.h;
        rect.top  += pContext->offset.v;
    }

    // create a popup menu
    int offset;
    if (style & kPopupStyle)
        offset = (int)(rect.top + size.top);
    else
        offset = (int)(rect.top + size.bottom);

    XtSetArg (args[n], XmNx, rect.left + size.left); n++;
    XtSetArg (args[n], XmNy, offset); n++;
    XtSetArg (args[n], XmNmenuHistory, currentIndex); n++;
    XtSetArg (args[n], XmNtraversalOn, true); n++;

    platformControl = (void*)XmCreatePopupMenu ((Widget)(getFrame ()->getSystemWindow ()),
            "popup", args, n);

    XtAddCallback ((Widget)platformControl, XmNunmapCallback, _unmapCallback, this);

    // insert the menu items
    for (long i = 0; i < nbEntries; i++)
    {
        if (!strcmp (entry[i], kMenuSeparator))
        {
            itemWidget[i] = (void*)XtCreateManagedWidget ("separator",
                             xmSeparatorGadgetClass, (Widget)platformControl, 0, 0);
        }
        else
        {
            if (multipleCheck)
            {
                itemWidget[i] = (void*)XtVaCreateManagedWidget (entry[i],
                    xmToggleButtonWidgetClass, (Widget)platformControl,
                    XmNset, check[i], XmNvisibleWhenOff, false, 0);
                XtAddCallback ((Widget)itemWidget[i], XmNvalueChangedCallback, _activateCallback, this);
            }
            else if (style & kCheckStyle)
            {
                itemWidget[i] = (void*)XtVaCreateManagedWidget (entry[i],
                    xmToggleButtonWidgetClass, (Widget)platformControl,
                    XmNset, (i == currentIndex) ? true : false, XmNvisibleWhenOff, false, 0);
                XtAddCallback ((Widget)itemWidget[i], XmNvalueChangedCallback, _activateCallback, this);
            }
            else
            {
                itemWidget[i] = (void*)XtVaCreateManagedWidget (entry[i],
                    xmPushButtonWidgetClass, (Widget)platformControl, 0);
                XtAddCallback ((Widget)itemWidget[i], XmNactivateCallback, _activateCallback, this);
            }
        }
    }

    XtManageChild ((Widget)platformControl);
    getFrame ()->setFocusView (0);
    endEdit();
*/
}

//------------------------------------------------------------------------
void COptionMenu::looseFocus (CDrawContext *pContext)
{
    if (platformControl == 0)
        return;

/*
    for (long i = 0; i < nbEntries; i++)
        if (itemWidget[i])
            XtDestroyWidget ((Widget)itemWidget[i]);

    if (platformControl)
    {
        XtUnmanageChild ((Widget)platformControl);
        XtDestroyWidget ((Widget)platformControl);
    }
*/
    platformControl = 0;
}

//------------------------------------------------------------------------
void COptionMenu::setCurrentSelected (void *itemSelected)
{
    // retrieve the current index
    if (itemSelected != 0)
    {
        for (long i = 0; i < nbEntries; i++)
            if (itemWidget[i] == itemSelected)
            {
                currentIndex = i;
                break;
            }
    }

    // update dependency
    CDrawContext *pContext = new CDrawContext (getFrame (), (void*)getFrame ()->getGC (), (void*)getFrame ()->getBackBuffer ());

    setValue (currentIndex);

    if (listener)
        listener->valueChanged (pContext, this);
    delete pContext;
}


//------------------------------------------------------------------------
// CAnimKnob
//------------------------------------------------------------------------
/*! @class CAnimKnob
Such as a CKnob control object, but there is a unique pixmap which contains different views (subpixmaps) of this knob.
According to the value, a specific subpixmap is displayed. The different subpixmaps are stacked in the pixmap object.
*/
CAnimKnob::CAnimKnob (const CRect &size, CControlListener *listener, long tag,
                      CBitmap *background, CPoint &offset)
: CKnob (size, listener, tag, background, 0, offset), bInverseBitmap (false)
{
    heightOfOneImage = size.height ();
    subPixmaps = (short)(background->getHeight () / heightOfOneImage);
    inset = 0;
}

//------------------------------------------------------------------------
CAnimKnob::CAnimKnob (const CRect &size, CControlListener *listener, long tag,
                      long subPixmaps,         // number of subPixmaps
                      CCoord heightOfOneImage,   // height of one image in pixel
                      CBitmap *background, CPoint &offset)
: CKnob (size, listener, tag, background, 0, offset),
   subPixmaps (subPixmaps), heightOfOneImage (heightOfOneImage), bInverseBitmap (false)
{
    inset = 0;
}

//------------------------------------------------------------------------
CAnimKnob::~CAnimKnob ()
{}

//-----------------------------------------------------------------------------------------------
bool CAnimKnob::isDirty () const
{
    if (!bDirty)
    {
        CPoint p;
        valueToPoint (p);
        if (p == lastDrawnPoint)
            return false;
    }
    return CKnob::isDirty ();
}

//------------------------------------------------------------------------
void CAnimKnob::draw (CDrawContext *pContext)
{
//    DBG ("CAnimKnob::draw");

    CPoint where (0, 0);
    if (value >= 0.f)
    {
        CCoord tmp = heightOfOneImage * (subPixmaps - 1);
        if (bInverseBitmap)
            where.v = (long)((1 - value) * (float)tmp);
        else
            where.v = (long)(value * (float)tmp);
        for (CCoord realY = 0; realY <= tmp; realY += heightOfOneImage)
        {
            if (where.v < realY)
            {
                where.v = realY - heightOfOneImage;
                if (where.v < 0)
                    where.v = 0;
                break;
            }
        }
    }

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    valueToPoint (lastDrawnPoint);
    setDirty (false);
}

//------------------------------------------------------------------------
// CVerticalSwitch
//------------------------------------------------------------------------
/*! @class CVerticalSwitch
Define a switch with a given number of positions, the current position is defined by the position
of the last click on this object (the object is divided in its height by the number of position).
Each position has its subpixmap, each subpixmap is stacked in the given handle pixmap.
By clicking Alt+Left Mouse the default value is used.
*/
CVerticalSwitch::CVerticalSwitch (const CRect &size, CControlListener *listener, long tag,
                                  CBitmap *background, CPoint &offset)
: CControl (size, listener, tag, background), offset (offset)
{
    heightOfOneImage = size.height ();
    subPixmaps = (long)(background->getHeight () / heightOfOneImage);
    iMaxPositions = subPixmaps;

    setDefaultValue (0.f);
}

//------------------------------------------------------------------------
CVerticalSwitch::CVerticalSwitch (const CRect &size, CControlListener *listener, long tag,
                                  long subPixmaps,       // number of subPixmaps
                                  CCoord heightOfOneImage, // height of one image in pixel
                                  long iMaxPositions,
                                  CBitmap *background, CPoint &offset)
: CControl (size, listener, tag, background), offset (offset),
    subPixmaps (subPixmaps), heightOfOneImage (heightOfOneImage),
    iMaxPositions (iMaxPositions)
{
    setDefaultValue (0.f);
}

//------------------------------------------------------------------------
CVerticalSwitch::~CVerticalSwitch ()
{}

//------------------------------------------------------------------------
void CVerticalSwitch::draw (CDrawContext *pContext)
{
//    DBG ("CVerticalSwitch::draw");

    if (pBackground)
    {
        // source position in bitmap
        CPoint where (0, heightOfOneImage * ((long)(value * (iMaxPositions - 1) + 0.5f)));

        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    setDirty (false);
}

//------------------------------------------------------------------------
void CVerticalSwitch::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();
    if (!(button & kLButton))
        return;

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    // check if default value wanted
    if (checkDefaultValue (pContext, button))
        return;

    double coef = (double)heightOfOneImage / (double)iMaxPositions;

    // begin of edit parameter
    beginEdit ();
    do
    {
        value = (long)((where.v - size.top) / coef) / (float)(iMaxPositions - 1);
        if (value > 1.f)
            value = 1.f;
        else if (value < 0.f)
            value = 0.f;

        if (isDirty () && listener)
            listener->valueChanged (pContext, this);

        getMouseLocation (pContext, where);

        doIdleStuff ();
    }
    while (pContext->getMouseButtons () == button);

    // end of edit parameter
    endEdit ();
}


//------------------------------------------------------------------------
// CHorizontalSwitch
//------------------------------------------------------------------------
/*! @class CHorizontalSwitch
Same as the CVerticalSwitch but horizontal.
*/
CHorizontalSwitch::CHorizontalSwitch (const CRect &size, CControlListener *listener, long tag,
                                  CBitmap *background, CPoint &offset)
: CControl (size, listener, tag, background), offset (offset)
{
    heightOfOneImage = size.width ();
    subPixmaps = (long)(background->getWidth () / heightOfOneImage);
    iMaxPositions = subPixmaps;

    setDefaultValue (0.f);
}

//------------------------------------------------------------------------
CHorizontalSwitch::CHorizontalSwitch (const CRect &size, CControlListener *listener, long tag,
                                  long subPixmaps,   // number of subPixmaps
                                  CCoord heightOfOneImage, // height of one image in pixel
                                  long iMaxPositions,
                                  CBitmap *background, CPoint &offset)
: CControl (size, listener, tag, background), offset (offset),
    subPixmaps (subPixmaps),
    iMaxPositions (iMaxPositions),
	heightOfOneImage (heightOfOneImage)
{
    setDefaultValue (0.f);
}

//------------------------------------------------------------------------
CHorizontalSwitch::~CHorizontalSwitch ()
{}

//------------------------------------------------------------------------
void CHorizontalSwitch::draw (CDrawContext *pContext)
{
//    DBG ("CHorizontalSwitch::draw");

    if (pBackground)
    {
        // source position in bitmap
        CPoint where (0, heightOfOneImage * ((long)(value * (iMaxPositions - 1) + 0.5f)));

        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    setDirty (false);
}

//------------------------------------------------------------------------
void CHorizontalSwitch::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (!(button & kLButton))
        return;

    // check if default value wanted
    if (checkDefaultValue (pContext, button))
        return;

    double coef = (double)pBackground->getWidth () / (double)iMaxPositions;

    // begin of edit parameter
    beginEdit ();
    do
    {
        value = (long)((where.h - size.left) / coef) / (float)(iMaxPositions - 1);
        if (value > 1.f)
            value = 1.f;
        else if (value < 0.f)
            value = 0.f;

        if (isDirty () && listener)
            listener->valueChanged (pContext, this);

        getMouseLocation (pContext, where);

        doIdleStuff ();
    }
    while (pContext->getMouseButtons () == button);

    // end of edit parameter
    endEdit ();
}


//------------------------------------------------------------------------
// CRockerSwitch
//------------------------------------------------------------------------
/*! @class CRockerSwitch
Define a rocker switch with 3 states using 3 subpixmaps.
One click on its leftside, then the first subpixmap is displayed.
One click on its rightside, then the third subpixmap is displayed.
When the mouse button is relaxed, the second subpixmap is framed.
*/
CRockerSwitch::CRockerSwitch (const CRect &size, CControlListener *listener, long tag,              // identifier tag (ID)
                              CBitmap *background, CPoint &offset, const long style)
:    CControl (size, listener, tag, background), offset (offset), style (style)
{
    heightOfOneImage = size.width ();
}

//------------------------------------------------------------------------
CRockerSwitch::CRockerSwitch (const CRect &size, CControlListener *listener, long tag,              // identifier tag (ID)
                              CCoord heightOfOneImage, // height of one image in pixel
                              CBitmap *background, CPoint &offset, const long style)
:    CControl (size, listener, tag, background), offset (offset),
    heightOfOneImage (heightOfOneImage), style (style)
{}

//------------------------------------------------------------------------
CRockerSwitch::~CRockerSwitch ()
{}

//------------------------------------------------------------------------
void CRockerSwitch::draw (CDrawContext *pContext)
{
//   DBG ("CRockerSwitch::draw");

    CPoint where (offset.h, offset.v);

    if (value == 1.f)
        where.v += 2 * heightOfOneImage;
    else if (value == 0.f)
        where.v += heightOfOneImage;

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    setDirty (false);
}

//------------------------------------------------------------------------
void CRockerSwitch::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (!(button & kLButton))
        return;

    float fEntryState = value;

    CCoord  width_2  = size.width () / 2;
    CCoord  height_2 = size.height () / 2;

    // begin of edit parameter
    beginEdit ();

    if (button)
    {
        do
        {
            if (style & kHorizontal)
            {
                if (where.h >= size.left && where.v >= size.top  &&
                    where.h <= (size.left + width_2) && where.v <= size.bottom)
                    value = -1.0f;
                else if (where.h >= (size.left + width_2) && where.v >= size.top  &&
                    where.h <= size.right && where.v <= size.bottom)
                    value = 1.0f;
                else
                    value = fEntryState;
            }
            else
            {
                if (where.h >= size.left && where.v >= size.top  &&
                    where.h <= size.right && where.v <= (size.top + height_2))
                    value = -1.0f;
                else if (where.h >= size.left && where.v >= (size.top + height_2) &&
                    where.h <= size.right && where.v <= size.bottom)
                    value = 1.0f;
                else
                    value = fEntryState;
            }

            if (isDirty () && listener)
                listener->valueChanged (pContext, this);

            getMouseLocation (pContext, where);

            doIdleStuff ();
        }
        while (pContext->getMouseButtons ());
    }
    else
    {
        if (where.h >= size.left && where.v >= size.top  &&
                where.h <= (size.left + width_2) && where.v <= size.bottom)
            value = -1.0f;
        else if (where.h >= (size.left + width_2) && where.v >= size.top  &&
                where.h <= size.right && where.v <= size.bottom)
            value = 1.0f;

        if (listener)
            listener->valueChanged (pContext, this);
    }

    value = 0.f;  // set button to UNSELECTED state
    if (listener)
        listener->valueChanged (pContext, this);

    // end of edit parameter
    endEdit ();
}

//------------------------------------------------------------------------
bool CRockerSwitch::onWheel (CDrawContext *pContext, const CPoint &where, float distance)
{
    if (!bMouseEnabled)
        return false;

    if (distance > 0)
        value = -1.0f;
    else
        value = 1.0f;

    // begin of edit parameter
    beginEdit ();

    if (isDirty () && listener)
        listener->valueChanged (pContext, this);

    value = 0.0f;  // set button to UNSELECTED state
    if (listener)
        listener->valueChanged (pContext, this);

    // end of edit parameter
    endEdit ();

    return true;
}


//------------------------------------------------------------------------
// CMovieBitmap
//------------------------------------------------------------------------
/*! @class CMovieBitmap
A movie pixmap allows to display different subpixmaps according to its current value.
*/
CMovieBitmap::CMovieBitmap (const CRect &size, CControlListener *listener, long tag,
                            CBitmap *background, CPoint &offset)
  :    CControl (size, listener, tag, background), offset (offset),
        subPixmaps (subPixmaps), heightOfOneImage (heightOfOneImage)
{
    heightOfOneImage = size.height ();
    subPixmaps = (long)(background->getHeight () / heightOfOneImage);
}

//------------------------------------------------------------------------
CMovieBitmap::CMovieBitmap (const CRect &size, CControlListener *listener, long tag,
                            long subPixmaps,        // number of subPixmaps
                            CCoord heightOfOneImage,  // height of one image in pixel
                            CBitmap *background, CPoint &offset)
  :    CControl (size, listener, tag, background), offset (offset),
        subPixmaps (subPixmaps), heightOfOneImage (heightOfOneImage)
{}

//------------------------------------------------------------------------
CMovieBitmap::~CMovieBitmap ()
{}

//------------------------------------------------------------------------
void CMovieBitmap::draw (CDrawContext *pContext)
{
 //   DBG ("CMovieBitmap::draw");

    CPoint where (offset.h, offset.v);

    if (value > 1.0f)
        value = 1.0f;

     if (value > 0.0f)
        where.v += heightOfOneImage * (int)(value * (subPixmaps - 1) + 0.5);

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    setDirty (false);
}


//------------------------------------------------------------------------
// CMovieButton
//------------------------------------------------------------------------
/*! @class CMovieButton
A movie button is a bi-states button with 2 subpixmaps. These subpixmaps are stacked in the pixmap.
*/
CMovieButton::CMovieButton (const CRect &size, CControlListener *listener, long tag,              // identifier tag (ID)
                            CBitmap *background, CPoint &offset)
: CControl (size, listener, tag, background), offset (offset), buttonState (value)
{
    heightOfOneImage = size.height ();
}

//------------------------------------------------------------------------
CMovieButton::CMovieButton (const CRect &size, CControlListener *listener, long tag,
                            CCoord heightOfOneImage, // height of one image in pixel
                            CBitmap *background, CPoint &offset)
    :    CControl (size, listener, tag, background), offset (offset),
        heightOfOneImage (heightOfOneImage), buttonState (value)
{}

//------------------------------------------------------------------------
CMovieButton::~CMovieButton ()
{}

//------------------------------------------------------------------------
void CMovieButton::draw (CDrawContext *pContext)
{
  //  DBG ("CMovieButton::draw");

    CPoint where;

    where.h = 0;

    bounceValue ();

    if (value)
        where.v = heightOfOneImage;
    else
        where.v = 0;

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    buttonState = value;

    setDirty (false);
}

//------------------------------------------------------------------------
void CMovieButton::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (!(button & kLButton))
        return;

    // this simulates a real windows button
    float fEntryState = value;

    // begin of edit parameter
    beginEdit ();

    if (pContext->getMouseButtons ())
    {
        do
        {
            if (where.h >= size.left &&
                    where.v >= size.top  &&
                    where.h <= size.right &&
                    where.v <= size.bottom)
                value = !fEntryState;
            else
                value = fEntryState;

            if (isDirty () && listener)
                listener->valueChanged (pContext, this);

            getMouseLocation (pContext, where);

            doIdleStuff ();
        }
        while (pContext->getMouseButtons () == button);
    }
    else
    {
        value = !value;
        if (listener)
            listener->valueChanged (pContext, this);
    }

    // end of edit parameter
    endEdit ();

    buttonState = value;
}


//------------------------------------------------------------------------
// CAutoAnimation
//------------------------------------------------------------------------
/*! @class CAutoAnimation
An auto-animation control contains a given number of subpixmap which can be displayed in loop.
Two functions allows to get the previous or the next subpixmap (these functions increase or decrease the current value of this control).
*/
// displays bitmaps within a (child-) window
CAutoAnimation::CAutoAnimation (const CRect &size, CControlListener *listener, long tag,
                                CBitmap *background, CPoint &offset)
: CControl (size, listener, tag, background), offset (offset), bWindowOpened (false)
{
    heightOfOneImage = size.height ();
    subPixmaps = (long)(background->getHeight () / heightOfOneImage);

    totalHeightOfBitmap = heightOfOneImage * subPixmaps;
}

//------------------------------------------------------------------------
CAutoAnimation::CAutoAnimation (const CRect &size, CControlListener *listener, long tag,
                                long subPixmaps,     // number of subPixmaps...
                                CCoord heightOfOneImage, // height of one image in pixel
                                CBitmap *background, CPoint &offset)
    :    CControl (size, listener, tag, background), offset (offset),
        subPixmaps (subPixmaps), heightOfOneImage (heightOfOneImage),
        bWindowOpened (false)
{
    totalHeightOfBitmap = heightOfOneImage * subPixmaps;
}

//------------------------------------------------------------------------
CAutoAnimation::~CAutoAnimation ()
{}

//------------------------------------------------------------------------
void CAutoAnimation::draw (CDrawContext *pContext)
{
 //   DBG ("CAutoAnimation::draw");

    if (isWindowOpened ())
    {
        CPoint where;
        where.v = (long)value + offset.v;
        where.h = offset.h;

        if (pBackground)
        {
            if (bTransparencyEnabled)
                pBackground->drawTransparent (pContext, size, where);
            else
                pBackground->draw (pContext, size, where);
        }
    }
    setDirty (false);
}

//------------------------------------------------------------------------
void CAutoAnimation::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (!(button & kLButton))
        return;

    if (!isWindowOpened ())
    {
        value = 0;
        openWindow ();
        setDirty (); // force to redraw
        if (listener)
            listener->valueChanged (pContext, this);
    }
    else
    {
        // stop info animation
        value = 0; // draw first pic of bitmap
        setDirty ();
        closeWindow ();
    }
}

//------------------------------------------------------------------------
void CAutoAnimation::openWindow ()
{
    bWindowOpened = true;
}

//------------------------------------------------------------------------
void CAutoAnimation::closeWindow ()
{
    bWindowOpened = false;
}

//------------------------------------------------------------------------
void CAutoAnimation::nextPixmap ()
{
    value += heightOfOneImage;
    if (value >= (totalHeightOfBitmap - heightOfOneImage))
        value = 0;
}

//------------------------------------------------------------------------
void CAutoAnimation::previousPixmap ()
{
    value -= heightOfOneImage;
    if (value < 0.f)
        value = (float)(totalHeightOfBitmap - heightOfOneImage - 1);
}


//------------------------------------------------------------------------
// CSlider
//------------------------------------------------------------------------
/*! @class CSlider
Define a slider with a given background and handle.
The range of variation of the handle should be defined.
By default the handler is drawn with transparency (white color).
By clicking Alt+Left Mouse the default value is used.
*/
CSlider::CSlider (const CRect &rect, CControlListener *listener, long tag,
                  long      iMinPos, // min position in pixel
                  long      iMaxPos, // max position in pixel
                  CBitmap  *handle,  // bitmap of slider
                  CBitmap  *background, // bitmap of background
                  CPoint   &offset,  // offset in the background
                  const long style)  // style (kBottom,kRight,kTop,kLeft,kHorizontal,kVertical)
  :    CControl (rect, listener, tag, background),    offset (offset), pHandle (handle),
    pOScreen (0), style (style), bFreeClick (true)
{
    setDrawTransparentHandle (true);

    if (pHandle)
    {
        pHandle->remember ();
        widthOfSlider  = pHandle->getWidth ();
        heightOfSlider = pHandle->getHeight ();
    }
    else
    {
        widthOfSlider  = 1;
        heightOfSlider = 1;
    }

    widthControl  = size.width ();
    heightControl = size.height ();

    if (style & kHorizontal)
    {
        minPos = iMinPos - size.left;
        rangeHandle = iMaxPos - iMinPos;
        CPoint p (0, 0);
        setOffsetHandle (p);
    }
    else
    {
        minPos = iMinPos - size.top;
        rangeHandle = iMaxPos - iMinPos;
        CPoint p (0, 0);
        setOffsetHandle (p);
    }

    zoomFactor = 10.f;

    setWantsFocus (true);
}

//------------------------------------------------------------------------
CSlider::CSlider (const CRect &rect, CControlListener *listener, long tag,
                  CPoint   &offsetHandle,    // handle offset
                  long     _rangeHandle, // size of handle range
                  CBitmap  *handle,     // bitmap of slider
                  CBitmap  *background, // bitmap of background
                  CPoint   &offset,     // offset in the background
                  const long style)     // style (kBottom,kRight,kTop,kLeft,kHorizontal,kVertical)
:    CControl (rect, listener, tag, background), offset (offset), pHandle (handle),
    pOScreen (0), style (style), minPos (0), bFreeClick (true)
{
    setDrawTransparentHandle (true);

    if (pHandle)
    {
        pHandle->remember ();
        widthOfSlider  = pHandle->getWidth ();
        heightOfSlider = pHandle->getHeight ();
    }
    else
    {
        widthOfSlider  = 1;
        heightOfSlider = 1;
    }

    widthControl  = size.width ();
    heightControl = size.height ();
    if (style & kHorizontal)
        rangeHandle = _rangeHandle - widthOfSlider;
    else
        rangeHandle = _rangeHandle - heightOfSlider;

    setOffsetHandle (offsetHandle);

    zoomFactor = 10.f;

    setWantsFocus (true);
}

//------------------------------------------------------------------------
CSlider::~CSlider ()
{
    if (pHandle)
        pHandle->forget ();
}

//------------------------------------------------------------------------
void CSlider::setOffsetHandle (CPoint &val)
{
    offsetHandle = val;

    if (style & kHorizontal)
    {
        minTmp = offsetHandle.h + minPos;
        maxTmp = minTmp + rangeHandle + widthOfSlider;
    }
    else
    {
        minTmp = offsetHandle.v + minPos;
        maxTmp = minTmp + rangeHandle + heightOfSlider;
    }
}

//-----------------------------------------------------------------------------
bool CSlider::attached (CView *parent)
{
    if (pOScreen)
        delete pOScreen;

    pOScreen = 0; // @TODO - faking offscreen
//    pOScreen = new COffscreenContext (getFrame (), widthControl, heightControl, kBlackCColor);

    return CControl::attached (parent);
}

//-----------------------------------------------------------------------------
bool CSlider::removed (CView *parent)
{
    if (pOScreen)
    {
        delete pOScreen;
        pOScreen = 0;
    }
    return CControl::removed (parent);
}

//------------------------------------------------------------------------
void CSlider::draw (CDrawContext *pContext)
{
 //   DBG ("CSlider::draw");

    CDrawContext* drawContext = pOScreen ? pOScreen : pContext;

    if (pOScreen && bTransparencyEnabled)
        pOScreen->copyTo (pContext, size);

    float fValue;
    if (style & kLeft || style & kTop)
        fValue = value;
    else
        fValue = 1.f - value;

    // (re)draw background
    CRect rect (0, 0, widthControl, heightControl);
    if (!pOScreen)
        rect.offset (size.left, size.top);
    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (drawContext, rect, offset);
        else
            pBackground->draw (drawContext, rect, offset);
    }

    // calc new coords of slider
    CRect   rectNew;
    if (style & kHorizontal)
    {
        rectNew.top    = offsetHandle.v;
        rectNew.bottom = rectNew.top + heightOfSlider;

        rectNew.left   = offsetHandle.h + (int)(fValue * rangeHandle);
        rectNew.left   = (rectNew.left < minTmp) ? minTmp : rectNew.left;

        rectNew.right  = rectNew.left + widthOfSlider;
        rectNew.right  = (rectNew.right > maxTmp) ? maxTmp : rectNew.right;
    }
    else
    {
        rectNew.left   = offsetHandle.h;
        rectNew.right  = rectNew.left + widthOfSlider;

        rectNew.top    = offsetHandle.v + (int)(fValue * rangeHandle);
        rectNew.top    = (rectNew.top < minTmp) ? minTmp : rectNew.top;

        rectNew.bottom = rectNew.top + heightOfSlider;
        rectNew.bottom = (rectNew.bottom > maxTmp) ? maxTmp : rectNew.bottom;
    }
    if (!pOScreen)
        rectNew.offset (size.left, size.top);

    // draw slider at new position
    if (pHandle)
    {
        if (bDrawTransparentEnabled)
            pHandle->drawTransparent (drawContext, rectNew);
        else
            pHandle->draw (drawContext, rectNew);
    }

    if (pOScreen)
        pOScreen->copyFrom (pContext, size);

    setDirty (false);
}

//------------------------------------------------------------------------
void CSlider::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    // check if default value wanted
    if (checkDefaultValue (pContext, button))
        return;

    // allow left mousebutton only
    if (!(button & kLButton))
        return;

    CCoord delta;
    if (style & kHorizontal)
        delta = size.left + offsetHandle.h;
    else
        delta = size.top + offsetHandle.v;
    if (!bFreeClick)
    {
        float fValue;
        if (style & kLeft || style & kTop)
            fValue = value;
        else
            fValue = 1.f - value;
        CCoord actualPos;
        CRect rect;

        if (style & kHorizontal)
        {
            actualPos = offsetHandle.h + (int)(fValue * rangeHandle) + size.left;

            rect.left   = actualPos;
            rect.top    = size.top  + offsetHandle.v;
            rect.right  = rect.left + widthOfSlider;
            rect.bottom = rect.top  + heightOfSlider;

            if (!where.isInside (rect))
                return;
            else
                delta += where.h - actualPos;
        }
        else
        {
            actualPos = offsetHandle.v + (int)(fValue * rangeHandle) + size.top;

            rect.left   = size.left  + offsetHandle.h;
            rect.top    = actualPos;
            rect.right  = rect.left + widthOfSlider;
            rect.bottom = rect.top  + heightOfSlider;

            if (!where.isInside (rect))
                return;
            else
                delta += where.v - actualPos;
        }
    }
    else
    {
        if (style & kHorizontal)
            delta += widthOfSlider / 2 - 1;
        else
            delta += heightOfSlider / 2 - 1;
    }

    float oldVal    = value;
    long  oldButton = button;

    // begin of edit parameter
    beginEdit ();

    while (1)
    {
        button = pContext->getMouseButtons ();
        if (!(button & kLButton))
            break;

        if ((oldButton != button) && (button & kShift))
        {
            oldVal    = value;
            oldButton = button;
        }
        else if (!(button & kShift))
            oldVal = value;

        if (style & kHorizontal)
            value = (float)(where.h - delta) / (float)rangeHandle;
        else
            value = (float)(where.v - delta) / (float)rangeHandle;

        if (style & kRight || style & kBottom)
            value = 1.f - value;

        if (button & kShift)
            value = oldVal + ((value - oldVal) / zoomFactor);
        bounceValue ();

        if (isDirty () && listener)
            listener->valueChanged (pContext, this);

        getMouseLocation (pContext, where);

        doIdleStuff ();
    }

    // end of edit parameter
    endEdit ();
}

//------------------------------------------------------------------------
bool CSlider::onWheel (CDrawContext *pContext, const CPoint &where, float distance)
{
    if (!bMouseEnabled)
        return false;

    long buttons = pContext->getMouseButtons ();
    if (buttons & kShift)
        value += 0.1f * distance * wheelInc;
    else
        value += distance * wheelInc;
    bounceValue ();

    if (isDirty () && listener)
    {
        // begin of edit parameter
        beginEdit ();

        listener->valueChanged (pContext, this);

        // end of edit parameter
        endEdit ();
    }

    return true;
}

//------------------------------------------------------------------------
long CSlider::onKeyDown (VstKeyCode& keyCode)
{
    switch (keyCode.virt)
    {
    case VKEY_UP :
    case VKEY_RIGHT :
    case VKEY_DOWN :
    case VKEY_LEFT :
        {
            float distance = 1.f;
            if (keyCode.virt == VKEY_DOWN || keyCode.virt == VKEY_LEFT)
                distance = -distance;

            if (keyCode.modifier & MODIFIER_SHIFT)
                value += 0.1f * distance * wheelInc;
            else
                value += distance * wheelInc;
            bounceValue ();

            if (isDirty () && listener)
            {
                // begin of edit parameter
                beginEdit ();

                listener->valueChanged (0, this);

                // end of edit parameter
                endEdit ();
            }
        } return 1;
    }
    return -1;
}

//------------------------------------------------------------------------
void CSlider::setHandle (CBitmap *_pHandle)
{
    if (pHandle)
        pHandle->forget ();
    pHandle = _pHandle;
    if (pHandle)
    {
        pHandle->remember ();
        widthOfSlider  = pHandle->getWidth ();
        heightOfSlider = pHandle->getHeight ();
    }
}


//------------------------------------------------------------------------
// CVerticalSlider
//------------------------------------------------------------------------
/*! @class CVerticalSlider
This is the vertical slider. See CSlider.
*/
CVerticalSlider::CVerticalSlider (const CRect &rect, CControlListener *listener, long tag,
                                  long      iMinPos, // min position in pixel
                                  long      iMaxPos, // max position in pixel
                                  CBitmap  *handle,  // bitmap of slider
                                  CBitmap  *background, // bitmap of background
                                  CPoint   &offset,  // offset in the background
                                  const long style)  // style (kLeft, kRight)
  :    CSlider (rect, listener, tag, iMinPos, iMaxPos, handle, background, offset, style|kVertical)
{}

//------------------------------------------------------------------------
CVerticalSlider::CVerticalSlider (const CRect &rect, CControlListener *listener, long tag,
                          CPoint   &offsetHandle,    // handle offset
                          long     rangeHandle, // size of handle range
                          CBitmap  *handle,     // bitmap of slider
                          CBitmap  *background, // bitmap of background
                          CPoint   &offset,     // offset in the background
                          const long style)     // style (kLeft, kRight)
:    CSlider (rect, listener, tag, offsetHandle, rangeHandle, handle, background, offset, style|kVertical)
{}


//------------------------------------------------------------------------
// CHorizontalSlider
//------------------------------------------------------------------------
/*! @class CHorizontalSlider
This is the horizontal slider. See CSlider.
*/
CHorizontalSlider::CHorizontalSlider (const CRect &rect, CControlListener *listener, long tag,
                                  long      iMinPos, // min Y position in pixel
                                  long      iMaxPos, // max Y position in pixel
                                  CBitmap  *handle,  // bitmap of slider
                                  CBitmap  *background, // bitmap of background
                                  CPoint   &offset,  // offset in the background
                                  const long style)  // style (kLeft, kRight)
  :    CSlider (rect, listener, tag, iMinPos, iMaxPos, handle, background, offset, style|kHorizontal)
{}

//------------------------------------------------------------------------
CHorizontalSlider::CHorizontalSlider (const CRect &rect, CControlListener *listener, long tag,
                          CPoint   &offsetHandle,    // handle offset
                          long     rangeHandle, // size of handle range
                          CBitmap  *handle,     // bitmap of slider
                          CBitmap  *background, // bitmap of background
                          CPoint   &offset,     // offset in the background
                          const long style)     // style (kLeft, kRight)
:    CSlider (rect, listener, tag, offsetHandle, rangeHandle, handle, background, offset, style|kHorizontal)
{}


//------------------------------------------------------------------------
// CSpecialDigit
//------------------------------------------------------------------------
/*! @class CSpecialDigit
Can be used to display a counter with maximum 7 digits.
All digit have the same size and are stacked in height in the pixmap.
*/
CSpecialDigit::CSpecialDigit (const CRect &size,
                              CControlListener *listener,
                              long      tag,        // tag identifier
                              long     dwPos,      // actual value
                              long      iNumbers,   // amount of numbers (max 7)
                              long      *xpos,      // array of all XPOS
                              long      *ypos,      // array of all YPOS
                              long      width,      // width of ONE number
                              long      height,     // height of ONE number
                              CBitmap  *background)    // bitmap numbers
  :    CControl (size, listener, tag, background),
        iNumbers (iNumbers), width (width), height (height)
{
    setValue ((float)dwPos);          // actual value

    if (iNumbers > 7)
        iNumbers = 7;

    if (xpos == NULL)
    {
        // automatically init xpos/ypos if not provided by caller
        const int numw = (const int)background->getWidth();
        int x = (int)size.left;
        for (long i = 0; i < iNumbers; i++)
        {
            this->xpos[i] = x;
            this->ypos[i] = (long)size.top;
            x += numw;
        }
    }
    else
    {
        // store coordinates of x/y pos of each digit
        for (long i = 0; i < iNumbers; i++)
        {
            this->xpos[i] = xpos[i];
            this->ypos[i] = ypos[i];
        }
    }

    setMax ((float)pow (10., (double)iNumbers) - 1.0f);
    setMin (0.0f);
}

//------------------------------------------------------------------------
CSpecialDigit::~CSpecialDigit ()
{}

//------------------------------------------------------------------------
void CSpecialDigit::draw (CDrawContext *pContext)
{
//    DBG ("CSpecialDigit::draw");

    CPoint  where;
    CRect   rectDest;
    long    i, j;
    long    dwValue;
    long     one_digit[16];

    if ((long)value >= getMax ())
        dwValue = (long)getMax ();
    else if ((long)value < getMin ())
        dwValue = (long)getMin ();
    else
        dwValue = (long)value;

    for (i = 0, j = ((long)getMax () + 1) / 10; i < iNumbers; i++, j /= 10)
    {
        one_digit[i] = dwValue / j;
        dwValue -= (one_digit[i] * j);
    }

    where.h = 0;
    for (i = 0; i < iNumbers; i++)
    {
        j = one_digit[i];
        if (j > 9)
            j = 9;

        rectDest.left   = xpos[i];
        rectDest.top    = ypos[i];

        rectDest.right  = rectDest.left + width;
        rectDest.bottom = rectDest.top  + height;

        // where = src from bitmap
        where.v = j * height;
        if (pBackground)
        {
            if (bTransparencyEnabled)
                pBackground->drawTransparent (pContext, rectDest, where);
            else
                pBackground->draw (pContext, rectDest, where);
        }
    }

    setDirty (false);
}

//------------------------------------------------------------------------
float CSpecialDigit::getNormValue () const
{
    float fTemp;
    fTemp = value / getMax ();
    if (fTemp > 1.0f)
        fTemp = 1.0f;
    else if (fTemp < 0.0f)
        fTemp = 0.0f;

    return fTemp;
}


//------------------------------------------------------------------------
// CKickButton
//------------------------------------------------------------------------
/*! @class CKickButton
Define a button with 2 states using 2 subpixmaps.
One click on it, then the second subpixmap is displayed.
When the mouse button is relaxed, the first subpixmap is framed.
*/
CKickButton::CKickButton (const CRect &size, CControlListener *listener, long tag,
                          CBitmap *background, CPoint &offset)
:    CControl (size, listener, tag, background), offset (offset)
{
    heightOfOneImage = size.height ();
}

//------------------------------------------------------------------------
CKickButton::CKickButton (const CRect &size, CControlListener *listener, long tag,
                          CCoord heightOfOneImage, // height of one image in pixel
                          CBitmap *background, CPoint &offset)
:    CControl (size, listener, tag, background), offset (offset),
    heightOfOneImage (heightOfOneImage)
{}

//------------------------------------------------------------------------
CKickButton::~CKickButton ()
{}

//------------------------------------------------------------------------
void CKickButton::draw (CDrawContext *pContext)
{
 //   DBG ("CKickButton::draw");

    CPoint where (offset.h, offset.v);

    bounceValue ();

    if (value)
        where.v += heightOfOneImage;

    if (pBackground)
    {
        if (bTransparencyEnabled)
            pBackground->drawTransparent (pContext, size, where);
        else
            pBackground->draw (pContext, size, where);
    }
    setDirty (false);
}

//------------------------------------------------------------------------
void CKickButton::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (!(button & kLButton))
        return;

    // this simulates a real windows button
    float fEntryState = value;

    // begin of edit parameter
    beginEdit ();

    if (pContext->getMouseButtons () == kLButton)
    {
        do
        {
            if (where.h >= size.left && where.v >= size.top  &&
                where.h <= size.right && where.v <= size.bottom)
                value = !fEntryState;
            else
                value = fEntryState;

            if (isDirty () && listener)
                listener->valueChanged (pContext, this);

            getMouseLocation (pContext, where);

            doIdleStuff ();
        }
        while (pContext->getMouseButtons () == kLButton);
    }
    else
    {
        value = !value;
        if (listener)
            listener->valueChanged (pContext, this);
    }

    value = 0.0f;  // set button to UNSELECTED state
    if (listener)
        listener->valueChanged (pContext, this);

    // end of edit parameter
    endEdit ();
}


//------------------------------------------------------------------------
// CSplashScreen
//------------------------------------------------------------------------
/*! @class CSplashScreen
One click on its activated region and its pixmap is displayed, in this state the other control can not be used,
an another click on the displayed area reinstalls the normal frame.
This can be used to display a help view over the other views.
*/
// one click draw its pixmap, an another click redraw its parent
CSplashScreen::CSplashScreen (const CRect &size, CControlListener *listener, long tag,
                              CBitmap *background,
                              CRect   &toDisplay,
                              CPoint  &offset)
:    CControl (size, listener, tag, background),
    toDisplay (toDisplay), offset (offset), bitmapTransparency (255)
{}

//------------------------------------------------------------------------
CSplashScreen::~CSplashScreen ()
{}

//------------------------------------------------------------------------
void CSplashScreen::setBitmapTransparency (unsigned char transparency)
{
    bitmapTransparency = transparency;
    setTransparency (bitmapTransparency != 255);
}

//------------------------------------------------------------------------
void CSplashScreen::draw (CDrawContext *pContext)
{
 //   DBG ("CSplashScreen::draw");

    if (value && pBackground)
    {
        if (bTransparencyEnabled)
        {
            if (bitmapTransparency)
                pBackground->drawAlphaBlend (pContext, toDisplay, offset, bitmapTransparency);
            else
            pBackground->drawTransparent (pContext, toDisplay, offset);
        }
        else
            pBackground->draw (pContext, toDisplay, offset);
    }
    setDirty (false);
}

//------------------------------------------------------------------------
bool CSplashScreen::hitTest (const CPoint& where, const long buttons)
{
    bool result = CView::hitTest (where, buttons);
    if (result && !(buttons & kLButton))
        return false;
    return result;
}

//------------------------------------------------------------------------
void CSplashScreen::mouse (CDrawContext *pContext, CPoint &where, long button)
{
    if (!bMouseEnabled)
        return;

    if (button == -1) button = pContext->getMouseButtons ();

    if (listener && button & (kAlt | kShift | kControl | kApple))
    {
        if (listener->controlModifierClicked (pContext, this, button) != 0)
            return;
    }

    if (!(button & kLButton))
        return;

    value = !value;
    if (value)
    {
        if (getFrame () && getFrame ()->setModalView (this))
        {
            keepSize = size;
            size = toDisplay;
            mouseableArea = size;
//            draw (pContext);
            if (listener)
                listener->valueChanged (pContext, this);
        }
        setDirty ();
    }
    else
    {
        size = keepSize;
        mouseableArea = size;
        if (listener)
            listener->valueChanged (pContext, this);
        if (getFrame ())
        {
            getFrame ()->setDirty (true);
            getFrame ()->setModalView (NULL);
        }
    }
}

//------------------------------------------------------------------------
void CSplashScreen::unSplash ()
{
    setDirty ();
    value = 0.f;

    size = keepSize;
    if (getFrame ())
    {
        if (getFrame ()->getModalView () == this)
        {
            getFrame ()->setModalView (NULL);
            getFrame ()->redraw ();
        }
    }
}

//------------------------------------------------------------------------
// CVuMeter
//------------------------------------------------------------------------
CVuMeter::CVuMeter (const CRect &size, CBitmap *onBitmap, CBitmap *offBitmap,
                    long nbLed, const long style)
    : CControl (size, 0, 0),
      onBitmap (onBitmap), offBitmap (offBitmap), pOScreen (0),
      nbLed (nbLed), style (style)
{
    bUseOffscreen = false;

    setDecreaseStepValue (0.1f);

    if (onBitmap)
        onBitmap->remember ();
    if (offBitmap)
        offBitmap->remember ();

    rectOn  (size.left, size.top, size.right, size.bottom);
    rectOff (size.left, size.top, size.right, size.bottom);
}

//------------------------------------------------------------------------
CVuMeter::~CVuMeter ()
{
    if (onBitmap)
        onBitmap->forget ();
    if (offBitmap)
        offBitmap->forget ();
}

//------------------------------------------------------------------------
void CVuMeter::setDirty (const bool val)
{
    CView::setDirty (val);
}

//-----------------------------------------------------------------------------
bool CVuMeter::attached (CView *parent)
{
    if (pOScreen)
        delete pOScreen;
/*
    if (bUseOffscreen)
    {
        pOScreen = new COffscreenContext (getFrame (), (long)size.width (), (long)size.height (), kBlackCColor);
        rectOn  (0, 0, size.width (), size.height ());
        rectOff (0, 0, size.width (), size.height ());
    }
    else
*/
    {
        rectOn  (size.left, size.top, size.right, size.bottom);
        rectOff (size.left, size.top, size.right, size.bottom);
    }

    return CControl::attached (parent);
}

//------------------------------------------------------------------------
void CVuMeter::setUseOffscreen (bool val)
{
//    bUseOffscreen = val; // @TODO - faking offscreen
    bUseOffscreen = false;
}

//-----------------------------------------------------------------------------
bool CVuMeter::removed (CView *parent)
{
    if (pOScreen)
    {
        delete pOScreen;
        pOScreen = 0;
    }
    return CControl::removed (parent);
}

//------------------------------------------------------------------------
void CVuMeter::draw (CDrawContext *_pContext)
{
 //   DBG ("CVuMeter::draw");

    if (!onBitmap)
        return;

    CPoint pointOn;
    CPoint pointOff;
    CDrawContext *pContext = _pContext;

    bounceValue ();

    float newValue = oldValue - decreaseValue;
    if (newValue < value)
        newValue = value;
    oldValue = newValue;

/*
    if (bUseOffscreen)
    {
        if (!pOScreen)
        {
            pOScreen = new COffscreenContext (getFrame (), (long)size.width (), (long)size.height (), kBlackCColor);
            rectOn  (0, 0, size.width (), size.height ());
            rectOff (0, 0, size.width (), size.height ());
        }
        pContext = pOScreen;
    }
*/

    if (style & kHorizontal)
    {
        CCoord tmp = (long)(((long)(nbLed * newValue + 0.5f) / (float)nbLed) * onBitmap->getWidth ());
        pointOff (tmp, 0);
        if (!bUseOffscreen)
        tmp += size.left;

        rectOff.left = tmp;
        rectOn.right = tmp;
    }
    else
    {
        CCoord tmp = (long)(((long)(nbLed * (getMax () - newValue) + 0.5f) / (float)nbLed) * onBitmap->getHeight ());
        pointOn (0, tmp);
        if (!bUseOffscreen)
        tmp += size.top;

        rectOff.bottom = tmp;
        rectOn.top     = tmp;
    }

    if (offBitmap)
    {
        if (bTransparencyEnabled)
            offBitmap->drawTransparent (pContext, rectOff, pointOff);
        else
            offBitmap->draw (pContext, rectOff, pointOff);
    }

    if (bTransparencyEnabled)
        onBitmap->drawTransparent (pContext, rectOn, pointOn);
    else
        onBitmap->draw (pContext, rectOn, pointOn);

    if (pOScreen)
        pOScreen->copyFrom (_pContext, size);

    setDirty (false);
}

} // namespace VSTGUI

//------------------------------------------------------------------------
// END.
//------------------------------------------------------------------------
