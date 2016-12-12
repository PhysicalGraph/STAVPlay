// snippets adapted from https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/transcoding.c

#ifndef TRANSCODE_AAC_H_
#define TRANSCODE_AAC_H_


#include <stdio.h>
#include "common.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libavformat/avio.h"

#include "libavcodec/avcodec.h"

#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"

#include "libswresample/swresample.h"
}


/**
 * Convert an error code into a text message.
 * @param error Error code to be converted
 * @return Corresponding error text (not thread-safe)
 */
static const char *get_error_text(const int error) {
	static char error_buffer[255];
	av_strerror(error, error_buffer, sizeof(error_buffer));
	return error_buffer;
}

/** Initialize one data packet for reading or writing. */
static void init_packet(AVPacket *packet) {
	av_init_packet(packet);
	/** Set the packet data and size so that it is recognized as being empty. */
	packet->data = NULL;
	packet->size = 0;
}

/** Initialize one audio frame for reading from the input file */
static int init_input_frame(AVFrame **frame) {
	if (!(*frame = av_frame_alloc())) {
		LOG_ERROR("Could not allocate input frame");
		return AVERROR(ENOMEM);
	}
	return 0;
}

/**
 * Initialize the audio resampler based on the input and output codec settings.
 * If the input and output sample formats differ, a conversion is required
 * libswresample takes care of this, but requires initialization.
 */
static int init_resampler(AVCodecContext *input_codec_context,
                          AVCodecContext *output_codec_context,
                          SwrContext **resample_context) {
	int error;

	/**
	 * Create a resampler context for the conversion.
	 * Set the conversion parameters.
	 * Default channel layouts based on the number of channels
	 * are assumed for simplicity (they are sometimes not detected
	 * properly by the demuxer and/or decoder).
	 */
	*resample_context = swr_alloc_set_opts(NULL,
	                                       av_get_default_channel_layout(output_codec_context->channels),
	                                       output_codec_context->sample_fmt,
	                                       output_codec_context->sample_rate,
	                                       av_get_default_channel_layout(input_codec_context->channels),
	                                       input_codec_context->sample_fmt,
	                                       input_codec_context->sample_rate,
	                                       0, NULL);
	if (!*resample_context) {
		LOG_ERROR("Could not allocate resample context");
		return AVERROR(ENOMEM);
	}
	/**
	 * Perform a sanity check so that the number of converted samples is
	 * not greater than the number of samples to be converted.
	 * If the sample rates differ, this case has to be handled differently
	 */
	av_assert0(output_codec_context->sample_rate == input_codec_context->sample_rate);

	/** Open the resampler with the specified parameters. */
	if ((error = swr_init(*resample_context)) < 0) {
		LOG_ERROR("Could not open resample context");
		swr_free(resample_context);
		return error;
	}
	return 0;
}

/** Initialize a FIFO buffer for the audio samples to be encoded. */
static int init_fifo(AVAudioFifo **fifo, AVCodecContext *output_codec_context) {
	/** Create the FIFO buffer based on the specified output sample format. */
	if (!(*fifo = av_audio_fifo_alloc(output_codec_context->sample_fmt,
	                                  output_codec_context->channels, 1))) {
		LOG_ERROR("Could not allocate FIFO");
		return AVERROR(ENOMEM);
	}
	return 0;
}

/**
 * Initialize a temporary storage for the specified number of audio samples.
 * The conversion requires temporary storage due to the different format.
 * The number of audio samples to be allocated is specified in frame_size.
 */
static int init_converted_samples(uint8_t ***converted_input_samples,
                                  AVCodecContext *output_codec_context,
                                  int frame_size) {
	int error;

	/**
	 * Allocate as many pointers as there are audio channels.
	 * Each pointer will later point to the audio samples of the corresponding
	 * channels (although it may be NULL for interleaved formats).
	 */
	if (!(*converted_input_samples = (uint8_t **)calloc(output_codec_context->channels,
	                                 sizeof(**converted_input_samples)))) {
		LOG_ERROR("Could not allocate converted input sample pointers");
		return AVERROR(ENOMEM);
	}

	/**
	 * Allocate memory for the samples of all channels in one consecutive
	 * block for convenience.
	 */
	if ((error = av_samples_alloc(*converted_input_samples, NULL,
	                              output_codec_context->channels,
	                              frame_size,
	                              output_codec_context->sample_fmt, 0)) < 0) {
		LOG_ERROR("Could not allocate converted input samples (error '%s')",
		          get_error_text(error));
		av_freep(&(*converted_input_samples)[0]);
		free(*converted_input_samples);
		return error;
	}
	return 0;
}

/**
 * Convert the input audio samples into the output sample format.
 * The conversion happens on a per-frame basis, the size of which is specified
 * by frame_size.
 */
