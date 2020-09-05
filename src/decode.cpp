//
// Created by fhq on 2020/9/1.
//


#include "decode.h"
#include <iostream>
#include <glog/logging.h>

using std::cerr;
using std::endl;
using std::cout;

static int decode_packet(AVPacket* pkt, AVCodecContext** dec_ctx, int stream_idx,
                         AudioData* audioData, SwrContext* swr, ResampleOpt* in_opt, ResampleOpt* out_opt) {
    int ret;
    int data_size ;
    int decoded = pkt->size;
    AVFrame* out_fr = av_frame_alloc();
    AVFrame* frame = av_frame_alloc();

    if (pkt->stream_index == stream_idx) {
        // decode audio frame
        // send the packet with the compressed data to the decoder
            ret = avcodec_send_packet(*dec_ctx, pkt);
        if (ret < 0) {
            LOG(ERROR) << "Error Code [" << ret << "]" << endl
                       << "error submitting the packet to the decoder.";
        }

        // read all the output frames (in general there may be any number of them
        while (ret >= 0) {
            ret = avcodec_receive_frame(*dec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                return ret;
            else if (ret < 0) {
                LOG(ERROR) << "Error during decoding" << endl;
                return -1;
            }
            data_size = av_get_bytes_per_sample((*dec_ctx)->sample_fmt);
            if (data_size < 0) {
                LOG(ERROR) << "Failed to calculate data size." << endl;
                return -1;
            }

            if(in_opt != out_opt){
                resample(swr, frame, out_fr, in_opt, out_opt);
                for (int ch = 0; ch < out_opt->channels; ch++) {
                    auto* data_per_channel = (float*) out_fr->data[ch];
                    for (int i = 0; i < out_fr->nb_samples; i++){
                        audioData->samples[ch].push_back(data_per_channel[i]);
                    }
                }
                audioData->nb_samples += out_fr->nb_samples;
            } else {
                for (int ch = 0; ch < (*dec_ctx)->channels; ch++) {
                    auto *data = (float *) frame->data[ch];
                    for (int i = 0; i < frame->nb_samples; i++)
                        audioData->samples[ch].push_back(data[i]);
                }
                audioData->nb_samples += frame->nb_samples;
            }
        }
    }
    av_frame_unref(out_fr);
    return decoded;
}

static void setup_array(uint8_t* out[SWR_CH_MAX], AVFrame* in_frame, int format){
    if (av_sample_fmt_is_planar((AVSampleFormat)format)){
        for (int i = 0; i < in_frame->channels; i++){
            out[i] = in_frame->data[i];
        }
    }
    else{
        out[0] = in_frame->data[0];
    }
}

int resample(SwrContext* swr, AVFrame* in, AVFrame* out, ResampleOpt* in_opt, ResampleOpt* out_opt){
    int ret;
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
        if(ret < 0){
            LOG(ERROR) << "Error to alloc samples." << endl;
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
    AVCodecContext *dec_ctx = nullptr;
    AVFormatContext *fmt_ctx = nullptr;

    // open video or audio file.
    ret = avformat_open_input(&fmt_ctx, infile, nullptr, nullptr);
    if(ret < 0){
        LOG(ERROR) << "Error code " << ret << ": \n"
                   << "when open " << infile << "\n";
        return -1;
    }
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if(ret < 0){
        LOG(ERROR) << "Error code " << ret << ": \n"
                   << "when get stream of" << infile << "\n";
        return -1;
    }
    // init decoder.
    ret = open_codec_context(&st_ind, &dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO);
    if(ret < 0){
        cerr << "Error code " << ret << ": \n"
             << "when open " << infile << "\n";
        return -2;
    }

    ResampleOpt in_opt(dec_ctx->sample_rate,
                       dec_ctx->channels,
                       dec_ctx->sample_fmt);
    audio.channels = out_opt.channels;
    audio.sample_rate = out_opt.sr;

    SwrContext * swr;
    if(in_opt != out_opt){
        swr = swr_alloc();
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
    // iterate frames.
    AVPacket pkt;
    AVFrame* fr = av_frame_alloc();
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;
    // read all the output frames (in general there may be any number of them
    for(int i = 0; i < out_opt.channels; i++){
        audio.samples.emplace_back();
    }
    while(av_read_frame(fmt_ctx, &pkt) >= 0){
        AVPacket orig_pkt = pkt;
        while(pkt.size > 0){
            ret = decode_packet(&pkt, &dec_ctx, st_ind,
                                &audio, swr, &in_opt, &out_opt);
            if(ret < 0) break;
            pkt.data += ret;
            pkt.size -= ret;
        }
        av_packet_unref(&orig_pkt);
    }
    av_packet_unref(&pkt);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&fr);
    audio.duration = (double) audio.nb_samples / (double)audio.sample_rate;
    return 0;
}

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx,
                              AVFormatContext *fmt_ctx, enum AVMediaType type){
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec;
    AVDictionary *opts = nullptr;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
    if (ret < 0) {
        LOG(ERROR) << "Could not find stream in input file" << endl;
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            LOG(ERROR) << "Failed to find codec\n";
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            LOG(ERROR) << "Failed to allocate the codec context\n";
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            LOG(ERROR) << "Failed to copy %s codec parameters to decoder context\n";
            return ret;
        }

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            LOG(ERROR) << "Failed to open codec\n";
            return ret;
        }
        *stream_idx = stream_index;
    }
    return 0;
}

int main(int argc, char** argv){
    if(argc < 2){
        LOG(INFO) << "Usage: ./Decode file_path sample_rate."
                  << "Decode given audio file or video file with specific sample rate.";
    }
    const char* infile = argv[1];
    int sample_rate = (int)strtol(argv[2], nullptr, 10);

    ResampleOpt out(sample_rate, 2, AV_SAMPLE_FMT_FLTP);
    AudioData audio;
    decode(infile, out, audio);
    LOG(INFO)
        << "\nsample rate:         " << audio.sample_rate << endl
        << "number samples:      " << audio.nb_samples << endl
        << "channels:            " << audio.channels << endl
        << "duration:            " << audio.duration << " [sec]" << endl
        << "sample in 1.ch 60000 " << audio.samples[0][60000] << endl;
    return 0;
}
