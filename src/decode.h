//
// Created by Matebook13 on 2020/9/3.
//

#ifndef AUDIODECODER_DECODE_H
#define AUDIODECODER_DECODE_H

#include <vector>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libswresample/swresample.h>
#define SWR_CH_MAX 16
}

class ResampleOpt{
public:
    int sr;
    int channels;
    AVSampleFormat fmt;
    ResampleOpt(int s, int c, AVSampleFormat f){
        sr = s;
        channels = c;
        fmt = f;
    };
    bool operator==(ResampleOpt& a) const{
        if(a.channels == this->channels&&a.sr == this->sr&&a.fmt == this->fmt) return true;
        return false;
    }
};

class AudioData{
public:
    std::vector<std::vector<float>> samples;
    int sample_rate;
    int channels;
    int nb_samples;
    double duration;
};

/*/
 * resample a frame to given resample opt.
 */
int resample(SwrContext* swr, AVFrame* in, AVFrame* out, ResampleOpt* in_opt, ResampleOpt* out_opt);

/*
 * open audio file using ffmpeg AVCodec tool.
 */
int open_context(AVCodecContext** cCtx, AVFormatContext* fmt_ctx, int* st_ind);

int decode(const char* infile, ResampleOpt out_opt);


#endif //AUDIODECODER_DECODE_H
