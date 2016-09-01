/*!
 * player_listeners.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
*/

#include "player_listeners.h"

using Samsung::NaClPlayer::TimeTicks;
using Samsung::NaClPlayer::MediaPlayerError;
using pp::Var;

void MediaPlayerListener::OnTimeUpdate(TimeTicks time) {
  if (auto message_sender = message_sender_.lock()) {
    message_sender->CurrentTimeUpdate(time);
  }
}

void MediaPlayerListener::OnEnded() {
  LOG_INFO("Event: Media ended.");
  if (auto message_sender = message_sender_.lock()) {
     message_sender->StreamEnded();
  }
}

void MediaPlayerListener::OnError(MediaPlayerError error) {
  LOG_INFO("Event: Error occurred. Error no: %d.", error);
}

void MediaBufferingListener::OnBufferingStart() {
  LOG_INFO("Event: Buffering started, wait for the end.");
}

void MediaBufferingListener::OnBufferingProgress(uint32_t percent) {
  LOG_INFO("Event: Buffering progress: %d %%.", percent);
}

void MediaBufferingListener::OnBufferingComplete() {
  LOG_INFO("Event: Buffering complete! Now you may play.");
  if (auto message_sender = message_sender_.lock()) {
    message_sender->BufferingCompleted();
  }
}
