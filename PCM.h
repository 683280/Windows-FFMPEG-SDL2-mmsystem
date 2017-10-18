//
// Created by 13342 on 2017/10/18.
//

#ifndef FFMPEG_PCM_H
#define FFMPEG_PCM_H

int init(int channel,int bit,int sample_rate);
void playbackPCM(char *buf,int len);
class PCM {
public:
    PCM();
};


#endif //FFMPEG_PCM_H
