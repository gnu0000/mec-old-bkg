/* bkg.c
 *
 * Craig Fitzgerald
 * Mar 1991
 */


#include <windows.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "bkg.h"

typedef unsigned char FAR *PSZ;
#define USHORT unsigned int
#define SHORT  short int

#define VERTICAL     0
#define HORIZONTAL   1
#define VERTCTR      2
#define HORZCTR      3

#define VERTCTR2     12
#define HORZCTR2     13

FARPROC pfnOldwp;
FARPROC lpfnSetColorDlgProc;
FARPROC lpfnAboutDlgProc;


HANDLE  hInstance;
HWND    hwndPopup;
char    szApp[] = "BKG";
LONG    lTop, lBottom;
USHORT  uMethod;
BOOL    bDlgUp = FALSE;
BOOL    bUseMouse = TRUE;

USHORT  uSTRIPES = 64;


BYTE GetVal (PSZ *ppsz)
   {
   USHORT i = 0;

   /*--- skip non # ---*/
   while (**ppsz < '0' || **ppsz > '9')
       (*ppsz)++;
   while (TRUE)
      {
      if (**ppsz < '0' || **ppsz > '9')
         break;
      i = i * 10 + **ppsz - '0';
      (*ppsz)++;
      }
   return (BYTE) i;
   }



DWORD GetProfileColor (PSZ pszKey, PSZ pszDefault)
   {
   char  szBuf[80];
   PSZ   p;
   BYTE  r,g,b;

   GetProfileString (szApp, pszKey, pszDefault, szBuf, 80);
   p = szBuf;
   r = GetVal (&p);
   g = GetVal (&p);
   b = GetVal (&p);
   return (RGB( r, g, b));
   }


char *ColorString (LONG lColor, char *pszColor)
   {
   char    szTmp[40];

   strcpy (pszColor, strcat (itoa (GetRValue(lColor), szTmp,10), " "));
   strcat (pszColor, strcat (itoa (GetGValue(lColor), szTmp,10), " "));
   strcat (pszColor, itoa (GetBValue(lColor), szTmp,10));
   return pszColor;
   }



void GetProfile (HWND hwndPopup)
   {
   char    szTmp[40];

   lTop    = GetProfileColor ("Top",    "0 0 0");
   lBottom = GetProfileColor ("Bottom", "0 0 0");
   lBottom = (lTop==0 && lBottom==0 ? RGB(0,0,255) : lBottom);
   GetProfileString (szApp, "Method", "Vertical", szTmp, sizeof szTmp);
   if (!stricmp (szTmp, "Vertical"))
      uMethod = VERTICAL;
   else if (!stricmp (szTmp, "Horizontal"))
      uMethod = HORIZONTAL;
   else if (!stricmp (szTmp, "Vertical Ctr"))
      uMethod = VERTCTR;
   else if (!stricmp (szTmp, "Horizontal Ctr"))
      uMethod = HORZCTR;
   else
      uMethod = VERTICAL;

   GetProfileString (szApp, "UseMouse", "Yes", szTmp, sizeof szTmp);
   bUseMouse  = !(szTmp[0] == 'n' || szTmp[0] == 'N');

   GetProfileString (szApp, "Stripes", "64", szTmp, sizeof szTmp);
   uSTRIPES = atoi (szTmp);
   if (!uSTRIPES) uSTRIPES = 64;
   }




void PutProfile (HWND hwndPopup)
   {
   char     *psz;
   char     szTmp [40];

   WriteProfileString (szApp, "Top",    ColorString (lTop,    szTmp));
   WriteProfileString (szApp, "Bottom", ColorString (lBottom, szTmp));
   switch (uMethod)
      {
      case VERTICAL:   psz = "Vertical";   break;
      case HORIZONTAL: psz = "Horizontal"; break;
      case VERTCTR:    psz = "Vertical Ctr";   break;
      case HORZCTR:    psz = "Horizontal Ctr"; break;
      default:         psz = "Vertical";   break;
      }
   WriteProfileString (szApp, "Method", psz);
   WriteProfileString (szApp, "Stripes", itoa (uSTRIPES, szTmp, 10));
   }



long del (long min, long max, USHORT stripes, long idx)
   {
   return min + ((max - min) * idx) / (long) stripes;
   }


