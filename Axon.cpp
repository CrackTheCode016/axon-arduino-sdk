#include "Axon.h"

Axon::Axon(Node* node,
           GenerationHash* genHash,
           Key* ownerPublicKey,
           String deviceId) {
  _node = node;
  _ownerPublicKey = ownerPublicKey;
  _genHash = genHash;
  _usbSerial = &Serial;
  _deviceId = deviceId;
}

void Axon::init() {
  HandshakeRequest request = {AxonHandshakeType::HandshakeConnect,
                              AxonMessageType::StateMessage};
  while (true) {
    Axon::sendHandshakeRequest(request);
    delay(1000);
    String data = _serial->readString();
    if (Axon::isHandshake(data)) {
      HandshakeResponse response = toHandshakeResponse(data);
      if (response.handshakeType == AxonHandshakeType::HandshakeAccept) {
        log(String("Device connected with status " + _connectionStatus.status));
        Axon::notifyState();
        break;
      }
    }
  }
}

void Axon::begin(Stream& serial) {
  _serial = &serial;
  delay(4000);
  Axon::init();
}

void Axon::notifyState() {
  String state = Axon::serializeState();
  _serial->println(state);
}

String Axon::serializeRecord(Record record, RecordType type) {
  const size_t capacity =
      JSON_OBJECT_SIZE(RECORD_OBJ_SIZE) + RECORD_EXTRA_BYTES_AMOUNT;

  DynamicJsonDocument doc(capacity);
  doc["node"] = record.node;
  doc["data"] = record.data;
  doc["deviceId"] = _deviceId;
  doc["recipient"] = record.recipient;
  doc["recordType"] = static_cast<char>(type);
  doc["sensorName"] = record.sensorName;
  doc["encrypted"] = record.encrypted;

  String output;
  serializeJson(doc, output);
  return output;
}

String Axon::serializeState() {
  const size_t capacity =
      JSON_OBJECT_SIZE(STATE_OBJ_SIZE) + STATE_EXTRA_BYTES_AMOUNT;
  DynamicJsonDocument doc(capacity);
  doc["ownerPublicKey"] = _ownerPublicKey;
  doc["genHash"] = _genHash;
  doc["node"] = _node;

  String output;
  serializeJson(doc, output);
  return output;
}

void Axon::send(Record record, RecordType type) {
  String serialized = Axon::serializeRecord(record, type);
  _serial->println(serialized);
  log(String("Record sent for " + record.sensorName));
}

void Axon::log(String message) {
  if (_logging)
    _usbSerial->println("[DEBUG] " + message);
}

void Axon::debug(bool option) {
  _logging = option;
}

Command Axon::watch() {
  Command command;
  HandshakeResponse response = {AxonHandshakeType::HandshakeAccept};
  String data = _serial->readString();
  if (Axon::isHandshake(data)) {
    log(String("Handshake detected."));
    HandshakeRequest message = toHandshakeRequest(data);
    if (message.handshakeType == AxonHandshakeType::HandshakeConnect) {
      sendHandshakeResponse(response);
      log(String("Device connected with status " + _connectionStatus.status));
      delay(1000);
      while (_serial->available() == 0) {
        log(String("Waiting for command..."));
      }
      String expectedCommand = _serial->readString();
      if (Axon::isCommand(expectedCommand)) {
        log(String("Command detected: " + expectedCommand));
        command = Axon::toCommand(expectedCommand);
      }
    }
  }
  return command;
}

template <typename T>
void Axon::setAxonStatus(T update, AxonStatus<T>& status) {
  status.status = update;
}

AxonStatus<AxonHandshakeType>& Axon::getConnectionStatus() {
  return _connectionStatus;
}

// Handshake

HandshakeResponse Axon::toHandshakeResponse(String input) {
  const size_t capacity = JSON_OBJECT_SIZE(HANDSHAKE_RESPONSE_OBJ_SIZE) +
                          HANDSHAKE_EXTRA_BYTES_AMOUNT;
  String dataString = input.substring(1, input.length());
  StaticJsonDocument<capacity> doc;
  DeserializationError err = deserializeJson(doc, dataString);
  if (err == DeserializationError::Ok) {
    log(String("parsed successfully"));
  } else {
    log(String("failure: not valid json"));
  }

  HandshakeResponse response = {
      static_cast<AxonHandshakeType>(doc["handshakeType"].as<int>())};
  return response;
}

HandshakeRequest Axon::toHandshakeRequest(String input) {
  const size_t capacity = JSON_OBJECT_SIZE(HANDSHAKE_REQUEST_OBJ_SIZE) +
                          HANDSHAKE_EXTRA_BYTES_AMOUNT;
  String dataString = input.substring(1, input.length());
  StaticJsonDocument<capacity> doc;
  DeserializationError err = deserializeJson(doc, dataString);
  if (err == DeserializationError::Ok) {
    log(String("parsed successfully"));
  } else {
    log(String("failure: not valid json"));
  }
  HandshakeRequest request = {
      static_cast<AxonHandshakeType>(doc["handshakeType"].as<int>()),
      static_cast<AxonMessageType>(doc["messageType"].as<int>())};

  return request;
}

bool Axon::isHandshake(String data) {
  if (data.charAt(0) == 'H') {
    return true;
  }
  return false;
}

void Axon::sendHandshakeRequest(HandshakeRequest request) {
  const size_t capacity = JSON_OBJECT_SIZE(HANDSHAKE_REQUEST_OBJ_SIZE) +
                          HANDSHAKE_EXTRA_BYTES_AMOUNT;
  DynamicJsonDocument doc(capacity);
  doc["handshakeType"] = request.handshakeType;
  doc["messageType"] = request.messageType;
  String output;
  serializeJson(doc, output);
  _serial->println(output);
}

void Axon::sendHandshakeResponse(HandshakeResponse response) {
  const size_t capacity = JSON_OBJECT_SIZE(HANDSHAKE_RESPONSE_OBJ_SIZE) +
                          HANDSHAKE_EXTRA_BYTES_AMOUNT;
  DynamicJsonDocument doc(capacity);
  doc["handshakeType"] = response.handshakeType;
  String output;
  serializeJson(doc, output);
  _serial->println(output);
}

// Commands

Command Axon::toCommand(String input) {
  const size_t capacity =
      JSON_OBJECT_SIZE(COMMAND_OBJ_SIZE) + COMMAND_EXTRA_BYTES_AMOUNT;
  String dataString = input.substring(1, input.length());
  StaticJsonDocument<capacity> doc;
  DeserializationError err = deserializeJson(doc, dataString);
  if (err == DeserializationError::Ok) {
    log(String("parsed successfully"));
  } else {
    log(String("failure: not valid json"));
  }
  Command command = {String(doc["operation"].as<char*>()),
                     doc["command"].as<int>(), doc["pin"].as<int>()};
  return command;
}

bool Axon::isCommand(String input) {
  if (input.charAt(0) == 'C') {
    return true;
  }
  return false;
}

void Axon::executeCommand(Command command) {
  digitalWrite(command.pin, command.command);
  CommandResponse response = {command, true};
  sendCommandResponse(response);
}

void Axon::sendCommandResponse(CommandResponse response) {
  setAxonStatus<CommandResponse>(response, _commandStatus);
}
