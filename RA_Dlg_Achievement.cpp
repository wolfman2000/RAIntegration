#include "RA_Dlg_Achievement.h"

#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <assert.h>
#include <shellapi.h>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"

#include "RA_Achievement.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_GameTitle.h"
#include "RA_httpthread.h"
#include "RA_md5factory.h"
#include "RA_User.h"


namespace
{
	const char* g_sColTitles[] = { "ID", "Title", "Author", "Achieved?", "Modified?" };
	const int g_nColSizes[] = { 35, 220, 80, 65, 65 };

	const char* g_sColTitlesUnofficial[] = { "ID", "Title", "Author", "Active", "Votes" };
	const char* g_sColTitlesUser[] = { "ID", "Title", "Author", "Active", "Votes" };

}

Dlg_Achievements g_AchievementsDialog;

int iSelect = -1;

Dlg_Achievements::Dlg_Achievements()
{
	m_hAchievementsDlg = NULL;
}

void Dlg_Achievements::SetupColumns( HWND hList )
{
	BOOL bDeleted = TRUE;
	while( bDeleted )
		bDeleted = ListView_DeleteColumn( hList, 0 );

	//	Remove all data.
	ListView_DeleteAllItems( hList );

	char buffer[256];

	LV_COLUMN col;
	ZeroMemory( &col, sizeof( col ) );

	const char* sColTitles[m_nNumCols];

	for( size_t i = 0; i < m_nNumCols; ++i )
	{	
		if( g_nActiveAchievementSet == AT_CORE )
			sColTitles[i] = g_sColTitles[i];
		else if( g_nActiveAchievementSet == AT_UNOFFICIAL )
			sColTitles[i] = g_sColTitlesUnofficial[i];
		else if( g_nActiveAchievementSet == AT_USER )
			sColTitles[i] = g_sColTitlesUser[i];
	}

	//const char*& sColTitles = (*psColTitles);

	for( size_t i = 0; i < m_nNumCols; ++i )
	{
		col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
		col.cx = g_nColSizes[i];
		col.cchTextMax = 255;
		sprintf_s( buffer, 256, sColTitles[i] );
		col.pszText = buffer;
		col.iSubItem = i;

		col.fmt = LVCFMT_LEFT|LVCFMT_FIXED_WIDTH;
		if( i == m_nNumCols-1 )
			col.fmt |= LVCFMT_FILL; 

		ListView_InsertColumn( hList, i, (LPARAM)&col );
	}

	m_lbxData.clear();

	//ListView_SetExtendedListViewStyle( hList, LVS_EX_CHECKBOXES|LVS_EX_FULLROWSELECT );
	ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT );
}


LRESULT ProcessCustomDraw (LPARAM lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

	switch(lplvcd->nmcd.dwDrawStage) 
	{
	case CDDS_PREPAINT : //Before the paint cycle begins
		//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: //Before an item is drawn
		{
			int nNextItem = (int)lplvcd->nmcd.dwItemSpec;
			//if (((int)lplvcd->nmcd.dwItemSpec%2)==0)
			BOOL bSelected = &g_pActiveAchievements->GetAchievement( nNextItem ) == g_AchievementEditorDialog.ActiveAchievement();
			BOOL bModified = g_pActiveAchievements->GetAchievement( nNextItem ).Modified();

			lplvcd->clrText   = bModified ? RGB(255,0,0) : RGB(0,0,0);
			lplvcd->clrTextBk = bSelected ? RGB(222,222,222) : RGB(255,255,255);

			return CDRF_NEWFONT;
		}
		break;

		//Before a subitem is drawn
// 	case CDDS_SUBITEM | CDDS_ITEMPREPAINT: 
// 		{
// 			if (iSelect == (int)lplvcd->nmcd.dwItemSpec)
// 			{
// 				if (0 == lplvcd->iSubItem)
// 				{
// 					//customize subitem appearance for column 0
// 					lplvcd->clrText   = RGB(255,128,0);
// 					lplvcd->clrTextBk = RGB(255,255,255);
// 
// 					//To set a custom font:
// 					//SelectObject(lplvcd->nmcd.hdc, 
// 					//    <your custom HFONT>);
// 
// 					return CDRF_NEWFONT;
// 				}
// 				else if (1 == lplvcd->iSubItem)
// 				{
// 					//customize subitem appearance for columns 1..n
// 					//Note: setting for column i 
// 					//carries over to columnn i+1 unless
// 					//      it is explicitly reset
// 					lplvcd->clrTextBk = RGB(255,0,0);
// 					lplvcd->clrTextBk = RGB(255,255,255);
// 
// 					return CDRF_NEWFONT;
// 				}
// 			}
// 		}
	}
	return CDRF_DODEFAULT;
}


void Dlg_Achievements::RemoveAchievement( HWND hList, size_t nIter )
{
	assert( nIter < ListView_GetItemCount( hList ) );
	ListView_DeleteItem( hList, nIter );
	m_lbxData.erase( m_lbxData.begin() + nIter );
}

