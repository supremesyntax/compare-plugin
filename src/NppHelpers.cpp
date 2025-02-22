/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2022 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include <cstdlib>
#include <vector>
#include <algorithm>

#include "NppHelpers.h"
#include "NppInternalDefines.h"

#include "icon_added.h"
#include "icon_removed.h"
#include "icon_changed.h"
#include "icon_moved.h"
#include "icon_arrows.h"


// Don't use "INDIC_CONTAINER + 1" since it conflicts with DSpellCheck plugin
#define INDIC_HIGHLIGHT		INDIC_CONTAINER + 7


HWND NppToolbarHandleGetter::hNppToolbar = NULL;


HWND NppToolbarHandleGetter::get()
{
	if (hNppToolbar == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, 0);

	return hNppToolbar;
}


BOOL CALLBACK NppToolbarHandleGetter::enumWindowsCB(HWND hwnd, LPARAM )
{
	TCHAR winClassName[64];

	::GetClassName(hwnd, winClassName, _countof(winClassName));

	if (!_tcscmp(winClassName, TOOLBARCLASSNAME))
	{
		hNppToolbar = hwnd;
		return FALSE;
	}

	return TRUE;
}


HWND NppTabHandleGetter::hNppTab[2] = { NULL, NULL };


HWND NppTabHandleGetter::get(int viewId)
{
	const int idx = (viewId == MAIN_VIEW) ? 0 : 1;

	if (hNppTab[idx] == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, idx);

	return hNppTab[idx];
}


BOOL CALLBACK NppTabHandleGetter::enumWindowsCB(HWND hwnd, LPARAM lParam)
{
	TCHAR winClassName[64];

	::GetClassName(hwnd, winClassName, _countof(winClassName));

	if (!_tcscmp(winClassName, WC_TABCONTROL))
	{
		RECT tabRect;
		RECT viewRect;

		::GetWindowRect(hwnd, &tabRect);
		::GetWindowRect(getView(static_cast<int>(lParam)), &viewRect);

		if ((tabRect.left <= viewRect.left) && (tabRect.top <= viewRect.top) &&
			(tabRect.right >= viewRect.right) && (tabRect.bottom >= viewRect.bottom))
		{
			hNppTab[lParam] = hwnd;
			return FALSE;
		}
	}

	return TRUE;
}


HWND NppStatusBarHandleGetter::hNppStatusBar = NULL;


HWND NppStatusBarHandleGetter::get()
{
	if (hNppStatusBar == NULL)
		::EnumChildWindows(nppData._nppHandle, enumWindowsCB, 0);

	return hNppStatusBar;
}


BOOL CALLBACK NppStatusBarHandleGetter::enumWindowsCB(HWND hwnd, LPARAM )
{
	TCHAR winClassName[64];

	::GetClassName(hwnd, winClassName, _countof(winClassName));

	if (!_tcscmp(winClassName, STATUSCLASSNAME))
	{
		hNppStatusBar = hwnd;
		return FALSE;
	}

	return TRUE;
}


