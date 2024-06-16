#include "ModbusTCP.h"

ModbusTCP::ModbusTCP()
    : initialized(false), server(502)
{
    WiFiServer server(502);
    WiFiClient client;
}

void ModbusTCP::begin()
{
    if (initialized)
    {
        Serial.print("ssid: ");
        Serial.println(ssid);
        Serial.print("password: ");
        Serial.println(password);
        Serial.print("port: ");
        Serial.println(port);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        Serial.println("");
        Serial.println("WiFi connected.");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());

        server.begin();
        Serial.println("ModbusTCP is ready");
    }
}

void ModbusTCP::init(const char *ssid, const char *password, int port)
{
    setSSID(ssid);
    setPassword(password);
    setPort(port);
    initialized = true;
}

void ModbusTCP::setSSID(const char *ssid)
{
    strncpy(this->ssid, ssid, sizeof(this->ssid) - 1);
}

void ModbusTCP::setPassword(const char *password)
{
    strncpy(this->password, password, sizeof(this->password) - 1);
}

void ModbusTCP::setPort(int port)
{
    this->port = port;
}

void ModbusTCP::setStatus(int value)
{
    switch (value)
    {
    case 3:

        break;

    case 6:

        break;

    case 8:

        break;

    case 17:

        break;

    default:

        break;
    }

    Serial.print("The status has been changed to: ");
    Serial.println("");
}

void ModbusTCP::iterate()
{
    client = server.available();

    if (client)
    {
        Serial.println("New Client.");
        while (client.connected())
        {
            if (client.available())
            {
                uint8_t request[256];
                int len = client.read(request, sizeof(request));

                if (len >= 8)
                {
                    handleRequest(client, request, len);
                }
            }
            updateOutput();
            handleCommand();
        }
        client.stop();
        Serial.println("Client disconnected.");
    }
    updateOutput();
    handleCommand();
}

void ModbusTCP::handleRequest(WiFiClient &client, uint8_t *request, int len)
{
    uint16_t transactionID = (request[0] << 8) | request[1];
    uint16_t protocolID = (request[2] << 8) | request[3];
    uint16_t length = (request[4] << 8) | request[5];
    uint8_t unitID = request[6];
    uint8_t functionCode = request[7];
    uint8_t response[256];

    // Parse Modbus TCP request
    switch (functionCode)
    {
    case 0x01: // Read Coils
        handleReadCoils(request, response, len);
        handleResponse(client, response, 6 + response[5]);
        break;
    case 0x02: // Read Discrete Inputs
        handleReadDiscreteInputs(request, response, len);
        handleResponse(client, response, 6 + response[5]);
        break;
    case 0x03: // Read Holding Registers
        handleReadHoldingRegisters(request, response, len);
        handleResponse(client, response, 6 + response[5]);
        break;
    case 0x04: // Read Input Registers
        handleReadInputRegisters(request, response, len);
        handleResponse(client, response, 6 + response[5]);
        break;
    case 0x05: // Write Single Coil
        handleWriteSingleCoil(request, response, 12);
        handleResponse(client, response, 12);
        break;
    case 0x06: // Write Single Register
        handleWriteSingleRegister(request, response);
        handleResponse(client, response, 12);
        break;
    case 0x0F: // Write Multiple Coils
        handleWriteMultipleCoils(request, response, len);
        handleResponse(client, response, 12);
        break;
    case 0x10: // Write Multiple Register
        handleWriteMultipleRegisters(request, response, len);
        handleResponse(client, response, 12);
        break;
    default:
        // Handle unsupported function codes
        response[0] = request[0];
        response[1] = request[1];
        response[2] = request[2];
        response[3] = request[3];
        response[4] = 0x00;
        response[5] = 0x03; // Length of the error response
        response[6] = request[6];
        response[7] = functionCode | 0x80; // Error response
        response[8] = 0x01;                // Illegal function
        handleResponse(client, response, 9);
        return;
    }
}

void ModbusTCP::handleResponse(WiFiClient &client, uint8_t *response, int len)
{
    printResponse(response, len);
    client.write(response, len);
}

