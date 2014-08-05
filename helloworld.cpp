/*
 #####  #     # #     #          #####            Pocket GnuGo
#     # ##    # #     #         #     #   ####    for
#       # #   # #     #         #        #    #   Pocket PC
#  #### #  #  # #     #         #  ####  #    #   and Windows CE
#     # #   # # #     #         #     #  #    #   ---------     
#     # #    ## #     #         #     #  #    #   Version     
 #####  #     #  #####           #####    ####    2.6.2     

(c) Ivan Davtchev (davtchev@yahoo.com), Alexander K. Seewald (alex@seewald.at)
Based on Pocket GNU Go 1.2.0 by Ivan Datchev and 2.6.0 by Alex Seewald

Includes code from:
GNUGO - the game of Go (Wei-Chi) Version 2.6
Copyright (C) Free Software Foundation, Inc.
written by Man Li,Daniel Bump, David Denholm, Gunnar Farneback, Nils Lohner, 
Jerome Dumonteil, Tommy Thorn, Nicklas Ekstrand, Inge Wallin, Thomas Traber, 
Douglas Ridgway, Teun Burgers, Tanguy Urvoy and Thien-Thi Nguyen.

*** IMPORTANT: ***

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation - version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Please report any bug/fix, modification, suggestion to

           davtchev@yahoo.com and alex@seewald.at
*/

#include "stdafx.h"
#include "Helloworld.h"
#include <commctrl.h>
#include <aygshell.h>
#include <sipapi.h>

#include "liberty.h"
#include "interface.h"
#include "ttsgf.h"
#include "sgfana.h"

#define MAX_LOADSTRING 100
#define STATUS_LEN 45

// board/stones related
int STONE_SIZE=10;
int CELL_SIZE=12;
// should be uppercase
int half_stone=5;
int half_cell=6;
int cell_plus=13;
int BOARD_SIZE=19;
// fuseki vars
int corner_done=0;
int fuseki_ended=0;
int firstmove = 1;

// board's screen boundaries
#if _WIN32_WCE >= 300
  #define TOP_EDGE 12
  #define LEFT_EDGE 12
  #define BOTTOM_EDGE 220
  #define RIGHT_EDGE 228
#else
  #define TOP_EDGE 12+MENU_HEIGHT
  #define LEFT_EDGE 12
  #define BOTTOM_EDGE 220+MENU_HEIGHT
  #define RIGHT_EDGE 228
#endif

// game states
#define PLAYING 1
#define END_GAME 0
#define SCORING 2

// game types
#define HUMAN_HUMAN 0
#define HUMAN_CPU 1
#define HUMAN_IRDA 2

#define NODES BOARD_SIZE*BOARD_SIZE
#define ENDLIST 1000
#define QSIZE 150
#define MAXTRY 400
#define line(x) (abs(x - 9))
#define CPU_CAPTURED (cpu_color== WHITE ? black_captured : white_captured)
#define HUMAN_CAPTURED (human_color== WHITE ? black_captured : white_captured)

#if _WIN32_WCE >= 300
  #define SHGetSubMenu(hWndMB,ID_MENU) (HMENU)SendMessage((hWndMB), SHCMBM_GETSUBMENU, (WPARAM)0, (LPARAM)ID_MENU);
#endif

// Global Variables:
HINSTANCE			hInst;					// The current instance
HWND				hwndCB;					// The command bar handle
HWND				hWnd;					// The main and only window
HDC					hdc;					// Current screen "device"

// Debug Mode Flags & Vars!
TCHAR	szDebugText[30] = TEXT("");
//#define DEBUG001 1

int pass = 0;  /* two passes and its over */
int play=PLAYING;
SGFNodeP curnode = sgf_root;

float memory = MEMORY;
int DEPTH=12;
int BACKFILL_DEPTH=7;
int FOURLIB_DEPTH=4;
int KO_DEPTH=6;

int		bThinking = FALSE; // boolean flag, is GNU go thinking
HANDLE	hThinking; // event handle for the think thread
HANDLE	hThinkThread; // handle to thinking thread itself

int gameType = HUMAN_CPU; // by default it is human vs PPC

TCHAR   szCurrentText[STATUS_LEN] = TEXT(""); // the current state info on the screen
int		human_color = BLACK;
int		cpu_color = WHITE;
int		lastI, lastJ, lastX, lastY; // coords of last stone placed
TCHAR	szScore[] = TEXT("Score:");
TCHAR	szCPUCaptured[] = TEXT("P/PC captured ");
TCHAR	szHumanCaptured[] = TEXT("you captured ");
TCHAR	szWhiteCaptured[] = TEXT("W captured ");
TCHAR	szBlackCaptured[] = TEXT("B captured ");
TCHAR	szPass[] = TEXT("pass");
TCHAR   szUndo[] = TEXT("UNDO");
TCHAR	szFormatA[] = TEXT("%s");
TCHAR	szFormatB[] = TEXT("%d");
TCHAR	szFormatNorm[] = TEXT("(%C-%d) %s%d; %s%d");
TCHAR	szFormatPass[] = TEXT("(%s) %s%d, %s%d");
TCHAR	szFormatScore[] = TEXT("%s %s%.1f, %s%.1f (k=%.1f,H=%d)");
TCHAR	szScoring[] = TEXT("Everything ok? If yes: Edit>Pass");
TCHAR	szWelcome[] = TEXT("Welcome to Pocket GNU Go!");
TCHAR   szGnuGoThinks[4][40] = { TEXT("GNU Go thinks..."), TEXT("GNU Go contemplates your move..."), TEXT("GNU Go evaluates its position..."), TEXT("GNU Go is deep in thought...") };
TCHAR	szCongrats[] = TEXT("Congrats!");
TCHAR	szIWin[] = TEXT("I won!");
TCHAR   szTie[] = TEXT("It's a tie!");
TCHAR	szBlack[] = TEXT("B:");
TCHAR	szWhite[] = TEXT("W:");
HGDIOBJ	oldBrush, bbrush, wbrush, gbrush, cbrush, deadWhiteBrush, deadBlackBrush; // various brushes used for drawing
HPEN	oldPen, redPen; // pens for drawing
COLORREF	rgbBoard = 0x0066CCFF; // color for board
COLORREF	rgbCrimson = 0x00010195; // color for last piece so far
COLORREF    rgbDeadWhite = 0x00c0c0c0;
COLORREF    rgbDeadBlack = 0x00808080;

