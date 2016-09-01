/*!
 * player_provider.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Michal Murgrabia
 */

#include "player_provider.h"

#include "rtsp_player_controller.h"
#include "logger.h"

using Samsung::NaClPlayer::Rect;

std::shared_ptr<PlayerController> PlayerProvider::CreatePlayer(
                    PlayerType type, const std::string& url, const double& audio_level_cb_frequency,
                    const Samsung::NaClPlayer::Rect view_rect,
                    const std::string& subtitle, const std::string& encoding) {
  switch (type) {
    case kRTSP: {
      std::shared_ptr<RTSPPlayerController> controller =
          std::make_shared<RTSPPlayerController>(instance_, message_sender_);
      controller->SetViewRect(view_rect);
      controller->InitPlayer(url,audio_level_cb_frequency);
      return controller;
    }
    default:
      Logger::Error("Not known type of player %d", type);
  }

  return 0;
}
