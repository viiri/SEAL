/*
 * $Id: mp.c 1.13 1996/09/13 18:18:38 chasan released $
 *
 * Module player demonstration
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"

#if defined(__WINDOWS__)
#include <conio.h>
#else
#define kbhit() 0
#endif


struct {
    AUDIOINFO Info;
    AUDIOCAPS Caps;
    LPAUDIOMODULE lpModule;
    UINT nVolume;
    BOOL bStopped;
} State;

void Assert(UINT nErrorCode)
{
    static CHAR szText[80];

    if (nErrorCode != AUDIO_ERROR_NONE) {
        AGetErrorText(nErrorCode, szText, sizeof(szText) - 1);
        printf("%s\n", szText);
        exit(1);
    }
}

#ifdef __MSC__
void __cdecl CleanUp(void)
#else
    void CleanUp(void)
#endif
{
    ACloseAudio();
}

void Banner(void)
{

/* Linux/386 */
#ifdef __LINUX__
#define _SYSTEM_ "Linux"
#endif

/* Win32 */
#if defined(__WINDOWS__)
#ifdef __BORLANDC__
#define _SYSTEM_ "Win32/BC32"
#endif
#ifdef __WATCOMC__
#define _SYSTEM_ "Win32/WC32"
#endif
#ifdef __MSC__
#define _SYSTEM_ "Win32/MSC32"
#endif
#endif

#ifndef _SYSTEM_
#define _SYSTEM_ "UNKNOWN"
#endif
    
    printf("MOD/MTM/S3M/XM Module Player Version 0.6 (" _SYSTEM_ " version)\n");
    printf("Please send bug reports to: chasan@dcc.uchile.cl\n");
}

void Usage(void)
{
    UINT n;

    printf("Usage: mp [options] file...\n");
    printf("  -c device   select audio device\n");
    for (n = 0; n < AGetAudioNumDevs(); n++) {
        AGetAudioDevCaps(n, &State.Caps);
        printf("\t\t%d=%s\n", n, State.Caps.szProductName);
    }
    printf("  -r rate     set sampling rate\n");
    printf("  -v volume   change global volume\n");
    printf("  -8          8-bit sample precision\n");
    printf("  -m          monophonic output\n");
    printf("  -i          enable filtering\n");
    printf("\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    LPSTR lpszOption, lpszOptArg, lpszFileName;
    int n;

    /* initialize the audio system library */
    AInitialize();

    /* display program information */
    Banner();
    if (argc < 2) Usage();

    /* setup default initialization parameters */
    State.Info.nDeviceId = AUDIO_DEVICE_MAPPER;
    State.Info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    State.Info.nSampleRate = 44100;
    State.nVolume = 96;

    /* parse command line options */
    for (n = 1; n < argc && (lpszOption = argv[n])[0] == '-'; n++) {
        lpszOptArg = &lpszOption[2];
        if (strchr("crv", lpszOption[1]) && !lpszOptArg[0] && n < argc - 1)
            lpszOptArg = argv[++n];
        switch (lpszOption[1]) {
        case 'c':
            State.Info.nDeviceId = atoi(lpszOptArg);
            break;
        case '8':
            State.Info.wFormat &= ~AUDIO_FORMAT_16BITS;
            break;
        case 'm':
            State.Info.wFormat &= ~AUDIO_FORMAT_STEREO;
            break;
        case 'i':
            State.Info.wFormat |= AUDIO_FORMAT_FILTER;
            break;
        case 'r':
            State.Info.nSampleRate = (UINT) atoi(lpszOptArg);
            if (State.Info.nSampleRate < 1000)
                State.Info.nSampleRate *= 1000;
            break;
        case 'v':
            State.nVolume = atoi(lpszOptArg);
            break;
        default:
            Usage();
            break;
        }
    }

    /* initialize and open the audio system */
    Assert(AOpenAudio(&State.Info));
    atexit(CleanUp);

    /* display playback audio device information */
    Assert(AGetAudioDevCaps(State.Info.nDeviceId, &State.Caps));
    printf("%s playing at %d-bit %s %u Hz\n", State.Caps.szProductName,
	   State.Info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
	   State.Info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
	   State.Info.nSampleRate);

    while (n < argc && (lpszFileName = argv[n++]) != NULL) {
        /* load module file from disk */
        printf("Loading: %s\n", lpszFileName);
        Assert(ALoadModuleFile(lpszFileName, &State.lpModule, 0L));

        /* play the module file */
        printf("Playing: %s\n", State.lpModule->szModuleName);
        Assert(AOpenVoices(State.lpModule->nTracks));
        Assert(APlayModule(State.lpModule));
        Assert(ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME, State.nVolume));
#if defined(__WINDOWS__)
        printf("Press enter to quit\n");
        getchar();
#else
        while (!kbhit()) {
            Assert(AGetModuleStatus(&State.bStopped));
            if (State.bStopped) break;
            Assert(AUpdateAudio());
	}
#endif
        Assert(AStopModule());
        Assert(ACloseVoices());

        /* release the module file */
        Assert(AFreeModuleFile(State.lpModule));
    }

    return 0;
}