HMENU	myMenu; // for graying out items etc

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass	(HINSTANCE, LPTSTR);
BOOL				InitInstance	(HINSTANCE, int);
LRESULT CALLBACK	WndProc			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	NewBox			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	ExplainBox		(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	HelpBox			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    OptionsBox      (HWND, UINT, WPARAM, LPARAM);
HWND				CreateRpCommandBar(HWND);

static int			fval(int newlib, int minlib);

// transform from screen coordinates to board position
// 0,0 is valid origin in both
BOOL ScreenToBoard	(int x, // screen x
					 int y, // screen y
					 int *i, // board x
					 int *j) // board y
{
	x-=LEFT_EDGE; x+=half_cell;
	y-=TOP_EDGE; y+=half_cell;
	if (x < 0 || y < 0) return FALSE;

    int rx, ry;
	rx = x / CELL_SIZE;
	ry = y / CELL_SIZE;

	if (rx<0||rx>=BOARD_SIZE||ry<0||ry>=BOARD_SIZE) {
	  return FALSE;
	} else {
	  *i=rx; *j=ry;
	  return TRUE;
	}
}

BOOL ValidMove(int i, int j) { // check the validity of a move
	if (!legal(i,j,get_tomove()))
		return FALSE;
	return TRUE;
}

void ComputerMove(void) { // generate computer move
	if (play == PLAYING)	{

		wsprintf(szCurrentText, szFormatA, szGnuGoThinks[Random() % 4]);
		InvalidateRect(hWnd, NULL, TRUE); // refresh
		SendMessage(hWnd, WM_PAINT, NULL, NULL);

		if (get_move(&lastI, &lastJ,cpu_color)!=0) // no pass
		{
			pass = 0; // clear passes
            curnode = sgfAddPlay(curnode, cpu_color, lastI, lastJ);

			// generate status string
			if (gameType == HUMAN_CPU) {
				wsprintf(szCurrentText, szFormatNorm, (CHAR)(lastI+'A'), BOARD_SIZE-lastJ,
						szCPUCaptured, CPU_CAPTURED, szHumanCaptured, HUMAN_CAPTURED);
			} else {
				wsprintf(szCurrentText, szFormatPass, szPass,
						szBlackCaptured, black_captured, szWhiteCaptured, white_captured);
			}
			switch_tomove();
		} else {
			pass++;
            curnode = sgfAddPlay(curnode, cpu_color, BOARD_SIZE, BOARD_SIZE);
			play = (pass>1)? END_GAME : PLAYING;
			lastI = lastJ = lastX = lastY = -1;
			// generate status string
			if (gameType == HUMAN_CPU) {
				wsprintf(szCurrentText, szFormatPass, szPass,
						szCPUCaptured, CPU_CAPTURED, szHumanCaptured, HUMAN_CAPTURED);
			} else {
				wsprintf(szCurrentText, szFormatPass, szPass,
						szBlackCaptured, black_captured, szWhiteCaptured, white_captured);
			}
			switch_tomove();
		} 

    	if (play == END_GAME) {
			// temporarily disable Pass on Edit menu
			EnableMenuItem(myMenu, IDM_EDIT_PASS, MF_GRAYED | MF_BYCOMMAND);
			// disable Undo until New game
			EnableMenuItem(myMenu, IDM_EDIT_UNDO, MF_GRAYED | MF_BYCOMMAND);
			DialogBox(hInst, (LPCTSTR)IDD_EXPLAIN, hWnd, (DLGPROC)ExplainBox);
		} else {
			// restore the "Pass" item on the Edit menu
			EnableMenuItem(myMenu, IDM_EDIT_PASS, MF_ENABLED | MF_BYCOMMAND);
		}

		InvalidateRect(hWnd, NULL, TRUE); // refresh
	}
}

void SetupGo(float komi) { // set up variables etc at the beginning
	// Initialize Go variables
	init_board();
	init_ginfo();
	init_gopt();
    set_boardsize(BOARD_SIZE);
	set_computer_player(cpu_color);
	set_komi(komi);

    // init hash memory
    float nodes;
	if (!movehash) {
      nodes = (float)( (memory * 1024 * 1024) / (sizeof(Hashnode) + sizeof(Read_result) * 1.4));
      movehash = hashtable_new((int) (1.5 * nodes), (int)(nodes), (int)(nodes * 1.4) );
	}
    hash_init();

    /* clear some caches */
    clear_wind_cache();
    clear_safe_move_cache();

	sgfCreateHeaderNode(get_komi());
	curnode = sgf_root;

	corner_done=0;
	fuseki_ended=0;
    firstmove = 1;

	play = PLAYING;
	pass = 0;
	// clear the status string
	wsprintf(szCurrentText, szFormatA, szWelcome);
	lastI=lastJ=lastX=lastY = -1;
}

void CountFinalScore(void) { 
    int black,white;
	int CPUWins;
	float komi;
	float black_score, white_score;

	komi=get_komi();

	pushgo();
	evaluate_territory(&white,&black);
	black_score=black+white_captured-komi;
	white_score=white+black_captured+0.0f;
	popgo();

	if (human_color==BLACK) {
		CPUWins=(white_score>black_score);
	} else {
		CPUWins=(black_score>white_score);
	}
	if (gameType == HUMAN_CPU) {
		wsprintf(szCurrentText, szFormatScore, (CPUWins ? szIWin : (white_score==black_score ? szTie : szCongrats)),
		          szBlack, black_score, szWhite, white_score, komi, get_handicap());
	} else {
		wsprintf(szCurrentText, szFormatScore, szScore,
		          szBlack, black_score, szWhite, white_score, komi, get_handicap());
	}
	// disable (gray) the Undo command, although it should already be disabled..
	EnableMenuItem(myMenu, IDM_EDIT_UNDO, MF_GRAYED | MF_BYCOMMAND);
}

DWORD WINAPI ThinkThread (PVOID pvoid) {
	// entry point to the thread that does the thinking
	// bThinking is a global boolean flag that it (re)sets
	while (TRUE) {
		// wait for thinking signal
		WaitForSingleObject (hThinking, INFINITE); // resets automatically
		bThinking = TRUE;
		// and now generate move
		if (gameType == HUMAN_CPU) { // if against PC
			ComputerMove();
		} else { // assume against human
			InvalidateRect(hWnd, NULL, TRUE); // refresh
			SendMessage(hWnd, WM_PAINT, NULL, NULL);
			int temp = human_color;
			human_color = cpu_color;
			cpu_color = temp;
		}
		// signal end of thinking
		bThinking = FALSE;
	}
	return TRUE;
}

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	DWORD ThreadID;
	void *p;
    
	// use this memory for hashtable: maximum available MB - 3
	// sorry for the hack, but GlobalMemoryStatus does not seem to work
	// on my device
	memory = 64; // towards the Compaq iPaq w/ 64MB ;-)
	while ((p=malloc((int)memory * 1024 * 1024)) == NULL)
	  memory--;
	free(p);
	memory/=2;
	
	SetupGo(5.5f); // go setup
    sgf_write_game_info(get_boardsize(), get_handicap(),get_komi(), get_seed(), "WinCE");

	// Initialize brushes etc
	bbrush = GetStockObject(BLACK_BRUSH);
	wbrush = GetStockObject(WHITE_BRUSH);
	gbrush = GetStockObject(LTGRAY_BRUSH);
	cbrush = CreateSolidBrush(rgbBoard);
	deadWhiteBrush = CreateSolidBrush(rgbDeadWhite);
	deadBlackBrush = CreateSolidBrush(rgbDeadBlack);
	redPen = CreatePen(PS_SOLID, 2, rgbCrimson);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow)) 
	{
		return FALSE;
	}
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_HELLOWORLD);

