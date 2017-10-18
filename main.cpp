#include <iostream>
#include <sdl/SDL.h>
#include <windef.h>
#include <afxres.h>
#include "CPcmSpeaker.h"
#pragma  comment(lib, "winmm.lib")
extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}
int get_decodec_ctx(AVFormatContext* formatCtx,AVMediaType type,AVCodecContext** video_codec_ctx,int* index){
    auto i = av_find_best_stream(formatCtx,type,-1,-1,NULL,0);
    auto codecpar = formatCtx->streams[i]->codecpar;
    auto codec = avcodec_find_decoder(codecpar->codec_id);
    *video_codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(*video_codec_ctx,codecpar);
    avcodec_open2(*video_codec_ctx,codec,NULL);
    *index = i;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SDL_Window *window;
    SDL_Renderer *sdlRenderer;
    SDL_Texture *sdlTexture;
    SDL_Rect sdlRect;
    int width = 320;
    int height = 480;
    const char *url = "http://127.0.0.1/1.flv";
    SDL_Init(SDL_INIT_VIDEO);

    //FFMPEG
    av_register_all();
    avformat_network_init();
    AVFormatContext *formatCtx = avformat_alloc_context();
    avformat_open_input(&formatCtx, url, NULL, NULL);
    avformat_find_stream_info(formatCtx, NULL);
    AVCodecContext*video_codec_ctx,*audio_codec_ctx;
    int video_stream_idx,audio_stream_idx;
    get_decodec_ctx(formatCtx,AVMEDIA_TYPE_VIDEO,&video_codec_ctx,&video_stream_idx);
    get_decodec_ctx(formatCtx,AVMEDIA_TYPE_AUDIO,&audio_codec_ctx,&audio_stream_idx);
    AVPacket pkt;
    AVFrame *aframe = av_frame_alloc();
    AVFrame *vframe = av_frame_alloc();
//    width = video_codec_ctx->width;
//    height = video_codec_ctx->height;
    auto sws_ctx = sws_getContext(video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt, width, height,
                                  AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    auto sws_frame = av_frame_alloc();
    int image_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, width, height, 1) * sizeof(uint8_t);
    uint8_t *buffer = static_cast<uint8_t *>(av_malloc(image_size));
    av_image_fill_arrays(sws_frame->data, sws_frame->linesize, buffer, AV_PIX_FMT_YUV420P, width, height, 1);
    //SDL
    window = SDL_CreateWindow("WS Video Play SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    sdlRenderer = SDL_CreateRenderer(window, -1, 0);
    auto pixformat = SDL_PIXELFORMAT_IYUV;
    //创建纹理（Texture）
    sdlTexture = SDL_CreateTexture(sdlRenderer, pixformat, SDL_TEXTUREACCESS_STREAMING, width, height);
    //FIX: If window is resize
    sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = width;
    sdlRect.h = height;
    //PCM
    CPcmSpeaker* ps = new CPcmSpeaker();
//    init(audio_codec_ctx->channels,16,audio_codec_ctx->sample_rate);
    int ret = ps->init(audio_codec_ctx->channels,audio_codec_ctx->sample_rate,16);
    SwrContext* swr_ctx = swr_alloc();
    av_opt_set_int(swr_ctx, "in_channel_layout", audio_codec_ctx->channel_layout, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", audio_codec_ctx->channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", audio_codec_ctx->sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", audio_codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_codec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
//    swr_alloc_set_opts(swr_ctx,1,AV_SAMPLE_FMT_U8,8000,audio_codec_ctx->channel_layout,audio_codec_ctx->sample_fmt,audio_codec_ctx->sample_rate,0,0);
//    swr_alloc_set_opts(swr_ctx,1,AV_SAMPLE_FMT_S16,16000,audio_codec_ctx->channel_layout,audio_codec_ctx->sample_fmt,audio_codec_ctx->sample_rate,0,0);

    swr_init(swr_ctx);
    uint8_t * pcm_buffer = (uint8_t *)(malloc(8196));
    SDL_Event event;
    while (1) {
        if (av_read_frame(formatCtx, &pkt) < 0) {
            break;
        }
        if (pkt.stream_index == video_stream_idx) {
            avcodec_send_packet(video_codec_ctx, &pkt);
            if (avcodec_receive_frame(video_codec_ctx, vframe) == 0) {
                int i = sws_scale(sws_ctx, (const uint8_t *const *) (vframe->data), vframe->linesize, 0, vframe->height,
                                  sws_frame->data, sws_frame->linesize);
                av_frame_unref(vframe);
                //设置纹理的数据
                SDL_UpdateTexture(sdlTexture, NULL, sws_frame->data[0], width);
                SDL_RenderClear(sdlRenderer);
                //将纹理的数据拷贝给渲染器
                SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
                //显示
                SDL_RenderPresent(sdlRenderer);
            }
        } else if(pkt.stream_index == audio_stream_idx){
            avcodec_send_packet(audio_codec_ctx, &pkt);
            if (avcodec_receive_frame(audio_codec_ctx, aframe) == 0) {
                int size;
                if (audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16P) {
                    size = av_samples_get_buffer_size(aframe->linesize, audio_codec_ctx->channels,
                                                      audio_codec_ctx->frame_size, audio_codec_ctx->sample_fmt, 1);
                } else {
                    av_samples_get_buffer_size(&size, audio_codec_ctx->channels, audio_codec_ctx->frame_size,
                                               audio_codec_ctx->sample_fmt, 1);
                }
                int i = swr_convert(swr_ctx, &pcm_buffer, size, (uint8_t const **) (aframe->extended_data),
                                    aframe->nb_samples);
                av_frame_unref(aframe);
                ps->toSpeaker(pcm_buffer,size);
            }
        }
        av_packet_unref(&pkt);
        SDL_PollEvent(&event);
        switch( event.type ) {
            case SDL_QUIT:
                break;
            default:
                break;
        }
    }
    SDL_Quit();//退出系统
    exit(0);
}
