#include <functional>
#include <limits>
#include <utility>
#include <sys/time.h>

#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/instance.h"
#include "nacl_player/error_codes.h"
#include "nacl_player/es_data_source.h"
#include "nacl_player/elementary_stream_listener.h"
#include "rtsp_player_controller.h"
#include "transcode_utils.h"

using Samsung::NaClPlayer::ErrorCodes;
using Samsung::NaClPlayer::ESDataSource;
using Samsung::NaClPlayer::MediaDataSource;
using Samsung::NaClPlayer::MediaPlayer;
using Samsung::NaClPlayer::MediaPlayerState;
using Samsung::NaClPlayer::Rect;
using Samsung::NaClPlayer::TimeTicks;
using Samsung::NaClPlayer::ESPacket;
using Samsung::NaClPlayer::Rational;
using Samsung::NaClPlayer::Size;
using std::make_shared;
using std::placeholders::_1;
using std::shared_ptr;
using std::unique_ptr;
using pp::AutoLock;


static const uint32_t kMicrosecondsPerSecond = 1000000;
static const uint32_t kVideoStreamProbeSize = 32;
static const TimeTicks kOneMicrosecond = 1.0 / kMicrosecondsPerSecond;
static const AVRational kMicrosBase = {1, kMicrosecondsPerSecond};

static TimeTicks ToTimeTicks(int64_t time_ticks, AVRational time_base) {
	int64_t us = av_rescale_q(time_ticks, time_base, kMicrosBase);
	return us * kOneMicrosecond;
}

static inline uint64_t tv2ms(struct timeval *tv) {
	return ((tv->tv_sec * (uint64_t)1000) + (tv->tv_usec / 1000));
}

static inline uint64_t nowms() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv2ms(&tv);
}

enum MediaType { kVideoType, kAudioType };
static pp::Lock packets_lock_;
static pp::Lock mute_lock_;

class ESListener : public Samsung::NaClPlayer::ElementaryStreamListener {
	public:
		ESListener(RTSPPlayerController *controller, MediaType media_type) {
			controller_ = controller;
			media_type_ = media_type;
		}

		void OnNeedData(int32_t bytes_max) {
			AutoLock critical_section(packets_lock_);
			if (media_type_ == kVideoType) {
				controller_->need_video_data_ = true;
			} else if (media_type_ == kAudioType) {
				controller_->need_audio_data_ = true;
			}
		}

		void OnEnoughData() {
			AutoLock critical_section(packets_lock_);
			if (media_type_ == kVideoType) {
				controller_->need_video_data_ = false;
			} else if (media_type_ == kAudioType) {
				controller_->need_audio_data_ = false;
			}
		}

		void OnSeekData(Samsung::NaClPlayer::TimeTicks new_position) {
			//
		}
	private:
		RTSPPlayerController* controller_;
		MediaType media_type_;
};

void av_log_callback(void *ptr, int level, const char *fmt, va_list vargs) {
	if (level <= AV_LOG_VERBOSE) {
		char buff[256];
		vsnprintf(buff, 256, fmt, vargs);
		LOG_INFO(buff);
	}
}

void RTSPPlayerController::InitPlayer(const std::string& url, const double& audio_level_cb_frequency,
                                      const std::string& crt_path) {
	LOG_INFO("Loading media from: '%s'", url.c_str());
	CleanPlayer();

	player_ = make_shared<MediaPlayer>();
	listeners_.player_listener =
	    make_shared<MediaPlayerListener>(message_sender_);
	listeners_.buffering_listener =
	    make_shared<MediaBufferingListener>(message_sender_, shared_from_this());

	player_->SetMediaEventsListener(listeners_.player_listener);
	player_->SetBufferingListener(listeners_.buffering_listener);
	int32_t ret = player_->SetDisplayRect(view_rect_);

	if (ret != ErrorCodes::Success) {
		LOG_ERROR("Failed to set display rect [(%d - %d) (%d - %d)], code: %d",
		          view_rect_.x(), view_rect_.y(), view_rect_.width(), view_rect_.height(),
		          ret);
	}

	player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
	parser_thread_ = MakeUnique<pp::SimpleThread>(instance_);
	player_thread_->Start();

	// create media data source
	auto es_data_source = std::make_shared<ESDataSource>();
	data_source_ = es_data_source;

	need_video_data_ = false;
	need_audio_data_ = false;
	audio_level_cb_frequency_ = audio_level_cb_frequency;

	player_thread_->message_loop().PostWork(cc_factory_.NewCallback(
		&RTSPPlayerController::InitializeStreams, url, crt_path));
}

