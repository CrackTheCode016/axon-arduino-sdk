#include "Axon.h"

Axon::Axon(String node, String genHash, String privateKey, String deviceId) {
  _node = node;
  _privateKey = privateKey;
  _genHash = genHash;
  _usbSerial = &Serial;
  _deviceId = deviceId;
}

void Axon::init() {
  String data = _serial->readString();
  while (!Axon::isHandshake(data)) {
    Axon::requestInit();
    delay(1000);
    data = _serial->readString();
    if (Axon::isHandshake(data)) {
      HandshakeMessage message = toHandshake(data);
      if (message == HandshakeMessage::Connect) {
        sendHandshakeResponse(HandshakeMessage::ConnectionAccepted);
        log(String("Device connected with status " + _connectionStatus.status));
        Axon::notifyState();
        sendHandshakeResponse(HandshakeMessage::ConnectionClosed);
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
  const size_t capacity = JSON_OBJECT_SIZE(3) + 190;
  DynamicJsonDocument doc(capacity);
  doc["user_private_key"] = _privateKey;
  doc["node_ip"] = _node;
  doc["gen_hash"] = _genHash;
  String output;
  output.concat("SI");
  serializeJson(doc, output);
  _serial->println(output);
}

void Axon::send(Record record, RecordType recordType) {
  const size_t capacity = JSON_OBJECT_SIZE(7) + 202;
  DynamicJsonDocument doc(capacity);
  doc["node"] = record.node;
  doc["data"] = record.data;
  doc["deviceId"] = _deviceId;
  doc["recipient"] = record.recipient;
  doc["recordType"] = static_cast<char>(recordType);
  doc["sensorName"] = record.sensorName;
  doc["encrypted"] = record.encrypted;

  String output;
  serializeJson(doc, output);
  _serial->println(output);
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
  String data = _serial->readString();
  if (Axon::isHandshake(data)) {
    log(String("Handshake detected."));
    HandshakeMessage message = toHandshake(data);
    if (message == HandshakeMessage::Connect) {
      sendHandshakeResponse(HandshakeMessage::ConnectionAccepted);
      log(String("Device connected with status " + _connectionStatus.status));
      delay(1000);
      while (_serial->available() == 0) {
        log(String("Waiting for command..."));
      }
      String expectedCommand = _serial->readString();
      if (Axon::isCommand(expectedCommand)) {
        log(String("Command detected: " + expectedCommand));
        command = Axon::toCommand(expectedCommand);
        sendHandshakeResponse(HandshakeMessage::ConnectionClosed);
      }

      else {
        sendHandshakeResponse(HandshakeMessage::ConnectionClosed);
      }
    }

    else {
      sendHandshakeResponse(HandshakeMessage::ConnectionRefused);
    }
  }
  return command;
}

template <typename T>
void Axon::setAxonStatus(T update, AxonStatus<T>& status) {
  status.status = update;
}

AxonStatus<HandshakeMessage>& Axon::getConnectionStatus() {
  return _connectionStatus;
}

// Handshake

HandshakeMessage Axon::toHandshake(String data) {
  String handshake = data.substring(1, data.length());
  return static_cast<HandshakeMessage>(handshake.toInt());
}

bool Axon::isHandshake(String data) {
  if (data.charAt(0) == 'H') {
    return true;
  }
  return false;
}

void Axon::sendHandshakeResponse(HandshakeMessage code) {
  _serial->print("H");
  _serial->print(code);
  _serial->print("\n");
  setAxonStatus<HandshakeMessage>(code, _connectionStatus);
}

void Axon::requestInit() {
  _serial->print("I");
  _serial->print(1);
  _serial->print("\n");
}

// Commands

Command Axon::toCommand(String data) {
  const int capacity = JSON_OBJECT_SIZE(8);
  StaticJsonDocument<capacity> doc;
  String commandString = data.substring(1, data.length());

  DeserializationError err = deserializeJson(doc, commandString);
  if (err == DeserializationError::Ok) {
    log(String("parsed successfully"));
  } else {
    log(String("failure: not valid json"));
  }
  Command command = {String(doc["operation"].as<char*>()),
                     doc["command"].as<int>(), doc["pin"].as<int>()};
  return command;
}

bool Axon::isCommand(String data) {
  if (data.charAt(0) == 'C') {
    return true;
  }
  return false;
}

void Axon::executeCommand(Command command) {
  digitalWrite(command.pin, command.command);
  CommandResponse response = {command.operation, true, command.pin};
  sendCommandResponse(response);
}

void Axon::sendCommandResponse(CommandResponse response) {
  setAxonStatus<CommandResponse>(response, _commandStatus);
}