size_t Dlg_Achievements::AddAchievement( HWND hList, const Achievement& Ach )
{
	AchievementDlgRow newRow;
	newRow.reserve( NUM_COLS );

	//	Add to our local array:

	newRow[ ID ] = std::to_string( Ach.ID() );
	newRow[ Title ] = Ach.Title();
	newRow[ Author ] = Ach.Author();

	if( g_nActiveAchievementSet == AT_CORE )
	{
		newRow[ Achieved ] = !Ach.Active() ? "Yes" : "No";
		newRow[ Modified ] = Ach.Modified() ? "Yes" : "No";
	}
	else
	{
		newRow[ Active ] = Ach.Active() ? "Yes" : "No";
		newRow[ Votes ] = "N/A";
	}

	m_lbxData.push_back( newRow );
	
	LV_ITEM item;
	ZeroMemory( &item, sizeof( item ) );

	item.mask = LVIF_TEXT;
	item.cchTextMax = 256;
	item.iItem = static_cast<int>( m_lbxData.size() );
	item.pszText = const_cast<LPSTR>( m_lbxData.back()[ID].data() );

	for( item.iSubItem = 0; item.iSubItem < NUM_COLS; ++item.iSubItem )
	{
		item.pszText = const_cast<LPSTR>( m_lbxData.back()[ item.iSubItem ].data() );

		if( item.iSubItem == 0 )
			item.iItem = ListView_InsertItem( hList, &item );
		else
			ListView_SetItem( hList, &item );
	}

	assert( item.iItem == m_lbxData.size() );
	return static_cast<size_t>( item.iItem );
}

BOOL LocalValidateAchievementsBeforeCommit( int nLbxItems[1] )
{
	char buffer[2048];
	for( size_t i = 0; i < 1; ++i )
	{
		int nIter = nLbxItems[i];
		const Achievement& Ach = g_pActiveAchievements->GetAchievement( nIter );
		if( Ach.Title().length() < 2 )
		{
			sprintf_s( buffer, 2048, "Achievement title too short:\n%s\nMust be greater than 2 characters.", Ach.Title() );
			MessageBox( NULL, buffer, "Error!", MB_OK );
			return FALSE;
		}
		if( Ach.Title().length() > 63 )
		{
			sprintf_s( buffer, 2048, "Achievement title too long:\n%s\nMust be fewer than 80 characters.", Ach.Title() );
			MessageBox( NULL, buffer, "Error!", MB_OK );
			return FALSE;
		}

		if( Ach.Description().length() < 2 )
		{
			sprintf_s( buffer, 2048, "Achievement description too short:\n%s\nMust be greater than 2 characters.", Ach.Description() );
			MessageBox( NULL, buffer, "Error!", MB_OK );
			return FALSE;
		}
		if( Ach.Description().length() > 255 )
		{
			sprintf_s( buffer, 2048, "Achievement description too long:\n%s\nMust be fewer than 255 characters.", Ach.Description() );
			MessageBox( NULL, buffer, "Error!", MB_OK );
			return FALSE;
		}

		char sIllegalChars[] = { '&', ':' };

		const size_t nNumIllegalChars = sizeof(sIllegalChars) / sizeof(sIllegalChars[0]);
		for( size_t i = 0; i < nNumIllegalChars; ++i )
		{
			char cNextChar = sIllegalChars[i];

			if( strchr( Ach.Title().c_str(), cNextChar ) != NULL )
			{
				sprintf_s( buffer, 2048, "Achievement title contains an illegal character: '%c'\nPlease remove and try again", cNextChar );
				MessageBox( NULL, buffer, "Error!", MB_OK );
				return FALSE;
			}
			if( strchr( Ach.Description().c_str(), cNextChar ) != NULL )
			{
				sprintf_s( buffer, 2048, "Achievement description contains an illegal character: '%c'\nPlease remove and try again", cNextChar );
				MessageBox( NULL, buffer, "Error!", MB_OK );
				return FALSE;
			}
		}
	}

	return TRUE;
}

//BOOL ValidateGameTitleOrUpload()
//{
//	//if( Game == NULL ) //TBD
//	//	return FALSE;
//
//	if( strlen( g_pActiveAchievements->GameTitle() ) < 2 )
//	{
//		if( Dlg_GameTitle::DoModalDialog( g_AchievementsDialog.GetHWND(),  ) )
//		{
//			return TRUE;
//		}
//		else
//		{
//			return FALSE;	//	User cancelled - do not progress?
//		}
//	}
//	else
//	{
//		//	We have a title, no panic :)
//		return TRUE;
//	}
//}

void urlEncode( char stringsToEncode[][1024], const int numStrings )
{
	for( int i = 0; i < numStrings; ++i )
	{
		char* pString = stringsToEncode[i];
		char sArrayReplace[1024];
		strcpy_s( sArrayReplace, 1024, pString );

		char* pDestIter = &pString[0];

		int nNumCharsRead = 0;
		int nDestOffset = 0;
		char cNextChar = sArrayReplace[nNumCharsRead];
		while( cNextChar != '\0' )
		{
			if( cNextChar == '&' )
			{
				strcat_s( pDestIter, 1024, "&amp;" );
				nDestOffset += 5;
			}
			else
			{
				pDestIter[nDestOffset] = cNextChar;
				nDestOffset++;
				pDestIter++;
			}

			cNextChar = sArrayReplace[++nNumCharsRead];
		}
	}
}
							
//static
INT_PTR CALLBACK Dlg_Achievements::s_AchievementsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//	TBD: intercept any msgs?
	return g_AchievementsDialog.AchievementsProc( hDlg, uMsg, wParam, lParam );
}