void ModbusTCP::prepareResponse(uint8_t *request, uint8_t *response, uint8_t byteCount, int type)
{
    switch (type)
    {
    case 1: // response for write coil
        for (int i = 0; i < 12; i++)
        {
            response[i] = request[i];
        }
        break;
    case 2: // response for write reqisters and coils
        for (int i = 0; i < 12; i++)
        {
            if (i == 4)
            {
                response[i] = 0x00;
            }
            else if (i == 5)
            {
                response[i] = 0x06;
            }
            else
            {
                response[i] = request[i];
            }
        }
        break;
    case 3: // response for read
        response[0] = request[0];
        response[1] = request[1];
        response[2] = request[2];
        response[3] = request[3];
        response[4] = 0x00;
        response[5] = 3 + byteCount;
        response[6] = request[6];
        response[7] = request[7];
        response[8] = byteCount;
        break;
    case 4:                              // response for error data address
        response[0] = request[0];        // Transaction ID Hi
        response[1] = request[1];        // Transaction ID Lo
        response[2] = request[2];        // Protocol ID Hi
        response[3] = request[3];        // Protocol ID Lo
        response[4] = 0x00;              // Length Hi
        response[5] = 0x03;              // Length Lo
        response[6] = request[6];        // Unit ID
        response[7] = request[7] | 0x80; // Function Code + 0x80 (Error Indicator)
        response[8] = 0x02;              // Exception Code (Illegal Data Address)
        break;
    case 5: // response for error data value
        response[0] = request[0];
        response[1] = request[1];
        response[2] = request[2];
        response[3] = request[3];
        response[4] = 0x00;
        response[5] = 0x03;
        response[6] = request[6];
        response[7] = request[7] | 0x80;
        response[8] = 0x03;
        break;
    default:
        Serial.println("Unknown Response Type");
    }
}

void ModbusTCP::handleReadCoils(uint8_t *request, uint8_t *response, int len)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = (quantity + 7) / 8;

    if (!handleError(request, response, 1))
    {
        printPreparedRequest(startAddress, quantity, byteCount, "Read Coils");
        prepareResponse(request, response, byteCount, 3);

        for (int i = 0; i < byteCount; i++)
        {
            response[9 + i] = 0;
            for (int j = 0; j < 8; j++)
            {
                int coilIndex = startAddress + i * 8 + j;
                if (coilIndex < startAddress + quantity)
                {
                    if (coils[coilIndex])
                    {
                        response[9 + i] |= (1 << j);
                    }
                }
            }
        }
    }
}

void ModbusTCP::handleReadDiscreteInputs(uint8_t *request, uint8_t *response, int len)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = (quantity + 7) / 8;
    if (!handleError(request, response, 2))
    {

        printPreparedRequest(startAddress, quantity, byteCount, "Read Discrete Inputs");
        prepareResponse(request, response, byteCount, 3);

        for (int i = 0; i < byteCount; i++)
        {
            response[9 + i] = 0;
            for (int j = 0; j < 8; j++)
            {
                int inputIndex = startAddress + i * 8 + j;
                if (inputIndex < startAddress + quantity)
                {
                    if (discreteInputs[inputIndex])
                    {
                        response[9 + i] |= (1 << j);
                    }
                }
            }
        }
    }
}

void ModbusTCP::handleReadHoldingRegisters(uint8_t *request, uint8_t *response, int len)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = quantity * 2;
    if (!handleError(request, response, 3))
    {

        printPreparedRequest(startAddress, quantity, byteCount, "Read Holding Registers");
        prepareResponse(request, response, byteCount, 3);

        for (int i = 0; i < quantity; i++)
        {
            uint16_t value = holdingRegisters[startAddress + i];
            response[9 + i * 2] = value >> 8;       // High byte
            response[9 + i * 2 + 1] = value & 0xFF; // Low byte
        }
    }
}

void ModbusTCP::handleReadInputRegisters(uint8_t *request, uint8_t *response, int len)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = quantity * 2;
    if (!handleError(request, response, 4))
    {
        printPreparedRequest(startAddress, quantity, byteCount, "Read Input Registers");
        prepareResponse(request, response, byteCount, 3);

        for (int i = 0; i < quantity; i++)
        {
            uint16_t registerValue = inputRegisters[startAddress + i];
            response[9 + i * 2] = registerValue >> 8;       // Hi byte
            response[9 + i * 2 + 1] = registerValue & 0xFF; // Lo byte
        }
    }
}

void ModbusTCP::handleWriteSingleCoil(uint8_t *request, uint8_t *response, int len)
{
    uint16_t coilAddress = (request[8] << 8) | request[9];
    uint16_t coilValue = (request[10] << 8) | request[11];

    if (!handleError(request, response, coilAddress, 0))
    {
        Serial.print("Write Single Coil - Address: ");
        Serial.print(coilAddress);
        Serial.print(", Value: ");
        Serial.println(coilValue);

        coils[coilAddress] = (coilValue == 0xFF00) ? 1 : 0;

        prepareResponse(request, response, 0, 1);
    }
}

