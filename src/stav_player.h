/*!
 * native_player.h (https://github.com/SamsungDForum/STAVPlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_NATIVE_PLAYER_H_
#define NATIVE_PLAYER_INC_NATIVE_PLAYER_H_

#include <memory>

#include "ppapi/c/pp_rect.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"

#include "message_receiver.h"
#include "message_sender.h"
#include "player_provider.h"


/// @class STAVPlayer
/// This is a main class for this application. It holds a classes responsible
/// for communication and for controlling the player.

class STAVPlayer : public pp::Instance {
 public:
  /// A constructor.
  /// @param[in] instance a class identifier for NaCl engine.
  explicit STAVPlayer(PP_Instance instance)
      : pp::Instance(instance), player_thread_(this), cc_factory_(this) {}

  /// ~STAVPlayer()
  /// @private This method don't have to be included in Doxygen documentation
  ~STAVPlayer() override;

  /// DidChangeView() handles initialization or changing viewport
  /// @param[in] view new representation of vewport
  /// @see pp::Instance
  void DidChangeView(const pp::View& view) override;

  /// HandleMessage() is a default method for handling messages from another
  /// modules in this application this functionality is redirected to
  /// UiMessageReceiver.
  /// @param[in] var_message a container with received message
  /// @see pp::Instance.
  void HandleMessage(const pp::Var& var_message) override;

  /// Method called for instancce initialization.
  /// @see pp::Instance
  bool Init(uint32_t, const char**, const char**) override;

 private:
  /// Method called for initialize IO stream.
  void InitNaClIO();

  void DispatchMessage(pp::Var message);

  void DispatchMessageMessageOnSideThread(int32_t, pp::Var message);

  std::shared_ptr<Communication::MessageReceiver> message_receiver_;
  pp::SimpleThread player_thread_;
  pp::CompletionCallbackFactory<STAVPlayer> cc_factory_;
  pp::Rect rect_;
};

/// STAVPlayerModule creation
class STAVPlayerModule : public pp::Module {
 public:
  pp::Instance* CreateInstance(PP_Instance instance) override {
    return new STAVPlayer(instance);
  }
};

/// Module creation, very first call in NaCl
/// @see HelloWorld demo
namespace pp {
Module* CreateModule() { return new STAVPlayerModule(); }
}

#endif  // NATIVE_PLAYER_INC_NATIVE_PLAYER_H_
