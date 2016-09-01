/*!
 * message_sender.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGE_SENDER_H_
#define NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGE_SENDER_H_

#include <vector>

#include "common.h"
#include "nacl_player/common.h"
#include "nacl_player/media_common.h"

/// @file
/// @brief This file defines a MessageSender class.

namespace Communication {

/// @class MessageSender
/// @brief This class is designed to create and post messages from the player
/// using the communication channel. Possible messages, which the player can
/// send, are defined in <code>enum MessageFromPlayer</code>.
///
/// @see Communication The description of <code>Communication</code> namespace
///   provides a brief of the communication mechanism.
/// @see MessageFromPlayer

class MessageSender {
 public:
  /// Creates a <code>MessageSender</code> object.
  ///
  /// @param[in] instance A pointer to a module which will be used for sending
  ///   messages.
  explicit MessageSender(pp::Instance* instance) : instance_(instance) {}

  /// Destroys the <code>MessageSender</code> object.
  ~MessageSender() {}

  /// Prepares and posts a message with the duration of the content.
  ///
  /// @param[in] duration A total length in seconds of the loaded media
  ///   content.
  /// @see kSetDuration Main key value in the prepared message.
  void SetMediaDuration(Samsung::NaClPlayer::TimeTicks duration);

  /// Prepares and posts a message with a new current playback position.
  ///
  /// @param[in] time A current playback position.
  /// @see kTimeUpdate Main key value in the prepared message.
  void CurrentTimeUpdate(Samsung::NaClPlayer::TimeTicks time);

  /// Prepares and posts a message with the information that buffering has been
  /// finished, and playback is possible from this moment.
  ///
  /// @see kBufferingCompleted Main key value in the prepared message.
  void BufferingCompleted();

  /// Prepares and posts a message with the information that stream has ended
  /// playback is no more possible from this moment.
  ///
  /// @see kStreamEnded Main key value in the prepared message.
  void StreamEnded();

  void SetAudioLevel(Samsung::NaClPlayer::TimeTicks duration);
  void SendStats(int lost, int jitter, int bitrate);
 private:
  /// Send a provided message by the communication channel.
  ///
  /// @param[in] message An object which holds message content.
  /// @see pp::Instance
  inline void PostMessage(const pp::Var& message);

  pp::Instance* instance_;
};

}  // namespace Communication

#endif  // NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGE_SENDER_H_