void ViewLocation::save(int view, intptr_t centerLine)
{
	if (view != MAIN_VIEW && view != SUB_VIEW)
	{
		_view = -1;
		return;
	}

	_view		= view;
	_centerLine	= centerLine;
	_firstLine	= -1;

	if (_centerLine < 0)
	{
		_firstLine			= getFirstLine(view);
		_visibleLineOffset	= getFirstVisibleLineOffset(view, _firstLine);
	}

	LOGD(LOG_SYNC, "Store " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") + " view location\n");
}


bool ViewLocation::restore() const
{
	if (_view < 0)
		return false;

	if (_centerLine < 0)
	{
		const intptr_t firstVisibleLine =
				CallScintilla(_view, SCI_VISIBLEFROMDOCLINE, _firstLine, 0) - _visibleLineOffset;

		CallScintilla(_view, SCI_SETFIRSTVISIBLELINE, firstVisibleLine, 0);

		LOGD(LOG_SYNC, "Restore " + std::string(_view == MAIN_VIEW ? "MAIN" : "SUB") +
				" view location, first visible doc line: " +
				std::to_string(CallScintilla(_view, SCI_DOCLINEFROMVISIBLE, firstVisibleLine, 0) + 1) + "\n");
	}
	else
	{
		if (!isLineVisible(_view, _centerLine))
			centerAt(_view, _centerLine);

		LOGD(LOG_SYNC, "Restore " + std::string(_view == MAIN_VIEW ? "MAIN" : "SUB") +
				" view location, center doc line: " + std::to_string(_centerLine + 1) + "\n");
	}

	return true;
}


namespace // anonymous namespace
{

const int cBlinkCount		= 3;
const int cBlinkInterval_ms	= 100;

bool compareMode[2]		= { false, false };
int blankStyle[2]		= { 0, 0 };
bool endAtLastLine[2]	= { true, true };


void defineColor(int type, int color)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINE,	type, SC_MARK_BACKGROUND);
	CallScintilla(MAIN_VIEW,	SCI_MARKERSETBACK,	type, color);

	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINE,	type, SC_MARK_BACKGROUND);
	CallScintilla(SUB_VIEW,		SCI_MARKERSETBACK,	type, color);
}


void defineRgbaSymbol(int type, const unsigned char* rgba)
{
	CallScintilla(MAIN_VIEW,	SCI_MARKERDEFINERGBAIMAGE,	type, (LPARAM)rgba);
	CallScintilla(SUB_VIEW,		SCI_MARKERDEFINERGBAIMAGE,	type, (LPARAM)rgba);
}


void setTextStyle(int transparency)
{
	static const int cMinAlpha = 0;
	static const int cMaxAlpha = 100;

	const int alpha = ((100 - transparency) * (cMaxAlpha - cMinAlpha) / 100) + cMinAlpha;

	CallScintilla(MAIN_VIEW, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT,	INDIC_ROUNDBOX);
	CallScintilla(MAIN_VIEW, SCI_INDICSETFLAGS,	INDIC_HIGHLIGHT,	SC_INDICFLAG_VALUEFORE);
	CallScintilla(MAIN_VIEW, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT,	alpha);

	CallScintilla(SUB_VIEW, SCI_INDICSETSTYLE,	INDIC_HIGHLIGHT,	INDIC_ROUNDBOX);
	CallScintilla(SUB_VIEW, SCI_INDICSETFLAGS,	INDIC_HIGHLIGHT,	SC_INDICFLAG_VALUEFORE);
	CallScintilla(SUB_VIEW, SCI_INDICSETALPHA,	INDIC_HIGHLIGHT,	alpha);
}


void setBlanksStyle(int view, int blankColor)
{
	if (blankStyle[view] == 0)
		blankStyle[view] = static_cast<int>(CallScintilla(view, SCI_ALLOCATEEXTENDEDSTYLES, 1, 0));

	CallScintilla(view, SCI_ANNOTATIONSETSTYLEOFFSET,	blankStyle[view], 0);
	CallScintilla(view, SCI_STYLESETEOLFILLED,			blankStyle[view], 1);
	CallScintilla(view, SCI_STYLESETBACK,				blankStyle[view], blankColor);
	CallScintilla(view, SCI_STYLESETBOLD,				blankStyle[view], true);
	CallScintilla(view, SCI_ANNOTATIONSETVISIBLE,		ANNOTATION_STANDARD, 0);
}

} // anonymous namespace


intptr_t otherViewMatchingLine(int view, intptr_t line, intptr_t adjustment, bool check)
{
	const int		otherView		= getOtherViewId(view);
	const intptr_t	otherLineCount	= CallScintilla(otherView, SCI_GETLINECOUNT, 0, 0);

	const intptr_t otherLine = CallScintilla(otherView, SCI_DOCLINEFROMVISIBLE,
			CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) + adjustment, 0);

	if (check && (otherLine < otherLineCount) && (otherViewMatchingLine(otherView, otherLine, -adjustment) != line))
		return -1;

	return (otherLine >= otherLineCount) ? otherLineCount - 1 : otherLine;
}


