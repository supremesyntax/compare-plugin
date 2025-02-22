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

#pragma once

#include <cstdint>
#include <vector>
#include <utility>

#include "Compare.h"


enum Marker_t
{
	MARKER_CHANGED_LINE = 0,
	MARKER_ADDED_LINE,
	MARKER_REMOVED_LINE,
	MARKER_MOVED_LINE,
	MARKER_BLANK,
	MARKER_CHANGED_SYMBOL,
	MARKER_CHANGED_LOCAL_SYMBOL,
	MARKER_ADDED_SYMBOL,
	MARKER_ADDED_LOCAL_SYMBOL,
	MARKER_REMOVED_SYMBOL,
	MARKER_REMOVED_LOCAL_SYMBOL,
	MARKER_MOVED_LINE_SYMBOL,
	MARKER_MOVED_BLOCK_BEGIN_SYMBOL,
	MARKER_MOVED_BLOCK_MID_SYMBOL,
	MARKER_MOVED_BLOCK_END_SYMBOL,
	MARKER_ARROW_SYMBOL
};


constexpr int MARKER_MASK_CHANGED		=	(1 << MARKER_CHANGED_LINE)	|	(1 << MARKER_CHANGED_SYMBOL);
constexpr int MARKER_MASK_CHANGED_LOCAL	=	(1 << MARKER_CHANGED_LINE)	|	(1 << MARKER_CHANGED_LOCAL_SYMBOL);
constexpr int MARKER_MASK_ADDED			=	(1 << MARKER_ADDED_LINE)	|	(1 << MARKER_ADDED_SYMBOL);
constexpr int MARKER_MASK_ADDED_LOCAL	=	(1 << MARKER_ADDED_LINE)	|	(1 << MARKER_ADDED_LOCAL_SYMBOL);
constexpr int MARKER_MASK_REMOVED		=	(1 << MARKER_REMOVED_LINE)	|	(1 << MARKER_REMOVED_SYMBOL);
constexpr int MARKER_MASK_REMOVED_LOCAL	=	(1 << MARKER_REMOVED_LINE)	|	(1 << MARKER_REMOVED_LOCAL_SYMBOL);
constexpr int MARKER_MASK_MOVED_LINE	=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_LINE_SYMBOL);
constexpr int MARKER_MASK_MOVED_BEGIN	=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_BLOCK_BEGIN_SYMBOL);
constexpr int MARKER_MASK_MOVED_MID		=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_BLOCK_MID_SYMBOL);
constexpr int MARKER_MASK_MOVED_END		=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_BLOCK_END_SYMBOL);
constexpr int MARKER_MASK_MOVED			=	(1 << MARKER_MOVED_LINE)	|	(1 << MARKER_MOVED_LINE_SYMBOL) |
																			(1 << MARKER_MOVED_BLOCK_BEGIN_SYMBOL) |
																			(1 << MARKER_MOVED_BLOCK_MID_SYMBOL) |
																			(1 << MARKER_MOVED_BLOCK_END_SYMBOL);

constexpr int MARKER_MASK_BLANK			=	(1 << MARKER_BLANK);
constexpr int MARKER_MASK_ARROW			=	(1 << MARKER_ARROW_SYMBOL);

constexpr int MARKER_MASK_LINE			=	(1 << MARKER_CHANGED_LINE) |
											(1 << MARKER_ADDED_LINE) |
											(1 << MARKER_REMOVED_LINE) |
											(1 << MARKER_MOVED_LINE);

constexpr int MARKER_MASK_SYMBOL		=	(1 << MARKER_CHANGED_SYMBOL) |
											(1 << MARKER_CHANGED_LOCAL_SYMBOL) |
											(1 << MARKER_ADDED_SYMBOL) |
											(1 << MARKER_ADDED_LOCAL_SYMBOL) |
											(1 << MARKER_REMOVED_SYMBOL) |
											(1 << MARKER_REMOVED_LOCAL_SYMBOL) |
											(1 << MARKER_MOVED_LINE_SYMBOL) |
											(1 << MARKER_MOVED_BLOCK_BEGIN_SYMBOL) |
											(1 << MARKER_MOVED_BLOCK_MID_SYMBOL) |
											(1 << MARKER_MOVED_BLOCK_END_SYMBOL);

constexpr int MARKER_MASK_ALL			=	MARKER_MASK_LINE | MARKER_MASK_SYMBOL;

constexpr int MARGIN_NUM = 4;


/**
 *  \class
 *  \brief
 */
class NppToolbarHandleGetter
{
public:
	static HWND get();

private:
	static HWND	hNppToolbar;

	static BOOL CALLBACK enumWindowsCB(HWND hwnd, LPARAM lParam);
};