BOOL AttemptUploadAchievementBlocking( const Achievement& Ach, unsigned int nFlags, Document& doc )
{	
	const std::string sMem = Ach.CreateMemString();

	//	Deal with secret:
	char sPostCode[2048];
	sprintf_s( sPostCode, "%sSECRET%dSEC%s%dRE2%d",
		RAUsers::LocalUser.Username().c_str(),
		Ach.ID(),
		sMem,
		Ach.Points(),
		Ach.Points()*3 );

	char sTestHash[33];						
	md5_GenerateMD5( sPostCode, strlen( sPostCode ), sTestHash );

	PostArgs args;
	args['u'] = RAUsers::LocalUser.Username();
	args['p'] = RAUsers::LocalUser.Token();
	args['i'] = std::to_string( Ach.ID() );
	args['g'] = std::to_string( g_pActiveAchievements->GetGameID() );
	args['t'] = Ach.Title();
	args['d'] = Ach.Description();
	args['m'] = sMem;
	args['z'] = std::to_string( Ach.Points() );
	args['f'] = std::to_string( nFlags );
	args['b'] = Ach.BadgeImageURI();
	args['h'] = sTestHash;

	return( RAWeb::DoBlockingRequest( RequestSubmitAchievementData, args, doc ) );
}

void Dlg_Achievements::OnClickAchievementSet( AchievementSetType nAchievementSet )
{
	SetAchievementCollection( nAchievementSet );

	if( nAchievementSet == AT_CORE )
	{
		BOOL bAllowCoreAchievementEdit = FALSE;

		//	TBD: User authority
		//if( RAUsers::LocalUser.GetAuthority() == 5 )
		//	bAllowCoreAchievementEdit = TRUE;

		OnLoad_NewRom( g_pActiveAchievements->GetGameID() );
		g_AchievementEditorDialog.OnLoad_NewRom();

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_ADD_ACH ), bAllowCoreAchievementEdit );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_CLONE_ACH ), TRUE );	//	Clone to user
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DEL_ACH ), bAllowCoreAchievementEdit );

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_RESET_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_REVERTSELECTED ), TRUE );

		ShowWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), FALSE );
		SetWindowText( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), "Demote From Core" );

		SetWindowText( GetDlgItem( m_hAchievementsDlg, IDC_RA_RESET_ACH ), "Reset Achieved Status" );

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_VOTE_POS ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_VOTE_NEG ), FALSE );

		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, TRUE );
		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, FALSE );
		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, FALSE );
	}
	else if( nAchievementSet == AT_UNOFFICIAL )
	{
		OnLoad_NewRom( g_pActiveAchievements->GetGameID() );
		g_AchievementEditorDialog.OnLoad_NewRom();

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_ADD_ACH ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_CLONE_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DEL_ACH ), FALSE );

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_RESET_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_REVERTSELECTED ), TRUE );

		ShowWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), TRUE );
		SetWindowText( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), "Promote To Core" );

		SetWindowText( GetDlgItem( m_hAchievementsDlg, IDC_RA_RESET_ACH ), "Activate" );

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_VOTE_POS ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_VOTE_NEG ), TRUE );

		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, FALSE );
		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, TRUE );
		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, FALSE );
	}
	else if( nAchievementSet == AT_USER )
	{
		OnLoad_NewRom( g_pActiveAchievements->GetGameID() );
		g_AchievementEditorDialog.OnLoad_NewRom();

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_ADD_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_CLONE_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DEL_ACH ), TRUE );

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_RESET_ACH ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_REVERTSELECTED ), TRUE );

		ShowWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), TRUE );
		SetWindowText( GetDlgItem( m_hAchievementsDlg, IDC_RA_SAVELOCAL ), "Save Local" );

		SetWindowText( GetDlgItem( m_hAchievementsDlg, IDC_RA_RESET_ACH ), "Activate" );

		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_VOTE_POS ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_VOTE_NEG ), FALSE );

		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_CORE, FALSE );
		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_UNOFFICIAL, FALSE );
		CheckDlgButton( m_hAchievementsDlg, IDC_RA_ACTIVE_LOCAL, TRUE );
	}

}

INT_PTR Dlg_Achievements::AchievementsProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	BOOL bHandled = FALSE;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			RECT r;
			GetWindowRect( g_RAMainWnd, &r );
			SetWindowPos(hDlg, NULL, r.left, r.bottom, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			m_hAchievementsDlg = hDlg;

			SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, (WPARAM)0, (LONG)0 );
			SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, (WPARAM)0, (LONG)0 );
			SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, (WPARAM)0, (LONG)0 );

			switch( g_nActiveAchievementSet )
			{
			case AT_CORE:
				SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_CORE, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				break;
			case AT_UNOFFICIAL:
				SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_UNOFFICIAL, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				break;
			case AT_USER:
				SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_LOCAL, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				break;
			default:
				assert(0);
				//SendDlgItemMessage( hDlg, IDC_ACTIVE_NONE, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				break;
			}

			//	Continue as if a new rom had been loaded
			OnLoad_NewRom( g_pActiveAchievements->GetGameID() );

			CheckDlgButton( hDlg, IDC_RA_CHKACHPROCESSINGACTIVE, g_pActiveAchievements->ProcessingActive() );
			//HWND hList = GetDlgItem( g_AchievementsDialog.m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS );
			//g_AchievementsDialog.LoadAchievements( hList );

			//	Click the core 
			OnClickAchievementSet( AT_CORE );
			//SendDlgItemMessage( hDlg, IDC_RA_ACTIVE_CORE, WM_COMMAND, MAKELONG(IDC_RA_ACTIVE_CORE, 0), 0 );
			//AchievementsProc( hDlg, 
		}

		bHandled = TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_RA_ACTIVE_CORE:
			{
				OnClickAchievementSet( AT_CORE );
				bHandled = TRUE;
			}
			break;
		case IDC_RA_ACTIVE_UNOFFICIAL:
			{
				OnClickAchievementSet( AT_UNOFFICIAL );
				bHandled = TRUE;
			}
			break;
		case IDC_RA_ACTIVE_LOCAL:
			{
				OnClickAchievementSet( AT_USER );
				bHandled = TRUE;
			}
			break;
