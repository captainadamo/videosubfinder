						    //VideoSubFinder.h//                                
//////////////////////////////////////////////////////////////////////////////////
//																				//
// Author:  Simeon Kosnitsky													//
//          skosnits@gmail.com													//
//																				//
// License:																		//
//     This software is released into the public domain.  You are free to use	//
//     it in any way you like, except that you may not sell this source code.	//
//																				//
//     This software is provided "as is" with no expressed or implied warranty.	//
//     I accept no liability for any damage or loss of business that this		//
//     software may cause.														//
//																				//
//////////////////////////////////////////////////////////////////////////////////

#include "VideoSubFinderWXW.h"		

bool CVideoSubFinderApp::OnInit() 
{
	m_pMainWnd = new CMainFrame("VideoSubFinder " VSF_VERSION " Version");
	
	m_pMainWnd->Init();	

	m_pMainWnd->Show(true);

	return true;
}

CVideoSubFinderApp::~CVideoSubFinderApp()
{
}