/**
 *  \class
 *  \brief
 */
class NppTabHandleGetter
{
public:
	static HWND get(int viewId);

private:
	static HWND	hNppTab[2];

	static BOOL CALLBACK enumWindowsCB(HWND hwnd, LPARAM lParam);
};


/**
 *  \class
 *  \brief
 */
class NppStatusBarHandleGetter
{
public:
	static HWND get();

private:
	static HWND	hNppStatusBar;

	static BOOL CALLBACK enumWindowsCB(HWND hwnd, LPARAM lParam);
};


/**
 *  \struct
 *  \brief
 *  \warning  Don't use that helper struct if somewhere in its scope the view document is changed!!!
 */
struct ScopedViewWriteEnabler
{
	ScopedViewWriteEnabler(int view) : _view(view)
	{
		_isRO = static_cast<bool>(CallScintilla(_view, SCI_GETREADONLY, 0, 0));

		if (_isRO)
			CallScintilla(_view, SCI_SETREADONLY, false, 0);
	}

	~ScopedViewWriteEnabler()
	{
		if (_isRO)
			CallScintilla(_view, SCI_SETREADONLY, true, 0);
	}

private:
	int		_view;
	bool	_isRO;
};


/**
 *  \struct
 *  \brief
 *  \warning  Don't use that helper struct if somewhere in its scope the view document is changed!!!
 */
struct ScopedViewUndoCollectionBlocker
{
	ScopedViewUndoCollectionBlocker(int view) : _view(view)
	{
		_isUndoOn = static_cast<bool>(CallScintilla(_view, SCI_GETUNDOCOLLECTION, 0, 0));

		if (_isUndoOn)
		{
			CallScintilla(_view, SCI_SETUNDOCOLLECTION, false, 0);
			CallScintilla(_view, SCI_EMPTYUNDOBUFFER, 0, 0);
		}
	}

	~ScopedViewUndoCollectionBlocker()
	{
		if (_isUndoOn)
			CallScintilla(_view, SCI_SETUNDOCOLLECTION, true, 0);
	}

private:
	int		_view;
	bool	_isUndoOn;
};


/**
 *  \struct
 *  \brief
 *  \warning  Don't use that helper struct if somewhere in its scope the view document is changed!!!
 */
struct ScopedViewUndoAction
{
	ScopedViewUndoAction(int view) : _view(view)
	{
		CallScintilla(_view, SCI_BEGINUNDOACTION, 0, 0);
	}

	~ScopedViewUndoAction()
	{
		CallScintilla(_view, SCI_ENDUNDOACTION, 0, 0);
	}

private:
	int	_view;
};


/**
 *  \struct
 *  \brief
 */
struct ScopedFirstVisibleLineStore
{
	ScopedFirstVisibleLineStore(int view) : _view(view)
	{
		_firstVisibleLine = CallScintilla(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
	}

	~ScopedFirstVisibleLineStore()
	{
		if (_firstVisibleLine >= 0)
			CallScintilla(_view, SCI_SETFIRSTVISIBLELINE, _firstVisibleLine, 0);
	}

	void set(intptr_t newFirstVisible)
	{
		_firstVisibleLine = newFirstVisible;
	}

private:
	int			_view;
	intptr_t	_firstVisibleLine;
};


/**
 *  \struct
 *  \brief
 */
struct ViewLocation
{
	ViewLocation() : _view(-1) {}

	ViewLocation(int view, intptr_t centerLine)
	{
		save(view, centerLine);
	}

	ViewLocation(int view)
	{
		save(view);
	}

#ifdef DLOG

	~ViewLocation()
	{
		LOGD(LOG_SYNC, "Clear view location\n");
	}

#endif

	void save(int view, intptr_t centerLine = -1);
	bool restore() const;

	inline int getView() const
	{
		return _view;
	}

private:
	int			_view;
	intptr_t	_centerLine;
	intptr_t	_firstLine;
	intptr_t	_visibleLineOffset;
};


inline bool isRTLwindow(HWND hWin)
{
	return ((::GetWindowLongPtr(hWin, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0);
}


inline bool isSingleView()
{
	return (!::IsWindowVisible(nppData._scintillaSecondHandle) || !::IsWindowVisible(nppData._scintillaMainHandle));
}


inline bool isFileEmpty(int view)
{
	return (CallScintilla(view, SCI_GETLENGTH, 0, 0) == 0);
}


inline int getNotepadVersion()
{
	return (int)::SendMessage(nppData._nppHandle, NPPM_GETNPPVERSION, 1, 0);
}


inline bool getWrapMode()
{
	HMENU hMenu = (HMENU)::SendMessage(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	return (::GetMenuState(hMenu, IDM_VIEW_WRAP, MF_BYCOMMAND) & MF_CHECKED) != 0;
}


inline int getNumberOfFiles()
{
	return static_cast<int>((::IsWindowVisible(nppData._scintillaMainHandle) ?
				::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, PRIMARY_VIEW) : 0) +
			(::IsWindowVisible(nppData._scintillaSecondHandle) ?
				::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, SECOND_VIEW) : 0));
}


inline int getNumberOfFiles(int viewId)
{
	return static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0,
			viewId == MAIN_VIEW ? PRIMARY_VIEW : SECOND_VIEW));
}


