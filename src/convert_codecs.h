/*!
 * convert_codecs.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Jacob Tarasiewicz
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_CONVERT_CODECS_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_CONVERT_CODECS_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

#include "common.h"
#include "stream_demuxer.h"

Samsung::NaClPlayer::AudioCodec_Type ConvertAudioCodec(AVCodecID codec);
Samsung::NaClPlayer::SampleFormat ConvertSampleFormat(AVSampleFormat format);
Samsung::NaClPlayer::ChannelLayout ChannelLayoutFromChannelCount(int channels);
Samsung::NaClPlayer::ChannelLayout ConvertChannelLayout(uint64_t layout,
                                                        int channels);
Samsung::NaClPlayer::AudioCodec_Profile ConvertAACAudioCodecProfile(int profile);
Samsung::NaClPlayer::VideoCodec_Type ConvertVideoCodec(AVCodecID codec);
Samsung::NaClPlayer::VideoCodec_Profile ConvertH264VideoCodecProfile(int profile);
Samsung::NaClPlayer::VideoCodec_Profile ConvertMPEG2VideoCodecProfile(int profile);
Samsung::NaClPlayer::VideoFrame_Format ConvertVideoFrameFormat(int format);

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_CONVERT_CODECS_H_
