/*!
 * message_sender.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#include "message_sender.h"

#include <string>

#include "ppapi/cpp/var_dictionary.h"

#include "messages.h"

using pp::Var;
using pp::VarDictionary;
using Samsung::NaClPlayer::TimeTicks;
using Samsung::NaClPlayer::TextTrackInfo;

namespace Communication {

void MessageSender::SetMediaDuration(TimeTicks duration) {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kSetDuration);
  message.Set(kKeyTime, duration);
  PostMessage(message);
}

void MessageSender::CurrentTimeUpdate(TimeTicks time) {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kTimeUpdate);
  message.Set(kKeyTime, time);
  PostMessage(message);
}

void MessageSender::BufferingCompleted() {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kBufferingCompleted);
  PostMessage(message);
}

void MessageSender::SetAudioLevel(TimeTicks audio_level) { 
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kSetAudioLevel);
  message.Set(kKeyAudioLevel,audio_level);//key value pair
  PostMessage(message);
}

void MessageSender::SendStats(int lost, int jitter, int bitrate) {
	VarDictionary message;
	message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kSendStats);
	message.Set(kKeyStatsLost, lost);
	message.Set(kKeyStatsJitter, jitter);
	message.Set(kKeyStatsBitrate, bitrate);
	PostMessage(message);
}

void MessageSender::StreamEnded() {
  VarDictionary message;
  message.Set(kKeyMessageFromPlayer, MessageFromPlayer::kStreamEnded);
  PostMessage(message);
}

void MessageSender::PostMessage(const Var& message) {
  instance_->PostMessage(message);
}

}  // namespace Communication
