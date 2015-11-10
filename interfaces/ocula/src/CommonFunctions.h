#ifndef __CommonFunctions_H
#define __CommonFunctions_H

#include <string>
#include <set>
#include <vector>
#include <cassert>

#include "RpcInterface.h"
using namespace std;

#define USERDIR_VISUALIZER ".scenario_player"
#define LOGFILE_VISUALIZER "scenario_player.log"
#define LOGFILE_SCENARIO_SERVER "scenario_server.log"
#define PRODUCT_TITLE "Scalable Scenario Player"
#define SETTINGSFILE_VISUALIZER "scenario_player.settings"

// Socket connection timeout of 5 seconds
#define SOCKET_CONNECT_TIMEOUT_SECONDS 5

struct ConnectionInfo
{
    int portno;    
    std::string hostname;
    std::string ipAddress;
    std::string scenarioName;

    ConnectionInfo();
    bool operator< (const ConnectionInfo& connInfo) const; 
};

/**
 * FUNCTION  getUserFilePath
 * PURPOSE   Retrieves the file path to the user file under the user's home directory.
 * PARAMETERS
 *   returnPath - The string variable where the requested path is set
 *   filename - The file name of the user file
*/
extern void getUserFilePath(std::string &returnPath, const std::string &filename);

/**
 * FUNCTION  extractDirPath
 * PURPOSE   Returns the directory part of the given file path 
 * PARAMETERS
 *   filePath - The file path
*/
extern std::string extractDirPath(const std::string &filePath);


/**
 * FUNCTION  extractFileName
 * PURPOSE   Returns the file part of the given file path 
 * PARAMETERS
 *   filePath - The file path
*/
extern std::string extractFileName(const std::string &filePath);


/**
 * FUNCTION  normalizedFilePath
 * PURPOSE   Returns a file path string normalized with the Unix convention directory 
 *           separator.  Unix style directory paths are compatible with Windows.
 * PARAMETERS
 *   filePath - The file path
*/
extern std::string normalizedFilePath(const std::string &filePath);


/**
 * FUNCTION  logMessageToFile
 * PURPOSE   Logs a message to the log file.
 * PARAMETERS
 *   prefix - A prefix to the log indicating the severity of the message.
 *   msg - The message to log.
*/
extern void logMessageToFile(const std::string &prefix, const std::string &msg);

/**
 * FUNCTION  getRunningScenarios
 * PURPOSE   Retrieves the list of scenarios that are running in the network.
 * PARAMETERS
 *   scenariosVector - The vector that gets populated with the list.
 */
void getRunningScenarios(std::vector<ConnectionInfo>& scenariosVector);

/**
 * FUNCTION  requestScenarioInfo
 * PURPOSE   Retrieves the scenario running on hostname::port if it running
             or returns an empty string if not.
 * PARAMETERS
 *   portNo - The port number to connect to.
 *   hostname - The hostname to connect to.
 */
std::string requestScenarioInfo(const int portNo, const char* hostname);

/**
 * FUNCTION  truncateString
 * PURPOSE   Truncates the string if it is more than length characters 
 *           and adds .. in between.
 * PARAMETERS
 *   str - The string to be truncated
 *   pos_from_start - show this many characters from start
 *   pos_before_end - show this many characters at the end
 *   truncate only if the string is more than length characters
*/
void truncateString(std::string& str, int pos_from_start, int pos_before_end, int length);

/**
 * FUNCTION  translateErrorCode
 * PURPOSE   Translates the error code to a readable error message.
 * PARAMETERS
 *   errorCode - The error code to be translated
*/
std::string translateErrorCode(int errorCode);

#endif
