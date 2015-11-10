#include "CommonFunctions.h"

#include <string.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <time.h>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#define err_no (WSAGetLastError())

#pragma comment (lib, "Ws2_32.lib")
#else
#include <linux/limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define err_no errno
#define SOCKET int
#endif
static SOCKET sock;
#define LEN 2048

ConnectionInfo::ConnectionInfo()
    : portno(0)
    , hostname("localhost")
    , ipAddress("127.0.0.1")
    , scenarioName("default")
    {}

bool ConnectionInfo::operator< (const ConnectionInfo& connInfo) const 
{
    bool ret = false;
    if (this->hostname == connInfo.hostname)
    {
        ret = this->scenarioName < connInfo.scenarioName;
    }
    else
    {       
        ret = this->hostname < connInfo.hostname;
    }
    return ret;
}

void getUserFilePath(std::string &returnPath, const std::string &filename) 
{
    std::stringstream buf;
#ifdef _WIN32
    std::string homedrive(getenv("HOMEDRIVE"));
    std::string homepath(getenv("HOMEPATH"));
    buf << homedrive << homepath << "\\" << USERDIR_VISUALIZER << "\\";
    CreateDirectory(buf.str().c_str(), NULL);
    buf << filename;
    returnPath = buf.str();
#else
    std::string home(getenv("HOME"));
    buf << home << "/" << USERDIR_VISUALIZER << "/";
    mkdir(buf.str().c_str(), 0777);
    buf << filename;
    returnPath = buf.str();
#endif
}


std::string extractDirPath(const std::string &filePath) 
{ 
    std::string retval;
    size_t found = filePath.rfind("/");
    if (found != std::string::npos)
    {
        retval = filePath.substr(0, found);
    }
    return retval;
}


std::string extractFileName(const std::string &filePath) 
{ 
    std::string retval;
    size_t found = filePath.rfind("/");
    if (found != std::string::npos)
    {
        retval = filePath.substr(found + 1);
    }
    return retval;
}


std::string normalizedFilePath(const std::string &filePath) 
{ 
    std::string retval(filePath);

#ifdef _WIN32
    TCHAR fullPath[MAX_PATH];
    DWORD len = GetFullPathName(
        filePath.c_str(),
        MAX_PATH,
        fullPath,
        NULL);
    if (len > 0)
    {
        retval = fullPath;
    }
#else
    char fullPath[PATH_MAX];
    char* ret;
    ret = realpath(filePath.c_str(), fullPath);
    if (ret != NULL)
    {
        retval = fullPath;
    }
#endif

    std::replace(retval.begin(), retval.end(), '\\', '/');

    return retval;
}


void logMessageToFile(const std::string &prefix, const std::string &msg)
{
    static bool wasCalled = false;
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%Y-%m-%d %X] ", &tstruct);

    std::string logPath;
    getUserFilePath(logPath, LOGFILE_SCENARIO_SERVER);
    std::ofstream messageLog;
    if (!wasCalled) // blank out log file
    {
        messageLog.open(logPath.c_str());
    }
    else // append to log file
    {
        messageLog.open(logPath.c_str(), std::ios_base::app);
    }
    messageLog << buf << prefix << msg << std::endl;
    messageLog.flush();
    messageLog.close();
    wasCalled = true;
}

void getRunningScenarios(std::vector<ConnectionInfo>& scenariosVector)
{
    std::multiset<ConnectionInfo> scenarios;
    ifstream infile;
    int portNo = 0;
    std::string hostname;
    std::string ipAddress;
    std::string connectionName;

    infile.open("SimSettings.config");
    if (!infile.fail())
    {   
        while (infile >> connectionName >> hostname >> portNo >> ipAddress)
        {
            ConnectionInfo connInfo;

            connInfo.hostname = hostname;
            connInfo.ipAddress = ipAddress;
            connInfo.portno = portNo;
            connInfo.scenarioName = requestScenarioInfo(portNo, ipAddress.c_str());
            if (!connInfo.scenarioName.empty())
            {
                scenarios.insert(connInfo);
            }
        }

        for (std::multiset<ConnectionInfo>::const_iterator it = scenarios.begin(); it != scenarios.end(); it++)
        {
            scenariosVector.push_back(*it);
        }
    }
}