void activateBufferID(LRESULT buffId)
{
	if (buffId != getCurrentBuffId())
	{
		LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
		::SendMessage(nppData._nppHandle, NPPM_ACTIVATEDOC, index >> 30, index & 0x3FFFFFFF);
	}
}


std::pair<intptr_t, intptr_t> getSelectionLines(int view)
{
	if (isSelectionVertical(view) || isMultiSelection(view))
		return std::make_pair(-1, -1);

	const intptr_t selectionStart = CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0);
	const intptr_t selectionEnd = CallScintilla(view, SCI_GETSELECTIONEND, 0, 0);

	if (selectionEnd - selectionStart == 0)
		return std::make_pair(-1, -1);

	intptr_t startLine	= CallScintilla(view, SCI_LINEFROMPOSITION, selectionStart, 0);
	intptr_t endLine	= CallScintilla(view, SCI_LINEFROMPOSITION, selectionEnd, 0);

	if (selectionEnd == getLineStart(view, endLine))
		--endLine;

	return std::make_pair(startLine, endLine);
}


int showArrowSymbol(int view, intptr_t line, bool down)
{
	const bool isRTL = isRTLwindow(getView(view));
	const unsigned char* rgba = down ?
			(isRTL ? icon_arrow_down_rtl : icon_arrow_down) : (isRTL ? icon_arrow_up_rtl : icon_arrow_up);

	CallScintilla(view,	SCI_MARKERDEFINERGBAIMAGE,	MARKER_ARROW_SYMBOL, (LPARAM)rgba);

	return static_cast<int>(CallScintilla(view, SCI_MARKERADD, line, MARKER_ARROW_SYMBOL));
}


void blinkLine(int view, intptr_t line)
{
	const int marker = CallScintilla(view, SCI_MARKERGET, line, 0) & MARKER_MASK_ALL;
	HWND hView = getView(view);

	for (int i = cBlinkCount; ;)
	{
		if (marker)
			clearMarks(view, line);
		else
			CallScintilla(view, SCI_MARKERADDSET, line, MARKER_MASK_BLANK);

		::UpdateWindow(hView);
		::Sleep(cBlinkInterval_ms);

		if (marker)
			CallScintilla(view, SCI_MARKERADDSET, line, marker);
		else
			CallScintilla(view, SCI_MARKERDELETE, line, MARKER_BLANK);

		::UpdateWindow(hView);

		if (--i == 0)
			break;

		::Sleep(cBlinkInterval_ms);
	}
}


void blinkRange(int view, intptr_t startPos, intptr_t endPos)
{
	ViewLocation loc(view);
	const std::pair<intptr_t, intptr_t> sel = getSelection(view);

	for (int i = cBlinkCount; ;)
	{
		setSelection(view, startPos, endPos, true);
		::UpdateWindow(getView(view));
		::Sleep(cBlinkInterval_ms);

		if (--i == 0)
			break;

		setSelection(view, startPos, startPos);
		::UpdateWindow(getView(view));
		::Sleep(cBlinkInterval_ms);
	}

	setSelection(view, sel.first, sel.second);
	loc.restore();
}


