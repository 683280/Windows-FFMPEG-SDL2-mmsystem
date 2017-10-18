//
// Created by 13342 on 2017/10/18.
//

#include <afxres.h>
#include <stdio.h>
#include "PCM.h"
static HWAVEOUT        playWavOut;
static WAVEHDR         playWavHdr;
static WAVEFORMATEX    playWaveform;
static HANDLE          playWait;
int init(int channel,int bit,int sample_rate){
    playWaveform.wFormatTag = WAVE_FORMAT_PCM;
    playWaveform.nChannels = channel;
    playWaveform.nSamplesPerSec = sample_rate;
    playWaveform.wBitsPerSample = bit;
    playWaveform.nBlockAlign = (channel*bit)/8;
    playWaveform.nAvgBytesPerSec = sample_rate*playWaveform.nBlockAlign;
    playWaveform.cbSize = 0;
    if(playWait){
        CloseHandle(playWait);
        playWait=NULL;
    }
    playWait = CreateEvent(NULL, 0, 0, NULL);
    if (!waveOutGetNumDevs() ){
        printf("waveOutGetNumDevs ERR audio.c 95 \n");
//        _have_init_playback = -1;
        return -1;
    }
    if(waveOutOpen (0,0,&playWaveform,0,0,WAVE_FORMAT_QUERY))
    {
        printf("wave设备初始化失败～");
        return false;
    }
    int ret;
    ret = waveOutOpen(&playWavOut, WAVE_MAPPER, &playWaveform, (DWORD_PTR)playWait, 0L, CALLBACK_EVENT);
    printf("*******    waveOutOpen   *******\n");
    //ret = waveOutOpen(&playWavOut, WAVE_MAPPER, &playWaveform, NULL, 0L, CALLBACK_EVENT);
    if(ret != MMSYSERR_NOERROR){
        printf("waveOutOpen ERR audio.c 102 \n");
//        _have_init_playback = -1;
        return -1;
    }
}
void playbackPCM(char *buf,int len)
{
    int mRet = 0;
    ZeroMemory(&playWavHdr, sizeof(WAVEHDR));
    playWavHdr.lpData = buf;
    playWavHdr.dwBufferLength = len;
    playWavHdr.dwFlags = 0L;
    playWavHdr.dwLoops = 1L;
    mRet = waveOutPrepareHeader(playWavOut, &playWavHdr, sizeof(WAVEHDR));
    if( mRet != MMSYSERR_NOERROR )  {
        printf("waveOutPrepareHeader ERR  %d\n",mRet);
    }
    mRet = waveOutWrite(playWavOut, &playWavHdr, sizeof(WAVEHDR));
    if( mRet != MMSYSERR_NOERROR )  {
        printf("waveOutWrite ERR  %d\n",mRet);
    }
    waveOutUnprepareHeader(playWavOut,&playWavHdr,sizeof(WAVEHDR));
    WaitForSingleObject(playWait, INFINITE);
}
PCM::PCM() {
    
}