void FAR Fade (HDC hdc, RECT rc, long lTopColor, long lBottomColor, USHORT uMethod)
   {
   long     r,g,b,R,G,B,rx,gx,bx;
   HBRUSH   hBrush;
   long     i, rt, rb, rl, rr;

   rt = (long) rc.top;
   rb = (long) rc.bottom;
   rr = (long) rc.right;
   rl = (long) rc.left;

   r = (long) GetRValue (lTopColor);
   g = (long) GetGValue (lTopColor);
   b = (long) GetBValue (lTopColor);
   R = (long) GetRValue (lBottomColor);
   G = (long) GetGValue (lBottomColor);
   B = (long) GetBValue (lBottomColor);

   for (i = 0; i < (long)uSTRIPES; i++)
      {       
      rx = del (r, R, uSTRIPES-1, i); 
      gx = del (g, G, uSTRIPES-1, i);
      bx = del (b, B, uSTRIPES-1, i);
      hBrush = CreateSolidBrush (RGB ((BYTE)rx, (BYTE)gx, (BYTE)bx));
      switch (uMethod)
         {
         case VERTICAL:
            rc.top    = (USHORT) del (rt, rb, uSTRIPES, i);
            rc.bottom = (USHORT) del (rt, rb, uSTRIPES, i + 1);
            break;
         case HORIZONTAL:
            rc.left   = (USHORT) del (rl, rr, uSTRIPES, i);
            rc.right  = (USHORT) del (rl, rr, uSTRIPES, i +1);
            break;
         case VERTCTR:
            rc.top    = (USHORT) del (rt, rb, uSTRIPES * 2, i);
            rc.bottom = (USHORT) del (rt, rb, uSTRIPES * 2, i + 1);
            break;
         case HORZCTR:
            rc.left   = (USHORT) del (rl, rr, uSTRIPES * 2, i);
            rc.right  = (USHORT) del (rl, rr, uSTRIPES * 2, i + 1);
            break;
         }
      FillRect (hdc, &rc, hBrush);

      if (uMethod == VERTCTR)
         {
         rc.top    = (USHORT) del (rt, rb, uSTRIPES * 2, uSTRIPES * 2 - i - 1);
         rc.bottom = (USHORT) del (rt, rb, uSTRIPES * 2, uSTRIPES * 2 - i);
         FillRect (hdc, &rc, hBrush);
         }
      if (uMethod == HORZCTR)
         {
         rc.left   = (USHORT) del (rl, rr, uSTRIPES * 2, uSTRIPES * 2 - i - 1);
         rc.right  = (USHORT) del (rl, rr, uSTRIPES * 2, uSTRIPES * 2 - i);
         FillRect (hdc, &rc, hBrush);
         }
      DeleteObject (hBrush);
      }
   }



void SetClr (HWND hwnd, USHORT uId, BYTE val)
   {
   char  sz[40];

   SetScrollPos (GetDlgItem(hwnd, uId + SCR), SB_CTL, (USHORT)val, TRUE);
   SetDlgItemText (hwnd, uId + EDT, itoa ((USHORT)val, sz,10));
   }



void DoEdt (HWND hwnd, USHORT uId)
   {
   char sz[40];

   GetDlgItemText (hwnd, uId, sz, 40);
   SetClr (hwnd, uId - EDT, (BYTE) atoi (sz));
   }



void Init (HWND hwnd, USHORT uId, BYTE val)
   {
   SetScrollRange (GetDlgItem (hwnd, uId + SCR), SB_CTL, 0, 255, 0);
   SetClr (hwnd, uId, val);
   }



void DoScroll (HWND hwnd, LONG lParam, USHORT wParam)
   {
   SHORT    uOld, i;
   HWND     hwndScr;

   hwndScr = (HWND) HIWORD (lParam);
   uOld = GetScrollPos (hwndScr, SB_CTL);
   switch (wParam)
      {
      case SB_BOTTOM     : i = 255               ; break;
      case SB_TOP        : i = 0                 ; break;
      case SB_LINEDOWN   : i = uOld + 1          ; break;
      case SB_LINEUP     : i = uOld - 1          ; break;
      case SB_PAGEDOWN   : i = uOld + 32 - uOld % 16 ; break;
      case SB_PAGEUP     : i = uOld - 32 + uOld % 16 ; break;
      case SB_THUMBTRACK : i = LOWORD (lParam)   ; break;
      default            : i = uOld;
      }
   i = min (255, max (0, i));
   SetClr (hwnd, GetWindowWord (hwndScr, GWW_ID) - SCR, (BYTE)i);
   }



