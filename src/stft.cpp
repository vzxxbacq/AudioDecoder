//
// Created by Matebook13 on 2020/9/8.
//

#include "decode.h"
#include "stft.h"
#include <fftw3.h>


void hamming(float* buffer, int window_len){

    for(int i=0; i < window_len; i++){
        buffer[i] = (float)(0.54 - (0.46 * cos(2 * M_PI * (i / (((double)window_len-1) * 1.0)))));
    }
}


void STFT(std::vector<float>& audio, AmpSpectrum& res, int hop_length, int frame_length){
    // padding.
    audio.resize(audio.size() + (hop_length - audio.size() % hop_length) % hop_length, 0);

    int frame_num = (int) audio.size() / hop_length;

    double * data;
    fftw_complex *fftw_res;
    data = (double*) malloc(sizeof(double) * frame_length);
    fftw_res = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * frame_length);
    fftw_plan plan_forward = fftw_plan_dft_r2c_1d(frame_length, data, fftw_res, FFTW_MEASURE);
    // create hamming window and frame buffer.
    std::vector<float> window(frame_length);
    hamming(window.data(), window.size());
    AmpSpectrum amp(frame_num, frame_length / 2 + 1, hop_length);
    for(int i=0; i<frame_num; i++){
        // apply window.
        int offset = i * hop_length;
        float* aud_buf = audio.data() + offset;
        for(int j=0; j<frame_length; j++) {
            data[j] = (double)aud_buf[j] * window[j];
        }
        fftw_execute(plan_forward);
        for(int j=0; j<frame_length/2 + 1; j++){
            amp.data[i][j] = sqrt(fftw_res[j][0] * fftw_res[j][0] + fftw_res[j][1] * fftw_res[j][1]);
        }
    }
    res = std::move(amp);
}