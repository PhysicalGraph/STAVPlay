/*!
 * message_receiver.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#include "message_receiver.h"

#include <string>

#include "ppapi/cpp/var_dictionary.h"

#include "messages.h"

using pp::Var;
using pp::VarDictionary;

namespace Communication {

void MessageReceiver::HandleMessage(pp::InstanceHandle /*instance*/,
                                      const Var& message_data) {
  LOG_INFO("MessageHandler - HandleMessage");
  if (!message_data.is_dictionary()) {
    LOG_ERROR("Not supported message format.");
    if (message_data.is_string())
      LOG_ERROR("Message content: %s", message_data.AsString().c_str());
    return;
  }

  VarDictionary msg(message_data);
  Var action_var = msg.Get(kKeyMessageToPlayer);
  if (!action_var.is_int()) {
    LOG_ERROR("Invalid message - 'action' should be an integer!");
    return;
  }
  LOG_INFO("Action type: %d", action_var.AsInt());
  auto action = static_cast<MessageToPlayer>(action_var.AsInt());

  switch (action) {
    case MessageToPlayer::kClosePlayer:
      ClosePlayer();
      break;
    case MessageToPlayer::kLoadMedia:
      LoadMedia(msg.Get(kKeyType),
                msg.Get(kKeyUrl),
                msg.Get(kKeyUpdateFrequency),
                msg.Get(kKeyArloCrtPath)
                );
      break;
    case MessageToPlayer::kPlay:
      Play();
      break;
    case MessageToPlayer::kStop:
      Stop();
      break;
    case MessageToPlayer::kChangeViewRect:
      ChangeViewRect(msg.Get(kKeyXCoordination),
                     msg.Get(kKeyYCoordination),
                     msg.Get(kKeyWidth),
                     msg.Get(kKeyHeight));
      break;
    case MessageToPlayer::kMute:
          Mute();
          break;
    default:
      LOG_ERROR("Not supported action code!");
  }
}

Var MessageReceiver::HandleBlockingMessage(
    pp::InstanceHandle /*instance*/, const Var& /*message_data*/) {
  return Var();
}

void MessageReceiver::ClosePlayer() { player_controller_.reset(); }

void MessageReceiver::LoadMedia(const Var& type, const Var& url,
                                const Var& audio_level_cb_frequency,
                                const Var& crt_path) {
  if (!type.is_int() || !url.is_string()) {
    LOG_ERROR("Invalid message - 'url' should be a string");
    return;
  }
  PlayerProvider::PlayerType player_type = PlayerProvider::kUnknown;
  switch (static_cast<ClipTypeEnum>(type.AsInt())) {
    case ClipTypeEnum::kRTSP:
      player_type = PlayerProvider::kRTSP;
      break;
    default:
      LOG_ERROR("Not known type of player %d", type.AsInt());
      return;
  }


  player_controller_ =
      player_provider_->CreatePlayer(player_type, view_rect_, url.AsString(),
                                     audio_level_cb_frequency.AsDouble(),
                                     crt_path.AsString());
}

void MessageReceiver::Play() {
  if (player_controller_) player_controller_->Play();
}

void MessageReceiver::Stop() {
  if (player_controller_) player_controller_->Stop();
}

void MessageReceiver::Mute() {
  if (player_controller_) player_controller_->Mute();
}
void MessageReceiver::ChangeViewRect(const Var& x_position,
    const Var& y_position, const Var& width, const Var& height) {
  if (!x_position.is_int() || !y_position.is_int() || !width.is_int() ||
      !height.is_int()) {
    LOG_ERROR("Invalid message - some params are not an int type");
    return;
  }
  view_rect_ = Samsung::NaClPlayer::Rect(
      x_position.AsInt(), y_position.AsInt(), width.AsInt(), height.AsInt());
  if (player_controller_) {
    player_controller_->SetViewRect(view_rect_);
  }
}

}  // namespace Communication