#ifdef DEBUG001
	MessageBox(NULL, L"WinMain!", L"Debug", MB_OK); 
#endif
	
	// init event handle for thinking thread
	hThinking = CreateEvent (NULL, FALSE, FALSE, NULL);
	// nameless event, automatic reset, init state = non-signaled

#ifdef DEBUG001
	if (hThinking)
		MessageBox(NULL, L"CreateEvent OK!", L"Debug", MB_OK); 
	else
		MessageBox(NULL, L"CreateEvent FAIL!", L"Debug", MB_OK);
#endif

	// create non-suspended thinking thread here
	hThinkThread = CreateThread(NULL, 0, ThinkThread, NULL, 0, &ThreadID);

#ifdef DEBUG001
	if (hThinkThread)
		MessageBox(NULL, L"CreateThread OK!", L"Debug", MB_OK);
	else
		MessageBox(NULL, L"CreateThread FAIL!", L"Debug", MB_OK);
#endif

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) 
		{
			TranslateMessage(&msg);
			if (bThinking && (msg.message == WM_LBUTTONDOWN)) {
				// user clicked on board while thinking, ignore!
			} else {
				DispatchMessage(&msg);
			}
		}
	}
	return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    It is important to call this function so that the application 
//    will get 'well formed' small icons associated with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS	wc;

    wc.style			= CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc		= (WNDPROC) WndProc;
    wc.cbClsExtra		= 0;
    wc.cbWndExtra		= 0;
    wc.hInstance		= hInstance;
    wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HELLOWORLD));
    wc.hCursor			= 0;
//    wc.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
	wc.hbrBackground	= NULL;
    wc.lpszMenuName		= 0;
    wc.lpszClassName	= szWindowClass;

	return RegisterClass(&wc);
}