inline HWND getView(int viewId)
{
	return (viewId == MAIN_VIEW) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}


inline int getViewId(HWND view)
{
	return (view == nppData._scintillaMainHandle) ? MAIN_VIEW : SUB_VIEW;
}


inline int getViewIdSafe(HWND view)
{
	return (view == nppData._scintillaMainHandle) ? MAIN_VIEW :
			(view == nppData._scintillaSecondHandle) ? SUB_VIEW : -1;
}


inline int getCurrentViewId()
{
	return static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0));
}


inline HWND getCurrentView()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ?
			nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}


inline int getOtherViewId()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW;
}


inline HWND getOtherView()
{
	return (::SendMessage(nppData._nppHandle, NPPM_GETCURRENTVIEW, 0, 0) == MAIN_VIEW) ?
			nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


inline int getOtherViewId(int view)
{
	return (view == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW;
}


inline HWND getOtherView(int view)
{
	return (view == MAIN_VIEW) ? nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


inline HWND getOtherView(HWND view)
{
	return (view == nppData._scintillaMainHandle) ? nppData._scintillaSecondHandle : nppData._scintillaMainHandle;
}


inline int viewIdFromBuffId(LRESULT buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	return static_cast<int>(index >> 30);
}


inline int posFromBuffId(LRESULT buffId)
{
	LRESULT index = ::SendMessage(nppData._nppHandle, NPPM_GETPOSFROMBUFFERID, buffId, 0);
	return static_cast<int>(index & 0x3FFFFFFF);
}


inline LRESULT getCurrentBuffId()
{
	return ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);
}


inline int getEncoding(LRESULT buffId)
{
	return static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETBUFFERENCODING, buffId, 0));
}


inline intptr_t getDocId(int view)
{
	return CallScintilla(view, SCI_GETDOCPOINTER, 0, 0);
}


inline intptr_t getLineStart(int view, intptr_t line)
{
	return CallScintilla(view, SCI_POSITIONFROMLINE, line, 0);
}


inline intptr_t getLineEnd(int view, intptr_t line)
{
	return CallScintilla(view, SCI_GETLINEENDPOSITION, line, 0);
}


inline intptr_t getCurrentLine(int view)
{
	return CallScintilla(view, SCI_LINEFROMPOSITION, CallScintilla(view, SCI_GETCURRENTPOS, 0, 0), 0);
}


inline intptr_t getCurrentVisibleLine(int view)
{
	return CallScintilla(view, SCI_VISIBLEFROMDOCLINE, getCurrentLine(view), 0);
}


inline intptr_t getFirstVisibleLine(int view)
{
	return CallScintilla(view, SCI_GETFIRSTVISIBLELINE, 0, 0);
}


inline intptr_t getFirstLine(int view)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, getFirstVisibleLine(view), 0);
}


inline intptr_t getLastVisibleLine(int view)
{
	return (getFirstVisibleLine(view) + CallScintilla(view, SCI_LINESONSCREEN, 0, 0) - 1);
}


inline intptr_t getLastLine(int view)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, getLastVisibleLine(view), 0);
}


inline intptr_t getUnhiddenLine(int view, intptr_t line)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0), 0);
}


inline intptr_t getPreviousUnhiddenLine(int view, intptr_t line)
{
	return CallScintilla(view, SCI_DOCLINEFROMVISIBLE, CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - 1, 0);
}


inline void gotoClosestUnhiddenLine(int view)
{
	CallScintilla(view, SCI_GOTOLINE, getUnhiddenLine(view, getCurrentLine(view)), 0);
}


inline void gotoClosestUnhiddenLine(int view, intptr_t line)
{
	CallScintilla(view, SCI_GOTOLINE, getUnhiddenLine(view, line), 0);
}


inline intptr_t getWrapCount(int view, intptr_t line)
{
	return CallScintilla(view, SCI_WRAPCOUNT, line, 0);
}


inline intptr_t getLineAnnotation(int view, intptr_t line)
{
	return CallScintilla(view, SCI_ANNOTATIONGETLINES, line, 0);
}


inline intptr_t getFirstVisibleLineOffset(int view, intptr_t line)
{
	return (CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0) - getFirstVisibleLine(view));
}


