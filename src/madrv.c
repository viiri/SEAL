/*
 * $Id: madrv.c 1.0 2024/06/13 15:59:00 x0r released $
 *
 * miniaduio driver.
 *
 * Copyright (C) 2024 Sergei "x0r" Kolzun
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "audio.h"
#include "drivers.h"

/*
 * miniaudio driver configuration structure
 */
static struct {
    ma_device device;
    LPFNAUDIOWAVE lpfnAudioWave;
    WORD    wFormat;
} Audio;

/*
 * miniaudio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_MINIAUDIO, "miniaudio",
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

static void UpdateAudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    /* compute frame size */
    if (Audio.wFormat & AUDIO_FORMAT_16BITS) frameCount <<= 1;
    if (Audio.wFormat & AUDIO_FORMAT_STEREO) frameCount <<= 1;

    /* send PCM samples to the DSP audio device */
    if (Audio.lpfnAudioWave != NULL)
        Audio.lpfnAudioWave(pOutput, (UINT) frameCount);
    else
        memset(pOutput, Audio.wFormat & AUDIO_FORMAT_16BITS ? 0x00 : 0x80, frameCount);

    (void)pInput;   /* Unused. */
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    ma_device_config deviceConfig;

    memset(&Audio, 0, sizeof(Audio));

    Audio.wFormat = lpInfo->wFormat;

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = lpInfo->wFormat & AUDIO_FORMAT_16BITS ? ma_format_s16 : ma_format_u8;
    deviceConfig.playback.channels = lpInfo->wFormat & AUDIO_FORMAT_STEREO ? 2 : 1;
    deviceConfig.sampleRate        = lpInfo->nSampleRate;
    deviceConfig.dataCallback      = UpdateAudioCallback;

    if (ma_device_init(NULL, &deviceConfig, &Audio.device) != MA_SUCCESS)
        return AUDIO_ERROR_BADFORMAT;

    if (ma_device_start(&Audio.device) != MA_SUCCESS) {
        ma_device_uninit(&Audio.device);
        return AUDIO_ERROR_DEVICEBUSY;
    }

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    ma_device_uninit(&Audio.device);
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
 * miniaudio driver public interface
 */
AUDIOWAVEDRIVER miniaudioAudioDriver =
{
        GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
        UpdateAudio, SetAudioCallback
};

AUDIODRIVER miniaudioDriver =
{
        &miniaudioAudioDriver, NULL
};