void RTSPPlayerController::UpdateAudioConfig() {
	//Hardcode settings to AAC
	audio_config_.codec_type = Samsung::NaClPlayer::AUDIOCODEC_TYPE_AAC;
	audio_config_.codec_profile = Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_LOW;
	audio_config_.channel_layout = Samsung::NaClPlayer::CHANNEL_LAYOUT_MONO;
	audio_config_.sample_format = Samsung::NaClPlayer::SAMPLEFORMAT_PLANARF32;
	audio_config_.bits_per_channel = 6; // bitrate divided by sample rate
	audio_config_.samples_per_second = 8000;

	LOG_INFO("audio configuration - codec: %d, profile: %d, sample_format: %d,"
	         " bits_per_channel: %d, channel_layout: %d, samples_per_second: %d, extras:%d",
	         audio_config_.codec_type, audio_config_.codec_profile,
	         audio_config_.sample_format, audio_config_.bits_per_channel,
	         audio_config_.channel_layout, audio_config_.samples_per_second, audio_config_.extra_data.size());

	LOG_INFO("audio configuration updated");
}
void RTSPPlayerController::UpdateVideoConfig() {
	AVStream* s = format_context_->streams[video_stream_idx_];

	video_config_.codec_type = ConvertVideoCodec(s->codecpar->codec_id);
	switch (video_config_.codec_type) {
		case Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP8:
			video_config_.codec_profile =
			    Samsung::NaClPlayer::VIDEOCODEC_PROFILE_VP8_MAIN;
			break;
		case Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP9:
			video_config_.codec_profile =
			    Samsung::NaClPlayer::VIDEOCODEC_PROFILE_VP9_MAIN;
			break;
		case Samsung::NaClPlayer::VIDEOCODEC_TYPE_H264:
			video_config_.codec_profile = ConvertH264VideoCodecProfile(s->codecpar->profile);
			break;
		case Samsung::NaClPlayer::VIDEOCODEC_TYPE_MPEG2:
			video_config_.codec_profile = ConvertMPEG2VideoCodecProfile(s->codecpar->profile);
			break;
		default:
			video_config_.codec_profile = Samsung::NaClPlayer::VIDEOCODEC_PROFILE_UNKNOWN;
	}

	video_config_.frame_format = ConvertVideoFrameFormat(s->codecpar->format);

	AVDictionaryEntry* webm_alpha = av_dict_get(s->metadata, "alpha_mode", NULL, 0);
	if (webm_alpha && !strcmp(webm_alpha->value, "1"))
		video_config_.frame_format = Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12A;

	video_config_.size = Size(s->codecpar->width, s->codecpar->height);

	LOG_INFO("r_frame_rate %d. %d#", s->r_frame_rate.num, s->r_frame_rate.den);
	video_config_.frame_rate = Rational(s->r_frame_rate.num, s->r_frame_rate.den);

	if (s->codecpar->extradata_size > 0) {
		video_config_.extra_data.assign(
		    s->codecpar->extradata, s->codecpar->extradata + s->codecpar->extradata_size);
	}

	char fourcc[20];
	av_get_codec_tag_string(fourcc, sizeof(fourcc), s->codecpar->codec_tag);
	LOG_INFO("video configuration - codec: %d, profile: %d, codec_tag: (%s), "
	         "frame: %d, visible_rect: %d %d ",
	         video_config_.codec_type, video_config_.codec_profile, fourcc,
	         video_config_.frame_format, video_config_.size.width,
	         video_config_.size.height);

	LOG_INFO("video configuration updated");
}