inline bool isLineVisible(int view, intptr_t line)
{
	line = CallScintilla(view, SCI_VISIBLEFROMDOCLINE, line, 0);

	return ((line >= getFirstVisibleLine(view)) && (line <= getLastVisibleLine(view)));
}


inline bool isLineWrapped(int view, intptr_t line)
{
	return (CallScintilla(view, SCI_WRAPCOUNT, line, 0) > 1);
}


inline bool isLineAnnotated(int view, intptr_t line)
{
	return (getLineAnnotation(view, line) > 0);
}


inline bool isLineMarked(int view, intptr_t line, int markMask)
{
	return ((CallScintilla(view, SCI_MARKERGET, line, 0) & markMask) != 0);
}


inline bool isLineEmpty(int view, intptr_t line)
{
	return ((getLineEnd(view, line) - getLineStart(view, line)) == 0);
}


inline bool isSelection(int view)
{
	return (CallScintilla(view, SCI_GETSELECTIONEND, 0, 0) - CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0) != 0);
}


inline bool isSelectionVertical(int view)
{
	return (CallScintilla(view, SCI_SELECTIONISRECTANGLE, 0, 0) != 0);
}


inline bool isMultiSelection(int view)
{
	return (CallScintilla(view, SCI_GETSELECTIONS, 0, 0) > 1);
}


inline std::pair<intptr_t, intptr_t> getSelection(int view)
{
	return std::make_pair(CallScintilla(view, SCI_GETSELECTIONSTART, 0, 0),
			CallScintilla(view, SCI_GETSELECTIONEND, 0, 0));
}


inline void clearSelection(int view)
{
	const intptr_t currentPos = CallScintilla(view, SCI_GETCURRENTPOS, 0, 0);
	CallScintilla(view, SCI_SETEMPTYSELECTION, currentPos, 0);
}


inline void setSelection(int view, intptr_t start, intptr_t end, bool scrollView = false)
{
	if (scrollView)
	{
		CallScintilla(view, SCI_SETSEL, start, end);
	}
	else
	{
		CallScintilla(view, SCI_SETSELECTIONSTART, start, 0);
		CallScintilla(view, SCI_SETSELECTIONEND, end, 0);
	}
}


intptr_t otherViewMatchingLine(int view, intptr_t line, intptr_t adjustment = 0, bool check = false);
void activateBufferID(LRESULT buffId);
std::pair<intptr_t, intptr_t> getSelectionLines(int view);

int showArrowSymbol(int view, intptr_t line, bool down);

void blinkLine(int view, intptr_t line);
void blinkRange(int view, intptr_t startPos, intptr_t endPos);

void centerAt(int view, intptr_t line);

void markTextAsChanged(int view, intptr_t start, intptr_t length, int color);
void clearChangedIndicator(int view, intptr_t start, intptr_t length);

void setNormalView(int view);
void setCompareView(int view, int blankColor, int caretLineTransp);


inline bool isDarkModeNPP()
{
	return (bool)::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0);
}


bool isCurrentFileSaved();

bool isDarkMode();

void setStyles(UserSettings& settings);

void clearWindow(int view);
void clearMarks(int view, intptr_t line);
void clearMarks(int view, intptr_t startLine, intptr_t length);
intptr_t getPrevUnmarkedLine(int view, intptr_t startLine, int markMask);
intptr_t getNextUnmarkedLine(int view, intptr_t startLine, int markMask);

std::pair<intptr_t, intptr_t> getMarkedSection(int view, intptr_t startLine, intptr_t endLine, int markMask,
		bool excludeNewLine = false);
std::vector<int> getMarkers(int view, intptr_t startLine, intptr_t length, int markMask, bool clearMarkers = true);
void setMarkers(int view, intptr_t startLine, const std::vector<int> &markers);

void showRange(int view, intptr_t line, intptr_t length);
void hideOutsideRange(int view, intptr_t startLine, intptr_t endLine);
void hideUnmarked(int view, int markMask);

bool isAdjacentAnnotation(int view, intptr_t line, bool down);
bool isAdjacentAnnotationVisible(int view, intptr_t line, bool down);


inline void clearAnnotation(int view, intptr_t line)
{
	CallScintilla(view, SCI_ANNOTATIONSETTEXT, line, (LPARAM)NULL);
}


void clearAnnotations(int view, intptr_t startLine, intptr_t length);

std::vector<char> getText(int view, intptr_t startPos, intptr_t endPos);
void toLowerCase(std::vector<char>& text);

void addBlankSection(int view, intptr_t line, intptr_t length, intptr_t selectionMarkPosition = 0,
		const char *text = nullptr);
void addBlankSectionAfter(int view, intptr_t line, intptr_t length);
