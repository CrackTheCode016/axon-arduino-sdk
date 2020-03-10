#pragma once
#include <Arduino.h>

#define COMMAND_OBJ_SIZE 4
#define COMMAND_EXTRA_BYTES_AMOUNT 118

#define HANDSHAKE_REQUEST_OBJ_SIZE 2
#define HANDSHAKE_RESPONSE_OBJ_SIZE 1

#define HANDSHAKE_EXTRA_BYTES_AMOUNT 50

#define RECORD_OBJ_SIZE 7
#define RECORD_EXTRA_BYTES_AMOUNT 250

#define STATE_OBJ_SIZE 3
#define STATE_EXTRA_BYTES_AMOUNT 250

using Address = char[47];
using Key = char[65];
using GenerationHash = char[65];
using Node = String;
using Data = String;