void RTSPPlayerController::InitializeStreams(int32_t, const std::string& url,
                                             const std::string& crt_path) {
	// add ElementaryStreamListeners
	std::shared_ptr<ESListener> video_listener = std::make_shared<ESListener>(this, kVideoType);
	video_stream_ = std::make_shared<Samsung::NaClPlayer::VideoElementaryStream>();
	data_source_->AddStream(*video_stream_, video_listener);

	std::shared_ptr<ESListener> audio_listener = std::make_shared<ESListener>(this, kAudioType);
	audio_stream_ = std::make_shared<Samsung::NaClPlayer::AudioElementaryStream>();
	data_source_->AddStream(*audio_stream_, audio_listener);

	// init ffmpeg
	format_context_ = avformat_alloc_context();

	av_log_set_level(AV_LOG_VERBOSE);
	av_log_set_callback(av_log_callback);

	// TODO: Is it safe to call this multiple times?
	av_register_all();
	avformat_network_init();

	format_context_->probesize = kVideoStreamProbeSize;

	LOG_INFO("avformat_open_input");

	AVDictionary *opts = 0;
	av_dict_set(&opts, "rtsp_transport", "tcp", 0);

	if (strncmp(url.c_str(), "rtsps", strlen("rtsps")) == 0) {
		LOG_DEBUG("RTSPS protocol.");
		av_dict_set(&opts, "ca_file", ("/http/" + crt_path).c_str(), 0);
		av_dict_set(&opts, "tls_verify", "1", 0);
	}
	int ret = avformat_open_input(&format_context_, url.c_str(), NULL, &opts);
	av_dict_free(&opts);

	if (ret < 0) {
		LOG_ERROR("input not opened, result: %s", get_error_text(ret));
	} else {
		LOG_INFO("input successfully opened");
	}

	ret = avformat_find_stream_info(format_context_, NULL);
	if (ret < 0) {
		LOG_ERROR("Cannot find stream info: %s", get_error_text(ret));
	} else {
		LOG_INFO("Got stream info: %d", format_context_->nb_streams);
	}

	video_stream_idx_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	audio_stream_idx_ = av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

	if (video_stream_idx_ >= 0) {
		LOG_INFO("video index: %d", video_stream_idx_);
		UpdateVideoConfig();


		// fill video_config with proper information
		video_stream_->SetVideoCodecType(video_config_.codec_type);
		video_stream_->SetVideoCodecProfile(video_config_.codec_profile);
		video_stream_->SetVideoFrameFormat(video_config_.frame_format);
		video_stream_->SetVideoFrameSize(video_config_.size);
		video_stream_->SetFrameRate(video_config_.frame_rate);
		video_stream_->SetCodecExtraData(video_config_.extra_data.size(),
		                                 &video_config_.extra_data.front());
		video_stream_->InitializeDone();
	}

	if (audio_stream_idx_ >= 0) {
		LOG_INFO("audio index: %d", audio_stream_idx_);
		UpdateAudioConfig();
		// fill audio_config with proper information
		audio_stream_->SetAudioCodecType(audio_config_.codec_type);
		audio_stream_->SetAudioCodecProfile(audio_config_.codec_profile);
		audio_stream_->SetSampleFormat(audio_config_.sample_format);
		audio_stream_->SetChannelLayout(audio_config_.channel_layout);
		audio_stream_->SetBitsPerChannel(audio_config_.bits_per_channel);
		audio_stream_->SetSamplesPerSecond(audio_config_.samples_per_second);
		//audio_stream_->SetCodecExtraData(audio_config_.extra_data.size(),
		//                             &audio_config_.extra_data.front());
		audio_stream_->InitializeDone();
	}

	FinishStreamConfiguration();

	parser_thread_->Start();
	parser_thread_->message_loop().PostWork(
	    cc_factory_.NewCallback(&RTSPPlayerController::StartParsing));
}

void RTSPPlayerController::Play() {
	int32_t ret = player_->Play();
	if (ret == ErrorCodes::Success) {
		LOG_INFO("Play called successfully");
	} else {
		LOG_ERROR("Play call failed, code: %d", ret);
	}
}

void RTSPPlayerController::Stop() {
	int32_t ret = player_->Stop();
	if (ret == ErrorCodes::Success) {
		LOG_INFO("Stop called successfully");
	} else {
		LOG_ERROR("Stop call failed, code: %d", ret);
	}
	//is_parsing_finished_ = true;
}

void RTSPPlayerController::CleanPlayer() {
	LOG_INFO("Cleaning player.");
	if (player_) return;
	player_thread_.reset();
	parser_thread_.reset();
	data_source_.reset();
	state_ = PlayerState::kUnitialized;
	// TODO: Do we need to close the input first?
	avformat_free_context(format_context_);
	LOG_INFO("Finished closing.");
}