void centerAt(int view, intptr_t line)
{
	const intptr_t linesOnScreen = CallScintilla(view, SCI_LINESONSCREEN, 0, 0);
	const intptr_t firstVisible = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - linesOnScreen / 2;

	CallScintilla(view, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
}


void setNormalView(int view)
{
	if (compareMode[view])
	{
		compareMode[view] = false;

		CallScintilla(view, SCI_SETENDATLASTLINE, endAtLastLine[view], 0);

		CallScintilla(view, SCI_SETMARGINMASKN, MARGIN_NUM, 0);
		CallScintilla(view, SCI_SETMARGINWIDTHN, MARGIN_NUM, 0);
		CallScintilla(view, SCI_SETMARGINSENSITIVEN, MARGIN_NUM, false);

		const intptr_t caretLineColor = CallScintilla(view, SCI_GETELEMENTCOLOUR,  SC_ELEMENT_CARET_LINE_BACK, 0);

		if (caretLineColor)
			CallScintilla(view, SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK, caretLineColor & 0xFFFFFF);

		CallScintilla(view, SCI_SETCARETLINELAYER, SC_LAYER_BASE, 0);
	}
}


void setCompareView(int view, int blankColor, int caretLineTransp)
{
	if (!compareMode[view])
	{
		compareMode[view] = true;

		endAtLastLine[view] = (CallScintilla(view, SCI_GETENDATLASTLINE, 0, 0) != 0);
		CallScintilla(view, SCI_SETENDATLASTLINE, false, 0);

		CallScintilla(view, SCI_SETMARGINMASKN, MARGIN_NUM, (LPARAM)(MARKER_MASK_SYMBOL | MARKER_MASK_ARROW));
		CallScintilla(view, SCI_SETMARGINWIDTHN, MARGIN_NUM, 16);
		CallScintilla(view, SCI_SETMARGINSENSITIVEN, MARGIN_NUM, true);
	}

	const intptr_t caretLineColor = CallScintilla(view, SCI_GETELEMENTCOLOUR,  SC_ELEMENT_CARET_LINE_BACK, 0);

	if (caretLineColor)
	{
		const intptr_t alpha = ((100 - caretLineTransp) * SC_ALPHA_OPAQUE / 100);

		CallScintilla(view, SCI_SETELEMENTCOLOUR, SC_ELEMENT_CARET_LINE_BACK,
				(caretLineColor & 0xFFFFFF) | (alpha << 24));
		CallScintilla(view, SCI_SETCARETLINELAYER, SC_LAYER_UNDER_TEXT, 0);
	}

	// For some reason the annotation blank styling is lost on Sci doc switch thus we need to reapply it
	setBlanksStyle(view, blankColor);
}


bool isCurrentFileSaved()
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	MENUITEMINFOA menuItemInfo	= { 0 };
	menuItemInfo.cbSize			= sizeof(menuItemInfo);
	menuItemInfo.fMask			= MIIM_STATE;

	::GetMenuItemInfoA(hMenu, IDM_FILE_SAVE, FALSE, &menuItemInfo);

	return (menuItemInfo.fState & MFS_DISABLED);
}


bool isDarkMode()
{
	if (::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0))
		return true;

	const int bg = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0));

	const int r = bg & 0xFF;
	const int g = bg >> 8 & 0xFF;
	const int b = bg >> 16 & 0xFF;

	return (((r + g + b) / 3) < 128);
}


void setStyles(UserSettings& settings)
{
	const int bg = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETEDITORDEFAULTBACKGROUNDCOLOR, 0, 0));

	settings.colors()._default = bg;

	int r = bg & 0xFF;
	int g = bg >> 8 & 0xFF;
	int b = bg >> 16 & 0xFF;

	static const int colorShift = 20;

	r = (r > colorShift) ? (r - colorShift) & 0xFF : 0;
	g = (g > colorShift) ? (g - colorShift) & 0xFF : 0;
	b = (b > colorShift) ? (b - colorShift) & 0xFF : 0;

	settings.colors().blank = r | (g << 8) | (b << 16);

	defineColor(MARKER_ADDED_LINE,		settings.colors().added);
	defineColor(MARKER_REMOVED_LINE,	settings.colors().removed);
	defineColor(MARKER_MOVED_LINE,		settings.colors().moved);
	defineColor(MARKER_CHANGED_LINE,	settings.colors().changed);
	defineColor(MARKER_BLANK,			settings.colors().blank);

	defineRgbaSymbol(MARKER_CHANGED_SYMBOL,				icon_changed);
	defineRgbaSymbol(MARKER_CHANGED_LOCAL_SYMBOL,		icon_changed_local);
	defineRgbaSymbol(MARKER_ADDED_SYMBOL,				icon_added);
	defineRgbaSymbol(MARKER_ADDED_LOCAL_SYMBOL,			icon_added_local);
	defineRgbaSymbol(MARKER_REMOVED_SYMBOL,				icon_removed);
	defineRgbaSymbol(MARKER_REMOVED_LOCAL_SYMBOL,		icon_removed_local);
	defineRgbaSymbol(MARKER_MOVED_LINE_SYMBOL,			icon_moved_line);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_BEGIN_SYMBOL,	icon_moved_block_start);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_MID_SYMBOL,		icon_moved_block_middle);
	defineRgbaSymbol(MARKER_MOVED_BLOCK_END_SYMBOL,		icon_moved_block_end);

	setTextStyle(settings.colors().highlight_transparency);

	setBlanksStyle(MAIN_VIEW,	settings.colors().blank);
	setBlanksStyle(SUB_VIEW,	settings.colors().blank);
}


