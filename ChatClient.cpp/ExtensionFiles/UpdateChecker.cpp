//UpdateChecker.cpp
//File handles checking GitHub releases for client updates.

#include "../ExtensionHeaderFiles/UpdateChecker.h"
#include <iostream>
#include <string>
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")
using namespace std;

//Remove version variable from here and moved to main for convience sake

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

//Finds the browser download URL for the chatClient.exe inside the GitHub Release
//This lets the updater download the new exe automatically.
string getChatClientDownloadUrl(const string& jsonText) {
	//Looks for the release asset named ChatClient.exe
	size_t assetNamePosition = jsonText.find("ChatClient.exe");
	//Checks if it was successful
	if (assetNamePosition == string::npos) {
		return "";
	}

	//After finding the new exe, loof for its browser_download_url.
	string searchKey = "\"browser_download_url\":";
	size_t urlKeyPosition = jsonText.find(searchKey, assetNamePosition);
	//Checks as a back up asset search
	if (urlKeyPosition == string::npos) {
		urlKeyPosition = jsonText.rfind(searchKey, assetNamePosition);
	}
	//Checks if it was successful
	if (urlKeyPosition == string::npos) {
		return "";
	}

	// Finds the colon after "browser_download_url".
	size_t colonPosition = jsonText.find(":", urlKeyPosition);

	if (colonPosition == string::npos)
	{
		return "";
	}

	//First quote
	size_t firstQuote = jsonText.find("\"", colonPosition);
	//Checks if it was successful
	if (firstQuote == string::npos) {
		return "";
	}

	//Second quote
	size_t secondQuote = jsonText.find("\"", firstQuote + 1);
	//Test if successful
	if (secondQuote == string::npos) {
		return "";
	}

	//Returns the url
	return jsonText.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

//Gets the folder of the chatclient.exe
string getCurrentExeFolder() {
	//creates character array large enough for file paths
	char exePath[MAX_PATH];
	//Gets the full path of the currently running exe and stores it
	//This is ran when the chatclient exe is running
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	//Converts the character array into a useable C++ string
	string fullPath = exePath;
	//Finds the position of the final slash in the path
	//SEperates the executable filename from folder path.
	size_t lastSlash = fullPath.find_last_of("\\/");
	//If no slash was found, a valid path could not be determined.
	if (lastSlash == string::npos) {
		return"";
	}
	//Returns everything before the final slash,
	//Removing the executable filename from the path.
	return fullPath.substr(0, lastSlash);
}

//Returns the full path of the running ChatClient.exe
string getCurrentExePath() {
	char exePath[MAX_PATH];
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	return string(exePath);
}

//Starts up Updater.exe and passes it to the download URL and current exe path.
bool launchUpdater(const string& downloadUrl) {
	//Gets the exe pathing
	string exeFolder = getCurrentExeFolder();
	string currentExePath = getCurrentExePath();
	string updaterPath = exeFolder + "\\Updater.exe";

	//Check if Updater.exe exists beside ChatClient.exe.
	DWORD updaterAttributes = GetFileAttributesA(updaterPath.c_str());
	if (updaterAttributes == INVALID_FILE_ATTRIBUTES) {
		cout << "Updater.exe was not found beside ChatClient.exe." << endl;
		cout << "Place Updater.exe in the same folder as ChatClient.exe." << endl;
		return false;
	}

	//Command Format:
	//Updater.exe "downloadUrl""targetExePath""restartExePath"
	string commandLine = "\"" + updaterPath + "\" " +
		"\"" + downloadUrl + "\" " + "\"" + currentExePath + "\" " +
		"\"" + currentExePath + "\"";

	//Startup info
	//Stores settings windows needs to start the updater
	STARTUPINFOA startupInfo;
	//stores information about the newly created updater process, including 
	//its process and thread handles
	PROCESS_INFORMATION processInfo;
	//Clears all fields in startupInfo so it doesn't contain leftover
	//or unused memory values
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	//Clears all fields in processInfo before CreateProcess uses it.
	ZeroMemory(&processInfo, sizeof(processInfo));

	//Tells windows the size of the STARTUPINFOA structure
	//Create process requires this field to be set up correctly.
	startupInfo.cb = sizeof(startupInfo);

	//CreateProcessA needs a writable char buffer.
	string commandBuffer = commandLine;
	BOOL started = CreateProcessA(
		NULL, &commandBuffer[0], NULL, NULL, FALSE, 0, NULL,
		exeFolder.c_str(), &startupInfo, &processInfo
	);
	//Checks if successful
	if (!started) {
		cout << "Failed to start Updater.exe. Error: " << GetLastError() << endl;
		return false;
	}

	//Clean up
	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	return true;
}

//Checks GitHub Releases to see if a new version of the client exists
//Function only checks and prints the result
//It does not download or replace files yet.
void checkForUpdates(const string& currentVersion) {
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
		cout << "Current version: " << currentVersion << endl;
		return;
	}
	cout << "Current version: " << currentVersion << endl;
	cout << "Latest version: " << latestVersion << endl;

	//Simple and basic version check
	//Works as long as it's a simple version,
	//example: v1.0.0, v1.0.1, v2.0.0, etc...
	if (latestVersion != currentVersion) {
		cout << "Update available." << endl;
		//Gets the chatclientdownloader
		string downloadUrl = getChatClientDownloadUrl(responseText);
		//Checks if downloadUrl was successful
		if (downloadUrl.empty()) {
			cout << "Could not find ChatClient.exe in the latest GitHub release assets." << endl;
			return;
		}

		//Prompts the user if they are ready to download the updated client
		cout << "Download and install update now? (y/n): ";
		//Gets the user's input
		string answer;
		getline(cin, answer);
		//If (y)es
		if (answer == "y" || answer == "Y") {
			cout << "Starting updater..." << endl;
			if (launchUpdater(downloadUrl)) {
				cout << "ChatClient will now close so the updater can replace it." << endl;
				ExitProcess(0);
			}
		}
		else {
			cout << "Update skipped." << endl;
		}
	}
	else {
		cout << "You are using the latest version." << endl;
	}
}