static int convert_samples(const AVFrame *input_frame, uint8_t **converted_data,
		                   SwrContext *resample_context, AVSampleFormat avformat, bool isMute) {
	int error;

	const uint8_t **input_data = (const uint8_t **)input_frame->extended_data;
	const int frame_size = (const int)input_frame->nb_samples;
	if (isMute==true) {
		if ((error = av_samples_set_silence(converted_data, 0,
				                       input_frame->nb_samples, input_frame->channels,
			                           avformat)) < 0) {
			LOG_ERROR("Set silence frame failed",
			          get_error_text(error));
		}
	} else {
	/** Convert the samples using the resampler. */
	if ((error = swr_convert(resample_context,
	                         converted_data, frame_size,
	                         input_data    , frame_size)) < 0) {
		LOG_ERROR("Could not convert input samples (error '%s')",
		          get_error_text(error));
		return error;
		}
	}

	return 0;
}

/** Add converted input audio samples to the FIFO buffer for later processing. */
static int add_samples_to_fifo(AVAudioFifo *fifo, uint8_t **converted_input_samples,
                               const int frame_size) {
	int error;

	/**
	 * Make the FIFO as large as it needs to be to hold both,
	 * the old and the new samples.
	 */
	if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0) {
		LOG_ERROR("Could not reallocate FIFO");
		return error;
	}

	/** Store the new samples in the FIFO buffer. */
	if (av_audio_fifo_write(fifo, (void **)converted_input_samples,
	                        frame_size) < frame_size) {
		LOG_ERROR("Could not write data to FIFO");
		return AVERROR_EXIT;
	}
	av_freep(&(converted_input_samples)[0]);
	free(converted_input_samples);
	return 0;
}

/**
 * Initialize one input frame for writing to the output file.
 * The frame will be exactly frame_size samples large.
 */
static int init_output_frame(AVFrame **frame, AVCodecContext *output_codec_context,
                             int frame_size) {
	int error;

	/** Create a new frame to store the audio samples. */
	if (!(*frame = av_frame_alloc())) {
		LOG_ERROR("Could not allocate output frame");
		return AVERROR_EXIT;
	}

	/**
	 * Set the frame's parameters, especially its size and format.
	 * av_frame_get_buffer needs this to allocate memory for the
	 * audio samples of the frame.
	 * Default channel layouts based on the number of channels
	 * are assumed for simplicity.
	 */

	(*frame)->nb_samples     = frame_size;
	(*frame)->channels       = output_codec_context->channels;
	(*frame)->channel_layout = av_get_default_channel_layout((*frame)->channels);
	(*frame)->format         = output_codec_context->sample_fmt;
	(*frame)->sample_rate    = output_codec_context->sample_rate;

	/**
	 * Allocate the samples of the created frame. This call will make
	 * sure that the audio frame can hold as many samples as specified.
	 */
	if ((error = av_frame_get_buffer(*frame, 0)) < 0) {
		LOG_ERROR("Could not allocate output frame samples (error '%s')",
		          get_error_text(error));
		av_frame_free(frame);
		return error;
	}

	return 0;
}

static int flush_encoder(AVCodecContext *avctx) {
	int ret;
	AVPacket pkt;
	init_packet(&pkt);

	ret = avcodec_send_frame(avctx, NULL);  // enter draining mode
	if (ret < 0) {
		av_packet_unref(&pkt);
		return ret;
	}

	while (true) {
		ret = avcodec_receive_packet(avctx, &pkt);
		if (ret == AVERROR_EOF) {
			break;
		} else if (ret < 0) {
			av_packet_unref(&pkt);
			return ret;
		}
	}
	av_packet_unref(&pkt);
	avcodec_flush_buffers(avctx);
	return 0;
}

static int flush_decoder(AVCodecContext *avctx) {
	int ret;
	AVFrame *input_frame;
	init_input_frame(&input_frame);

	ret = avcodec_send_packet(avctx, NULL); // enter draining mode
	if (ret < 0) {
		av_frame_free(&input_frame);
		return ret;
	}

	while (true) {
		ret = avcodec_receive_frame(avctx, input_frame);
		if (ret == AVERROR_EOF) {
			break;
		} else if (ret < 0) {
			av_frame_free(&input_frame);
			return ret;
		}
	}
	av_frame_free(&input_frame);
	avcodec_flush_buffers(avctx);
	return 0;
}

// https://blogs.gentoo.org/lu_zero/tag/api/
int encode(AVCodecContext *avctx, AVPacket *pkt, int *got_packet, AVFrame *frame) {
	int ret;

	*got_packet = 0;

	ret = avcodec_send_frame(avctx, frame);
	if (ret < 0) {
		return ret;
	}

	ret = avcodec_receive_packet(avctx, pkt);
	if (!ret) {
		*got_packet = 1;
	}
	if (ret == AVERROR(EAGAIN)) {
		return 0;
	}

	return ret;
}

int decode(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt) {
	int ret;

	*got_frame = 0;

	if (pkt) {
		ret = avcodec_send_packet(avctx, pkt);
		// In particular, we don't expect AVERROR(EAGAIN), because we read all
		// decoded frames with avcodec_receive_frame() until done.
		if (ret < 0) {
			fprintf(stderr, "avcodec_send_packet %s\n", av_err2str(ret));
			return ret == AVERROR_EOF ? 0 : ret;
		}
	}

	ret = avcodec_receive_frame(avctx, frame);
	if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
		fprintf(stderr, "avcodec_receive_frame %s\n", av_err2str(ret));
		return ret;
	}
	if (ret >= 0) {
		*got_frame = 1;
	}

	return 0;
}

