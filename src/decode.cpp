//
// Created by Matebook13 on 2020/9/3.
//

#include "decode.h"
#include <iostream>

using std::cerr;
using std::endl;
using std::cout;

static void setup_array(uint8_t* out[SWR_CH_MAX], AVFrame* in_frame, int format){
    if (av_sample_fmt_is_planar((AVSampleFormat)format)){
        int i;
        //int plane_size = av_get_bytes_per_sample((AVSampleFormat)(format & 0xFF)) * samples;
        // format = format & 0xFF;

        //从decoder出来的frame中的data数据不是连续分布的，所以不能这样写：
        //in_frame->data[0]+i*plane_size;
        for (i = 0; i < in_frame->channels; i++){
            out[i] = in_frame->data[i];
        }
    }
    else{
        out[0] = in_frame->data[0];
    }
}

int resample(SwrContext* swr, AVFrame* in, AVFrame* out, ResampleOpt* in_opt, ResampleOpt* out_opt){
    int ret;
    //int64_t dst_nb_samples;
    int64_t src_nb_samples = in->nb_samples;
    out->pts = in->pts;
    if (swr != nullptr){
        out->nb_samples = av_rescale_rnd(swr_get_delay(swr, out_opt->sr) + src_nb_samples,
                                               out_opt->sr, in_opt->sr, AV_ROUND_UP);

        ret = av_samples_alloc(out->data,
                               &out->linesize[0],
                               out_opt->channels,
                               out->nb_samples,
                               out_opt->fmt, 0);

        if (ret < 0) {
            av_log(nullptr, AV_LOG_WARNING,
                   "[%s.%d %s() Could not allocate samples Buffer\n", __FILE__, __LINE__, __FUNCTION__);
            return -1;
        }

        //输入也可能是分平面的，所以要做如下处理
        uint8_t* m_ain[SWR_CH_MAX];
        setup_array(m_ain, in, in_opt->fmt);

        //注意这里，out_count和in_count是samples单位，不是byte
        //所以这样av_get_bytes_per_sample(in_fmt_ctx->streams[audio_index]->codec->sample_fmt) * src_nb_samples是错的
        swr_convert(swr, out->data, out->nb_samples, (const uint8_t**)m_ain, src_nb_samples);
    }
    return 0;
}

int decode(const char* infile, ResampleOpt out_opt, AudioData& audio){
    // declare ffmpeg decoder and params.
    int ret, st_ind;
    AVStream *st;
    AVCodec * dec = nullptr;
    AVCodecContext *dec_ctx = nullptr;
    AVFormatContext *fmt_ctx = nullptr;

    // open video or audio file.
    ret = avformat_open_input(&fmt_ctx, infile, nullptr, nullptr);
    if(ret < 0){
        cerr << "Error code " << ret << ": \n"
             << "when open " << infile << "\n";
        return -1;
    }

    // init decoder.
    ret = open_context(&dec_ctx, fmt_ctx, &st_ind);
    if(ret < 0){
        cerr << "Error code " << ret << ": \n"
             << "when open " << infile << "\n";
        return -2;
    }

    // iterate frames.
    AVPacket* packet;
    AVFrame* fr = av_frame_alloc();

    ResampleOpt in_opt(dec_ctx->sample_rate,
                       dec_ctx->channels,
                       dec_ctx->sample_fmt);
    audio.channels = out_opt.channels;
    audio.sample_rate = out_opt.sr;

    SwrContext * swr;
    if(in_opt == out_opt){
        swr = swr_alloc_set_opts(
                swr,
                dec_ctx->channel_layout,
                out_opt.fmt,
                out_opt.sr,
                dec_ctx->channel_layout,
                dec_ctx->sample_fmt,
                dec_ctx->sample_rate,
                0, nullptr);
        swr_init(swr);
    }

    while(av_read_frame(fmt_ctx, packet) >= 0){
        ret = avcodec_send_packet(dec_ctx, packet);
        while(ret >= 0){
            ret = avcodec_receive_frame(dec_ctx, fr);
            if(ret >= 0){
                if(in_opt == out_opt){
                    for(int i=0; i<dec_ctx->channels; i++){
                        auto* data = (float*)fr->data[i];
                        for(int j=0; j<fr->nb_samples; j++){
                            audio.samples[i].push_back(data[j]);
                        }
                    }
                } else{
                    AVFrame* out_frame = av_frame_alloc();
                    resample(swr, fr, out_frame, &in_opt, &out_opt);
                    for(int i=0; i<dec_ctx->channels; i++){
                        auto* data = (float*)fr->data[i];
                        for(int j=0; j<fr->nb_samples; j++){
                            audio.samples[i].push_back(data[j]);
                        }
                    }
                }
            }
        }
        av_frame_unref(fr);
    }
    av_packet_unref(packet);
}

int open_context(AVCodecContext** ctx, AVFormatContext* fmt_ctx, int* st_ind){
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec;
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr,0);
    if(ret < 0){
        cerr << "Error code " << ret << ": \n"
             << "when find best stream. \n";
        return -1;
    }
    stream_index = ret;
    st = fmt_ctx->streams[*st_ind];
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if(!dec){
        cerr << "Can't find Codec." << endl;
    }
    *ctx = avcodec_alloc_context3(dec);
    if(!*ctx){
        cerr << "Can't open codec context" << endl;
    }
    ret = avcodec_parameters_to_context(*ctx, st->codecpar);
    if(ret < 0){
        cerr << "Can't apply params to context" << endl;
    }
    *st_ind = stream_index;
    return 0;
}

int main(int argc, char** argv){
    if(argc < 2){
        cout << "Usage: ./Decode file_path sample_rate."
             << "Decode given audio file or video file with specific sample rate.";
    }
    const char* infile = argv[1];
    int sample_rate = (int)strtol(argv[2], nullptr, 10);

    ResampleOpt out(sample_rate, 2, AV_SAMPLE_FMT_FLTP);
    AudioData audio;
    decode(infile, out, audio);
}