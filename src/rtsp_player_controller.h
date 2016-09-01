#ifndef RTSP_PLAYER_CONTROLLER_H_
#define RTSP_PLAYER_CONTROLLER_H_

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <list>

#include "nacl_player/es_data_source.h"
#include "nacl_player/media_data_source.h"
#include "nacl_player/media_player.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"

#include "common.h"
#include "player_controller.h"
#include "player_listeners.h"
#include "message_sender.h"

#include "convert_codecs.h"

extern "C" {
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavutil/audio_fifo.h"
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#include "sys/socket.h"
#include "rtsp-hack.h"
}

class RTSPPlayerController : public PlayerController,
	public std::enable_shared_from_this<PlayerController> {
	public:
		/// Creates an <code>RTSPPlayerController</code> object. NaCl Player must
		/// be initialized using the <code>InitPlayer()</code> method before a
		/// playback can be started.
		///
		/// <code>RTSPPlayerController</code> is created by a
		/// <code>PlayerProvider</code> factory.
		///
		/// @param[in] instance An <code>InstanceHandle</code> identifying Native
		///   Player object.
		/// @param[in] message_sender A <code>MessageSender</code> object pointer
		///   which will be used to send messages through the communication channel.
		///
		/// @see RTSPPlayerController::InitPlayer()
		RTSPPlayerController(const pp::InstanceHandle& instance,
		                     std::shared_ptr<Communication::MessageSender> message_sender)
			: PlayerController(),
			  instance_(instance),
			  cc_factory_(this),
			  message_sender_(message_sender),
			  state_(PlayerState::kUnitialized) {}

		/// Destroys an <code>RTSPPlayerController</code> object. This also
		/// destroys a <code>MediaPlayer</code> object and thus a player pipeline.
		~RTSPPlayerController() override {};

		/// Initializes NaCl Player and prepares it to play a given content.
		///
		/// This method can be called several times to change a multimedia content
		/// that should be played with NaCl Player.
		///
		/// @param[in] url An address of an RTSP feed to be prepared for a
		///   playback in NaCl Player.
		/// @see RTSPPlayerController::RTSPPlayerController()
		void InitPlayer(const std::string& url,const double& audio_level_cb_frequency);

		// Overloaded methods defined by PlayerController, don't have to be commented
		void Play() override;
		void Stop() override;
		void Mute() override;
		void SetViewRect(const Samsung::NaClPlayer::Rect& view_rect) override;
		PlayerState GetState() override;
		bool need_video_data_;
		bool need_audio_data_;
	private:
		/// @public
		/// Marks end of configuration of all media streams.
		void FinishStreamConfiguration();
		void InitializeStreams(int32_t, const std::string& url);

		void OnSetDisplayRect(int32_t);

		enum Message {
			kError = -1,
			kInitialized = 0,
			kFlushed = 1,
			kClosed = 2,
			kEndOfStream = 3,
			kAudioPkt = 4,
			kVideoPkt = 5,
		};

		void CleanPlayer();
		void UpdateVideoConfig();
		void UpdateAudioConfig();

		std::unique_ptr<ElementaryStreamPacket> MakeESPacketFromAVPacket(AVPacket* pkt);
		std::unique_ptr<ElementaryStreamPacket> MakeESPacketFromAVPacketTranscode(
		    AVPacket* input_packet, AVAudioFifo *fifo, AVCodecContext* in_codec_ctx,
		    AVCodecContext* out_codec_ctx, SwrContext* resample_context);

		void StartParsing(int32_t);
		void calculateAudioLevel(AVFrame *, AVSampleFormat, AVRational);

		typedef std::tuple<
		Message, std::unique_ptr<ElementaryStreamPacket>> EsPktCallbackData;

		pp::InstanceHandle instance_;
		std::unique_ptr<pp::SimpleThread> player_thread_;
		std::unique_ptr<pp::SimpleThread> parser_thread_;
		pp::CompletionCallbackFactory<RTSPPlayerController> cc_factory_;

		PlayerListeners listeners_;

		std::shared_ptr<Samsung::NaClPlayer::VideoElementaryStream> video_stream_;
		std::shared_ptr<Samsung::NaClPlayer::AudioElementaryStream> audio_stream_;
		std::shared_ptr<Samsung::NaClPlayer::ESDataSource> data_source_;
		std::shared_ptr<Samsung::NaClPlayer::MediaPlayer> player_;

		std::shared_ptr<Communication::MessageSender> message_sender_;

		void EsPktCallback(int32_t, const std::shared_ptr<EsPktCallbackData>& data);
		void RTPCheckAndSendBackStats(RTPDemuxContext *s, uint64_t *stats_last_sent,
		                              int *bits_this_sec);

		PlayerState state_;
		Samsung::NaClPlayer::Rect view_rect_;

		AVFormatContext* format_context_;
		int video_stream_idx_;
		int audio_stream_idx_;
		VideoConfig video_config_;
		AudioConfig audio_config_;
		Samsung::NaClPlayer::TimeTicks timestamp_;
		bool is_parsing_finished_;
		bool is_mute_;
		float audio_rms_;
		double prev_audio_ts_;
		double audio_level_cb_frequency_;
};

#endif
