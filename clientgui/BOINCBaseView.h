// $Id$
//
// The contents of this file are subject to the BOINC Public License
// Version 1.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://boinc.berkeley.edu/license_1.0.txt
// 
// Software distributed under the License is distributed on an "AS IS"
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
// License for the specific language governing rights and limitations
// under the License. 
// 
// The Original Code is the Berkeley Open Infrastructure for Network Computing. 
// 
// The Initial Developer of the Original Code is the SETI@home project.
// Portions created by the SETI@home project are Copyright (C) 2002
// University of California at Berkeley. All Rights Reserved. 
// 
// Contributor(s):
//
// Revision History:
//
// $Log$
// Revision 1.5  2004/09/25 21:33:22  rwalton
// *** empty log message ***
//
// Revision 1.4  2004/09/24 22:18:53  rwalton
// *** empty log message ***
//
// Revision 1.3  2004/09/24 02:01:45  rwalton
// *** empty log message ***
//
// Revision 1.2  2004/09/23 08:28:50  rwalton
// *** empty log message ***
//
// Revision 1.1  2004/09/21 01:26:24  rwalton
// *** empty log message ***
//
//

#ifndef _BOINCBASEVIEW_H_
#define _BOINCBASEVIEW_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "BOINCBaseView.cpp"
#endif


class CBOINCTaskCtrl;
class CBOINCListCtrl;

class CBOINCBaseView : public wxPanel
{
    DECLARE_DYNAMIC_CLASS( CBOINCBaseView )

public:
    CBOINCBaseView();
    CBOINCBaseView(wxNotebook* pNotebook, wxWindowID iHtmlWindowID, wxWindowID iListWindowID);

    ~CBOINCBaseView();

    virtual wxString        GetViewName();
    virtual char**          GetViewIcon();

    virtual void            OnTaskRender( wxTimerEvent& event );
    virtual void            OnListRender( wxTimerEvent& event );
    virtual bool            OnSaveState( wxConfigBase* pConfig );
    virtual bool            OnRestoreState( wxConfigBase* pConfig );

    virtual void            OnListCacheHint( wxListEvent& event );
    virtual void            OnListSelected( wxListEvent& event );
    virtual void            OnListDeselected( wxListEvent& event );
    virtual wxString        OnListGetItemText( long item, long column ) const;
    virtual int             OnListGetItemImage( long item ) const;
    virtual wxListItemAttr* OnListGetItemAttr( long item ) const;

    virtual void            OnTaskLinkClicked( const wxHtmlLinkInfo& link );
    virtual void            OnTaskCellMouseHover( wxHtmlCell* cell, wxCoord x, wxCoord y );

    wxString                GetCurrentQuickTip();
    wxString                GetCurrentQuickTipText();
    void                    SetCurrentQuickTip( const wxString& strQuickTip, const wxString& strQuickTipText );

    virtual void            UpdateSelection();
    virtual void            UpdateTaskPane();

    bool                    m_bProcessingTaskRenderEvent;
    bool                    m_bProcessingListRenderEvent;
    wxInt32                 m_iCacheFrom;
    wxInt32                 m_iCacheTo;

    wxString                m_strQuickTip;
    wxString                m_strQuickTipText;

    CBOINCTaskCtrl*         m_pTaskPane;
    CBOINCListCtrl*         m_pListPane;

};


#endif