void RTSPPlayerController::SetViewRect(const Rect& view_rect) {
	view_rect_ = view_rect;
	if (!player_) return;

	LOG_DEBUG("Set view rect to %d, %d", view_rect_.width(), view_rect_.height());
	auto callback = WeakBind(&RTSPPlayerController::OnSetDisplayRect,
	                         std::static_pointer_cast<RTSPPlayerController>(
	                             shared_from_this()), _1);
	int32_t ret = player_->SetDisplayRect(view_rect_, callback);
	if (ret < ErrorCodes::CompletionPending)
		LOG_ERROR("SetDisplayRect result: %d", ret);
}

PlayerController::PlayerState RTSPPlayerController::GetState() {
	return state_;
}

void RTSPPlayerController::FinishStreamConfiguration() {
	LOG_INFO("All streams configured, attaching data source.");
	// Audio and video stream should be initialized already.
	if (!player_) {
		LOG_DEBUG("player_ is null!, quit function");
		return;
	}
	int32_t ret = player_->AttachDataSource(*data_source_);

	if (ret == ErrorCodes::Success && state_ != PlayerState::kError) {
		state_ = PlayerState::kReady;
		LOG_INFO("Data Source attached");
	} else {
		state_ = PlayerState::kError;
		LOG_ERROR("Failed to AttachDataSource! Error code: %d", ret);
	}
}

void RTSPPlayerController::OnSetDisplayRect(int32_t ret) {
	LOG_DEBUG("SetDisplayRect result: %d", ret);
}

void RTSPPlayerController::Mute() {
    AutoLock critical_section(mute_lock_);
	if (is_mute_==true) {
		LOG_INFO("Mute flag is false - UnMuted");
		is_mute_=false;
	} else {
		LOG_INFO("Mute flag is true - Muted");
		is_mute_=true;
	}
}

void RTSPPlayerController::RTPCheckAndSendBackStats(RTPDemuxContext *s, uint64_t *stats_last_sent,
                                                    int *bits_this_sec) {
	// based on https://www.ffmpeg.org/doxygen/trunk/rtpdec_8c-source.html
	RTPStatistics *stats = &s->statistics;
	uint32_t lost;
	uint32_t extended_max;
	uint32_t expected;
	uint64_t now;

	extended_max = stats->cycles + stats->max_seq;
	expected     = extended_max - stats->base_seq + 1;
	lost         = expected - stats->received;

	now = nowms();
	if (now >= *stats_last_sent + 1000) {
		message_sender_->SendStats(lost, stats->jitter, *bits_this_sec / 1000);
		*stats_last_sent = now;
		*bits_this_sec = 0;
	}
}

void RTSPPlayerController::StartParsing(int32_t) {
	//Open the Audio Decoder Context
	AVCodecContext *in_codec_ctx = NULL, *out_codec_ctx = NULL;
	SwrContext *resample_context = NULL;
	AVAudioFifo *fifo = NULL;
	AVStream* s = format_context_->streams[audio_stream_idx_];
	int ret = 0, bits_this_sec = 0;
	uint64_t stats_last_sent = 0;
	RTSPState *state;
	RTPDemuxContext *demux;

	init_transcoder(s->codecpar, &in_codec_ctx, &out_codec_ctx, &resample_context);

	// Initialize the FIFO buffer to store audio samples to be encoded
	init_fifo(&fifo, out_codec_ctx);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	//Audio Level update
	prev_audio_ts_ = 0;
	audio_level_ = 0;
	is_parsing_finished_ = false;
	is_mute_ = false;

	while (!is_parsing_finished_) {
		unique_ptr<ElementaryStreamPacket> es_pkt;

		Message packet_msg;
		int32_t ret = av_read_frame(format_context_, &pkt);
		state = (RTSPState*)format_context_->priv_data;
		demux = (RTPDemuxContext*)state->rtsp_streams[pkt.stream_index]->transport_priv;
		bits_this_sec += pkt.size;
		RTPCheckAndSendBackStats(demux, &stats_last_sent, &bits_this_sec);
		if (pkt.stream_index == audio_stream_idx_) {
			packet_msg = kAudioPkt;
		} else if (pkt.stream_index == video_stream_idx_) {
			packet_msg = kVideoPkt;
		} else {
			packet_msg = kError;
			LOG_INFO("Error! Packet stream index (%d) not recognized!",
			         pkt.stream_index);
			continue;
		}
		if (ret < 0) {
			if (ret == AVERROR_EOF) {
				is_parsing_finished_ = true;
				packet_msg = kEndOfStream;
			} else {  // Not handled error.
				char errbuff[1024];
				int32_t strerror_ret = av_strerror(ret, errbuff, 1024);
				LOG_INFO("av_read_frame error: %d [%s], av_strerror ret: %d", ret,
				         errbuff, strerror_ret);
				break;
			}
		} else {
			if (pkt.stream_index == audio_stream_idx_) {
					es_pkt = MakeESPacketFromAVPacketTranscode(&pkt, fifo, in_codec_ctx, out_codec_ctx, resample_context);
				//es_pkt = MakeESPacketFromAVPacket(&pkt);
			} else {
				es_pkt = MakeESPacketFromAVPacket(&pkt);
			}
		}

		if (es_pkt != NULL || packet_msg == kEndOfStream) {
			auto es_pkt_callback = std::make_shared<EsPktCallbackData>(packet_msg, std::move(es_pkt));
			player_thread_->message_loop().PostWork(cc_factory_.NewCallback(
			        &RTSPPlayerController::EsPktCallback, es_pkt_callback));
		}

		av_packet_unref(&pkt);
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;
	}

	LOG_INFO("Flushing decoder/encoder");
	if (flush_decoder(in_codec_ctx) < 0)
		LOG_ERROR("Could not flush decoder (error '%s')", get_error_text(ret));

	if (flush_encoder(out_codec_ctx) < 0)
		LOG_ERROR("Could not flush encoder (error '%s')", get_error_text(ret));

	if (fifo)
		av_audio_fifo_free(fifo);

	swr_free(&resample_context);
	avcodec_free_context(&in_codec_ctx);
	avcodec_free_context(&out_codec_ctx);
	LOG_INFO("Finished parsing data. parser: %p", this);
}

