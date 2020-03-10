#pragma once

#include <Arduino.h>
#include "ArduinoJson.h"
#include "types.h"

enum SensorCommand {
  ON = 0x01,
  OFF = 0x0,
};

struct Command {
  String operationDescription;
  int currencyAmount;
  int command;
  int pin;
};

struct CommandResponse {
  Command command;
  bool status;
};

enum AxonHandshakeType { HandshakeConnect = 18499, HandshakeAccept = 18497 };
enum AxonMessageType {
  RecordMessage = 0,
  StateMessage = 1,
  CommandMessage = 2
};
enum RecordType { Simple = 83, Multi = 78 };

struct HandshakeRequest {
  AxonHandshakeType handshakeType;
  AxonMessageType messageType;
};

struct HandshakeResponse {
  AxonHandshakeType handshakeType;
};

struct Record {
  Node* node;
  Address* recipient;
  String data;
  String sensorName;
  bool encrypted;
};

template <typename T>
struct AxonStatus {
  T status;
};

class Axon {
 public:
  Axon(Node* node,
       GenerationHash* genHash,
       Key* ownerPublicKey,
       String deviceId);

  /* Starts a serial stream with a serial stream object */
  void begin(Stream& serial);

  /* Writes a record to the serial port */
  void send(Record record, RecordType type);

  // Notifies the host of a state config change
  void notifyState();

  /* Gets the last known status of Axon (handshake, command response) */
  AxonStatus<AxonHandshakeType>& getConnectionStatus();
  AxonStatus<CommandResponse>& getCommandStatus();

  /* Watches (reads) the Serial stream, and parses a command.
  The function takes in an array of pins, which it then will trigger if it
  senses a command for that specific pin.
  If it's a handshake, it handles that too. */
  Command watch();

  // Checks if a string is handshake
  bool isHandshake(String data);

  // Serializes a record
  String serializeRecord(Record record, RecordType type);

  // Serializes state
  String serializeState();

  // Parses a command into a record
  Command toCommand(String input);

  // Checks if a string is command
  bool isCommand(String data);

  // Executes a given command
  void executeCommand(Command command);

  // init process
  void init();

  // send a response to a handshake in the serial port
  void sendHandshakeResponse(HandshakeResponse response);

  // send a request for a handshake in the serial port
  void sendHandshakeRequest(HandshakeRequest request);

  //
  HandshakeResponse toHandshakeResponse(String input);

  //
  HandshakeRequest toHandshakeRequest(String input);

  // send an acknowledgement that the command was received
  void sendCommandResponse(CommandResponse response);

  // enables or disables debug
  void debug(bool option);

  // Updates axon with latest status update
  template <typename T>
  void setAxonStatus(T update, AxonStatus<T>& status);

 private:
  Stream* _serial;
  Stream* _usbSerial;
  Node* _node;
  String _deviceId;
  GenerationHash* _genHash;
  Key* _ownerPublicKey;
  bool _logging;
  void log(String message);
  AxonStatus<AxonHandshakeType> _connectionStatus;
  AxonStatus<CommandResponse> _commandStatus;
  AxonStatus<Record> _recordStatus;
};