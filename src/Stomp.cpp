#include "Stomp.h"


bool StompClient::getCurChar()
{

    if (_client->available())
    {
        curChar = (char) _client->read();
        return true;
    }
    return false;
}

bool StompClient::resolveCommand()
{
    if (curChar == 0)
        return false;
    
    trimCollectedData();

    if (collectedData =="CONNECTED")
    {
        curCommand = CMD_CONNECTED;
    }
    else if (collectedData == "MESSAGE")
    {
        curCommand = CMD_MESSAGE;
    }
    else if (collectedData == "RECEIPT")
    {
        curCommand = CMD_RECEIPT;
    }
    else if (collectedData == "ACK")
    {
        curCommand = CMD_ACK;
    }
    else if (collectedData == "NACK")
    {
        curCommand = CMD_NACK;
    }
    else if (collectedData == "ERROR")
    {
        curCommand = CMD_ERROR;
    }
    else
    {
        return false;
    }
    collectedData = "";
    return true;
}

char StompClient::collectHeader()
{
    trimCollectedData();
    if (collectedData.length() == 0)
        return 0;
    size_t delimiterIndex = collectedData.indexOf(":");

    if (delimiterIndex == 0 || delimiterIndex == -1 || delimiterIndex == collectedData.length() - 1) //invalid header
        return -1;
    
    String key = collectedData.substring(0, delimiterIndex);
    String value = collectedData.substring(delimiterIndex + 1);
    headers.insert(std::pair<String, String>(key, value));
    collectedData = "";
    return 1;
}


void StompClient::reset()
{
    curFrameState = CMD;
    collectedData = "";
    headers.clear();
}

void StompClient::processPacket()
{
    std::map<String, String>::iterator element;
    switch (curCommand)
    {
    case CMD_MESSAGE:
        if (this->_onMessage != NULL)
            this->_onMessage(headers, collectedData);
        break;
    case CMD_RECEIPT:
        element = headers.find("id");
        if (element != headers.end() && element->first == "id" && disconnectID == element->second)
        {
            connected = false;
        }
        break;
    case CMD_CONNECTED:
        connected = true;
        
        break;
    case CMD_ERROR:
        Serial.println("Error frame received!");
        Serial.println("==================Error message body======================");
        Serial.println(collectedData);
        Serial.println("==================Error message headers======================");
        for(std::map<String, String>::iterator errIt = headers.begin(); errIt != headers.end(); ++errIt)
        {
            Serial.print("error header key:");
            Serial.println(errIt->first);
            Serial.print("error header value:");
            Serial.println(errIt->second);
        }
        connected=false;
        _client->stop();
        break;
    }
}

void StompClient::trimCollectedData()
{
    collectedData.trim();
}

StompClient::StompClient(Client *client, StompAcknowledgeType ackType = ACK_AUTO)
{
    _client = client;
    theAckType = ackType;
    connected = false;
    this->_onMessage = NULL;
    reset();
}

StompClient::~StompClient()
{
    disconnect(disconnectID.c_str());
    reset();
}

void StompClient::loop()
{
    
    char collectedHeaderState = -1;
    while (getCurChar())
    {
        if (curChar == 0 || curChar == '\n')
        {
            switch (curFrameState)
            {
            case CMD:
                if (resolveCommand())
                {
                    curFrameState = HEADERS;
                    collectedData = "";
                }
                else
                    reset();
                break;
            case HEADERS:
                collectedHeaderState = collectHeader();
                if (collectedHeaderState == -1)
                {
                    reset();
                }

                if (collectedHeaderState == 0)
                {
                    curFrameState = BODY;
                }
                
                break;
            case BODY:
                if (curChar == 0)
                {
                    processPacket();
                    reset();
                }
                break;
            default:
                reset();

            }
        }
        else
            collectedData += curChar;
        Serial.println("Frame part:");
        Serial.println(collectedData);
    }

    //processHeartBeat
    long now = millis();
    long diff = (long)(2*360000/3);
    if ((now - lastHeartBeatSentTime)>=diff && connected)
    {
        Serial.println("HeartBeat Send:");
        sendHeartBeat();
        lastHeartBeatSentTime = now;
    }
}

bool StompClient::isConnected()
{
    return connected;
}

void StompClient::sendData(const char* data, bool endOfFrame)
{
    const char *start = data;
    while (*start != 0)
    {
        _client->write(*start);
        Serial.print(*start, HEX);
        start++;
    }
    if (endOfFrame)
        _client->write((char)0);
}

bool StompClient::connect(IPAddress &ip, int port, const char* protocolVersion)
{
    Serial.println(ip.toString());
    if (_client->connect(ip, port))
    {
        destinationHost=ip.toString();
        connectToStomp(protocolVersion);
        return true;
    }
    return false;
}

bool StompClient::connect(const char *hostName, int port, const char* protocolVersion)
{
    if (_client->connect(hostName, port))
    {
        destinationHost=hostName;
        connectToStomp(protocolVersion);
        return true;
    }
    return false;
}

void StompClient::connectToStomp(const char* protocolVersion)
{
    if (!isConnected() && protocolVersion != NULL)
    { 
       
       
            Serial.println("Connection attempt start!");
            sendData("CONNECT\r\n", false);
            sendData("accept-version:", false);
            if (protocolVersion != NULL)
                sendData(protocolVersion, false);
            else
                sendData("1.2", false);
            //host name || IP address
            sendData("\r\nhost:", false);
            sendData(destinationHost.c_str(), false);
            //No-body && end-of-frame
            sendData("\r\nheart-beat:360000,360000\r\n\r\n", true);
            Serial.println("Connection attempt end!");
            lastHeartBeatSentTime = millis();
        
    }
}

void StompClient::subscribe(const char* topic, const char *id)
{
    if (topic != NULL && id != NULL)
    {
        sendData("SUBSCRIBE\r\n", false);
        sendData("id:", false);
        sendData(id, false);
        sendData("\r\ndestination:", false);
        sendData(topic, false);
        sendData("\r\nack:", false);
        if (theAckType == ACK_CLIENT)
            sendData("client", false);
        else
            sendData("auto", false);
        //No-body && end-of-frame
        sendData("\r\n\r\n", true);
    }
}

void StompClient::unsubscribe(const char* topic, const char* id)
{
    if (topic != NULL && id != NULL)
    {
        sendData("UNSUBSCRIBE\r\n", false);
        sendData("id:", false);
        sendData(id, false);
        sendData("\r\n\r\n", true);
    }
}

void StompClient::disconnect(const char* id)
{
    if (id != NULL)
    {
        sendData("DISCONNECT\r\n", false);
        sendData("id:", false);
        sendData(id, false);
        sendData("\r\n\r\n", true);
        disconnectID = id;
    }
}

void StompClient::setMessageCallback(void (*f)(std::map<String, String> &headers, String &body))
{
    this->_onMessage = f;
}

void StompClient::publish(const char* topic, const char* contentType, String &payload)
{
    if (topic != NULL)
    {
        sendData("SEND\r\n", false);
        sendData("destination:", false);
        sendData(topic, false);
        sendData("\r\ncontent-type:", false);
        sendData(contentType, false);
        sendData("\r\n\r\n", false);
        sendData(payload.c_str(), true);
    }
}

void StompClient::sendHeartBeat()
{
    sendData("CONNECT\r\nheart-beat:360000,360000\r\n\r\n", true);
}