void GetColors (HWND hwnd, LONG *lTop, LONG *lBottom)
   {
   *lTop =    RGB ((BYTE)GetScrollPos(GetDlgItem(hwnd,ID_TOP+RED),  SB_CTL),
                   (BYTE)GetScrollPos(GetDlgItem(hwnd,ID_TOP+GREEN),SB_CTL),
                   (BYTE)GetScrollPos(GetDlgItem(hwnd,ID_TOP+BLUE), SB_CTL));
   *lBottom = RGB ((BYTE)GetScrollPos(GetDlgItem(hwnd,ID_BOTTOM+RED),  SB_CTL),
                   (BYTE)GetScrollPos(GetDlgItem(hwnd,ID_BOTTOM+GREEN),SB_CTL),
                   (BYTE)GetScrollPos(GetDlgItem(hwnd,ID_BOTTOM+BLUE), SB_CTL));
   }


/**********************************************************************/

void PaintBlock (HWND hwndRect, LONG lTop, LONG lBottom, USHORT uMeth)
         {
         HDC   hdc;
         RECT  rc;

         InvalidateRect (hwndRect, NULL, TRUE);
         UpdateWindow (hwndRect);
         hdc = GetDC (hwndRect);
         GetClientRect (hwndRect, &rc);
         rc.top    += 1;
         rc.bottom -= 1;
         rc.left   += 1;
         rc.right  -= 1;
         Fade (hdc, rc, lTop, lBottom, uMeth);
         ReleaseDC (hwndRect, hdc);
         }


void UpdateLabels (HWND hwnd, USHORT uMeth)
   {
   switch (uMeth)
      {
      case VERTICAL:
         SetDlgItemText (hwnd, ID_START, "Top Color");
         SetDlgItemText (hwnd, ID_END,   "Bottom Color");
         break;
      case HORIZONTAL:
         SetDlgItemText (hwnd, ID_START, "Left Color");
         SetDlgItemText (hwnd, ID_END,   "Right Color");
         break;
      case VERTCTR:
         SetDlgItemText (hwnd, ID_START, "Edge Color");
         SetDlgItemText (hwnd, ID_END,   "Center Color");
         break;
      case HORZCTR:
         SetDlgItemText (hwnd, ID_START, "Edge Color");
         SetDlgItemText (hwnd, ID_END,   "Center Color");
         break;
      }
   }