//
//  FUNCTION: InitInstance(HANDLE, int)
//
//  PURPOSE: Saves instance handle and creates main window
//
//  COMMENTS:
//
//    In this function, we save the instance handle in a global variable and
//    create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hWnd = NULL;
	TCHAR	szTitle[MAX_LOADSTRING];			// The title bar text
	TCHAR	szWindowClass[MAX_LOADSTRING];		// The window class name

	hInst = hInstance;		// Store instance handle in our global variable
	// Initialize global strings
	LoadString(hInstance, IDC_HELLOWORLD, szWindowClass, MAX_LOADSTRING);
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

	// If it is already running, then focus on the window
	hWnd = FindWindow(szWindowClass, szTitle);	
	if (hWnd) 
	{
		SetForegroundWindow ((HWND) (((DWORD)hWnd) | 0x01));    
		return 0;
	} 

	MyRegisterClass(hInstance, szWindowClass);
	
	RECT	rect;
	GetClientRect(hWnd, &rect);
	
	hWnd = CreateWindow(szWindowClass, szTitle, WS_CLIPCHILDREN | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	if (!hWnd)
	{	
		return FALSE;
	}


#if _WIN32_WCE >= 300
	//When the main window is created using CW_USEDEFAULT the height of the menubar (if one
	// is created is not taken into account). So we resize the window after creating it
	// if a menubar is present
	{
		RECT rc;
		GetWindowRect(hWnd, &rc);
		rc.bottom -= MENU_HEIGHT;
		if (hwndCB)
			MoveWindow(hWnd, rc.left, rc.top, rc.right, rc.bottom, FALSE);
	}
#endif

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	RECT rt;
	int movenumber, next;

	// ===========================================
	//			MESSAGE PROCESSING LOOP
	// ===========================================

	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_EDIT_UNDO:
                    movenumber = get_movenumber();
					movenumber--; // always two steps back, so human plays again
	                if ((movenumber > 0)&&(play==PLAYING)) {
					  black_captured = white_captured = 0;
	                  curnode = sgf_root;
	                  next = sgfPlayTree(&curnode, &movenumber);
	                  movenumber--;
	                  set_tomove(next);
	                  set_movenumber(movenumber);
					  
					  // update status text
					  if (gameType == HUMAN_CPU) {
							wsprintf(szCurrentText, szFormatPass, szUndo,
								szCPUCaptured, CPU_CAPTURED, szHumanCaptured, HUMAN_CAPTURED);
					  } else {
							wsprintf(szCurrentText, szFormatPass, szPass,
								szBlackCaptured, black_captured, szWhiteCaptured, white_captured);
					  }
					  lastX = lastY = lastI = lastJ = -1;
					  InvalidateRect(hWnd, NULL, TRUE); // refresh
					}
					break;
				case IDM_HELP_HELP:
					// pop up help box
					DialogBox(hInst, (LPCTSTR)IDD_HELP, hWnd, (DLGPROC)HelpBox);
					break;
				case IDM_EDIT_OPTIONS:
					// pop up help box
					DialogBox(hInst, (LPCTSTR)IDD_OPTIONS, hWnd, (DLGPROC)OptionsBox);
					break;
				case IDM_EDIT_PASS:
					if (play == SCORING) {
						play = END_GAME;
						// disable Pass on Edit menu
						EnableMenuItem(myMenu, IDM_EDIT_PASS, MF_GRAYED | MF_BYCOMMAND);
						// and count the score!!!
						CountFinalScore();
						InvalidateRect(hWnd, NULL, TRUE); // refresh
					}
					if (play == PLAYING) {
						pass++;
                        curnode = sgfAddPlay(curnode, human_color, BOARD_SIZE, BOARD_SIZE);
						play = (pass>1)? END_GAME : PLAYING;
						lastI = lastJ = -1;
						// generate status text
					    if (gameType == HUMAN_CPU) {
							wsprintf(szCurrentText, szFormatPass, szPass,
								szCPUCaptured, CPU_CAPTURED, szHumanCaptured, HUMAN_CAPTURED);
						} else {
							wsprintf(szCurrentText, szFormatPass, szPass,
								szBlackCaptured, black_captured, szWhiteCaptured, white_captured);
						}
						if (gameType == HUMAN_CPU) {
							// temporarily disable Pass on Edit menu
							EnableMenuItem(myMenu, IDM_EDIT_PASS, MF_GRAYED | MF_BYCOMMAND);
						}
						if (play == END_GAME) {
        					// disable Undo for scoring
							EnableMenuItem(myMenu, IDM_EDIT_UNDO, MF_GRAYED | MF_BYCOMMAND);
							DialogBox(hInst, (LPCTSTR)IDD_EXPLAIN, hWnd, (DLGPROC)ExplainBox);
						} else {
							//ComputerMove(); // unused in m-threaded version
							SetEvent(hThinking); // signal to thinking thread
						}
					}
					break;
				case IDM_HELP_ABOUT:
					// pop up dialog box
					DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				    break;
				case IDM_FILE_NEW:
					// pop up dialog box
					DialogBox(hInst, (LPCTSTR)IDD_NEWGAME, hWnd, (DLGPROC)NewBox);					
					if ((play==PLAYING)&&(is_computer_player(get_tomove()))) {
		                //ComputerMove(); // unused in m-threaded version
						SetEvent(hThinking); // signal to thinking thread
					}
					break;
				case IDM_FILE_EXIT:
					// exit command selected
					SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				case IDOK:
					SendMessage(hWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, 0), (LPARAM)hWnd);
					SendMessage(hWnd, WM_CLOSE, 0, 0);
					break;
				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		
		// Pen control
		case WM_LBUTTONDOWN:
			lastX = LOWORD(lParam);
			lastY = HIWORD(lParam);
			if (ScreenToBoard(lastX, lastY, &lastI, &lastJ)) {
				if ((play == PLAYING) && ValidMove(lastI, lastJ)) {
					put_move(lastI,lastJ,human_color);
					inc_movenumber();
					pass = 0; // clear passes
                    curnode = sgfAddPlay(curnode, human_color, lastI, lastJ);

					// generate status text
					// update status text
					if (gameType == HUMAN_CPU) {
						wsprintf(szCurrentText, szFormatNorm, (CHAR)(lastI+'A'), BOARD_SIZE-lastJ,
							szCPUCaptured, CPU_CAPTURED, szHumanCaptured, HUMAN_CAPTURED);
					} else {
						wsprintf(szCurrentText, szFormatNorm, (CHAR)(lastI+'A'), BOARD_SIZE-lastJ,
							szBlackCaptured, black_captured, szWhiteCaptured, white_captured);
					}
					switch_tomove();
					//ComputerMove(); // unused if multithreaded
					SetEvent(hThinking); // signal to thinking thread
				}
				if (play == SCORING) {
					// toggle status of strings...
					change_dragon_status(lastI,lastJ,dragon[lastI][lastJ].status == DEAD ? ALIVE : DEAD);
    				InvalidateRect(hWnd, NULL, TRUE);
				}
			} else {
				// indicate no successful move; to be used by UNDO
				lastX = lastY = lastI = lastJ = -1;
			}
			break;		
		case WM_CREATE:
			hwndCB = CreateRpCommandBar(hWnd);
