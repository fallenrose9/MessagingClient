//UpdateChecker.cpp
//File handles checking GitHub releases for client updates.

#include "../ExtensionHeaderFiles/UpdateChecker.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")
using namespace std;

//Current Version of this client
//Change/Update this value as I release a new version.
const string CURRENT_VERSION = "v1.0.0"; // Values is 1.2.3
//1. is main change, typically when major updates
//2. is smaller updates that don't warrant a big update, but still significant
//3. is small minor updates

//GitHub repo information used for updates
//keep the repo spelling exactly the same as it is on GitHub.
//TODO: Look into a more private service for future releases.
const wstring GITHUB_OWNER = L"fallenrose9";
const wstring GITHUB_REPO = L"MessagingClient";

//Finds a value inside the simple JSON response by searching for a key
//This is a basic helper as we don't need the full JSON library yet
//An example is key = "key_name" finds somthing similar to "v1.0.0"
string getJsonStringValue(const string& jsonText, const string& key) {
	//Builds the search pattern, ex. "key_name":
	string searchKey = "\"" + key + "\":";
	//Finds where the key appears in JSON text.
	size_t keyPosition = jsonText.find(searchKey);
	//If the key was not found, return an empty string
	if (keyPosition == string::npos) {
		return "";
	}

	//Finds the first quote after the key.
	size_t firstQuote = jsonText.find("\"", keyPosition + searchKey.length());
	//checks, returns an empty string if first quote not found
	if (firstQuote == string::npos) {
		return "";
	}

	//Finds the second quote
	size_t secondQuote = jsonText.find("\"", firstQuote + 1);
	//checks, returns an empty string if second quote not found
	if (secondQuote == string::npos) {
		return "";
	}
	//Returns the text between the two quotes.
	return jsonText.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

//Checks GitHub Releases to see if a new version of the client exists
//Function only checks and prints the result
//It does not download or replace files yet.
void checkForUpdates() {
	cout << "Checking for updates..." << endl;
	//GitHub API Server
	LPCWSTR serverName = L"api.github.com";
	//GitHub API path for latest release:
	// /repo/fallenrose9/MessagingClient/releases/latest
	wstring apiPath = L"/repos/" + GITHUB_OWNER + L"/" + GITHUB_REPO + L"/releases/latest";

	//Opens a WinHTTP session
	HINTERNET httpSession = WinHttpOpen(
	L"ChatClient Update Checker/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

	//Checks if the HTTP session failed to start
	if (!httpSession) {
		cout << "Update check failed: could not start HTTP session." << endl;
		return;
	}

	//Connects to the github api using HTTPS to pull updates
	HINTERNET httpConnection = WinHttpConnect(
		httpSession, serverName, INTERNET_DEFAULT_HTTPS_PORT, 0);
	//Checks if the connection failed
	if (!httpConnection) {
		cout << "Update check failed: could not connect to GitHub api." << endl;
		WinHttpCloseHandle(httpSession);
		return;
	}

	//Creates an HTTP GET request.
	HINTERNET httpRequest = WinHttpOpenRequest(
		httpConnection, L"GET", apiPath.c_str(), NULL, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE
	);
	//Checks if the request failed to be created
	if (!httpRequest) {
		cout << "Update check failed: could not create the request." << endl;
		WinHttpCloseHandle(httpConnection);
		WinHttpCloseHandle(httpSession);
		return;
	}

	//GitHub requires a User-Agent header for API requests
	wstring headers = L"User-Agent: ChatClient\r\n";
	//Sends the request to GitHub API
	BOOL requestSent = WinHttpSendRequest(
		httpRequest, headers.c_str(), static_cast<DWORD>(headers.length()),
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0
	);
	//Checks if request has failed
	if (!requestSent) {
		cout << "Update check failed: could not send request." << endl;
		WinHttpCloseHandle(httpRequest);
		WinHttpCloseHandle(httpConnection);
		WinHttpCloseHandle(httpSession);
		return;
	}

	//Waits for the response from GitHub.
	BOOL responseReceived = WinHttpReceiveResponse(httpRequest, NULL);
	//Checks for no response
	if (!responseReceived) {
		cout << "Update check failed: no response received." << endl;
		WinHttpCloseHandle(httpRequest);
		WinHttpCloseHandle(httpConnection);
		WinHttpCloseHandle(httpSession);
		return;
	}

	//Stores the full response from GitHub.
	string responseText;
	//Creates the bytesAVailable variable to check how much data is available 
	//from Github
	DWORD bytesAvailable = 0;
	//Reads the GitHub response 
	//Reads in chuncks to allow for better data handling
	do {
		//Resets the data bytes available to 0 before being updated
		//Happens only after every call to github
		bytesAvailable = 0;
		//Checks how much data is available to read
		if (!WinHttpQueryDataAvailable(httpRequest, &bytesAvailable)) {
			break;
		}
		//If no data is available, stops reading
		if (bytesAvailable == 0) {
			break;
		}
		//Creates a temp. buffer large enough for available data
		string buffer;
		buffer.resize(bytesAvailable);

		DWORD bytesRead = 0;

		//Reads the response into the buffer.
		if (!WinHttpReadData(httpRequest, &buffer[0], bytesAvailable, &bytesRead)) {
			break;
		}
		//adds the data that was read is then saved to the full response string
		responseText.append(buffer.c_str(), bytesRead);
	} while (bytesAvailable > 0);
	//Closes all the HTTP Handles
	WinHttpCloseHandle(httpRequest);
	WinHttpCloseHandle(httpConnection);
	WinHttpCloseHandle(httpSession);
	//Pulls the latest Release Tag from GitHub response.
	string latestVersion = getJsonStringValue(responseText, "tag_name");
	//If no tag_name was found, the repo hasn't been updated yet.
	if (latestVersion.empty()) {
		cout << "No GitHub release found yet." << endl;
		cout << "Current version: " << CURRENT_VERSION << endl;
		return;
	}
	cout << "Current version: " << CURRENT_VERSION << endl;
	cout << "Latest version: " << latestVersion << endl;

	//Simple and basic version check
	//Works as long as it's a simple version,
	//example: v1.0.0, v1.0.1, v2.0.0, etc...
	if (latestVersion != CURRENT_VERSION) {
		cout << "Update available." << endl;
		//Intend to offer to update from within the app and have it auto download
		//meaning updates this later to achieve
		cout << "Go to the GitHub release page to download the new version." << endl;
	}
	else {
		cout << "You are using the latest version." << endl;
	}
}