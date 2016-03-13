#include <windows.h>
#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "devaudio.h"

typedef	struct	Waveblock Waveblock;

struct Waveblock
{
	WAVEHDR	h;
	uchar		s[2048];
};

enum
{
	Channels	= 2,
	Rate		= 44100,
	Bits		= 16,
};

static	int			speed = Rate;
static	int			volleft = 100;
static	int			volright = 100;
static	HWAVEOUT	waveout;
static	Waveblock	blk[16];
static	uint			blkidx;

void
audiodevopen(void)
{
	WAVEFORMATEX f;
	DWORD v;

	f.nSamplesPerSec = speed;
	f.wBitsPerSample = Bits;
	f.nChannels = Channels;
	f.wFormatTag = WAVE_FORMAT_PCM;
	f.nBlockAlign = f.wBitsPerSample*f.nChannels/8;
	f.nAvgBytesPerSec = f.nBlockAlign*f.nSamplesPerSec;
	if(waveOutOpen(&waveout, WAVE_MAPPER, &f, 0, 0, 0) != MMSYSERR_NOERROR)
		oserror();
	v = MAKELONG(volleft*0xFFFF/100, volright*0xFFFF/100);
	waveOutSetVolume(waveout, v);
}

void
audiodevclose(void)
{
	if(waveOutReset(waveout)!=MMSYSERR_NOERROR || waveOutClose(waveout)!=MMSYSERR_NOERROR)
		oserror();
	waveout = nil;
}

int
audiodevread(void *a, int n)
{
	error("no reading");
	return -1;
}

int
audiodevwrite(void *a, int n)
{
	Waveblock *b;
	int m;

	m = 0;
	while(n > sizeof(b->s)){
		audiodevwrite(a, sizeof(b->s));
		a = (uchar*)a + sizeof(b->s);
		n -= sizeof(b->s);
		m += sizeof(b->s);
	}

	b = &blk[blkidx++ % nelem(blk)];
	if(b->h.dwFlags & WHDR_PREPARED)
		while(waveOutUnprepareHeader(waveout, &b->h, sizeof(b->h)) == WAVERR_STILLPLAYING)
			osmsleep(50);
	memmove(b->s, a, n);
	b->h.lpData = (void*)b->s;
	b->h.dwBufferLength = n;
	waveOutPrepareHeader(waveout, &b->h, sizeof(b->h));
	waveOutWrite(waveout, &b->h, sizeof(b->h));
	return m+n;
}

void
audiodevsetvol(int what, int left, int right)
{
	DWORD v;

	switch(what){
	case Vspeed:
		speed = left;
		if(waveout != nil){
			audiodevclose();
			audiodevopen();
		}
		break;
	case Vaudio:
		if(left >= 0)
			volleft = left;
		if(right >= 0)
			volright = right;
		if(waveout != nil){
			v = MAKELONG(volleft*0xFFFF/100, volright*0xFFFF/100);
			waveOutSetVolume(waveout, v);
		}
		break;
	}
}

void
audiodevgetvol(int what, int *left, int *right)
{
	switch(what){
	case Vspeed:
		*left = *right = speed;
		break;
	case Vaudio:
		*left = volleft;
		*right = volright;
		break;
	case Vtreb:
	case Vbass:
		*left = *right = 50;
		break;
	default:
		*left = *right = 0;
	}
}