void markTextAsChanged(int view, intptr_t start, intptr_t length, int color)
{
	if (length > 0)
	{
		const int curIndic = static_cast<int>(CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0));
		CallScintilla(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		CallScintilla(view, SCI_SETINDICATORVALUE, color | SC_INDICVALUEBIT, 0);
		CallScintilla(view, SCI_INDICATORFILLRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


void clearChangedIndicator(int view, intptr_t start, intptr_t length)
{
	if (length > 0)
	{
		const int curIndic = static_cast<int>(CallScintilla(view, SCI_GETINDICATORCURRENT, 0, 0));
		CallScintilla(view, SCI_SETINDICATORCURRENT, INDIC_HIGHLIGHT, 0);
		CallScintilla(view, SCI_INDICATORCLEARRANGE, start, length);
		CallScintilla(view, SCI_SETINDICATORCURRENT, curIndic, 0);
	}
}


std::vector<char> getText(int view, intptr_t startPos, intptr_t endPos)
{
	const intptr_t len = endPos - startPos;

	if (len <= 0)
		return std::vector<char>(1, 0);

	std::vector<char> text(len + 1, 0);

	Sci_TextRange tr;
	tr.chrg.cpMin = startPos;
	tr.chrg.cpMax = endPos;
	tr.lpstrText = text.data();

	CallScintilla(view, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

	return text;
}


void toLowerCase(std::vector<char>& text)
{
	const int len = static_cast<int>(text.size());

	if (len == 0)
		return;

	const int wLen = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), len, NULL, 0);

	std::vector<wchar_t> wText(wLen);

	::MultiByteToWideChar(CP_UTF8, 0, text.data(), len, wText.data(), wLen);

	wText.push_back(L'\0');
	::CharLowerW((LPWSTR)wText.data());
	wText.pop_back();

	::WideCharToMultiByte(CP_UTF8, 0, wText.data(), wLen, text.data(), len, NULL, NULL);
}


void clearWindow(int view)
{
	CallScintilla(view, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);
	CallScintilla(view, SCI_ANNOTATIONCLEARALL, 0, 0);

	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_CHANGED_LOCAL_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ADDED_LOCAL_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_REMOVED_LOCAL_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_LINE_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_BEGIN_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_MID_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_MOVED_BLOCK_END_SYMBOL, 0);
	CallScintilla(view, SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);

	clearChangedIndicator(view, 0, CallScintilla(view, SCI_GETLENGTH, 0, 0));

	CallScintilla(view, SCI_COLOURISE, 0, -1);
}


void clearMarks(int view, intptr_t line)
{
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_CHANGED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_ADDED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_REMOVED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_LINE);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_BLANK);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_CHANGED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_CHANGED_LOCAL_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_ADDED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_ADDED_LOCAL_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_REMOVED_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_REMOVED_LOCAL_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_LINE_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_BLOCK_BEGIN_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_BLOCK_MID_SYMBOL);
	CallScintilla(view, SCI_MARKERDELETE, line, MARKER_MOVED_BLOCK_END_SYMBOL);
}