#if _WIN32_WCE >= 300
			myMenu = (HMENU)SHGetSubMenu(hwndCB, IDM_EDIT);
#else
			myMenu = GetSubMenu((HMENU)CommandBar_GetMenu(hwndCB, 0), 1);
#endif
			break;
		case WM_PAINT:
			int i,j,distBorder, res;
			HDC hdc2; HBITMAP hbm; HGDIOBJ oldObj;
			hdc = BeginPaint(hWnd, &ps); // redraw flashes - maybe two hdcs & switch?
			hdc2 = hdc; hdc = CreateCompatibleDC(hdc2);
			GetClientRect(hWnd, &rt); // gets the rect size from the device
			hbm=CreateCompatibleBitmap(hdc2,rt.right,rt.bottom);
			oldObj = SelectObject(hdc,hbm);
			// clear screen
			oldBrush = SelectObject(hdc, wbrush);
			Rectangle(hdc,0,0,rt.right-1,rt.bottom-1);

#if _WIN32_WCE < 300
			rt.top+=MENU_HEIGHT;
			rt.bottom = 255+MENU_HEIGHT; // set the bottom border by hand
#else
			rt.bottom = 255;
#endif

			// write status text
			oldBrush = SelectObject(hdc, bbrush);
			DrawText(hdc, szCurrentText, _tcslen(szCurrentText), &rt,
				DT_BOTTOM | DT_CENTER);
			// draw lines on the Go board
			// maybe draw only lines (faster)?
			// nice bitmap (scaled) as go board, same for stones..
			// MUCH LATER!!
			oldBrush = SelectObject(hdc, cbrush);
			for (i = LEFT_EDGE; i < RIGHT_EDGE; i += CELL_SIZE) {
				for (j = TOP_EDGE; j < BOTTOM_EDGE; j += CELL_SIZE) {
					Rectangle(hdc, i, j, i+cell_plus, j+cell_plus);
				}
			}
			
			// draw handicap dots
			oldBrush = SelectObject(hdc, bbrush);
			distBorder = ( BOARD_SIZE>9 ? 3 : 2);
			// draw four border points
			Ellipse(hdc, LEFT_EDGE+CELL_SIZE*distBorder-2,TOP_EDGE+CELL_SIZE*distBorder-2,
				         LEFT_EDGE+CELL_SIZE*distBorder+3,TOP_EDGE+CELL_SIZE*distBorder+3);
			Ellipse(hdc, LEFT_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)-2,TOP_EDGE+CELL_SIZE*distBorder-2,
				         LEFT_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)+3,TOP_EDGE+CELL_SIZE*distBorder+3);
			Ellipse(hdc, LEFT_EDGE+CELL_SIZE*distBorder-2,TOP_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)-2,
				         LEFT_EDGE+CELL_SIZE*distBorder+3,TOP_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)+3);
			Ellipse(hdc, LEFT_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)-2,TOP_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)-2,
				         LEFT_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)+3,TOP_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)+3);
			// draw middle points for big board
			if (BOARD_SIZE==19) {
				Ellipse(hdc, LEFT_EDGE+CELL_SIZE*9-2,TOP_EDGE+CELL_SIZE*distBorder-2,
					         LEFT_EDGE+CELL_SIZE*9+3,TOP_EDGE+CELL_SIZE*distBorder+3);
				Ellipse(hdc, LEFT_EDGE+CELL_SIZE*9-2,TOP_EDGE+CELL_SIZE*9-2,
					         LEFT_EDGE+CELL_SIZE*9+3,TOP_EDGE+CELL_SIZE*9+3);
				Ellipse(hdc, LEFT_EDGE+CELL_SIZE*9-2,TOP_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)-2,
					         LEFT_EDGE+CELL_SIZE*9+3,TOP_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)+3);
				Ellipse(hdc, LEFT_EDGE+CELL_SIZE*distBorder-2,TOP_EDGE+CELL_SIZE*9-2,
					         LEFT_EDGE+CELL_SIZE*distBorder+3,TOP_EDGE+CELL_SIZE*9+3);
				Ellipse(hdc, LEFT_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)-2,TOP_EDGE+CELL_SIZE*9-2,
					         LEFT_EDGE+CELL_SIZE*(BOARD_SIZE-distBorder-1)+3,TOP_EDGE+CELL_SIZE*9+3);
			}

			// draw Go stones
			for (i = 0; i < BOARD_SIZE; i++) {
				for (j = 0; j < BOARD_SIZE; j++) {
					if (p[i][j] != EMPTY) {
						// if board position not empty
						// select right color for brush
						if (p[i][j] == BLACK) {
							if ((play==SCORING)&&(dragon[i][j].status==DEAD))
							  oldBrush = SelectObject(hdc, deadBlackBrush);
							else
							  oldBrush = SelectObject(hdc, bbrush);
						} else {
							if ((play==SCORING)&&(dragon[i][j].status==DEAD))
							  oldBrush = SelectObject(hdc, deadWhiteBrush);
							else
                              oldBrush = SelectObject(hdc, wbrush);
						}
						// and draw the stone
						Ellipse(hdc, LEFT_EDGE+CELL_SIZE*i-half_stone, TOP_EDGE+CELL_SIZE*j-half_stone, 
									LEFT_EDGE+CELL_SIZE*i+half_stone, TOP_EDGE+CELL_SIZE*j+half_stone);
					}
				}
			}
			// draw last stone differently, so human player can see it better
			if ((play == PLAYING) && (lastI >= 0)) {
				// set up colors
				oldPen = (HPEN)SelectObject(hdc, redPen);
				if (p[lastI][lastJ] == BLACK) {
					oldBrush = SelectObject(hdc, bbrush);
				} else {
					oldBrush = SelectObject(hdc, wbrush);
				}
				// draw stone
				Ellipse(hdc, LEFT_EDGE+CELL_SIZE*lastI-half_stone, TOP_EDGE+CELL_SIZE*lastJ-half_stone, 
							LEFT_EDGE+CELL_SIZE*lastI+half_stone, TOP_EDGE+CELL_SIZE*lastJ+half_stone);
				// restore black pen
				SelectObject(hdc, oldPen);
			}
			GetClientRect(hWnd, &rt); // gets the rect size from the device, again
			res=BitBlt(hdc2,0,0,rt.right,rt.bottom,hdc,0,0,SRCCOPY);
			res=DeleteDC(hdc);
			res=DeleteObject(hbm);
			EndPaint(hWnd, &ps);
			break; 
		case WM_DESTROY:
			free(movehash);
			CommandBar_Destroy(hwndCB);
			PostQuitMessage(0);
			break;
		case WM_SETTINGCHANGE:
     		break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