// 		case IDAPPLY:
// 			//TBD: deal with whatever 'OK' is supposed to do(?)
// 			bHandled = TRUE;
// 			break;
		case IDCLOSE:
			EndDialog( hDlg, TRUE );
			bHandled = TRUE;
			break;
// 		case ID_SELECT_ALL:
// 			{
// 				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
// 				for( size_t i = 0; i < m_nNumOccupiedRows; ++i )
// 					ListView_SetCheckState( hList, i, TRUE );
// 			}
// 			break;
// 		case ID_SELECT_NONE:
// 			{
// 				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
// 				for( size_t i = 0; i < m_nNumOccupiedRows; ++i )
// 					ListView_SetCheckState( hList, i, FALSE );
// 			}
// 			break;
		case IDC_RA_SAVELOCAL:
			{
				//	Replace with background upload?

				if( !RA_GameIsActive() )
				{
					MessageBox( hDlg, "ROM not loaded: please load a ROM first!", "Error!", MB_OK );
				}
				else
				{
					if( g_nActiveAchievementSet == AT_USER )
					{
						//if( g_pActiveAchievements->Save() )
						if( FALSE ) //NIMPL
						{
							MessageBox( hDlg, "Saved OK!", "OK", MB_OK );
						}
						else
						{
							MessageBox( hDlg, "Error during save!", "Error", MB_OK|MB_ICONWARNING );
						}
					}
					else if( g_nActiveAchievementSet == AT_UNOFFICIAL )
					{
						HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
						int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
						if( nSel == -1 )
							return FALSE;

						//	Promote to Core

						//	Note: specify that this is a one-way operation
						if( MessageBox( hDlg, 
							"Promote this achievement to the Core Achievement set.\n\n"
							"Please note this is a one-way operation, and will allow players\n"
							"to officially obtain this achievement and the points for it.\n"
							"Note: all players who have achieved it while it has been unofficial\n"
							"will have to earn this again now it is in the core group.\n",
							"Are you sure?", MB_YESNO|MB_ICONWARNING ) == IDYES )
						{
							const Achievement& selectedAch = g_pActiveAchievements->GetAchievement( nSel );
										
							unsigned int nFlags = 1<<0;	//	Active achievements! : 1
							if( g_nActiveAchievementSet == AT_UNOFFICIAL )
								nFlags |= 1<<1;			//	Official achievements: 3

							char buffer[1024];

							Document response;
							if( AttemptUploadAchievementBlocking( selectedAch, nFlags, response ) )
							{
								if( response["Success"].GetBool() )
								{
									const unsigned int nID = response["AchievementID"].GetUint();

									//	Remove the achievement from the local/user achievement set,
									//	 add it to the unofficial set.
									Achievement& newAch = CoreAchievements->AddAchievement();
									newAch.Set( selectedAch );
									UnofficialAchievements->RemoveAchievement( nSel );
									RemoveAchievement( hList, nSel );

									newAch.SetActive( TRUE );	//	Disable it: all promoted ach must be reachieved

									//CoreAchievements->Save();
									//UnofficialAchievements->Save();

									MessageBox( hDlg, "Successfully uploaded achievement!", "Success!", MB_OK );
								}
								else
								{
									sprintf_s( buffer, 1024, "Error in upload: response from server:%s\n", response["Error"].GetString() );
									MessageBox( hDlg, buffer, "Error in upload!", MB_OK );
								}
							}
							else
							{
								sprintf_s( buffer, 1024, "Error connecting to server... are you online?" );
								MessageBox( hDlg, buffer, "Error in upload!", MB_OK );
							}
						}

					}
// 					else if( g_nActiveAchievementSet == AT_CORE )
// 					{
// 						//	Demote from Core
// 
// 					}
				}
				
				//HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				//for( size_t i = 0; i < g_AchievementsDialog.m_nNumOccupiedRows; ++i )
				//	ListView_SetCheckState( hList, i, FALSE );
			}
			break;
		case IDC_RA_DOWNLOAD_ACH:
			{
				if( !RA_GameIsActive() )
					break;

				if( MessageBox( hDlg, 
					"Download fresh achievements from " RA_HOST "\n"
					"Are you sure? This will overwrite any changes you have made\n"
					"with fresh achievements from the server.\n",
					"Are you sure?",
					MB_YESNO|MB_ICONWARNING ) == IDYES )
				{
					const GameID nGameID = g_pActiveAchievements->GetGameID();
					if( nGameID != 0 )
					{
						AchievementSet::DeletePatchFile( g_nActiveAchievementSet, nGameID );

						//	Reload the achievements file (will fetch from server fresh)
						AchievementSet::LoadFromFile( nGameID );

						//	Refresh dialog contents:
						OnLoad_NewRom( nGameID );

						//	Cause it to reload!
						OnClickAchievementSet( g_nActiveAchievementSet );
					}

				}
			}
			break;
		case IDC_RA_ADD_ACH:
			{
				if( !RA_GameIsActive() )
				{
					MessageBox( hDlg, "ROM not loaded: please load a ROM first!", "Error!", MB_OK );
					break;
				}

				//	Add a new achievement with default params
				Achievement& Cheevo = g_pActiveAchievements->AddAchievement();
				Cheevo.SetAuthor( RAUsers::LocalUser.Username() );

				//	Reverse find where I am in the list:
				unsigned int nOffset = 0;
				for( ; nOffset < g_pActiveAchievements->NumAchievements(); ++nOffset )
				{
					if( &Cheevo == &g_pActiveAchievements->GetAchievement( nOffset ) )
						break;
				}
				assert( nOffset < g_pActiveAchievements->Count() );
				if( nOffset < g_pActiveAchievements->NumAchievements() )
					OnEditData( nOffset, Dlg_Achievements::Author, Cheevo.Author() );

				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				int nNewID = AddAchievement( hList, Cheevo );
				ListView_SetItemState( hList, nNewID, LVIS_FOCUSED|LVIS_SELECTED, -1 );
				ListView_EnsureVisible( hList, nNewID, FALSE );
			}
			
			break;
		case IDC_RA_CLONE_ACH:
			{
				if( !RA_GameIsActive() )
				{
					MessageBox( hDlg, "ROM not loaded: please load a ROM first!", "Error!", MB_OK );
					break;
				}

				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				if( nSel == -1 )
					return FALSE;

				//	Clone TO the user achievements
				const Achievement& Ach = g_pActiveAchievements->GetAchievement( nSel );

				//	switch to LocalAchievements
				Achievement& NewClone = LocalAchievements->AddAchievement();
				NewClone.Set( Ach );
				NewClone.SetID( 0 );
				NewClone.SetAuthor( RAUsers::LocalUser.Username() );
				char buffer[256];
				sprintf_s( buffer, 256, "%s (copy)", NewClone.Title() );
				NewClone.SetTitle( buffer );

				OnClickAchievementSet( AT_USER );

				ListView_SetItemState( hList, LocalAchievements->NumAchievements()-1, LVIS_FOCUSED|LVIS_SELECTED, -1 );
				ListView_EnsureVisible( hList, LocalAchievements->NumAchievements()-1, FALSE );
			}

			break;
		case IDC_RA_DEL_ACH:
			{
				if( !RA_GameIsActive() )
				{
					MessageBox( hDlg, "ROM not loaded: please load a ROM first!", "Error!", MB_OK );
					break;
				}

				//	Attempt to remove from list, but if it has an ID > 0, 
				//	 attempt to remove from DB
				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				if( nSel != -1 )
				{
					Achievement& Ach = g_pActiveAchievements->GetAchievement( nSel );
					
					if( Ach.ID() == 0 )
					{
						//	Local achievement
						if( MessageBox( hDlg, "Remove Achievement: are you sure?", "Are you sure?", MB_YESNO|MB_ICONWARNING ) == IDYES )
						{
							RemoveAchievement( hList, nSel );
							g_pActiveAchievements->RemoveAchievement( nSel );
						}
					}
					else
					{
						//	This achievement exists on the server: must call SQL to remove!
						//	Note: this is probably going to affect other users: frown on this D:
						MessageBox( hDlg, 
							"This achievement exists on " RA_HOST ".\n"
							"\n"
							"*Removing it will affect other gamers*\n"
							"\n"
							"Are you absolutely sure you want to delete this??", "Are you sure?", MB_YESNO|MB_ICONWARNING );
					}
				}
			}
			break;
		case IDC_RA_UPLOAD_ACH:
			{
				if( !RA_GameIsActive() )
					break;

				const int nMaxUploadLimit = 1;

				size_t nNumChecked = 0;
				int nIDsChecked[nMaxUploadLimit];
				int nLbxItemsChecked[nMaxUploadLimit];

				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				if( nSel != -1 )
				{
					Achievement& Ach = g_pActiveAchievements->GetAchievement( nSel );
					nLbxItemsChecked[0] = nSel;
					nIDsChecked[0] = Ach.ID();

					nNumChecked++;
				}

				if( nNumChecked == 0 )
					return FALSE;

				if( LocalValidateAchievementsBeforeCommit( nLbxItemsChecked ) == FALSE )
					return FALSE;

				char buffer[1024];
				sprintf_s( buffer, 1024, "Uploading the selected %d achievement(s)", nNumChecked );// with ID%s: ", nNumChecked, nNumChecked>1 ? "s" : "" );

				strcat_s( buffer, "\n"
					"Are you sure? This will update the server with your new achievements\n"
					"and players will be able to download them into their games immediately.");

				BOOL bErrorsEncountered = FALSE;

				if( MessageBox( hDlg, buffer, "Are you sure?",MB_YESNO|MB_ICONWARNING ) == IDYES )
				{
					for( size_t i = 0; i < nNumChecked; ++i )
					{
						Achievement& NextAch = g_pActiveAchievements->GetAchievement( nLbxItemsChecked[i] );

						BOOL bMovedFromUserToUnofficial = FALSE;

						unsigned int nFlags = 1<<0;	//	Active achievements! : 1
						if( g_nActiveAchievementSet == AT_CORE )
						{
							nFlags |= 1<<1;			//	Core: 3
						}
						else if( g_nActiveAchievementSet == AT_UNOFFICIAL )
						{
							nFlags |= 1<<2;			//	Retain at Unofficial: 5
						}
						else if( g_nActiveAchievementSet == AT_USER )
						{
							bMovedFromUserToUnofficial = TRUE;
							nFlags |= 1<<2;			//	Promote to Unofficial: 5
						}
						
						Document response;
						if( AttemptUploadAchievementBlocking( NextAch, nFlags, response ) )
						{
							if( response["Success"].GetBool() )
							{
								const AchievementID nAchID = response["AchievementID"].GetUint();
								NextAch.SetID( nAchID );

								//	Update listbox on achievements dlg

								//sprintf_s( LbxDataAt( nLbxItemsChecked[i], 0 ), 32, "%d", nAchID );
								LbxDataAt( nLbxItemsChecked[i], ID ) = std::to_string( nAchID );
									
								if( bMovedFromUserToUnofficial )
								{
									//	Remove the achievement from the local/user achievement set,
									//	 add it to the unofficial set.
									Achievement& NewAch = UnofficialAchievements->AddAchievement();
									NewAch.Set( NextAch );
									NewAch.SetModified( FALSE );
									LocalAchievements->RemoveAchievement( nLbxItemsChecked[0] );
									RemoveAchievement( hList, nLbxItemsChecked[0] );

									//LocalAchievements->Save();
									//UnofficialAchievements->Save();
								}
								else
								{
									//	Updated an already existing achievement, still the same position/ID.
									NextAch.SetModified( FALSE );

									//	Reverse find where I am in the list:
									size_t nIndex = g_pActiveAchievements->GetAchievementIndex( *g_AchievementEditorDialog.ActiveAchievement() );
									ASSERT( nIndex < g_pActiveAchievements->NumAchievements() );
									if( nIndex < g_pActiveAchievements->NumAchievements() )
									{
										if( g_nActiveAchievementSet == AT_CORE )
											OnEditData( nIndex, Dlg_Achievements::Modified, "No" );
									}

									//	Save em all - we may have changed any of them :S
									//CoreAchievements->Save();
									//UnofficialAchievements->Save();
									//LocalAchievements->Save();	// Will this one have changed? Maybe
								}
							}
							else
							{
								char buffer[1024];
								sprintf_s(buffer, 1024, "Error!!\n%s", response["Error"].GetString() );

								MessageBox( hDlg, buffer, "Error!", MB_OK );
								bErrorsEncountered = TRUE;
							}
						}
					}

					if( bErrorsEncountered )
					{
						MessageBox( hDlg, "Errors encountered!\nPlease recheck your data, or get latest.", "Errors!", MB_OK );
					}
					else
					{
						char buffer[512];
						sprintf_s( buffer, 512, "Successfully uploaded data for %d achievements!", nNumChecked );
						MessageBox( hDlg, buffer, "Success!", MB_OK );

						InvalidateRect( hDlg, NULL, TRUE );
					}
				}

			}
			break;
		case IDC_RA_GOTOWIKI:
			{
				char buffer[512];

				if( !RA_GameIsActive() || g_pActiveAchievements->GameTitle() == "" )
				{
					sprintf_s( buffer, 512, "\"http://%s/wiki/Main_Page\"", RA_HOST );
				}
				else
				{
					sprintf_s( buffer, 512, "\"http://%s/wiki/%s\"", RA_HOST, g_pActiveAchievements->GameTitle() );
				}
				
				ShellExecute( NULL,
					"open",
					buffer,
					NULL,
					NULL,
					SW_SHOWNORMAL );
			}
			break;
		case IDC_RA_GOTOWEB:
			{
				char buffer[512];

				if( !RA_GameIsActive() || (g_pActiveAchievements->GetGameID() == 0) )
				{
					sprintf_s( buffer, 512, "\"http://%s\"", RA_HOST );
				}
				else
				{
					sprintf_s( buffer, 512, "\"http://%s/Game/%d\"", RA_HOST, g_pActiveAchievements->GetGameID() );
				}

				ShellExecute( NULL,
					"open",
					buffer,
					NULL,
					NULL,
					SW_SHOWNORMAL );
			}
			break;
		case IDC_RA_REVERTSELECTED:
			{
				if( !RA_GameIsActive() )
					break;

				//	Attempt to remove from list, but if it has an ID > 0, 
				//	 attempt to remove from DB
				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				if( nSel != -1 )
				{
					Achievement& Cheevo = g_pActiveAchievements->GetAchievement( nSel );

					//	Ignore if it's not modified... no changes should be present...
					if( !Cheevo.Modified() )
						break;

					if( MessageBox( hDlg,
						"Attempt to revert this achievement from file?",
						"Revert from file?", MB_YESNO ) == IDYES )
					{
						//	Find Achievement with Ach.ID()
						unsigned int nID = Cheevo.ID();

						BOOL bFound = FALSE;

						//	Lots of stack use here... :S is this OK?
						AchievementSet TempSet( g_nActiveAchievementSet );
						if( TempSet.LoadFromFile( g_pActiveAchievements->GetGameID() ) )
						{
							Achievement* pAchBackup = TempSet.Find( nID );
							if( pAchBackup != NULL )
							{
								Cheevo.Set( *pAchBackup );
						
								Cheevo.SetActive( TRUE );
								Cheevo.SetModified( FALSE );
								//Cheevo.SetDirtyFlag();

								//	Reverse find where I am in the list:
								size_t nIndex = g_pActiveAchievements->GetAchievementIndex( Cheevo );
								assert( nIndex < g_pActiveAchievements->NumAchievements() );
								if( nIndex < g_pActiveAchievements->NumAchievements() )
								{
									if( g_nActiveAchievementSet == AT_CORE )
										OnEditData( nIndex, Dlg_Achievements::Achieved, "Yes" );
									else
										OnEditData( nIndex, Dlg_Achievements::Active, "No" );
									
									ReloadLBXData( nIndex );
								}

								//	Finally, reselect to update AchEditor
								g_AchievementEditorDialog.LoadAchievement( &Cheevo, FALSE );

								bFound = TRUE;
							}
						}

						if( !bFound )
						{
							MessageBox( hDlg, "Couldn't find!", "Error!", MB_OK );
						}
						else
						{
							//MessageBox( HWnd, "Reverted", "OK!", MB_OK );
						}
					}
				}
			}
			break;
			case IDC_RA_CHKACHPROCESSINGACTIVE:
			{
				g_pActiveAchievements->SetPaused( IsDlgButtonChecked( hDlg, IDC_RA_CHKACHPROCESSINGACTIVE ) == FALSE );
				bHandled = TRUE;
			}
			break;
			case IDC_RA_VOTE_POS:
			case IDC_RA_VOTE_NEG:
			{
				//Achievement* pActiveAch = g_AchievementEditorDialog.ActiveAchievement();
				//if( pActiveAch == NULL )
				//	break;

				//unsigned int nVote = (LOWORD(wParam)==IDC_RA_VOTE_POS) ? 1 : -1;

				//char bufferPost[1024];
				//sprintf_s( bufferPost, 1024, "u=%s&t=%s&v=%d&a=%d", 
				//	RAUsers::LocalUser.Username().c_str(), RAUsers::LocalUser.Token().c_str(), nVote, pActiveAch->ID() );

				//char bufferResponse[4096];
				//ZeroMemory( bufferResponse, 4096 );
				//char* pBufferResponse = &bufferResponse[0];
				//DWORD nCharsRead = 0;
				//if( DoBlockingHttpPost( "requestvote.php", bufferPost, pBufferResponse, 4096, &nCharsRead ) )
				//{
				//	//	Grab the response from the server and throw it into the listbox
				//	HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				//	int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				//	OnEditData( nSel, Votes, bufferResponse );

				//	//	Vote cast/updated
				//	MessageBox( hDlg, "Posted vote successfully!", "Success", MB_OK|MB_ICONWARNING );
				//}
				//else
				//{
				//	MessageBox( hDlg, bufferResponse, "Error!", MB_OK|MB_ICONWARNING );
				//}
			}
			break;
			case IDC_RA_RESET_ACH:
			{
				if( !RA_GameIsActive() )
					break;

				//	this could fuck up in so, so many ways. But fuck it, just reset achieved status.
				HWND hList = GetDlgItem( hDlg, IDC_RA_LISTACHIEVEMENTS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				if( nSel != -1 )
				{
					Achievement& Cheevo = g_pActiveAchievements->GetAchievement( nSel );
					if( !Cheevo.Active() )
					{
						const char* sMessage = "Temporarily reset 'achieved' state of this achievement?\n";
						if( g_nActiveAchievementSet != AT_CORE )
							sMessage = "Activate this achievement?";

						if( MessageBox( hDlg, sMessage, "Reset Achieved?", MB_YESNO ) == IDYES )
						{
							Cheevo.Reset();
							Cheevo.SetActive( true );

							size_t nIndex = g_pActiveAchievements->GetAchievementIndex( Cheevo );
							ASSERT( nIndex < g_pActiveAchievements->NumAchievements() );
							if( nIndex < g_pActiveAchievements->NumAchievements() )
							{
								if( g_nActiveAchievementSet == AT_CORE )
									OnEditData( nIndex, Dlg_Achievements::Achieved, "No" );
								else
									OnEditData( nIndex, Dlg_Achievements::Active, "Yes" );
							}

							InvalidateRect( hDlg, NULL, TRUE );

							//	Also needs to reinject text into IDC_RA_LISTACHIEVEMENTS
						}
					}
				}

				bHandled = TRUE;
			}
			break;
		}
		break;
	case WM_NOTIFY:
		{
			switch( (((LPNMHDR)lParam)->code) )
			{
				case LVN_ITEMCHANGED:
					{
						iSelect = -1;
						//MessageBox( NULL, "Item changed!", "TEST", MB_OK );
						LPNMLISTVIEW pLVInfo = (LPNMLISTVIEW)lParam;
						if( pLVInfo->iItem != -1 )
						{
							iSelect = pLVInfo->iItem;
							if( (pLVInfo->uNewState &= LVIS_SELECTED) != 0 )
							{
								int nNewIndexSelected = pLVInfo->iItem;
								g_AchievementEditorDialog.LoadAchievement( &g_pActiveAchievements->GetAchievement(nNewIndexSelected), FALSE );
							}
						}
					}
				break;
				case NM_DBLCLK:
					{
						LPNMITEMACTIVATE pLVInfo = (LPNMITEMACTIVATE)lParam;
						if( pLVInfo->iItem != -1 )
						{
							SendMessage( g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_ACHIEVEMENTEDITOR, 0 );

							int nNewIndexSelected = pLVInfo->iItem;
							g_AchievementEditorDialog.LoadAchievement( &g_pActiveAchievements->GetAchievement(nNewIndexSelected), FALSE );
						}
					}
					break;
				case NM_CUSTOMDRAW:
					{
						SetWindowLong(hDlg, DWL_MSGRESULT, (LONG)ProcessCustomDraw(lParam));
						bHandled = TRUE;
					}
					break;
			}

			break;
		}
	case WM_CLOSE:
		EndDialog(hDlg, true);
		bHandled = TRUE;
		break;
	}

	return bHandled;
}