void clearMarks(int view, intptr_t startLine, intptr_t length)
{
	intptr_t endLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0);

	if (startLine + length < endLine)
		endLine = startLine + length;

	const intptr_t startPos = getLineStart(view, startLine);

	clearChangedIndicator(view, startPos, getLineEnd(view, endLine - 1) - startPos);

	for (; startLine < endLine; ++startLine)
		clearMarks(view, startLine);
}


intptr_t getPrevUnmarkedLine(int view, intptr_t startLine, int markMask)
{
	intptr_t prevUnmarkedLine = startLine;

	for (; (prevUnmarkedLine >= 0) && isLineMarked(view, prevUnmarkedLine, markMask); --prevUnmarkedLine);

	return prevUnmarkedLine;
}


intptr_t getNextUnmarkedLine(int view, intptr_t startLine, int markMask)
{
	const intptr_t endLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1;
	intptr_t nextUnmarkedLine = startLine;

	for (; (nextUnmarkedLine <= endLine) && isLineMarked(view, nextUnmarkedLine, markMask); ++nextUnmarkedLine);

	return ((nextUnmarkedLine <= endLine) ? nextUnmarkedLine : -1);
}


std::pair<intptr_t, intptr_t> getMarkedSection(int view, intptr_t startLine, intptr_t endLine, int markMask,
	bool excludeNewLine)
{
	const intptr_t lastLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0) - 1;

	if ((startLine < 0) || (endLine > lastLine) || (startLine > endLine) || !isLineMarked(view, startLine, markMask))
		return std::make_pair(-1, -1);

	if ((startLine != endLine) && (!isLineMarked(view, endLine, markMask)))
		return std::make_pair(-1, -1);

	const intptr_t line1	= getPrevUnmarkedLine(view, startLine, markMask) + 1;
	intptr_t line2			= getNextUnmarkedLine(view, endLine, markMask);

	if (excludeNewLine)
		--line2;

	const intptr_t endPos = (line2 < 0) ? getLineEnd(view, lastLine) :
			(excludeNewLine ? getLineEnd(view, line2) : getLineStart(view, line2));

	return std::make_pair(getLineStart(view, line1), endPos);
}


std::vector<int> getMarkers(int view, intptr_t startLine, intptr_t length, int markMask, bool clearMarkers)
{
	std::vector<int> markers;

	if (length <= 0 || startLine < 0)
		return markers;

	const intptr_t linesCount = CallScintilla(view, SCI_GETLINECOUNT, 0, 0);

	if (startLine + length > linesCount)
		length = linesCount - startLine;

	if (clearMarkers)
	{
		const intptr_t startPos = getLineStart(view, startLine);
		clearChangedIndicator(view, startPos, getLineEnd(view, startLine + length - 1) - startPos);
	}

	markers.resize(length, 0);

	for (intptr_t line = CallScintilla(view, SCI_MARKERPREVIOUS, startLine + length - 1, markMask); line >= startLine;
		line = CallScintilla(view, SCI_MARKERPREVIOUS, line - 1, markMask))
	{
		markers[line - startLine] = CallScintilla(view, SCI_MARKERGET, line, 0) & markMask;

		if (clearMarkers)
			clearMarks(view, line);
	}

	return markers;
}


void setMarkers(int view, intptr_t startLine, const std::vector<int> &markers)
{
	const intptr_t linesCount = static_cast<intptr_t>(markers.size());

	if (startLine < 0 || linesCount == 0)
		return;

	const intptr_t startPos = getLineStart(view, startLine);
	clearChangedIndicator(view, startPos, getLineEnd(view, startLine + linesCount - 1) - startPos);

	for (intptr_t i = 0; i < linesCount; ++i)
	{
		clearMarks(view, startLine + i);

		if (markers[i])
			CallScintilla(view, SCI_MARKERADDSET, startLine + i, markers[i]);
	}
}