void ModbusTCP::handleWriteMultipleCoils(uint8_t *request, uint8_t *response, int len)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = request[12];

    if (!handleError(request, response, 1))
    {
        printPreparedRequest(startAddress, quantity, byteCount, "Write Multiple Coils");

        // Aktualizacja cewek
        for (int i = 0; i < quantity; i++)
        {
            int byteIndex = 13 + (i / 8);
            int bitIndex = i % 8;
            if (request[byteIndex] & (1 << bitIndex))
            {
                coils[startAddress + i] = 1;
            }
            else
            {
                coils[startAddress + i] = 0;
            }
        }

        for (int i = 0; i < quantity; i++)
        {
            Serial.print(startAddress + i);
            Serial.print(":");
            Serial.print(coils[startAddress + i]);
            Serial.print(", ");
        }
        Serial.println("");
        prepareResponse(request, response, byteCount, 2);
    }
}

void ModbusTCP::handleWriteSingleRegister(uint8_t *request, uint8_t *response)
{
    uint16_t registerAddress = (request[8] << 8) | request[9];
    uint16_t registerValue = (request[10] << 8) | request[11];

    if (!handleError(request, response, registerAddress, 1))
    {

        Serial.print("Write Single Register - Address: ");
        Serial.print(registerAddress);
        Serial.print(", Value: ");
        Serial.println(registerValue);

        holdingRegisters[registerAddress] = (request[10] << 8) | request[11];

        prepareResponse(request, response, 0, 2);
    }
}

void ModbusTCP::handleWriteMultipleRegisters(uint8_t *request, uint8_t *response, int len)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = request[12];

    printPreparedRequest(startAddress, quantity, byteCount, "Write Multiple Registers");

    if (!handleError(request, response, 3))
    {
        for (int i = 0; i < quantity; i++)
        {
            int byteIndex = 13 + (i * 2);
            holdingRegisters[startAddress + i] = (request[byteIndex] << 8) | request[byteIndex + 1];
        }

        for (int i = 0; i < quantity; i++)
        {
            Serial.print(startAddress + i);
            Serial.print(":");
            Serial.print(holdingRegisters[startAddress + i]);
            Serial.print(", ");
        }
        Serial.println("");

        prepareResponse(request, response, 0, 2);
    }
}

