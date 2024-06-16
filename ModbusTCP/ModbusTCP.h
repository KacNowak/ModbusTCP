#ifndef ModbusTCP_h
#define ModbusTCP_h

#include <ESP8266WiFi.h>

class ModbusTCP
{
protected:
  char ssid[32];
  char password[32];
  int port;
  bool status;
  bool initialized;
  uint8_t coils[10] = {0};
  uint8_t discreteInputs[10] = {0};
  uint16_t holdingRegisters[10] = {0};
  uint16_t inputRegisters[10] = {0};
  WiFiServer server;
  WiFiClient client;
  const int ledPin = LED_BUILTIN;

public:
  ModbusTCP();

  void begin();
  void init(const char *ssid, const char *password, int port);
  void iterate();

  // Get
  uint8_t getCoil(int id);
  uint16_t getHoldingRegister(int id);

  // Set
  void setStatus(int value);
  void setSSID(const char *ssid);
  void setPassword(const char *password);
  void setPort(int port);
  void setHoldingRegister(int id, uint16_t value);

  void handleRequest(WiFiClient &client, uint8_t *request, int len);
  void handleResponse(WiFiClient &client, uint8_t *response, int len);
  void prepareResponse(uint8_t *request, uint8_t *response, uint8_t byteCount, int type);
  bool handleError(uint8_t *request, uint8_t *response, int type);
  bool handleError(uint8_t *request, uint8_t *response, uint16_t address, bool type);

  // Handle Read
  void handleReadCoils(uint8_t *request, uint8_t *response, int len);
  void handleReadDiscreteInputs(uint8_t *request, uint8_t *response, int len);
  void handleReadHoldingRegisters(uint8_t *request, uint8_t *response, int len);
  void handleReadInputRegisters(uint8_t *request, uint8_t *response, int len);

  // Handle Read
  void handleWriteSingleCoil(uint8_t *request, uint8_t *response, int len);
  void handleWriteMultipleCoils(uint8_t *request, uint8_t *response, int len);
  void handleWriteSingleRegister(uint8_t *request, uint8_t *response);
  void handleWriteMultipleRegisters(uint8_t *request, uint8_t *response, int len);

  // Print
  void printRequest(uint8_t *request, int len);
  void printResponse(uint8_t *response, int len);
  void printPreparedRequest(uint16_t startAddress, uint16_t quantity, uint8_t byteCount, String functionName);

  // Control led
  void updateOutput();
  void handleCommand();
};
#endif