void showRange(int view, intptr_t line, intptr_t length)
{
	if (line >= 0 && length > 0)
	{
		const intptr_t linesCount = CallScintilla(view, SCI_GETLINECOUNT, 0, 0);

		if (line + length > linesCount)
			length = linesCount - line;

		CallScintilla(view, SCI_SHOWLINES, line, line + length - 1);
	}
}


void hideOutsideRange(int view, intptr_t startLine, intptr_t endLine)
{
	const intptr_t linesCount = CallScintilla(view, SCI_GETLINECOUNT, 0, 0);

	// First line (0) cannot be hidden so start from line 1
	if (startLine > 1)
		CallScintilla(view, SCI_HIDELINES, 1, startLine - 1);

	if (endLine > 0 && endLine + 1 < linesCount)
		CallScintilla(view, SCI_HIDELINES, endLine + 1, linesCount - 1);

	if (startLine >= 0 && (endLine > startLine && endLine < linesCount))
		CallScintilla(view, SCI_SHOWLINES, startLine, endLine);
}


void hideUnmarked(int view, int markMask)
{
	const intptr_t linesCount = CallScintilla(view, SCI_GETLINECOUNT, 0, 0);

	// First line (0) cannot be hidden so start from line 1
	for (intptr_t nextMarkedLine, nextUnmarkedLine = 1; nextUnmarkedLine < linesCount;
		nextUnmarkedLine = nextMarkedLine)
	{
		for (; (nextUnmarkedLine < linesCount) && isLineMarked(view, nextUnmarkedLine, markMask); ++nextUnmarkedLine);

		if (nextUnmarkedLine == linesCount)
			break;

		nextMarkedLine = CallScintilla(view, SCI_MARKERNEXT, nextUnmarkedLine, markMask);

		if (nextMarkedLine < 0)
			nextMarkedLine = linesCount;

		CallScintilla(view, SCI_HIDELINES, nextUnmarkedLine, nextMarkedLine - 1);
	}
}


bool isAdjacentAnnotation(int view, intptr_t line, bool down)
{
	if (down)
	{
		if (isLineAnnotated(view, line))
			return true;
	}
	else
	{
		if (line && isLineAnnotated(view, line - 1))
			return true;
	}

	return false;
}


bool isAdjacentAnnotationVisible(int view, intptr_t line, bool down)
{
	if (down)
	{
		if (!isLineAnnotated(view, line))
			return false;

		if (CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) + getWrapCount(view, line) >
				getLastVisibleLine(view))
			return false;
	}
	else
	{
		if (!line || !isLineAnnotated(view, line - 1))
			return false;

		if (CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - 1 < getFirstVisibleLine(view))
			return false;
	}

	return true;
}


void clearAnnotations(int view, intptr_t startLine, intptr_t length)
{
	intptr_t endLine = CallScintilla(view, SCI_GETLINECOUNT, 0, 0);

	if (startLine + length < endLine)
		endLine = startLine + length;

	for (; startLine < endLine; ++startLine)
		clearAnnotation(view, startLine);
}


void addBlankSection(int view, intptr_t line, intptr_t length, intptr_t textLinePos, const char *text)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');

	if (textLinePos > 0 && text != nullptr)
	{
		if (length < textLinePos)
			return;

		blank.insert(blank.begin() + textLinePos - 1, text, text + strlen(text));
	}

	blank.push_back('\0');

	CallScintilla(view, SCI_ANNOTATIONSETTEXT, getPreviousUnhiddenLine(view, line), (LPARAM)blank.data());
}


void addBlankSectionAfter(int view, intptr_t line, intptr_t length)
{
	if (length <= 0)
		return;

	std::vector<char> blank(length - 1, '\n');
	blank.push_back('\0');

	CallScintilla(view, SCI_ANNOTATIONSETTEXT, getUnhiddenLine(view, line), (LPARAM)blank.data());
}