void Dlg_Achievements::LoadAchievements( HWND hList )
{
	SetupColumns( hList );

	for( size_t i = 0; i < g_pActiveAchievements->NumAchievements(); ++i )
		AddAchievement( hList, g_pActiveAchievements->GetAchievement( i ) );
}

void Dlg_Achievements::OnLoad_NewRom( GameID nGameID )
{
	EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_GOTOWIKI ), FALSE );
	EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH ), FALSE );
	EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_UPLOAD_ACH ), FALSE );

	HWND hList = GetDlgItem( m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS );
	if( hList != NULL )
	{
		//SetupColumns( hList );
		LoadAchievements( hList );

		char buffer[256];
		sprintf_s( buffer, 256, " %d", nGameID );
		SetDlgItemText( m_hAchievementsDlg, IDC_RA_GAMEHASH, buffer );

		if( nGameID != 0 )
		{
			EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_GOTOWIKI ), TRUE );
			EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_DOWNLOAD_ACH ), TRUE);
			EnableWindow( GetDlgItem( m_hAchievementsDlg, IDC_RA_UPLOAD_ACH ), TRUE );
		}

		sprintf_s( buffer, " %d", g_pActiveAchievements->NumAchievements() );
		SetDlgItemText( m_hAchievementsDlg, IDC_RA_NUMACH, buffer );	
	}
}

