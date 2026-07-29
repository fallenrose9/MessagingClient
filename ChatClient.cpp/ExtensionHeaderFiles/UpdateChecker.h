//UpdateChecker.h is the file for the update function
//File declares the update check function for ChatClient.cpp
#pragma once
#include <string>

//Will check GitHub Releases to see if a newer client version exists
//Pulls currentVersion from the main ChatClient file,
//This change was made for development conviencence.
void checkForUpdates(const std::string& currentVersion);