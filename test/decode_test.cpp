//
// Created by fhq on 2020/9/6.
//

#include <gtest/gtest.h>
#include <iostream>
#include "decode.h"
#include "stft.h"

using std::string;


TEST(DecodeTest, NBSampleTest) {
    const char* infile = "/home/fhq/code/AudioDecoder/exp/test.mp3";
    ResampleOpt out(16000, 2, AV_SAMPLE_FMT_FLTP);
    AudioData audio_1;
    decode(infile, out, audio_1);
    EXPECT_EQ(audio_1.nb_samples, 5326326);
    EXPECT_NEAR(audio_1.duration, 332.895, 0.001);
}

TEST(DecodeTest, ResampleValueTest){
    const char* infile = "/home/fhq/code/AudioDecoder/exp/test.mp3";
    ResampleOpt out(16000, 2, AV_SAMPLE_FMT_FLTP);
    AudioData audio_2;
    decode(infile, out, audio_2);
    // calculate by python Librosa library.
    EXPECT_NEAR(audio_2.samples[0][60000], -0.08549299, 0.0001);
}

TEST(DecodeTest, DecodeValueTest){
    const char* infile = "/home/fhq/code/AudioDecoder/exp/test.mp3";
    ResampleOpt out(44100, 2, AV_SAMPLE_FMT_FLTP);
    AudioData audio;
    decode(infile, out, audio);
    // calculate by python Librosa library.
    EXPECT_NEAR(audio.samples[0][60000], -0.0047302246, 0.00001);
}

TEST(StftTest, StftTest){
    std::vector<float> data(32);
    for(int i=0; i<32; i++){
        data[i] = i;
    }
    AmpSpectrum amp;
    STFT(data, amp, 4, 8);
    EXPECT_EQ(amp.getFrameNum(), 8);  // sample / hop
    EXPECT_EQ(amp.getFrameSize(), 5); // n / 2 + 1
    EXPECT_NEAR(amp.data[0][0], 13.51, 0.001);
    EXPECT_NEAR(amp.data[0][2], 1.014, 0.001);
    EXPECT_NEAR(amp.data[1][4], 0.2666, 0.001);
    EXPECT_NEAR(amp.data[2][2], 2.378, 0.001);
    EXPECT_NEAR(amp.data[4][0], 75.27, 0.001);
}

int main(int argc, char **argv) {
    av_log_set_level(AV_LOG_ERROR);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}