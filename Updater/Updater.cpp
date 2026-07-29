// Updater.cpp :
//This program downlaods a newer ChatClient.exe, replaces old one,
//and restarts ChatClient.
//This is needed due to that an active exe can not replace itself while running
//in this case, ChatCLient.exe can not replace itself, so it closes itself and passes the
//download to this exe so this exe will update the chatclient.exe

#include <iostream>
#include <string>
#include <windows.h>
#include <urlmon.h>

//Links the Urlmon library. This is needed for usecase of URLDownloadToFileA().
#pragma comment(lib, "Urlmon.lib")

using namespace std;

int main(int argc, char* argv[])
{
    //argc will tell us how many command-line arguments were passed in.
    //argv stores the actual command-line argument values.
    //
    //Expected arguments will be
    //argv[0] = Updater.exe itself
    //argv[1] = download URL for the new ChatClient.exe
    //argv[2] = full path to the current ChatClient.exe that needs replacing
    //argv[3] = full path to restart after the update is complete.

    //if fewer than 4 arugments exists, the updater was not started correctly.
    if (argc < 4) {
        cout << "Updater missing required arugments." << endl;
        cout << "Usage: Updater.exe <downloadUrl><targetExePath><restartExePath>" << endl;
        //Keeps the window open so the user can actually read the error.
        system("pause");
        return 1;
    }
    //Stores the GitHub download URL for the new ChatClient.exe
    string downloadUrl = argv[1];
    //Stores the current ChatClient.exe path that will be replaced.
    string targetExePath = argv[2];
    //Stores the ChatClient.exe path that should be restarted after updating
    string restartExePath = argv[3];

    //Creates a temp download path
    //Example: ChatClient.exe.new
    //This allows it to avoid downloading over the old exe
    string tempDownloadPath = targetExePath + ".new";
    
    //Prompts the user about the update starting
    cout << "Updater started." << endl;
    cout << "Waiting for ChatClient.exe to close..." << endl;

    //Give the ChatClient.exe time to fully close before replacing it.
    Sleep(3000);
    //Tells user the download and update has started
    cout << "Downloading update..." << endl;
    //Downloads the new ChatClient.exe from GitHub
    //NULL means there isn't a special caller object
    //downloadUrl.c_str() is the web address of the new exe.
    //tempDownloadPath.c_str() is where the file will be saved locally.
    HRESULT downloadResult = URLDownloadToFileA(
        NULL, downloadUrl.c_str(), tempDownloadPath.c_str(), 0, NULL
    );
    //Failed check
    if (FAILED(downloadResult)) {
        cout << "Dlowload failed. Error code: " << downloadResult << endl;
        //Keeps the window open for user to see and read the error
        system("pause");
        return 1;
    }
    cout << "Download complete." << endl;

    //Deletes the old ChatClient.exe
    //The old one should be closed by now
    if (!DeleteFileA(targetExePath.c_str())) {
        cout << "Could not delete old ChatClient.exe. Error: " << GetLastError() << endl;
        //Keeps the window open for user to see error
        system("pause");
        return 1;
    }

    //Moves and renames the temp downloaded file into the original ChatClient.exe
    //Changes ChatClient.exe.new into simply ChatClient.exe
    //Checks if replace worked
    if (!MoveFileA(tempDownloadPath.c_str(), targetExePath.c_str())) {
        cout << "Could not replace ChatClient.exe. Error: " << GetLastError() << endl;
        //Keeps the window open for user to see error
        system("pause");
        return 1;
    }

    //If successful
    cout << "Update installed." << endl;
    cout << "Restarting ChatClient..." << endl;

    //STARTUPINFOA stores startup setting for the process we are about to launch.
    STARTUPINFOA startupInfo;
    //PROCESS_INFORMATION stores information about the new process after it starts.
    PROCESS_INFORMATION processInfo;
    //Clears both structs before using them.
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    
    //Required by CreateProcessA.
    startupInfo.cb = sizeof(startupInfo);

    //Builds the command used to restart ChatClient.exe
    //Quotes are added in case the path has spaces.
    string commandLine = "\"" + restartExePath + "\"";
    //Starts ChatClient.exe again.
    BOOL restarted = CreateProcessA(
        NULL, //Application name, NULL means use of commandLine
        &commandLine[0], //command line to run
        NULL, //Default process security.
        NULL, //Default thread security.
        FALSE, //Do not inherit handles
        0, //No special creation flags.
        NULL, //Use parent's environment.
        NULL, //Use parent's current directory
        &startupInfo, //Startup settings
        &processInfo //Recieves process information
    );
    //Checks if failed
    if (!restarted) {
        cout << "Could not restart ChatClient.exe. Error: " << GetLastError() << endl;
        //Once again keeps window open
        system("pause");
        return 1;
    }

    //Closes all the handles that CreateProcessA had opened.
    //Does not close ChatClient.exe, only cleans up updater's handles
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    cout << "Updater finished." << endl;
    return 0;
}