#if _WIN32_WCE >= 300
HWND CreateRpCommandBar(HWND hwnd)
{
	SHMENUBARINFO mbi;

	memset(&mbi, 0, sizeof(SHMENUBARINFO));
	mbi.cbSize     = sizeof(SHMENUBARINFO);
	mbi.hwndParent = hwnd;
	mbi.nToolBarId = IDM_MENU;
	mbi.hInstRes   = hInst;
	mbi.nBmpId     = 0;
	mbi.cBmpImages = 0;

	if (!SHCreateMenuBar(&mbi)) 
		return NULL;

	return mbi.hwndMB;
}
#else
HWND CreateRpCommandBar(HWND hwnd)
{
    HWND hwndCB;

	hwndCB = CommandBar_Create(hInst, hwnd, 1);
	CommandBar_InsertMenubar (hwndCB, hInst, IDM_MENU, 0);

	return hwndCB;
}
#endif

// Mesage handler for the About box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#if _WIN32_WCE >= 300
	SHINITDLGINFO shidi;
#endif
	switch (message)
	{
		case WM_INITDIALOG:
#if _WIN32_WCE >= 300
			// Create a Done button and size it.  
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);
#endif
			return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

// Mesage handler for the Help box.
LRESULT CALLBACK HelpBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#if _WIN32_WCE >= 300
	SHINITDLGINFO shidi;
#endif
	switch (message)
	{
		case WM_INITDIALOG:
#if _WIN32_WCE >= 300
			// Create a Done button and size it.  
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);
#endif
			return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

// Mesage handler for the Explain box about scoring at the end of the game
LRESULT CALLBACK ExplainBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
#if _WIN32_WCE >= 300
	SHINITDLGINFO shidi;