void RTSPPlayerController::calculateAudioLevel(AVFrame* input_frame, AVSampleFormat format, AVRational time_base) {
	uint8_t *buff16 = *input_frame->extended_data;
	int nb_samples = input_frame->nb_samples;
	int bytes_per_sample = av_get_bytes_per_sample(format);
	float sum = 0,decibel=0,sample;

	if (audio_level_cb_frequency_ <= 0.0)  //user expects no audio-updates
		return;

	switch(bytes_per_sample) {	  
	  case 1: //8-bit sample
	  {

		for (int i = 0; i < nb_samples; i++) {
			sample = (char)buff16[i];
			sample = fabs(sample);
			sum += sample;
		}
		break;
	  }
	  
	  case 2: //16-bit sample
	  {

		for (int i = 0; i < nb_samples; i++) {
			sample = (short)(((short)buff16[(i*2) + 1] << 8) | (short)buff16[i*2]);
			sample = fabs(sample);
			sum += sample;
		}
		break;
	  }

	  default:
		  sum = 0;
		break;
	}

	//rms = (float)sqrt(sum / (nb_samples));
	decibel = 20 * log(sum / nb_samples) * 0.4343; // Multiplying by 0.4343 for base 10 conversion
	audio_level_ = (audio_level_ + decibel) / 2.0; //Average Decibel.

	TimeTicks ts_now = ToTimeTicks(input_frame->best_effort_timestamp, time_base);
	if ((ts_now - prev_audio_ts_) > audio_level_cb_frequency_) {
		prev_audio_ts_ = ts_now;
		message_sender_->SetAudioLevel((double)audio_level_);
	}

}