std::string requestScenarioInfo(const int portNo, const char* hostname)
{
    std::string ret;
    int         nBytes;
    char        scenarioInfo[LEN];
    std::string rpcCmd;
    char*       pCmdStart;
    char*       pCmdEnd;
#if defined(_WIN32) || defined(_WIN64)
    int         iResult;
    WSADATA     wsaData;
#else
    sockaddr_in serv_addr;
#endif

    char scenarioInfoCmd[] = {RequestScenarioInfo,'(',ETX, '\0'};

#if defined(_WIN32) || defined(_WIN64)
    // holds address info for socket to connect to
    addrinfo *result = NULL;
    addrinfo *ptr = NULL;
    addrinfo hints;

    // Initialize Winsock2
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);

    // set address info
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //resolve server address and port on localhost 
    char aport[32];
    sprintf(aport, "%d", portNo); // assign the port we want to connect to here
    iResult = getaddrinfo(hostname, aport, &hints, &result);

    if( iResult != 0 ) 
    {
        std::stringstream buf;
        buf << "getaddrinfo failed with error: " << iResult;
        logMessageToFile("[warning] ", buf.str());
        WSACleanup();
        return ret;
    }

    // Attempt to connect to an address until one succeeds
    sock = INVALID_SOCKET;
    int numSockets = 0;
    int index = 0;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        numSockets++;
    }
    SOCKET * socketAry = new SOCKET[numSockets];
    for (index = 0; index < numSockets; index++)
    {
        socketAry[index] = INVALID_SOCKET;
    }

    bool socketConnected = false;

    for (ptr = result, index = 0; ptr != NULL; ptr = ptr->ai_next, index++)
    {
        if (socketAry[index] == INVALID_SOCKET)
        {
            socketAry[index] = 
                socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        }
        iResult = connect(
            socketAry[index], ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            std::stringstream buf;
            buf << "Could not connect via socket to " << hostname 
                << " on port " << portNo 
                << ". " << translateErrorCode(WSAGetLastError());
            logMessageToFile("[warning] ", buf.str());
        }
        else
        {
            sock = socketAry[index];
            socketConnected = true;
            break;
        }
    }    
    delete [] socketAry;
    if (!socketConnected)
    {
        return ret;
    }
    if (sock == INVALID_SOCKET)
    {
        std::stringstream buf;
        buf << "Invalid socket : " << sock; 
        logMessageToFile("[warning] ", buf.str());
        return ret;
    }
