#include "Arduino.h"
#include "ArduinoJson.h"

enum SensorCommand {
  ON = 0x01,
  OFF = 0x0,
};

struct CommandResponse {
  String operation;
  bool status;
  int pin;
};

struct Command {
  String operation;
  int command;
  int pin;
};

enum HandshakeMessage {
  Connect = 0x01,
  ConnectionAccepted = 0x02,
  ConnectionRefused = 0x03,
  ConnectionClosed = 0x04
};

enum RecordType { Simple = 'S', Multi = 'M' };

struct Record {
  String node;
  String recipient;
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
  Axon(String node, String genHash, String privateKey, String deviceId);

  /* Starts a serial stream with a serial stream object */
  void begin(Stream& serial);

  /* Writes to the serial port */
  void send(Record record, RecordType recordType);

  /* Gets the last known status of Axon (handshake, command response) */
  AxonStatus<HandshakeMessage>& getConnectionStatus();
  AxonStatus<CommandResponse>& getCommandStatus();

  /* Watches (reads) the Serial stream, and parses a command.
  The function takes in an array of pins, which it then will trigger if it
  senses a command for that specific pin.
  If it's a handhshake, it handles that too. */
  Command watch();

  // Converts a string to a handshake struct
  HandshakeMessage toHandshake(String data);

  // Checks if a string is handshake
  bool isHandshake(String data);

  // Conerts a string into a command struct
  Command toCommand(String data);

  // Checks if a string is command
  bool isCommand(String data);

  // Executes a given command
  void executeCommand(Command command);


  void notifyState();

  void init();

  void requestInit();

  // send a response to a handshake in the serial port
  void sendHandshakeResponse(HandshakeMessage code);

  // send an acknowledgement that the command was recieved
  void sendCommandResponse(CommandResponse response);

  // enables or disables debug
  void debug(bool option);

  // Updates axon with latest status update
  template <typename T>
  void setAxonStatus(T update, AxonStatus<T>& status);

 private:
  Stream* _serial;
  Stream* _usbSerial;
  String _node;
  String _deviceId;
  String _genHash;
  String _privateKey;
  bool _logging;
  void log(String message);
  AxonStatus<HandshakeMessage> _connectionStatus;
  AxonStatus<CommandResponse> _commandStatus;
};