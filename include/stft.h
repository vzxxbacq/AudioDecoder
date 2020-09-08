//
// Created by Matebook13 on 2020/9/8.
//

#ifndef AUDIODECODER_STFT_H
#define AUDIODECODER_STFT_H

#include <vector>
#include <cmath>
#include <fftw3.h>

class AmpSpectrum{
private:
    int frame_num;
    int frame_size;
    int hop_length;
public:
    std::vector<std::vector<double>> data;
    int getFrameNum() const{
        return frame_num;
    }
    int getFrameSize() const{
        return frame_size;
    }
    AmpSpectrum():frame_num(0), frame_size(0), hop_length(0){};

    AmpSpectrum(int frame_num, int frame_size, int hop_length):
        frame_num(frame_num), frame_size(frame_size), hop_length(hop_length){
        for(int i = 0; i < frame_num; i++){
            data.emplace_back();
            for(int j = 0; j < frame_size; j++){
                data[i].emplace_back();
            }
        };
    };
};


void hamming(float* buffer, int window_len);

void STFT(std::vector<float>& audio, AmpSpectrum& res, int hop_length, int frame_length);

#endif //AUDIODECODER_STFT_H