void ModbusTCP::printRequest(uint8_t *request, int len)
{
    Serial.print("Request: ");
    for (int i = 0; i < len; i++)
    {
        Serial.print(request[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
}

void ModbusTCP::printResponse(uint8_t *response, int len)
{
    Serial.print("Response: ");
    for (int i = 0; i < len; i++)
    {
        Serial.print(response[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
}

void ModbusTCP::printPreparedRequest(uint16_t startAddress, uint16_t quantity, uint8_t byteCount, String functionName)
{
    Serial.print(functionName);
    Serial.print(" -> Start Address: ");
    Serial.print(startAddress);
    Serial.print(", Quantity: ");
    Serial.print(quantity);
    Serial.print(", ByteCount: ");
    Serial.println(byteCount);
}

bool ModbusTCP::handleError(uint8_t *request, uint8_t *response, int type)
{
    uint16_t startAddress = (request[8] << 8) | request[9];
    uint16_t quantity = (request[10] << 8) | request[11];
    uint8_t byteCount = quantity * 2;

    bool error = false;

    switch (type)
    {
    case 1:
        if (startAddress + quantity > sizeof(coils) / sizeof(coils[0]))
        {
            prepareResponse(request, response, byteCount, 4);
            Serial.println("Error: Illegal Data Address");
            error = true;
        }

        if (quantity == 0 || quantity > sizeof(coils) / sizeof(coils[0]))
        {
            prepareResponse(request, response, byteCount, 5);
            Serial.println("Error: Illegal Data Value");
            error = true;
        }
        break;
    case 2:
        if (startAddress + quantity > sizeof(discreteInputs) / sizeof(discreteInputs[0]))
        {
            prepareResponse(request, response, byteCount, 4);
            Serial.println("Error: Illegal Data Address");
            error = true;
        }

        if (quantity == 0 || quantity > sizeof(discreteInputs) / sizeof(discreteInputs[0]))
        {
            prepareResponse(request, response, byteCount, 5);
            Serial.println("Error: Illegal Data Value");
            error = true;
        }
        break;
    case 3:
        if (startAddress + quantity > sizeof(holdingRegisters) / sizeof(holdingRegisters[0]))
        {
            prepareResponse(request, response, byteCount, 4);
            Serial.println("Error: Illegal Data Address");
            error = true;
        }

        if (quantity == 0 || quantity > sizeof(holdingRegisters) / sizeof(holdingRegisters[0]))
        {
            prepareResponse(request, response, byteCount, 5);
            Serial.println("Error: Illegal Data Value");
            error = true;
        }
        break;
    case 4:
        if (startAddress + quantity > sizeof(inputRegisters) / sizeof(inputRegisters[0]))
        {
            prepareResponse(request, response, byteCount, 4);
            Serial.println("Error: Illegal Data Address");
            error = true;
        }

        if (quantity == 0 || quantity > sizeof(inputRegisters) / sizeof(inputRegisters[0]))
        {
            prepareResponse(request, response, byteCount, 5);
            Serial.println("Error: Illegal Data Value");
            error = true;
        }
        break;
    default:
        error = false;
        break;
    }

    return error;
}

bool ModbusTCP::handleError(uint8_t *request, uint8_t *response, uint16_t address, bool type)
{
    bool error = false;
    if (type)
    {
        if (address >= sizeof(holdingRegisters) / sizeof(holdingRegisters[0]))
        {
            prepareResponse(request, response, 0, 4);
            Serial.println("Error: Illegal Data Address");
            error = true;
        }
    }
    else
    {
        if (address >= sizeof(coils) / sizeof(coils[0]))
        {
            prepareResponse(request, response, 0, 4);
            Serial.println("Error: Illegal Data Address");
            error = true;
        }
    }

    return error;
}

uint8_t ModbusTCP::getCoil(int id)
{
    return coils[id];
}

uint16_t ModbusTCP::getHoldingRegister(int id)
{
    return holdingRegisters[id];
}

void ModbusTCP::setHoldingRegister(int id, uint16_t value)
{
    holdingRegisters[id] = value;
}

void ModbusTCP::updateOutput()
{
    for (int i = 0; i < 10; i++)
    {
        if (i == 0 && getCoil(i))
        {
            digitalWrite(ledPin, LOW);
        }
        else if (i == 0 && !getCoil(i))
        {
            digitalWrite(ledPin, HIGH);
        }
    }
}

void ModbusTCP::handleCommand()
{
    if (Serial.available() > 0)
    {
        String input = Serial.readString();

        if (input.startsWith("AI"))
        {
            int equalSignIndex = input.indexOf('=');
            if (equalSignIndex > 2 && equalSignIndex < input.length() - 1)
            {
                String regNumStr = input.substring(2, equalSignIndex);
                String valueStr = input.substring(equalSignIndex + 1);

                int regNum = regNumStr.toInt();
                int value = valueStr.toInt();

                if (regNum >= 1 && regNum <= 10)
                {
                    inputRegisters[regNum - 1] = value;
                    Serial.print("Updated AI");
                    Serial.print(regNum);
                    Serial.print(" to ");
                    Serial.println(value);
                }
                else
                {
                    Serial.println("Invalid register number");
                }
            }
            else
            {
                Serial.println("Invalid command format");
            }
        }
        else if (input.startsWith("R"))
        {
            int equalSignIndex = input.indexOf('=');
            if (equalSignIndex > 1 && equalSignIndex < input.length() - 1)
            {
                String regNumStr = input.substring(1, equalSignIndex);
                String valueStr = input.substring(equalSignIndex + 1);

                int regNum = regNumStr.toInt();
                int value = valueStr.toInt();

                if (regNum >= 1 && regNum <= 10)
                {
                    holdingRegisters[regNum - 1] = value;
                    Serial.print("Updated R");
                    Serial.print(regNum);
                    Serial.print(" to ");
                    Serial.println(value);
                }
                else
                {
                    Serial.println("Invalid register number");
                }
            }
            else
            {
                Serial.println("Invalid command format");
            }
        }
        else if (input.startsWith("DI"))
        {
            int equalSignIndex = input.indexOf('=');
            if (equalSignIndex > 2 && equalSignIndex < input.length() - 1)
            {
                String regNumStr = input.substring(2, equalSignIndex);
                String valueStr = input.substring(equalSignIndex + 1);

                int regNum = regNumStr.toInt();
                int value = valueStr.toInt();

                if ((regNum >= 1 && regNum <= 10) && (value == 1 || value == 0))
                {
                    discreteInputs[regNum - 1] = value;
                    Serial.print("Updated Discrete Input");
                    Serial.print(regNum);
                    Serial.print(" to ");
                    Serial.println(value);
                }
                else
                {
                    Serial.println("Invalid register number");
                }
            }
            else
            {
                Serial.println("Invalid command format");
            }
        }
        else if (input.startsWith("DO"))
        {
            int equalSignIndex = input.indexOf('=');
            if (equalSignIndex > 2 && equalSignIndex < input.length() - 1)
            {
                String regNumStr = input.substring(2, equalSignIndex);
                String valueStr = input.substring(equalSignIndex + 1);

                int regNum = regNumStr.toInt();
                int value = valueStr.toInt();

                if ((regNum >= 1 && regNum <= 10) && (value == 1 || value == 0))
                {
                    coils[regNum - 1] = value;
                    Serial.print("Updated Discrete Input");
                    Serial.print(regNum);
                    Serial.print(" to ");
                    Serial.println(value);
                }
                else
                {
                    Serial.println("Invalid register number");
                }
            }
            else
            {
                Serial.println("Invalid command format");
            }
        }
        else
        {
            Serial.println("Unknown command");
        }
    }
}