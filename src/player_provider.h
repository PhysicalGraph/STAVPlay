/*!
 * player_provider.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_PLAYER_PLAYER_PROVIDER_H_
#define NATIVE_PLAYER_INC_PLAYER_PLAYER_PROVIDER_H_

#include <string>

#include "nacl_player/common.h"
#include "ppapi/cpp/instance.h"

#include "common.h"
#include "player_controller.h"
#include "message_sender.h"

/// @file
/// @brief This file defines <code>PlayerProvider</code> class.

/// @class PlayerProvider
/// @brief A factory of <code>PlayerController</code> objects.
///
/// API for controlling the player is specified by
/// <code>PlayerController</code>, but different implementations of it could
/// have a different initialization procedure. This class encapsulates it,
/// provided player controller is already initialized and ready to play the
/// content.

class PlayerProvider {
 public:
  /// @enum PlayerType
  /// This enum defines supported player types.
  enum PlayerType {
    /// This value is not related to any player type.
    kUnknown,
    kRTSP,
  };

  /// Creates a <code>PlayerProvider</code> object. Gathers objects which
  /// are provided to the <code>PlayerController</code> implementation if it
  /// is needed.
  ///
  /// @param[in] instance A handle to main plugin instance.
  /// @param[in] message_sender A class which will be used by the player to
  ///   post messages through the communication channel.
  /// @see pp::Instance
  /// @see Communication::MessageSender
  explicit PlayerProvider(const pp::InstanceHandle& instance,
      std::shared_ptr<Communication::MessageSender> message_sender)
      : instance_(instance), message_sender_((std::move(message_sender))) {}

  /// Destroys a <code>PlayerProvider</code> object. Created
  /// <code>PlayerController</code> objects will not be destroyed.
  ~PlayerProvider() {}

  /// Provides an initialized <code>PlayerController</code> of a given type.
  /// The <code>PlayerController</code> object is ready to use. This is the
  /// main method of this class.
  ///
  /// @param[in] type A type of the player controller which is needed.
  /// @param[in] url A URL address to a file which the player controller
  ///   should use for getting the content. This parameter could point to
  ///   different file types depending on the <code>type</code> parameter.
  ///   Check <code>PlayerType</code> for more information about supported
  ///   formats.
  /// @param[in] view_rect A position and size of the player window.
  /// @param[in] subtitle A URL address to an external subtitles file. It is
  ///   an optional parameter, if is not provided then external subtitles
  ///   are not be available.
  /// @param[in] encoding A code of subtitles formating. It is an optional
  ///   parameter, if is not specified then UTF-8 is used.
  /// @return A configured and initialized <code>PlayerController<code>.
  std::shared_ptr<PlayerController> CreatePlayer(PlayerType type,
                                     const Samsung::NaClPlayer::Rect view_rect,
                                     const std::string& url,
                                     const double& audio_level_cb_frequency,
                                     const std::string& crt_path);

 private:
  pp::InstanceHandle instance_;
  std::shared_ptr<Communication::MessageSender> message_sender_;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_PLAYER_PROVIDER_H_
