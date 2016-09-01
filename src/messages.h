/*!
 * messages.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGES_H_
#define NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGES_H_

#include <string>

/// @namespace Communication
/// In this namespace you can find a class designed for receiving messages from
/// UI, a class designed for sending messages to UI and key values used for a
/// message building. \n
/// It is assumed that UI is implemented in the separate module. In this
/// application JavaScript is used for this purpose. For communication purposes
/// the "post message" - "handle message" mechanism, characteristic for
/// JavaScript and supported in NaCl, is used.
/// All messages are <code>VarDictionary</code> objects and their values
/// are held in a "key" - "value" map. It is mandatory for the message to
/// contain a <code>kKeyAction</code> entry which defines message type.
namespace Communication {

/// @enum MessageToPlayer
/// Values defined in this enum are used to recognize the message type sent to
/// the player. The value of this type should be send in the
/// <code>VarDictionary</code> object with the <code>kKeyMessageToPlayer</code>
/// key. When an action requires additional parameters these values should be
/// also included in the same<code>VarDictionary</code> object.
/// Parameters can be found in the values description, each one's
/// type key identifier and description is provided.
/// @see kKeyMessageToPlayer
enum MessageToPlayer {
  /// A request to close the player; no additional parameters.
  kClosePlayer = 0,

  /// A request to load content specified in additional fields and
  /// prepare the player to play it.
  /// @param (int)kKeyType A specification of what kind of content have
  ///   to be loaded. The only values accepted for  this parameter are
  ///   the ones defined by <code>ClipEnumType</code>
  /// @param (string)kKeyUrl An URL to the content container. This points
  ///   to different file types depending on values of
  ///   <code>kKeyType</code>.
  /// @param (string)kKeySubtitle [optional] A path to the
  ///   file with external subtitles. If an application wants to play
  ///   content with external subtitles this field must be filled.
  /// @param (string)kKeyEncoding [optional] A subtitles encoding code.
  ///   If this parameter is not specified then UTF-8 will be used .
  /// @see Communication::ClipTypeEnum
  kLoadMedia = 1,

  /// A request to start playing; no additional parameters.
  kPlay = 2,

  kStop = 3,

  /// An information about the players position and size.
  /// @param (int)XCoordination An x position of the players left upper
  ///   corner.
  /// @param (int)YCoordination A y position of the players left upper
  ///   corner.
  /// @param (int)kKeyWidth A width of the players window.
  /// @param (int)kKeyHeight A height of the players window.
  kChangeViewRect = 4,
  kMute          =5,
};

/// @enum MessageFromPlayer
/// Values defined in this enum are used to recognize the message type sent
/// from the player. The value of this type should be send in the <code>
/// VarDictionary</code> object with the <code>kKeyMessageFromPlayer</code>
/// key. When an action requires additional parameters these values should be
/// also included in the same<code>VarDictionary</code> object.
/// Parameters can be found in the values description, each one's
/// type key identifier and description is provided.
/// @note Information about all tracks, representations and content duration is
///   send right after content loading is finished. Other messages are send
///   when accurate event occurs, or related operation have been completed.
/// @see kKeyMessageToPlayer
enum MessageFromPlayer {
  /// An information from the player about current playback position.
  /// @param (double)kKeyTime A value which holds current playback
  ///   position in seconds.
  kTimeUpdate = 100,

  /// An information from the player about the duration of the loaded
  /// content.
  /// @param (double)kKeyTime A value which holds the content duration
  ///   in seconds.
  kSetDuration = 101,

  /// An information from the player that buffering have been finished;
  /// no additional parameters.
  kBufferingCompleted = 102,

  /// An information from the player that stream has finished;
  /// no additional parameters.
  kStreamEnded   = 103,
  kSetAudioLevel = 104,
  kSendStats     = 105,
};

/// @enum ClipTypeEnum
/// This enum is used to define type of clip which is requested. Basing on
/// this information the player can prepare accurate playback pipeline. \n
/// It is used in a <code>MessageToPlayer::kLoadMedia</code> message.
enum class ClipTypeEnum {
  /// @private this type enum value is not supported.
  kUnknown = 0,
  kRTSP = 1,
};

/// A string value used in messages as a <code>VarDictionary</code> key.
/// <code>kKeyMessageToPlayer</code> has to be used in all messages addressed
/// to the player. The value sent in a field with this key is used to define
/// what kind of message it is. The only values accepted for this field are the
/// ones defined by the <code>enum MessageToPlayer</code>.
/// @see Communication::MessageToPlayer
const std::string kKeyMessageToPlayer = "messageToPlayer";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// <code>kKeyMessageFromPlayer</code> is used in all messages sent by the
/// player. The value in a field with this key is used to define what
/// kind of message it is. The only values the player can use for this field
/// are values defined by <code>enum MessageFromPlayer</code>
/// @see Communication::MessageFromPlayer
const std::string kKeyMessageFromPlayer = "messageFromPlayer";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyBitrate = "bitrate";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>double</code> type value.
const std::string kKeyDuration = "duration";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeyEncoding = "encoding";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyId = "id";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeyLanguage = "language";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeySubtitle = "subtitle";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>double</code> type value.
const std::string kKeyTime = "time";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyType = "type";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeyUrl = "url";

/// A string value used in messages as a <code>VarDictionary</code> key.
const std::string kKeyUpdateFrequency = "audio_level_cb_frequency";
/// This key maps to an <code>int</code> type value.
const std::string kKeyWidth = "width";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyHeight = "height";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyXCoordination = "x_coordinate";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code>type value.
const std::string kKeyYCoordination = "y_coordinate";

const std::string kKeyAudioLevel   = "audio_level";
const std::string kKeyStatsLost    = "stats_lost";
const std::string kKeyStatsJitter  = "stats_jitter";
const std::string kKeyStatsBitrate = "stats_bitrate";
}  // namespace Communication

#endif  // NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGES_H_