#endif
	switch (message)
	{
		case WM_INITDIALOG:
#if _WIN32_WCE >= 300
			// Create a Done button and size it.  
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);
#endif
			// start SCORING mode
			play = SCORING;
			make_worms();
			make_dragons();
            make_moyo(get_computer_player());

			// restore the "Pass" item on the Edit menu
			EnableMenuItem(myMenu, IDM_EDIT_PASS, MF_ENABLED | MF_BYCOMMAND);
			// change state message
			wsprintf(szCurrentText, szFormatA, szScoring);
			return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) {
				InvalidateRect(hWnd, NULL, TRUE); // last refresh
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

// Message handler for the New Game box.
LRESULT CALLBACK NewBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;
#if _WIN32_WCE >= 300
	SHINITDLGINFO shidi;
#endif
	TCHAR	szNumber[10];		// For the items on the list
	HWND	hwndHandListCtrl;	// Window handle to the handicap list box
	HWND	hwndSizeListCtrl;	// Window handle to the board size list box
	HWND	hwndBlackButton;	// Window handle to the black radio button box
	HWND	hwndAgainstCPU;		// Window handle to the game type check box
	int		iCurrItem = -1;		// Current item
	int handicap, bs;           // Handicap and board size
	float komi_ = 5.5f;

	switch (message)
	{
		case WM_INITDIALOG:
#if _WIN32_WCE >= 300
			// Create a Done button and size it.  
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = /*SHIDIF_DONEBUTTON |*/ SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
#endif
			// -----------------------------
			hwndHandListCtrl = GetDlgItem (hDlg, IDC_HANDICAP);
			hwndSizeListCtrl = GetDlgItem (hDlg, IDC_BOARD_SIZE);
			hwndBlackButton = GetDlgItem (hDlg, IDC_BLACK);
			hwndAgainstCPU = GetDlgItem (hDlg, IDC_AGAINSTCPU);
			SendMessage (hwndBlackButton, BM_SETCHECK, BST_CHECKED, 0); // check button
			SendMessage (hwndAgainstCPU, BM_SETCHECK, BST_CHECKED, 0); // check button
			for (i = 0; i < 10; i++) {
				// place list items
				wsprintf(szNumber, szFormatB, i);
				// Add the number to the list control.
				iCurrItem = SendMessage (hwndHandListCtrl, LB_ADDSTRING, 0, 
									(LPARAM)(LPCTSTR) szNumber);

				// Set a 32-bit value, (LPARAM) i, that is associated
				// with the newly added item in the list control.
				SendMessage (hwndHandListCtrl, LB_SETITEMDATA,      
									(WPARAM) iCurrItem, (LPARAM) i);  
			}
			// Select the first number in the list control.
			SendMessage (hwndHandListCtrl, LB_SETCURSEL, 0, 0);

			// Add list items & associate board sizes
			wsprintf(szNumber,TEXT("  9x9"));
			iCurrItem = SendMessage (hwndSizeListCtrl, LB_ADDSTRING, 0, 
				                (LPARAM)(LPCTSTR) szNumber);
			SendMessage (hwndSizeListCtrl, LB_SETITEMDATA,      
								(WPARAM) iCurrItem, (LPARAM) 9);
			wsprintf(szNumber,TEXT("13x13"));
			iCurrItem = SendMessage (hwndSizeListCtrl, LB_ADDSTRING, 0, 
				                (LPARAM)(LPCTSTR) szNumber);
			SendMessage (hwndSizeListCtrl, LB_SETITEMDATA,      
								(WPARAM) iCurrItem, (LPARAM) 13);
			wsprintf(szNumber,TEXT("19x19"));;
			iCurrItem = SendMessage (hwndSizeListCtrl, LB_ADDSTRING, 0, 
				                (LPARAM)(LPCTSTR) szNumber);
			SendMessage (hwndSizeListCtrl, LB_SETITEMDATA,
								(WPARAM) iCurrItem, (LPARAM) 19);

			// Select the first number in the list control.
			SendMessage (hwndSizeListCtrl, LB_SETCURSEL, 2, 0);
			// -----------------------------
#if _WIN32_WCE >= 300
			SHInitDialog(&shidi);
#endif
			return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) {
				// see what is selected and make new game
				hwndHandListCtrl = GetDlgItem (hDlg, IDC_HANDICAP);
     			hwndSizeListCtrl = GetDlgItem (hDlg, IDC_BOARD_SIZE);
				hwndBlackButton = GetDlgItem (hDlg, IDC_BLACK);
				hwndAgainstCPU = GetDlgItem (hDlg, IDC_AGAINSTCPU);
				// ask buttons for their state
				if (BST_CHECKED == SendMessage(hwndAgainstCPU, BM_GETCHECK, 0, 0)) {
					gameType = HUMAN_CPU;
				} else {
					gameType = HUMAN_HUMAN;
				}
				if (BST_CHECKED == SendMessage(hwndBlackButton, BM_GETCHECK, 0, 0)) {
					human_color = BLACK;
					cpu_color = WHITE;
				} else {
					human_color = WHITE;
					cpu_color = BLACK;
				}
				// ask list box about handicap
				handicap = SendMessage (hwndHandListCtrl, LB_GETCURSEL, 0, 0);
				if (handicap < 0 || handicap > 9) 
					handicap = 0; // sanity check

				iCurrItem = SendMessage (hwndSizeListCtrl, LB_GETCURSEL, 0, 0);
				bs = SendMessage (hwndSizeListCtrl, LB_GETITEMDATA, (WPARAM) iCurrItem, 0);

				if (handicap > 0) {
				  komi_=0.5f;
				} else {
				  switch (bs) {
				    case 19: komi_=5.5f; break;
				    case 13: komi_=7.5f; break;
				    case 9: komi_=9.5f; break;
				  }
				}

				SetupGo(komi_);

				// set handicap appropriately
				set_handicap(handicap);
				if (handicap > 0) {
					set_tomove(WHITE);
				} else {
					set_tomove(BLACK);
				}

				BOARD_SIZE=bs;
				set_boardsize(bs);

				CELL_SIZE=(RIGHT_EDGE-LEFT_EDGE)/(BOARD_SIZE-1);
				half_cell=CELL_SIZE/2;
				STONE_SIZE=CELL_SIZE*5/6;
				half_stone=STONE_SIZE/2;
				cell_plus=CELL_SIZE+1;

				// write game info
			    sgf_write_game_info(get_boardsize(), get_handicap(),get_komi(), get_seed(), "WinCE");
				// set handicap stones
				set_handicap(sethand(get_handicap()));

				play = PLAYING;
            	pass = 0;

				black_captured = white_captured = 0;

				// clear the status string
				wsprintf(szCurrentText, szFormatA, szWelcome);

				// enable Undo & Pass
				EnableMenuItem(myMenu, IDM_EDIT_UNDO, MF_ENABLED | MF_BYCOMMAND);
				EnableMenuItem(myMenu, IDM_EDIT_PASS, MF_ENABLED | MF_BYCOMMAND);

				InvalidateRect(hWnd, NULL, TRUE);
				EndDialog(hDlg, LOWORD(wParam));
				
				return TRUE;

			} else
			if (LOWORD(wParam) == IDCANCEL) {
				// just exit without any changes
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

// Mesage handler for the New Game box.
LRESULT CALLBACK OptionsBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR	szNumber[10];		// For numbers
#if _WIN32_WCE >= 300
	SHINITDLGINFO shidi;
#endif
	int iCurrItem;
	// Dialog items
	HWND hwndDepth, hwndBackfill, hwndFourlib, hwndKo;

	switch (message)
	{
		case WM_INITDIALOG:
#if _WIN32_WCE >= 300
			// Create a Done button and size it.  
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
#endif
			hwndDepth     = GetDlgItem(hDlg, IDC_EDITDEPTH);
			hwndBackfill  = GetDlgItem(hDlg, IDC_EDITBACKFILL);
			hwndFourlib   = GetDlgItem(hDlg, IDC_EDITFOURLIB);
			hwndKo        = GetDlgItem(hDlg, IDC_EDITKO);

			wsprintf(szNumber,szFormatB,DEPTH);
			SendMessage (hwndDepth, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szNumber);

			wsprintf(szNumber,szFormatB,BACKFILL_DEPTH);
			SendMessage (hwndBackfill, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szNumber);

			wsprintf(szNumber,szFormatB,FOURLIB_DEPTH);
			SendMessage (hwndFourlib, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szNumber);

			wsprintf(szNumber,szFormatB,KO_DEPTH);
			SendMessage (hwndKo, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szNumber);
#if _WIN32_WCE >= 300
			SHInitDialog(&shidi);
#endif
			return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) {
				hwndDepth     = GetDlgItem(hDlg, IDC_EDITDEPTH);
				hwndBackfill  = GetDlgItem(hDlg, IDC_EDITBACKFILL);
				hwndFourlib   = GetDlgItem(hDlg, IDC_EDITFOURLIB);
				hwndKo        = GetDlgItem(hDlg, IDC_EDITKO);
				
				SendMessage(hwndDepth,WM_GETTEXT,10,(LPARAM)(LPCTSTR)szNumber);
				iCurrItem = (int) wcstol(szNumber,NULL,10);
				if (iCurrItem!=DEPTH) {
				  DEPTH=iCurrItem; depth=iCurrItem;
				}
				
				SendMessage(hwndBackfill,WM_GETTEXT,10,(LPARAM)(LPCTSTR)szNumber);
				iCurrItem = (int) wcstol(szNumber,NULL,10);
				if (iCurrItem!=BACKFILL_DEPTH) {
                  BACKFILL_DEPTH=iCurrItem; backfill_depth=iCurrItem;
				}

				SendMessage(hwndFourlib,WM_GETTEXT,10,(LPARAM)(LPCTSTR)szNumber);
				iCurrItem = (int) wcstol(szNumber,NULL,10);
				if (iCurrItem!=FOURLIB_DEPTH) {
                  FOURLIB_DEPTH=iCurrItem; fourlib_depth=iCurrItem;
				}

				SendMessage(hwndKo,WM_GETTEXT,10,(LPARAM)(LPCTSTR)szNumber);
				iCurrItem = (int) wcstol(szNumber,NULL,10);
				if (iCurrItem!=KO_DEPTH) {
                  KO_DEPTH=iCurrItem; ko_depth=iCurrItem;
				}
				EndDialog(hDlg, LOWORD(wParam));

                InvalidateRect(hWnd, NULL, TRUE);
				return TRUE;
			} else
			if (LOWORD(wParam) == IDCANCEL) {
				// just exit without any changes
				EndDialog(hDlg, LOWORD(wParam));
                InvalidateRect(hWnd, NULL, TRUE);
				return TRUE;
			}
			break;
	}
    return FALSE;
}