std::unique_ptr<ElementaryStreamPacket> RTSPPlayerController::MakeESPacketFromAVPacketTranscode(
    AVPacket* input_packet, AVAudioFifo *fifo, AVCodecContext* in_codec_ctx,
    AVCodecContext* out_codec_ctx, SwrContext* resample_context) {
	int ret = 0;
	const int output_frame_size = out_codec_ctx->frame_size;

		//Transcode any non AAC audio stream
		int data_present = 0;
		if (av_audio_fifo_size(fifo) < output_frame_size) {
			// decode
			AVFrame *input_frame = NULL;
			uint8_t **converted_input_samples = NULL;
			init_input_frame(&input_frame);

			ret = decode(in_codec_ctx, input_frame, &data_present, input_packet);
			if (ret < 0) {
				LOG_ERROR("Could not decode frame (error '%s')", get_error_text(ret));
				//av_packet_unref(&input_packet);
				av_frame_free(&input_frame);
				return NULL;
			}
		// If there is decoded data, convert and store it
		if (data_present) {
			calculateAudioLevel(input_frame, in_codec_ctx->sample_fmt, in_codec_ctx->time_base);

			// Initialize the temporary storage for the converted input samples
			ret = init_converted_samples(&converted_input_samples, out_codec_ctx,
										 input_frame->nb_samples);
			/**
			 * Convert the input samples to the desired output sample format.
			 * This requires a temporary storage provided by converted_input_samples.
			 */

		    AutoLock critical_section(mute_lock_);
			ret = convert_samples((const AVFrame*) input_frame, converted_input_samples,
					               resample_context, in_codec_ctx->sample_fmt, is_mute_);

			// Add the converted input samples to the FIFO buffer for later processing
			ret = add_samples_to_fifo(fifo, converted_input_samples, input_frame->nb_samples);
		}
		//No Scope for input_frame after pushing the samples to fifo
		av_frame_free(&input_frame);
	}

	if (av_audio_fifo_size(fifo) >= output_frame_size) {
		// encode
		// Temporary storage of the output samples of the frame written to the file
		AVFrame *output_frame;
		/**
		 * Use the maximum number of possible samples per frame.
		 * If there is less than the maximum possible frame size in the FIFO
		 * buffer use this number. Otherwise, use the maximum possible frame size
		 */
		const int frame_size = FFMIN(av_audio_fifo_size(fifo), out_codec_ctx->frame_size);
		// Initialize temporary storage for one output frame
		ret = init_output_frame(&output_frame, out_codec_ctx, frame_size);

		/**
		 * Read as many samples from the FIFO buffer as required to fill the frame.
		 * The samples are stored in the frame temporarily.
		 */
		if (av_audio_fifo_read(fifo, (void **)output_frame->data, frame_size) < frame_size) {
			LOG_ERROR("Could not read data from FIFO");
			av_frame_free(&output_frame);
			return NULL;
		}
		// Encode one frame worth of audio samples
		// Packet used for temporary storage
		AVPacket *output_packet = input_packet;
		av_packet_unref(output_packet);
		ret = encode(out_codec_ctx, output_packet, &data_present, output_frame);
		if (ret < 0) {
			LOG_ERROR("Could not encode frame (error '%s')", get_error_text(ret));
			av_packet_unref(output_packet);
			av_frame_free(&output_frame);
			return NULL;
		}
		if (data_present) {
			av_frame_free(&output_frame);
			return MakeESPacketFromAVPacket(output_packet);
		}
	}
	return NULL; //No packets
}

std::unique_ptr<ElementaryStreamPacket> RTSPPlayerController::MakeESPacketFromAVPacket(
    AVPacket* pkt) {
	auto es_packet = MakeUnique<ElementaryStreamPacket>(pkt->data, pkt->size);

	AVStream* s = format_context_->streams[pkt->stream_index];

	es_packet->SetPts(ToTimeTicks(pkt->pts, s->time_base) + timestamp_);
	es_packet->SetDts(ToTimeTicks(pkt->dts, s->time_base) + timestamp_);
	es_packet->SetDuration(ToTimeTicks(pkt->duration, s->time_base));
	es_packet->SetKeyFrame(pkt->flags == 1);

	return es_packet;
}

void RTSPPlayerController::EsPktCallback(int32_t, const std::shared_ptr<EsPktCallbackData>& data) {
	Message msg = std::get<0>(*data);
	unique_ptr<ElementaryStreamPacket> es_pkt = std::move(std::get<1>(*data));

	switch (msg) {
		case Message::kEndOfStream: {
			AutoLock critical_section(packets_lock_);
			data_source_->SetEndOfStream();
			break;
		}
		case Message::kAudioPkt: {
			AutoLock critical_section(packets_lock_);
			if (need_audio_data_) {
				int32_t ret = ErrorCodes::Success;
				ret = audio_stream_->AppendPacket(es_pkt->GetESPacket());
				if (ret != ErrorCodes::Success) {
					LOG_ERROR("Failed to append audio packet! Error code: %d", ret);
				}
			}
			break;
		}
		case Message::kVideoPkt: {
			AutoLock critical_section(packets_lock_);
			if (need_video_data_) {
				int32_t ret = ErrorCodes::Success;
				ret = video_stream_->AppendPacket(es_pkt->GetESPacket());
				if (ret != ErrorCodes::Success) {
					LOG_INFO("Failed to append video packet! Error code: %d", ret);
				}
			}
			break;
		}
		default:
			LOG_ERROR("Not supported message type received!");
	}
}