BOOL FAR PASCAL SetColorDlgProc (HWND hwnd, WORD umsg, WORD wParam, LONG lParam)
   {
   static HWND hwndRect;
   static LONG lTop2, lBottom2;
   static USHORT uMethod2;

   switch (umsg)
      {
      case WM_INITDIALOG:
         {
         char sz [10];

         hwndRect = GetDlgItem (hwnd, ID_RECT);
         lTop2    = lTop;
         lBottom2 = lBottom;
         uMethod2 = uMethod;

         Init (hwnd, ID_TOP   + RED,   GetRValue(lTop));
         Init (hwnd, ID_TOP   + GREEN, GetGValue(lTop));
         Init (hwnd, ID_TOP   + BLUE,  GetBValue(lTop));
         Init (hwnd, ID_BOTTOM+ RED,   GetRValue(lBottom));
         Init (hwnd, ID_BOTTOM+ GREEN, GetGValue(lBottom));
         Init (hwnd, ID_BOTTOM+ BLUE,  GetBValue(lBottom));
         PaintBlock (GetDlgItem (hwnd, ID_RECT), lTop, lBottom, uMethod);
         SendMessage (GetDlgItem (hwnd, ID_DIR), CB_INSERTSTRING, VERTICAL,  (long)(PSZ)"Horizontal");
         SendMessage (GetDlgItem (hwnd, ID_DIR), CB_INSERTSTRING, HORIZONTAL,(long)(PSZ)"Vertical");
         SendMessage (GetDlgItem (hwnd, ID_DIR), CB_INSERTSTRING, VERTCTR,   (long)(PSZ)"Horizontal Ctr");
         SendMessage (GetDlgItem (hwnd, ID_DIR), CB_INSERTSTRING, HORZCTR,   (long)(PSZ)"Vertical Ctr");
         SendMessage (GetDlgItem (hwnd, ID_DIR), CB_SETCURSEL, uMethod, 0L);
         SetDlgItemText (hwnd, ID_STRIPES, itoa (uSTRIPES, sz,10));
         UpdateLabels (hwnd, uMethod);
         }
         break;

      case WM_HSCROLL:
         DoScroll (hwnd, lParam, wParam);
         GetColors (hwnd, &lTop2, &lBottom2);
         PaintBlock (GetDlgItem (hwnd, ID_RECT), lTop2, lBottom2, uMethod2);
         break;

      case WM_COMMAND:
         switch (wParam)
            {
            case ID_DIR:
               if (HIWORD (lParam) == CBN_SELCHANGE)
                  {
                  uMethod2 = (USHORT)SendMessage (LOWORD (lParam), CB_GETCURSEL, 0, 0L);
                  UpdateLabels (hwnd, uMethod2);
                  PaintBlock (GetDlgItem (hwnd, ID_RECT), lTop2, lBottom2, uMethod2);
                  }
               break;

            case ID_TOP    + EDT + RED:
            case ID_TOP    + EDT + GREEN:
            case ID_TOP    + EDT + BLUE:
            case ID_BOTTOM + EDT + RED:
            case ID_BOTTOM + EDT + GREEN:
            case ID_BOTTOM + EDT + BLUE:
               if (HIWORD (lParam) == EN_KILLFOCUS)
                  {
                  DoEdt (hwnd, wParam);
                  GetColors (hwnd, &lTop2, &lBottom2);
                  PaintBlock (GetDlgItem (hwnd, ID_RECT), lTop2, lBottom2, uMethod2);
                  }
               break;

            case ID_STRIPES:
               {
               char sz[20];

               if (HIWORD (lParam) == EN_KILLFOCUS)
                  {
                  GetDlgItemText (hwnd, ID_STRIPES, sz, 20);
                  uSTRIPES = atoi (sz);
                  if (!uSTRIPES)
                     {
                     uSTRIPES = 64;
                     SetDlgItemText (hwnd, ID_STRIPES, "64");
                     }
                  PaintBlock (GetDlgItem (hwnd, ID_RECT), lTop2, lBottom2, uMethod2);
                  }
               }
               break;


            case ID_SAVE:
            case IDOK:
               lTop = lTop2;
               lBottom = lBottom2;
               uMethod = uMethod2;
               InvalidateRect (GetDesktopWindow (), NULL, TRUE);
               if (wParam == ID_SAVE)
                  PutProfile (hwndPopup);
               EndDialog (hwnd, 0);
               break;

            case ID_ABOUT:
               DialogBox (hInstance, "ABOUT", hwnd, lpfnAboutDlgProc);
               break;


            case IDCANCEL:
               EndDialog (hwnd, 0);
               break;


            case ID_QUIT:
               EndDialog (hwnd, 0);
               SetWindowLong (GetDesktopWindow (), GWL_WNDPROC, (LONG) pfnOldwp);
               PostQuitMessage (0);
               break;

            default:
               return FALSE;
              }
         break;

      case WM_PAINT:
         PaintBlock (GetDlgItem (hwnd, ID_RECT), lTop2, lBottom2, uMethod2);
         return FALSE;

      default:
         return FALSE;
      }
   return TRUE;
   }


