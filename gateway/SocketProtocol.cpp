#include "SocketProtocol.h"
#include "Log.h"
#include "Util.h"
#include "Wifi.h"
#include "Base64.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <thread>

#define BUFLEN 1024

string msgReq = "{\"dsID\":\"HCRemoteMonitor\",\"reqType\":\"query\",\"queryAddr\":[\"Addr-811\"]}";

SocketProtocol::SocketProtocol(int port, string ip) : port(port), ip(ip)
{
	this->isRunning = false;
	this->port = port;
	this->ip = ip;
}

SocketProtocol::~SocketProtocol()
{
}

static void SocketHandleMessage(void *data)
{
	LOGI("Start SocketHandleMessage");
	SocketProtocol *socketProtocol = (SocketProtocol *)data;
	struct sockaddr_in server_address;

	char buffer[BUFLEN];
	ssize_t bytes_received;

	// create a socket
	if ((socketProtocol->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		LOGE("Socket die");
		exit(1);
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(socketProtocol->port);
	if (inet_pton(AF_INET, socketProtocol->ip.c_str(), &server_address.sin_addr) <= 0)
	{
		LOGE("Invalid address/ Address not supported");
		exit(1);
	}

	if (connect(socketProtocol->fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
	{
		LOGE("Connection failed");
		exit(1);
	}

	bool hasSending = false;
	while (socketProtocol->isRunning)
	{
		LOGI("Socket Waiting for data...");
		socketProtocol->mtxSendMessage.lock();
		if (socketProtocol->listMessages.size() > 0)
		{
			socketProtocol->sendMessage(socketProtocol->listMessages.at(0));
			hasSending = true;
		}
		socketProtocol->mtxSendMessage.unlock();
		fflush(stdout);

		// try to receive some data, this is a blocking call
		if (hasSending)
		{
			hasSending = false;
			if ((bytes_received = recv(socketProtocol->fd, buffer, BUFLEN, 0)) == -1)
			{
				LOGE("Socket die: recvfrom");
				exit(1);
			}

			buffer[bytes_received] = '\0';
			socketProtocol->SocketOnMessage(string(buffer));
		}
		usleep(500000);
		// sleep(1);
	}
	close(socketProtocol->fd);
}

void SocketProtocol::init()
{
	if (!isRunning)
	{
		isRunning = true;
		thread socketThread(SocketHandleMessage, this);
		socketThread.detach();
	}
}

void SocketProtocol::stop()
{
	isRunning = false;
}

int SocketProtocol::SocketCmdCallbackRegister(string cmd, OnRpcCallbackFunc onRpcCallbackFunc)
{
	LOGI("SocketCmdCallbackRegister cmd: %s", cmd.c_str());
	onRpcCallbackFuncList[cmd] = onRpcCallbackFunc;
	return CODE_OK;
}

void SocketProtocol::SocketOnMessage(string message)
{
	LOGI("SocketOnMessage message: %s", message.c_str());
	Json::Value respValue;
	Json::Value payloadJson;
	if (payloadJson.parse(message) && payloadJson.isObject())
	{
		string cmd = "";
		if (payloadJson.isMember("dsID") && payloadJson["dsID"].isString())
		{
			cmd = payloadJson["dsID"].asString();
		}
		if (cmd == "HCRemoteMonitor")
		{
			mtxSendMessage.lock();
			listMessages.clear();
			mtxSendMessage.unlock();
			
			if (onRpcCallbackFuncList.find(cmd) != onRpcCallbackFuncList.end())
			{
				OnRpcCallbackFunc onRpcCallbackFunc = onRpcCallbackFuncList[cmd];
				int rs = onRpcCallbackFunc(payloadJson, respValue);
				if (rs == CODE_OK)
				{
					LOGD("Call %s OK, rs: %d", cmd.c_str(), rs);
					if (respValue.toString() != "{}")
					{
						setMessageSend(respValue.toString());
					}
				}
				else if (rs == CODE_DATA_ARRAY)
				{
					LOGD("Call %s OK, rs: %d", cmd.c_str(), rs);
					if (respValue.isArray())
					{
						for (auto &respV : respValue)
						{
							// sendMessage(respV.toString());
							usleep(10000);
						}
					}
				}
				else if (rs == CODE_NOT_RESPONSE)
				{
					LOGD("Call %s OK, rs: %d", cmd.c_str(), rs);
				}
				else if (rs == CODE_EXIT)
				{
					LOGE("Call %s OK, rs: %d", cmd.c_str(), rs);
					// sendMessage(respValue.toString());
					exit(1);
				}
				else
				{
					LOGW("Call %s ERR rs: %d", cmd.c_str(), rs);
				}
			}
			else
			{
				LOGW("Method %s not registed", cmd.c_str());
			}
		}
		else
		{
			LOGW("UdpOnMessage message: %s", message.c_str());
		}
	}
}

int SocketProtocol::sendMessage(string message)
{
	LOGI("send message: %s", message.c_str());
	if (message != "")
		if (send(this->fd, message.c_str(), message.length(), 0) == -1)
		{
			LOGW("Socket send error");
			return CODE_ERROR;
		}
	// LOGE("Send success");
	return CODE_OK;
}

void SocketProtocol::setMessageSend(Json::Value data)
{
	mtxSendMessage.lock();
	listMessages.push_back(data.toString());
	mtxSendMessage.unlock();
}

void SocketProtocol::setMessageSend(string data)
{
	mtxSendMessage.lock();
	listMessages.push_back(data);
	mtxSendMessage.unlock();
}
