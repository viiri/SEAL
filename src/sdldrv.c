/*
 * $Id: sdldrv.c 1.0 2024/05/24 16:08:00 x0r released $
 *
 * SDL2 Audio driver.
 *
 * Copyright (C) 2024 Sergei "x0r" Kolzun
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <SDL.h>

#include "audio.h"
#include "drivers.h"

#define BUFFERSIZE      (1 << 12)

/*
 * SDL2 Audio driver configuration structure
 */
static struct {
    SDL_AudioDeviceID devId;
    LPFNAUDIOWAVE lpfnAudioWave;
    WORD    wFormat;
} Audio;

/*
 * SDL2 Audio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_SDL, "SDL2 Audio",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static void UpdateAudioCallback(void *udata, Uint8 *stream, int len)
{
    /* send PCM samples to the DSP audio device */
    if (Audio.lpfnAudioWave != NULL)
        Audio.lpfnAudioWave(stream, (UINT) len);
    else
        memset(stream, Audio.wFormat & AUDIO_FORMAT_16BITS ? 0x00 : 0x80, len);
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    SDL_AudioSpec audiospec;

    memset(&Audio, 0, sizeof(Audio));

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
        return AUDIO_ERROR_DEVICEBUSY;

    Audio.wFormat = lpInfo->wFormat;

    audiospec.freq = lpInfo->nSampleRate;
    audiospec.format = lpInfo->wFormat & AUDIO_FORMAT_16BITS ? AUDIO_S16 : AUDIO_U8;
    audiospec.channels = lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1;
    audiospec.samples = BUFFERSIZE;
    audiospec.callback = UpdateAudioCallback;

#ifdef __WINDOWS__
    if (SDL_OpenAudio(&audiospec, NULL) < 0)
        return AUDIO_ERROR_BADFORMAT;

    SDL_PauseAudio(0);
#else
    Audio.devId = SDL_OpenAudioDevice(NULL, 0, &audiospec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (Audio.devId == 0)
        return AUDIO_ERROR_BADFORMAT;

    SDL_PauseAudioDevice(Audio.devId, 0);
#endif

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
#ifdef __WINDOWS__
    SDL_CloseAudio();
#else
    SDL_CloseAudioDevice(Audio.devId);
#endif
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    /* set up DSP audio device user's callback function */
    Audio.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}

/*
 * SDL2 Audio driver public interface
 */
AUDIOWAVEDRIVER SDL2AudioDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER SDL2Driver =
{
    &SDL2AudioDriver, NULL
};
