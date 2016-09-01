/*!
 * native_player.cc (https://github.com/SamsungDForum/STAVPlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Michal Murgrabia
 */

#include "stav_player.h"

#include "nacl_io/nacl_io.h"
#include "ppapi/cpp/var_dictionary.h"

#include "messages.h"
#include "logger.h"

using Samsung::NaClPlayer::Rect;

const char* kLogCmd = "logs";
const char* kLogDebug = "debug";

STAVPlayer::~STAVPlayer() { UnregisterMessageHandler(); }

void STAVPlayer::DidChangeView(const pp::View& view) {
  const pp::Rect& pp_r = view.GetRect();
  if (rect_ == pp_r) return;

  rect_ = pp_r;
  pp::VarDictionary message;
  message.Set(Communication::kKeyMessageToPlayer,
            static_cast<int>(Communication::MessageToPlayer::kChangeViewRect));

  message.Set(Communication::kKeyXCoordination, pp_r.x());
  message.Set(Communication::kKeyYCoordination, pp_r.y());
  message.Set(Communication::kKeyWidth, pp_r.width());
  message.Set(Communication::kKeyHeight, pp_r.height());

  LOG_DEBUG("View changed to: (x:%d, y: %d), (w:%d, h:%d)", pp_r.x(), pp_r.y(),
            pp_r.width(), pp_r.height());

  DispatchMessage(message);
}

void STAVPlayer::HandleMessage(const pp::Var& message) {
  DispatchMessage(message);
}

bool STAVPlayer::Init(uint32_t argc, const char** argn, const char** argv) {
  Logger::InitializeInstance(this);
  LOG_INFO("Start Init");
  for (uint32_t i = 0; i < argc; i++) {
    if (strcmp(argn[i], kLogCmd) == 0 && strcmp(argv[i], kLogDebug) == 0)
      Logger::EnableDebugLogs(true);
  }

  std::shared_ptr<Communication::MessageSender> ui_message_sender =
      std::make_shared<Communication::MessageSender>(this);

  std::shared_ptr<PlayerProvider> player_provider =
      std::make_shared<PlayerProvider>(this, std::move(ui_message_sender));

  message_receiver_ =
      std::make_shared<Communication::MessageReceiver>(player_provider);

  InitNaClIO();
  player_thread_.Start();
  RegisterMessageHandler(message_receiver_.get(),
                         player_thread_.message_loop());

  LOG_INFO("Finished Init");

  return true;
}

void STAVPlayer::InitNaClIO() {
  nacl_io_init_ppapi(pp_instance(), pp::Module::Get()->get_browser_interface());
}

void STAVPlayer::DispatchMessage(pp::Var message) {
  player_thread_.message_loop().PostWork(
    cc_factory_.NewCallback(
        &STAVPlayer::DispatchMessageMessageOnSideThread, message));
}

void STAVPlayer::DispatchMessageMessageOnSideThread(int32_t,
    pp::Var message) {
  message_receiver_->HandleMessage(this, message);
}
