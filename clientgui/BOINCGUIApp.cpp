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
// Revision 1.9  2004/09/21 01:26:23  rwalton
// *** empty log message ***
//
// Revision 1.8  2004/05/27 06:17:57  rwalton
// *** empty log message ***
//
// Revision 1.7  2004/05/21 06:27:14  rwalton
// *** empty log message ***
//
// Revision 1.6  2004/05/17 22:15:08  rwalton
// *** empty log message ***
//
//

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "BOINCGUIApp.h"
#endif

#include "stdwx.h"
#include "BOINCGUIApp.h"
#include "MainFrame.h"
#include "MainDocument.h"


IMPLEMENT_APP(CBOINCGUIApp)
IMPLEMENT_DYNAMIC_CLASS(CBOINCGUIApp, wxApp)


bool CBOINCGUIApp::OnInit()
{

    SetVendorName("Space Sciences Laboratory, U.C. Berkeley");
    SetAppName("BOINC Manager");

    // Commandline parsing is done in wxApp::OnInit()
    if (!Inherited::OnInit())
        return false;

    // Enable image types we use for the task panes
    wxImage::AddHandler(new wxXPMHandler);

    // Enable the in memory virtual file system for
    //   storing images
    wxFileSystem::AddHandler(new wxMemoryFSHandler);

    // Initialize the internationalization module
    m_pLocale = new wxLocale();
    wxASSERT(NULL != m_pLocale);

    m_pLocale->Init();
    m_pLocale->AddCatalog(GetAppName());

    // Initialize the configuration storage module
    m_pConfig = new wxConfig(wxTheApp->GetAppName());
    wxConfigBase::Set(m_pConfig);
    wxASSERT(NULL != m_pConfig);

    // Initialize the main document
    m_pDocument = new CMainDocument();
    wxASSERT(NULL != m_pDocument);

    // Initialize the main gui window
    m_pFrame = new CMainFrame(GetAppName());
    wxASSERT(NULL != m_pFrame);

    SetTopWindow(m_pFrame);
    m_pFrame->Show();


    return true;
}


int CBOINCGUIApp::OnExit()
{
    if (m_pDocument)
        delete m_pDocument;

    if (m_pLocale)
        delete m_pLocale;

    return Inherited::OnExit();
}


void CBOINCGUIApp::OnInitCmdLine(wxCmdLineParser &parser)
{
    Inherited::OnInitCmdLine(parser);
    static const wxCmdLineEntryDesc cmdLineDesc[] = {
        { wxCMD_LINE_SWITCH, "e", "example", "example command line option"},
        { wxCMD_LINE_NONE}  //DON'T forget this line!!
    };
    parser.SetDesc(cmdLineDesc);
}


bool CBOINCGUIApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
    // Give default processing (-?, --help and --verbose) the chance to do something.
    Inherited::OnCmdLineParsed(parser);
    if (parser.Found("example")) {

        wxMessageDialog* pDlg = new wxMessageDialog(
            NULL, 
            _("You have specified -e on the commmand line. The application will exit now."),
            _("Example command line option"),
            wxOK | wxICON_INFORMATION,
            wxDefaultPosition
            );

        if (pDlg)
            pDlg->ShowModal();

        return false;
    } else {
        // Log a message, but only if --verbose has been set on the command line.
        wxLogVerbose("Commandline has been parsed. -e was not found.");
    }
    return true;
}

