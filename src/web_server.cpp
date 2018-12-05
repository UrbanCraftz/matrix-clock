#include "web_server.h"
#include <WebServer.h>
#include <SPIFFS.h>

WiFiServer server(80);

void StartWebServer()
{
	server.begin();
	SPIFFS.begin();
}

const char* APPLICATION_JSON = "application/json";
const char* TEXT_PLAIN = "text/plain";
const char* INDEX_HTML = "/index.html";

void SendOKHeader(WiFiClient& client, String contentType, size_t contentSize = 0)
{
	client.println("HTTP/1.1 200 OK");
	client.println("Connection: close");

	if (contentType.length())
		client.println("Content-Type: " + contentType);

	if (contentSize)
		client.println("Content-Length: " + String(contentSize, 10));
	
	client.println();
}

bool HandleStaticFile(const String& path, WiFiClient& client)
{
	// Currently we read all files from SPIFFS. In final version
	// Files should be stored on SD Card

	if (!SPIFFS.exists(path))
		return false;

	File f = SPIFFS.open(path, "r");
	if (!f)
		return false;
   
	String contentType = TEXT_PLAIN;

	int extensionPos = path.lastIndexOf(".");
	if (extensionPos > 0) 
	{
		String extension = path.substring(extensionPos + 1);
		if (extension == "ico")
			contentType = "image/x-icon";
		else if (extension == "html")
			contentType = "text/html; charset=UTF-8";
		else if (extension == "js")
			contentType = "application/javascript; charset=utf-8";
		else if (extension == "js")
			contentType = "text/css";
	}
	SendOKHeader(client, contentType, f.size());
	client.write(f);
	return true;
}

bool HandleRESTRequest(const String& mode, const String& path, WiFiClient& client)
{
	if (mode == "GET")
	{
		if (path == "/scan")
		{
			SendOKHeader(client, APPLICATION_JSON);
			int numSsid = WiFi.scanNetworks();
			for (int thisNet = 0; thisNet < numSsid; thisNet++) {
				client.print(thisNet);
				client.print(") ");
				client.print(WiFi.SSID(thisNet));
				client.print("\tSignal: ");
				client.print(WiFi.RSSI(thisNet));
				client.print(" dBm");
				client.println("");
			}
			return true;
		}
	}
	else if (mode == "POST")
	{
		if (path == "/wificonfig")
		{
			SendOKHeader(client, TEXT_PLAIN);
			client.write("not yet implemented");
		}
	}

	return false;
}

bool HandleRequest(const String& header, WiFiClient& client)
{
	int modePos = header.indexOf(" ");
	if (modePos <= 0)
		return false;
	String mode = header.substring(0, modePos);
	int pathPos = header.indexOf(" ", modePos + 1);
	if (pathPos <= 0)
		return false;
	String path = header.substring(modePos + 1, pathPos);

	Serial.println("Request: " + mode + " " + path);

	if (mode == "GET")
	{
		if (path == "/")
			path = INDEX_HTML;
		if (HandleStaticFile(path, client))
			return true;
	}

	if (HandleRESTRequest(mode, path, client))
		return true;

	// per default return always index.html
	HandleStaticFile(INDEX_HTML, client);
	return true;
}

void ProcessWebServerRequests()
{
	WiFiClient client = server.available();
	if (client)
	{
		Serial.println("request start");
		String currentLine;
		String header;

		while (client.connected())
		{ 
			// loop while the client's connected
			if (client.available())
			{
				char c = client.read();
				Serial.write(c);
				header += c;
				if (c == '\n')
				{ 
					// if the byte is a newline character
					// if the current line is blank, you got two newline characters in a row.
					// that's the end of the client HTTP request, so send a response:
					if (currentLine.length() == 0)
					{
						// Answer always with 200 - no errors can happen :)
						if (!HandleRequest(header, client))
						{
							client.println("HTTP/1.1 500 ER");
							client.println("Connection: close");
							client.println();
						}
						break;
					}
					else // if you got a newline, then clear currentLine
						currentLine = "";
				}
				else if (c != '\r') // if you got anything else but a carriage return character, add it to the end of the currentLine
					currentLine += c;
			}
		}

		client.stop();
		Serial.println("request end");
	}
}