void Dlg_Achievements::OnGet_Achievement( const Achievement& ach )
{
	size_t nIndex = g_pActiveAchievements->GetAchievementIndex( ach );

	if( g_nActiveAchievementSet == AT_CORE )
		OnEditData( nIndex, Achieved, "Yes" );
	else
		OnEditData( nIndex, Active, "No" );
}

void Dlg_Achievements::OnEditAchievement( const Achievement& ach )
{
	size_t nIndex = g_pActiveAchievements->GetAchievementIndex( ach );
	ASSERT( nIndex < g_pActiveAchievements->NumAchievements() );
	if( nIndex < g_pActiveAchievements->NumAchievements() )
	{
		if( g_nActiveAchievementSet == AT_CORE )
			OnEditData( nIndex, Dlg_Achievements::Modified, "Yes" );
		else
			OnEditData( nIndex, Dlg_Achievements::Active, "No" );
	}
}

void Dlg_Achievements::ReloadLBXData( int nOffset )
{
	//const char* g_sColTitles[]			= { "ID", "Title", "Author", "Achieved?", "Modified?" };
	//const char* g_sColTitlesUnofficial[]  = { "ID", "Title", "Author", "Active", "Votes" };

	Achievement& Ach = g_pActiveAchievements->GetAchievement( nOffset );
	if( g_nActiveAchievementSet == AT_CORE )
	{
		OnEditData( nOffset, Dlg_Achievements::Title, Ach.Title() );
		OnEditData( nOffset, Dlg_Achievements::Author, Ach.Author() );
		OnEditData( nOffset, Dlg_Achievements::Achieved, !Ach.Active() ? "Yes" : "No" );
		OnEditData( nOffset, Dlg_Achievements::Modified, Ach.Modified() ? "Yes" : "No" );
	}
	else
	{
		OnEditData( nOffset, Dlg_Achievements::Title, Ach.Title() );
		OnEditData( nOffset, Dlg_Achievements::Author, Ach.Author() );
		OnEditData( nOffset, Dlg_Achievements::Active, Ach.Active() ? "Yes" : "No" );
		
		//char buffer[256];
		//sprintf_s( buffer, 256, "%d/%d", Ach.Upvotes(), Ach.Downvotes() );
		//OnEditData( nOffset, Dlg_Achievements::Votes, buffer );
		OnEditData( nOffset, Dlg_Achievements::Votes, "N/A" );
	}
}

void Dlg_Achievements::OnEditData( size_t nItem, Column nColumn, const std::string& sNewData )
{
	if( nItem >= m_lbxData.size() )
		return;

	m_lbxData[nItem][nColumn] = sNewData;

	HWND hList = GetDlgItem( m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS );
	if( hList != NULL )
	{
		LV_ITEM item;
		ZeroMemory( &item, sizeof( item ) );

		item.mask = LVIF_TEXT;
		item.iItem = nItem;
		item.iSubItem = nColumn;
		item.cchTextMax = 256;
		item.pszText = const_cast<LPSTR>( m_lbxData[nItem][nColumn].data() );
		if( ListView_SetItem( hList, &item ) == FALSE )
		{
			ASSERT( !"Failed to ListView_SetItem!" );
		}
	}

	InvalidateRect( m_hAchievementsDlg, NULL, TRUE );
}

int Dlg_Achievements::GetSelectedAchievementIndex()
{
	HWND hList = GetDlgItem( m_hAchievementsDlg, IDC_RA_LISTACHIEVEMENTS );
	return ListView_GetSelectionMark( hList );
}
