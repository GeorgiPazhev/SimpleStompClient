#ifndef STOMP_CLIENT_H
#define STOMP_CLIENT_H
#include<string>
#include<map>
#include<vector>
#include <Arduino.h>
#include <Client.h>

enum StompFrameStates { CMD, HEADERS, BODY };
enum StompCommand {CMD_MESSAGE, CMD_RECEIPT, CMD_ERROR, CMD_CONNECTED, CMD_ACK, CMD_NACK};
enum StompAcknowledgeType {ACK_AUTO, ACK_CLIENT};

class StompClient
{

	StompFrameStates curFrameState;
	StompCommand curCommand;
	StompAcknowledgeType theAckType;
    Client* _client;
        
	char curChar;
	String collectedData;
	String destinationHost;
	std::map<String, String> headers;
	String disconnectID;
	bool connected;
	long lastHeartBeatSentTime;

	bool resolveCommand();
	char collectHeader();
	void reset();
	void processPacket();
	void trimCollectedData();
	void sendData(const char *data, bool endOfFrame);
	void connectToStomp(const char* protocolVersion = "1.2");
	void sendHeartBeat();
	void (*_onMessage)(std::map<String, String> &headers, String &body);

protected:
	bool getCurChar();	
	
public:
	StompClient(Client* client, StompAcknowledgeType acknowledgeType);
	~StompClient();
	void loop();
	bool isConnected();
	void publish(const char* topic, const char* contentType, String &payload);
	bool connect(IPAddress &ip, int port, const char* protocolVersion);
	bool connect(const char *hostName, int port, const char* protocolVersion);
	void subscribe(const char* topic, const char *id);
	void unsubscribe(const char* topic, const char* id);
	void disconnect(const char* id);
	void setMessageCallback(void (*f)(std::map<String, String> &headers, String &body));

};
#endif