/***************************************************************/
void PaintDlg (HWND hwnd)
   {
   HDC         hdc;
   PAINTSTRUCT ps;
   RECT        rcl;
   HWND        hwndChild;
   char        szStr[255];
   USHORT      i;

   hdc = BeginPaint (hwnd, &ps);
   GetClientRect (hwnd, &rcl);
   Fade (hdc, rcl, RGB (0, 0, 255), RGB (0, 0, 0), 0);
   EndPaint (hwnd, &ps);

   for (i = 100; i <= 106; i++)
      {
      hwndChild = GetDlgItem (hwnd, i);
      hdc = BeginPaint (hwndChild, &ps);
      SetTextColor (hdc, RGB (255, 255, 0));
      SetBkMode (hdc, TRANSPARENT);
      GetClientRect (hwndChild, &rcl);
      GetDlgItemText (hwnd, i, szStr, sizeof szStr);
      DrawText (hdc, szStr, -1, &rcl, DT_CENTER);
      EndPaint (hwndChild, &ps);
      }
   }


BOOL FAR PASCAL AboutDlgProc (HWND hwnd, WORD umsg, WORD wParam, LONG lParam)
   {
   if (umsg == WM_PAINT)
      {
      PaintDlg (hwnd);
      return FALSE;
      }
   else if (umsg == WM_COMMAND && wParam == IDOK)
      {
      EndDialog (hwnd, 0);
      return TRUE;
      }
   return FALSE;
   }

/**********************************************************************/



long FAR PASCAL JnkWndProc (HWND hwnd, WORD umsg, WORD wParam, LONG lParam)
   {
   switch (umsg)
      {
      case WM_CREATE:
         lpfnSetColorDlgProc = MakeProcInstance (SetColorDlgProc, hInstance);
         lpfnAboutDlgProc    = MakeProcInstance (AboutDlgProc,    hInstance);
         return 0;

      case WM_SETCOLOR:
         bDlgUp = TRUE;
         DialogBox (hInstance, "SETCOLOR", hwnd, lpfnSetColorDlgProc);
         bDlgUp = FALSE;
         return 0;

      case WM_DESTROY:
         SetWindowLong (GetDesktopWindow (), GWL_WNDPROC, (LONG) pfnOldwp);
         PostQuitMessage (0) ;
         return 0;
      default:
         return DefWindowProc (hwnd, umsg, wParam, lParam);
      }
   }
            


long FAR PASCAL WndProc (HWND hwnd, WORD message, WORD wParam, LONG lParam)
   {
   switch (message)
      {
      case WM_RBUTTONDOWN:
         if (!bDlgUp && bUseMouse)
            PostMessage (hwndPopup, WM_SETCOLOR, 0, 0L);
         return 0;

      case WM_PAINT:
      case WM_ERASEBKGND:
         {
         HDC         hdc;
         PAINTSTRUCT ps;
         RECT        rc;

         hdc       = BeginPaint (hwnd, &ps);
         GetClientRect (hwnd, &rc);
         Fade (hdc, rc, lTop, lBottom, uMethod);
         EndPaint (hwnd, &ps);
         return 1L;
         }
      }
   return pfnOldwp (hwnd, message, wParam, lParam);
   }





int PASCAL WinMain (HANDLE hI, HANDLE hPrevInstance,
                    LPSTR lpszCmdLine, int nCmdShow)
   {
   HWND     hwndDesktop;
   MSG      msg;
   WNDCLASS wndclass;

   if (hPrevInstance)
      return 1;

   hInstance   = hI;
   hwndDesktop = GetDesktopWindow ();

   wndclass.style         = CS_VREDRAW;
   wndclass.lpfnWndProc   = JnkWndProc;
   wndclass.cbClsExtra    = 10;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon (hI, "BLANK");
   wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
   wndclass.hbrBackground = GetStockObject (WHITE_BRUSH);
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = "BLANK";
   RegisterClass (&wndclass);

   hwndPopup = CreateWindow ("BLANK", "Shaded Background", WS_POPUP,
                              0, 0, 0, 0, NULL, NULL, hInstance, NULL);

   ShowWindow (hwndPopup, SW_SHOWNORMAL);
   GetProfile (hwndPopup);

   pfnOldwp = (FARPROC) GetWindowLong (hwndDesktop, GWL_WNDPROC);
   SetWindowLong (hwndDesktop, GWL_WNDPROC, 
                  (LONG) MakeProcInstance ((FARPROC)WndProc, hInstance));

   InvalidateRect (hwndDesktop, NULL, FALSE);
   while (GetMessage (&msg, NULL, 0, 0))
      {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
      }
   InvalidateRect (hwndDesktop, NULL, TRUE);
   return msg.wParam;
   }