static int init_transcoder(AVCodecParameters *codecpar, AVCodecContext** in_ctx,
                           AVCodecContext** out_ctx,
                           SwrContext** resample_ctx,bool is_transcode_) {
	int ret = 0;
	AVCodecContext *in_codec_ctx, *out_codec_ctx;

	LOG_INFO("Setup decoder");
	AVCodec *in_codec = avcodec_find_decoder(codecpar->codec_id);
	if (in_codec == NULL) {
		LOG_ERROR("No decoder found");
		return -1;
	}

	in_codec_ctx = avcodec_alloc_context3(in_codec);
	in_codec_ctx->profile        = codecpar->profile;
	in_codec_ctx->channels       = codecpar->channels;
	in_codec_ctx->sample_rate    = codecpar->sample_rate;
	in_codec_ctx->channel_layout = codecpar->channel_layout;
	in_codec_ctx->sample_fmt     = (AVSampleFormat)codecpar->format;
	in_codec_ctx->bit_rate       = codecpar->bit_rate;

	char in_layout_str[16];
	av_get_channel_layout_string(in_layout_str, sizeof(in_layout_str), -1,
	                             in_codec_ctx->channel_layout);

	LOG_INFO("IN codec: %s", in_codec->long_name);
	LOG_INFO("IN sample_fmt: %s", av_get_sample_fmt_name(in_codec_ctx->sample_fmt));
	LOG_INFO("IN sample_rate: %d", in_codec_ctx->sample_rate);
	LOG_INFO("IN channel_layout: %s", in_layout_str);
	LOG_INFO("IN channels: %d", in_codec_ctx->channels);
	LOG_INFO("IN bit_rate: %d", in_codec_ctx->bit_rate);
	LOG_INFO("IN bits_per_channel: %d", codecpar->bits_per_raw_sample / in_codec_ctx->channels);

	ret = avcodec_open2(in_codec_ctx, in_codec, NULL);
	if (ret < 0) {
		LOG_ERROR("Failed to open decoder %s", get_error_text(ret));
		return ret;
	}

	*in_ctx = in_codec_ctx;

	LOG_INFO("Setup encoder");
	AVCodec *out_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!out_codec) {
		LOG_ERROR("No encoder found for AAC");
		return -1;
	}
    if(is_transcode_){
	out_codec_ctx = avcodec_alloc_context3(out_codec);
	out_codec_ctx->profile        = FF_PROFILE_AAC_LOW;
	out_codec_ctx->channels       = 1;
	out_codec_ctx->sample_rate    = in_codec_ctx->sample_rate ;
	out_codec_ctx->channel_layout = av_get_default_channel_layout(out_codec_ctx->channels);
	out_codec_ctx->sample_fmt     = out_codec->sample_fmts[0];
	out_codec_ctx->bit_rate       = in_codec_ctx->bit_rate;
    }
    else{
    	out_codec_ctx = avcodec_alloc_context3(out_codec);
    	out_codec_ctx->profile        = codecpar->profile;
    	out_codec_ctx->channels       = codecpar->channels;
    	out_codec_ctx->sample_rate    = codecpar->sample_rate ;
    	out_codec_ctx->channel_layout = codecpar->channel_layout;
    	out_codec_ctx->sample_fmt     = (AVSampleFormat)codecpar->format;
    	out_codec_ctx->bit_rate       = codecpar->bit_rate;
    }
	char out_layout_str[16];
	av_get_channel_layout_string(out_layout_str, sizeof(out_layout_str), -1,
	                             out_codec_ctx->channel_layout);

	LOG_INFO("OUT codec: %s", out_codec->long_name);
	LOG_INFO("OUT profile: %s", av_get_profile_name(out_codec, out_codec_ctx->profile));
	LOG_INFO("OUT sample_fmt: %s", av_get_sample_fmt_name(out_codec_ctx->sample_fmt));
	LOG_INFO("OUT sample_rate: %d", out_codec_ctx->sample_rate);
	LOG_INFO("OUT channel_layout: %s", out_layout_str);
	LOG_INFO("OUT channels: %d", out_codec_ctx->channels);
	LOG_INFO("OUT bit_rate: %d", out_codec_ctx->bit_rate);
	LOG_INFO("OUT bits_per_channel: %d", out_codec_ctx->bit_rate / out_codec_ctx->sample_rate);

	ret = avcodec_open2(out_codec_ctx, out_codec, NULL);
	if (ret < 0) {
		LOG_ERROR("Failed to open encoder %s", get_error_text(ret));
		return ret;
	}

	*out_ctx = out_codec_ctx;

	// Initialize the resampler to be able to convert audio sample formats
	init_resampler(in_codec_ctx, out_codec_ctx, resample_ctx);
	return ret;
}
#endif