#else // LINUX
    // Linux version of the above
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::stringstream buf;
        buf << "Invalid socket : " << sock; 
        logMessageToFile("[warning] ", buf.str());
        return ret;
    }

    // Create server address
    memset(&serv_addr, 0, sizeof(sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(hostname);
    serv_addr.sin_port = htons(portNo);
    
    int err;
    bool socketConnected = false;
    while (!socketConnected)
    {
        // Connect to the server
       err = connect(sock, (sockaddr*)&serv_addr, sizeof(sockaddr_in));
        if (err < 0)
        {
            int socket_error_number = errno;
            std::stringstream buf;
            buf << "Could not connect via socket to " << hostname 
                << " on port " << portNo 
                << ". " << translateErrorCode(socket_error_number);
            logMessageToFile("[warning] ", buf.str());
        }
        else
        {
            socketConnected = true;
        }
    }
    if (!socketConnected)
    {
        return ret;
    }
#endif // LINUX

    if (send(sock, scenarioInfoCmd, strlen(scenarioInfoCmd), 0) < 0)
    {
        std::stringstream buf;
        buf << "Socket send error. " 
            << translateErrorCode(err_no);
        logMessageToFile("[warning] ", buf.str());
        return ret;
    }

	nBytes = recv(sock, scenarioInfo, LEN - 1, 0);

#if defined(_WIN32) || defined(_WIN64)
    shutdown(sock, SD_BOTH);
#else
    shutdown(sock, SHUT_RDWR);
#endif

    if (nBytes < 0)
    {
#if defined(_WIN32) || defined(_WIN64)
	    if (err_no != WSAECONNRESET)
	    {
		    std::stringstream buf;
		    buf << "Socket read error. " 
			    << translateErrorCode(WSAGetLastError());
		    logMessageToFile("[warning] ", buf.str());
	    }
#else
	    if (err_no != ECONNRESET)
	    {
		    std::stringstream buf;
		    buf << "Socket read error. " 
			    << translateErrorCode(err_no);
		    logMessageToFile("[warning] ", buf.str());
	    }
#endif
        return ret;
    }
    scenarioInfo[nBytes] = '\0';
	pCmdStart = scenarioInfo;

	pCmdEnd = strchr(pCmdStart, ETX);
	if (pCmdEnd == NULL)
	{          
		std::stringstream buf;
		buf << "Cannot find ETX message terminator from simulator at  " 
            << hostname << " : " << portNo;
		logMessageToFile("[warning] ", buf.str());
		return ret;
	}
	*pCmdEnd = '\0'; // Change ETX to terminator.
	{
		ret = pCmdStart;
        return ret;
	}
}

void truncateString(std::string& str, int pos_from_start, int pos_before_end, int length)
{
    int len = str.length();
    if (len > length)
    {
        str.insert(pos_from_start, "..");
        str.erase(str.begin() + pos_from_start + 2, str.end() - pos_before_end);
    }
}

std::string translateErrorCode(int errorCode) 
{
#if defined(_WIN32) || defined(_WIN64)
    switch (errorCode) {
        case WSA_INVALID_HANDLE:
            return "Specified event object handle is invalid.";
            break;
        case WSA_NOT_ENOUGH_MEMORY:
            return "Insufficient memory available.";
            break;
        case WSA_INVALID_PARAMETER:
            return "One or more parameters are invalid.";
            break;
        case WSA_OPERATION_ABORTED:
            return "Overlapped operation aborted.";
            break;
        case WSA_IO_INCOMPLETE:
            return "Overlapped I/O event object not in signaled state.";
            break;
        case WSA_IO_PENDING:
            return "Overlapped operations will complete later.";
            break;
        case WSAEINTR:
            return "Interrupted function call.";
            break;
        case WSAEBADF:
            return "File handle is not valid.";
            break;
        case WSAEACCES:
            return "Permission denied.";
            break;
        case WSAEFAULT:
            return "Bad address.";
            break;
        case WSAEINVAL:
            return "Invalid argument.";
            break;
        case WSAEMFILE:
            return "Too many open files.";
            break;
        case WSAEWOULDBLOCK:
            return "Resource temporarily unavailable.";
            break;
        case WSAEINPROGRESS:
            return "Operation now in progress.";
            break;
        case WSAEALREADY:
            return "Operation already in progress.";
            break;
        case WSAENOTSOCK:
            return "Socket operation on nonsocket.";
            break;
        case WSAEDESTADDRREQ:
            return "Destination address required.";
            break;
        case WSAEMSGSIZE:
            return "Message too long.";
            break;
        case WSAEPROTOTYPE:
            return "Protocol wrong type for socket.";
            break;
        case WSAENOPROTOOPT:
            return "Bad protocol option.";
            break;
        case WSAEPROTONOSUPPORT:
            return "Protocol not supported.";
            break;
        case WSAESOCKTNOSUPPORT:
            return "Socket type not supported.";
            break;
        case WSAEOPNOTSUPP:
            return "Operation not supported.";
            break;
        case WSAEPFNOSUPPORT:
            return "Protocol family not supported.";
            break;
        case WSAEAFNOSUPPORT:
            return "Address family not supported by protocol family.";
            break;
        case WSAEADDRINUSE:
            return "Address already in use.";
            break;
        case WSAEADDRNOTAVAIL:
            return "Cannot assign requested address.";
            break;
        case WSAENETDOWN:
            return "Network is down.";
            break;
        case WSAENETUNREACH:
            return "Network is unreachable.";
            break;
        case WSAENETRESET:
            return "Network dropped connection on reset.";
            break;
        case WSAECONNABORTED:
            return "Software caused connection abort.";
            break;
        case WSAECONNRESET:
            return "Connection reset by peer.";
            break;
        case WSAENOBUFS:
            return "No buffer space available.";
            break;
        case WSAEISCONN:
            return "Socket is already connected.";
            break;
        case WSAENOTCONN:
            return "Socket is not connected.";
            break;
        case WSAESHUTDOWN:
            return "Cannot send after socket shutdown.";
            break;
        case WSAETOOMANYREFS:
            return "Too many references.";
            break;
        case WSAETIMEDOUT:
            return "Connection timed out.";
            break;
        case WSAECONNREFUSED:
            return "Connection refused.";
            break;
        case WSAELOOP:
            return "Cannot translate name.";
            break;
        case WSAENAMETOOLONG:
            return "Name too long.";
            break;
        case WSAEHOSTDOWN:
            return "Host is down.";
            break;
        case WSAEHOSTUNREACH:
            return "No route to host.";
            break;
        case WSAENOTEMPTY:
            return "Directory not empty.";
            break;
        case WSAEPROCLIM:
            return "Too many processes.";
            break;
        case WSAEUSERS:
            return "User quota exceeded.";
            break;
        case WSAEDQUOT:
            return "Disk quota exceeded.";
            break;
        case WSAESTALE:
            return "Stale file handle reference.";
            break;
        case WSAEREMOTE:
            return "Item is remote.";
            break;
        case WSASYSNOTREADY:
            return "Network subsystem is unavailable.";
            break;
        case WSAVERNOTSUPPORTED:
            return "Winsock.dll version out of range.";
            break;
        case WSANOTINITIALISED:
            return "Successful WSAStartup not yet performed.";
            break;
        case WSAEDISCON:
            return "Graceful shutdown in progress.";
            break;
        case WSAENOMORE:
            return "No more results.";
            break;
        case WSAECANCELLED:
            return "Call has been canceled.";
            break;
        case WSAEINVALIDPROCTABLE:
            return "Procedure call table is invalid.";
            break;
        case WSAEINVALIDPROVIDER:
            return "Service provider is invalid.";
            break;
        case WSAEPROVIDERFAILEDINIT:
            return "Service provider failed to initialize.";
            break;
        case WSASYSCALLFAILURE:
            return "System call failure.";
            break;
        case WSASERVICE_NOT_FOUND:
            return "Service not found.";
            break;
        case WSATYPE_NOT_FOUND:
            return "Class type not found.";
            break;
        case WSA_E_NO_MORE:
            return "No more results.";
            break;
        case WSA_E_CANCELLED:
            return "Call was canceled.";
            break;
        case WSAEREFUSED:
            return "Database query was refused.";
            break;
        case WSAHOST_NOT_FOUND:
            return "Host not found.";
            break;
        case WSATRY_AGAIN:
            return "Nonauthoritative host not found.";
            break;
        case WSANO_RECOVERY:
            return "This is a nonrecoverable error.";
            break;
        case WSANO_DATA:
            return "Valid name, no data record of requested type.";
            break;
        case WSA_QOS_RECEIVERS:
            return "At least one QoS reserve has arrived.";
            break;
        case WSA_QOS_SENDERS:
            return "At least one QoS send path has arrived.";
            break;
        case WSA_QOS_NO_SENDERS:
            return "There are no QoS senders.";
            break;
        case WSA_QOS_NO_RECEIVERS:
            return "There are no QoS receivers.";
            break;
        case WSA_QOS_REQUEST_CONFIRMED:
            return "The QoS reserve request has been confirmed.";
            break;
        case WSA_QOS_ADMISSION_FAILURE:
            return "A QoS error occurred due to lack of resources.";
            break;
        case WSA_QOS_POLICY_FAILURE:
            return "QoS policy failure.";
            break;
        case WSA_QOS_BAD_STYLE:
            return "An unknown or conflicting QoS style was encountered.";
            break;
        case WSA_QOS_BAD_OBJECT:
            return "QoS bad object.";
            break;
        case WSA_QOS_TRAFFIC_CTRL_ERROR:
            return "QoS traffic control error.";
            break;
        case WSA_QOS_GENERIC_ERROR:
            return "QoS generic error.";
            break;
        case WSA_QOS_ESERVICETYPE:
            return "QoS service type error.";
            break;
        case WSA_QOS_EFLOWSPEC:
            return "QoS flowspec error.";
            break;
        case WSA_QOS_EPROVSPECBUF:
            return "Invalid QoS provider buffer.";
            break;
        case WSA_QOS_EFILTERSTYLE:
            return "Invalid QoS filter style.";
            break;
        case WSA_QOS_EFILTERTYPE:
            return "Invalid QoS filter type.";
            break;
        case WSA_QOS_EFILTERCOUNT:
            return "Incorrect QoS filter count.";
            break;
        case WSA_QOS_EOBJLENGTH:
            return "Invalid QoS object length.";
            break;
        case WSA_QOS_EFLOWCOUNT:
            return "Incorrect QoS flow count.";
            break;
        case WSA_QOS_EUNKOWNPSOBJ:
            return "Unrecognized QoS object.";
            break;
        case WSA_QOS_EPOLICYOBJ:
            return "Invalid QoS policy object.";
            break;
        case WSA_QOS_EFLOWDESC:
            return "Invalid QoS flow descriptor.";
            break;
        case WSA_QOS_EPSFLOWSPEC:
            return "Invalid QoS provider-specific flowspec.";
            break;
        case WSA_QOS_EPSFILTERSPEC:
            return "Invalid QoS provider-specific filterspec.";
            break;
        case WSA_QOS_ESDMODEOBJ:
            return "Invalid QoS shape discard mode object.";
            break;
        case WSA_QOS_ESHAPERATEOBJ:
            return "Invalid QoS shaping rate object.";
            break;
        case WSA_QOS_RESERVED_PETYPE:
            return "Reserved policy QoS element type.";
            break;
        default:
            return "";
    }
    return "";
#else
    return strerror(errorCode);
#endif
}