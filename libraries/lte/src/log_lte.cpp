/* (c) Copyright 2006-2007, by Kozo keikaku Enginnering Inc.
 * All Rights Reserved.
 *
 * This source code is licensed, not sold, and is subject to a written
 * license agreement.  Among other things, no portion of this source
 * code may be copied, transmitted, disclosed, displayed, distributed,
 * translated, used as the basis for a derivative work, or used, in
 * whole or in part, for any program or purpose other than its intended
 * use in compliance with the license agreement as part of the WiMAX
 * Protocol Library on QualNet software.  This source code and certain
 * of the algorithms contained within it are confidential trade secrets
 * of Kozo keikaku engneering,Inc. and may not be used as the basis for
 * any other software, hardware, product or service.
 */
/**
 * @file log_lte.cpp
 * @brief Declaration of loggin class for lte
 */
//#include "lte_plib.h"
#include "log_lte.h"
//#include "lte_common.h"
#include "layer2_lte.h"
#include "phy_lte.h"
#include <stdarg.h>
using std::string;

#define CLOSE_ON_FATAL // do nothing

#ifdef _WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

const char* LTE_STRING_LAYERS[NUM_LTE_STRING_LAYER_TYPE] =
{
    LTE_STRING_LAYER_TYPE_PROP,
    LTE_STRING_LAYER_TYPE_PHY,
    LTE_STRING_LAYER_TYPE_SCHEDULER,
    LTE_STRING_LAYER_TYPE_MAC,
    LTE_STRING_LAYER_TYPE_RLC,
    LTE_STRING_LAYER_TYPE_PDCP,
    LTE_STRING_LAYER_TYPE_RRC,
    LTE_STRING_LAYER_TYPE_EPC,
};

namespace lte
{

static LteLog theLteLog;

// Log level strings.
static const char* const LevelStr[] = {
    "FATAL", "WARN", "INFO", "DEBUG", "DETAIL"
};

// station type strings.
static const char* const StationTypeStr[] = {
    LTE_STRING_STATION_TYPE_ENB,
    LTE_STRING_STATION_TYPE_UE ,
    "-"
};

/**
 * Constructor.
 */
LteLog::LteLog()
        : fp(NULL)
{
    const char* value = NULL;
//    LogLevelType defaultLogLevel = LOG_LEVEL_DEBUG2;
    LogLevelType defaultLogLevel = INFO;
    fileNo = 0;
    // Get log level
    value = ::getenv("LTE_LOG_LEVEL");
    outLevel = (value) ? (LogLevelType)::atoi(value) : defaultLogLevel;
    if (value && !::isdigit(value[0])) {
        for (int i_lev = (int)FATAL; i_lev <= (int)LOG_LEVEL_DEBUG2; i_lev++)
        {
            if (strcmp(value, LevelStr[i_lev]) == 0) {
                outLevel = (LogLevelType) i_lev;
            }
        }
    }
    // Get Lines in a File.
    value = ::getenv("LTE_LOG_SIZE");
    linePerFile = (value) ? ::atoi(value) : MAX_LINES_DEFAULT;
    // Is stdout?
    value = ::getenv("LTE_LOG_STDOUT");
    isStdout = (value && !strcmp(value, "YES")) ? true : false;
    // Is Flush by record?
    value = ::getenv("LTE_LOG_FLUSH");
    isFlush = (value && !strcmp(value, "YES")) ? true : false;

    // Get output file name form Env
    value = ::getenv("LTE_LOG_FILE");
    if (!value) {
        value = "lte.log";
    }
    strncpy(fname, value, sizeof(fname));
    fname[sizeof(fname) - 1] = 0;
#ifdef LTE_LIB_LOG
    OpenNewFile();
#endif

#if ENABLE_FILTERING

    // Layer filter
    value = ::getenv("LTE_LOG_LAYER_FILTER");
    if (!value){
        layerFilterEnabled = false;
    }else{
        layerFilterEnabled = true;
        strncpy(layerMask, value, sizeof(layerMask));
    }

    // Time filter
    value = ::getenv("LTE_LOG_TIME_FILTER");
    if (!value){
        timeFilterEnabled = false;
    }else{
        timeFilterEnabled = true;

        double startTime_sec;
        double endTime_sec;

        sscanf(value, "%lf,%lf",&startTime_sec,&endTime_sec);

        outputStartTime = (double)(startTime_sec * SECOND);
        outputEndTime = (double)(endTime_sec * SECOND);
    }

    // Node filter
    value = ::getenv("LTE_LOG_NODE_FILTER");
    if (!value){
        nodeFilterEnabled = false;
    }else{
        nodeFilterEnabled = true;

        std::stringstream ss(value);

        char delim;
        int val;
        // Separeted by comma (ex. "2,6,7")
        while (!ss.eof()){
            ss >> val;
            outputNodes.insert(val);
            if (!ss.eof()){
                ss >> delim;
            }else{
                break;
            }
        }

        std::set < int > ::const_iterator it;
        for (it = outputNodes.begin(); it != outputNodes.end(); ++it)
        {
            printf("%d:\n",*it);
        }
    }

#endif // ENABLE_FILETERING

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
    // Node filter
    value = ::getenv("LTE_LOG_VALIDATION_STAT_OFFSET"); // [sec]
    if (!value){
        validationStatOffset = 5.0; // [sec]
    }else{
        validationStatOffset = ::atof(value);
    }


    // Node filter
    value = ::getenv("LTE_LOG_VALIDATION_PATTERN"); // [sec]

    if (!value)
    {
        validationPatternId = -1;
    }else
    {
        int i = 0;
        for (; i < NUM_VALIDATION_PATTENRS; ++ i)
        {
            if (strcmp(value,
                    validationPatternProfile[i].validationPatternIdStr) == 0)
            {
                validationPatternId = i;
                break;
            }
        }

        if (i == NUM_VALIDATION_PATTENRS)
        {
            validationPatternId = -1;
        }

    }


#endif
#endif
}

/**
 * Destructor
 */
LteLog::~LteLog()
{
    if (fp) {
        fflush(fp);
        fclose(fp);
    }
}

/**
 * Write fatal error.
 */
void LteLog::Fatal(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     log                 ///< Log message body.
)
{
    theLteLog.Write(node, index, FATAL, layerName, file, line, log);
}

/**
 * Write fatal error.
 */
void LteLog::FatalFormat(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    va_list args;
    va_start(args, logfmt);
    theLteLog.Write(
                    node, index, FATAL, layerName, file, line, logfmt, args);
    va_end(args);
}

/**
 * Is Enable Warning log
 *
 * @retval true     Enable output.
 * @retval false    Disable output.
 */
bool LteLog::IsEnableWarn()
{
    return theLteLog.outLevel >= WARN;
}

/**
 * Write warning.
 */
void LteLog::Warn(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     log                 ///< Log message body.
)
{
    if (theLteLog.outLevel >= WARN) {
        theLteLog.Write(node, index, WARN, layerName, log);
    }
}

/**
 * Write warning.
 */
void LteLog::WarnFormat(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    if (theLteLog.outLevel >= WARN) {
        va_list args;
        va_start(args, logfmt);
        theLteLog.Write(node, index, WARN, layerName, logfmt, args);
        va_end(args);
    }
}

/**
 * Is Enable Information log
 *
 * @retval true     Enable output.
 * @retval false    Disable output.
 */
bool LteLog::IsEnableInfo()
{
    return theLteLog.outLevel >= INFO;
}

/**
 * Write information.
 */
void LteLog::Info(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     log                 ///< Log message body.
)
{
    if (theLteLog.outLevel >= INFO) {
        theLteLog.Write(node, index, INFO, layerName, log);
    }
}

/**
 * Write info.
 */
void LteLog::InfoFormat(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    if (theLteLog.outLevel >= INFO) {
        va_list args;
        va_start(args, logfmt);
        theLteLog.Write(node, index, INFO, layerName, logfmt, args);
        va_end(args);
    }
}

/**
 * Is Enable Debug log
 *
 * @retval true     Enable output.
 * @retval false    Disable output.
 */
bool LteLog::IsEnableDebug()
{
    return theLteLog.outLevel >= LOG_LEVEL_DEBUG;
}

/**
 * Write debug print.
 */
void LteLog::Debug(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     log                 ///< Log message body.
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG) {
        theLteLog.Write(node, index, LOG_LEVEL_DEBUG, layerName, log);
    }
}

/**
 * Write debug print.
 */
void LteLog::DebugFormat(
    Node*           node,               ///< Node sruct.
    int             index,              ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG)
    {
        va_list args;
        va_start(args, logfmt);
        theLteLog.Write(
                    node, index, LOG_LEVEL_DEBUG, layerName, logfmt, args);
        va_end(args);
    }
}

/**
 * Write debug print.
 */
void LteLog::DebugWithFileLine(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     log                 ///< Log message body.
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG)
    {
        theLteLog.Write(
                node, index, LOG_LEVEL_DEBUG, layerName, file, line, log);
    }
}

/**
 * Write debug print.
 */
void LteLog::DebugWithFileLineFormat(
    Node*           node,               ///< Node sruct.
    int             index,              ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG) {
        va_list args;
        va_start(args, logfmt);
        theLteLog.Write(
            node,
            index,
            LOG_LEVEL_DEBUG,
            layerName,
            file,
            line,
            logfmt,
            args);
        va_end(args);
    }
}

/**
 * Is Enable more detail Debug log
 *
 * @retval true     Enable output.
 * @retval false    Disable output.
 */
bool LteLog::IsEnableDebug2()
{
    return theLteLog.outLevel >= LOG_LEVEL_DEBUG2;
}

/**
 * Write more detail debug print.
 */
void LteLog::Debug2(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     log                 ///< Log message body.
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG2) {
        theLteLog.Write(node, index, LOG_LEVEL_DEBUG2, layerName, log);
    }
}

/**
 * Write more detail debug print.
 */
void LteLog::Debug2Format(
    Node*           node,               ///< Node sruct.
    int             index,              ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG2) {
        va_list args;
        va_start(args, logfmt);
        theLteLog.Write(
                    node, index, LOG_LEVEL_DEBUG2, layerName, logfmt, args);
        va_end(args);
    }
}

/**
 * Write more detail debug print.
 */
void LteLog::Debug2WithFileLine(
    Node*           node,               ///< Node sruct.
    int             index,     ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     log                 ///< Log message body.
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG2) {
        theLteLog.Write(
                node, index, LOG_LEVEL_DEBUG2, layerName, file, line, log);
    }
}

/**
 * Write more detail debug print.
 */
void LteLog::Debug2WithFileLineFormat(
    Node*           node,               ///< Node sruct.
    int             index,              ///< Iterface index.
    const char*     layerName,         ///< Layer name.
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     logfmt,             ///< Log message body.
    ...
)
{
    if (theLteLog.outLevel >= LOG_LEVEL_DEBUG2) {
        va_list args;
        va_start(args, logfmt);
        theLteLog.Write(
            node,
            index,
            LOG_LEVEL_DEBUG2,
            layerName,
            file,
            line,
            logfmt,
            args);
        va_end(args);
    }
}

/**
 * Write log record.
 */
void LteLog::Write(
    Node*           node,             ///< Node sruct.
    int             index,            ///< Iterface(or other instance) index.
    LogLevelType    level,            ///< Log level.
    const char*     layerName,        ///< Layer name.
    const char*     log               ///< Log message body.
)
{
#if ENABLE_FILTERING
    if (timeFilterEnabled && (!FilterTime(node->getNodeTime()))){
        return;
    }

    if (nodeFilterEnabled && (!FilterNode(node->nodeId))){
        return;
    }

    if (layerFilterEnabled && (!FilterLayer(layerName))){
        return;
    }
#endif

    char bufRecord[MAX_LOG_LENGTH + 1];
    char* buf = bufRecord;
    int bufsz = MAX_LOG_LENGTH;

    buf = FormatRecordHeader(buf, bufsz, node, index, level, layerName);
    if (bufsz) {
        buf = FormatRecordBody(buf, bufsz, log);
    }
    bufRecord[MAX_LOG_LENGTH] = 0;
    Write(bufRecord);
    if (level == FATAL) {
        CLOSE_ON_FATAL;//if (fp) fclose(fp);
        ERROR_ReportError(bufRecord);
    }
}

/**
 * Write log record.
 */
void LteLog::Write(
    Node*           node,             ///< Node sruct.
    int             index,            ///< Iterface(or other instance) index.
    LogLevelType    level,            ///< Log level.
    const char*     layerName,        ///< Layer name.
    const char*     file,             ///< File path.
    int             line,             ///< Line no.
    const char*     log               ///< Log message body.
)
{
#if ENABLE_FILTERING
    if (timeFilterEnabled && (!FilterTime(node->getNodeTime()))){
        return;
    }

    if (nodeFilterEnabled && (!FilterNode(node->nodeId))){
        return;
    }

    if (layerFilterEnabled && (!FilterLayer(layerName))){
        return;
    }
#endif

    char bufRecord[MAX_LOG_LENGTH + 1];
    char* buf = bufRecord;
    int bufsz = MAX_LOG_LENGTH;

    buf = FormatRecordHeader(buf, bufsz, node, index, level, layerName);
    if (bufsz) {
        buf = FormatRecordBody(buf, bufsz, log);
        if (bufsz) {
            buf = FormatFileLine(buf, bufsz, file, line);
        }
    }
    bufRecord[MAX_LOG_LENGTH] = 0;
    Write(bufRecord);
    if (level == FATAL) {
        CLOSE_ON_FATAL;//if (fp) fclose(fp);
        ERROR_ReportError(bufRecord);
    }
}

/**
 * Write log record using printf formating.
 */
void LteLog::Write(
    Node*           node,
    int             index,
    LogLevelType    level,
    const char*     layerName,
    const char*     logfmt,
    va_list&        args
)
{
#if ENABLE_FILTERING
    if (node == NULL) {
        return;
    }

    if (timeFilterEnabled && (!FilterTime(node->getNodeTime()))){
        return;
    }

    if (nodeFilterEnabled && (!FilterNode(node->nodeId))){
        return;
    }

    if (layerFilterEnabled && (!FilterLayer(layerName))){
        return;
    }
#endif

    char bufRecord[MAX_LOG_LENGTH + 1];
    char* buf = bufRecord;
    int bufsz = MAX_LOG_LENGTH;

    buf = FormatRecordHeader(buf, bufsz, node, index, level, layerName);
    if (bufsz) {
        buf = FormatRecordBody(buf, bufsz, logfmt, args);
    }
    bufRecord[MAX_LOG_LENGTH] = 0;
    Write(bufRecord);
    if (level == FATAL) {
        CLOSE_ON_FATAL;//if (fp) fclose(fp);
        ERROR_ReportError(bufRecord);
    }
}

/**
 * Write log record using printf formating.
 */
void LteLog::Write(
    Node*           node,
    int             index,
    LogLevelType    level,
    const char*     layerName,
    const char*     file,               ///< File path.
    int             line,               ///< Line no.
    const char*     logfmt,
    va_list&        args
)
{
#if ENABLE_FILTERING
    if (timeFilterEnabled && (!FilterTime(node->getNodeTime()))){
        return;
    }

    if (nodeFilterEnabled && (!FilterNode(node->nodeId))){
        return;
    }

    if (layerFilterEnabled && (!FilterLayer(layerName))){
        return;
    }
#endif

    char bufRecord[MAX_LOG_LENGTH + 1];
    char* buf = bufRecord;
    int bufsz = MAX_LOG_LENGTH;

    buf = FormatRecordHeader(buf, bufsz, node, index, level, layerName);
    if (bufsz) {
        buf = FormatRecordBody(buf, bufsz, logfmt, args);
        if (bufsz) {
            buf = FormatFileLine(buf, bufsz, file, line);
        }
    }
    bufRecord[MAX_LOG_LENGTH] = 0;
    Write(bufRecord);
    if (level == FATAL) {
        CLOSE_ON_FATAL;//if (fp) fclose(fp);
        ERROR_ReportError(bufRecord);
    }
}

/**
 * Write record body.
 */
char* LteLog::FormatRecordBody(
    char*        buf,
    int&         bufsz,
    const char*  body
)
{
    int n = snprintf(buf, bufsz - 1, "%s", body);
    if (n < 0) {
        bufsz = 0;
        return NULL;
    }
    bufsz -= n;
    return buf + n;
}

/**
 * Write record body.
 */
char* LteLog::FormatRecordBody(
    char*               buf,
    int&                bufsz,
    const char*         bodyfmt,
    va_list&            args
)
{
    int n = vsnprintf(buf, bufsz - 1, bodyfmt, args);
    if (n < 0) {
        bufsz = 0;
        return NULL;
    }
    bufsz -= n;
    return buf + n;
}

/**
 * Write record header to buffer.
 * <br>
 * header format :<br>
 * time, NodeId, index, LogLevel, LayerName,
 */
char* LteLog::FormatRecordHeader(
    char*               buf,
    int&                bufsz,
    Node*       node,
    int                 index,
    LogLevelType        level,
    const char*         layerName
)
{
    int nodeId = -1;
    int stationType = 2; // StationTypeStr[2] = -
    Layer2DataLte* lte;
    string timeStr("-");
    if (node != NULL) {
        nodeId = node->nodeId;

        if (0 <= index){
            if (node->macData[index]->macVar != NULL){
                lte = (Layer2DataLte*)(node->macData[index]->macVar);
                stationType = lte->stationType;
            }
        }
        timeStr = ConvertTimeTypeToString(node->getNodeTime());
    }

    //int n = ::_snprintf(buf, bufsz - 1, "%s,%d,%d,%s,%s,",
    //                     timeStr.c_str(), nodeId, index,
    //                     LevelStr[(int)level], layerName);
    int n = snprintf( buf, bufsz - 1, "%s,%s,%s,%d,%d,%s,",
                      timeStr.c_str(), 
                      LevelStr[( int )level], 
                      StationTypeStr[( int )stationType],
                      nodeId,
                      index,
                      layerName );
    if (n < 0) {
        bufsz = 0;
        return NULL;
    }
    bufsz -= n;
    return buf + n;
}

/**
 * Write FILE and LINE to buffer.
 * <br>
 * format :<br>
 * file,line
 */
char* LteLog::FormatFileLine(
    char*               buf,
    int&                bufsz,
    const char*         file,
    int                 line
)
{
    int n = snprintf(buf, bufsz - 1, ",%s,%d", file, line);
    if (n < 0) {
        bufsz = 0;
        return NULL;
    }
    bufsz -= n;
    return buf + n;
}

/**
 * Write log record.
 */
void LteLog::Write(
    const char*     log                 ///< Log record.
)
{
    ThreadMutexLocker LockIt(mutex);    // Only one thread can run this
                                        // routine at a time.

    if (fp) {
        if (lineNo > linePerFile) {
            OpenNewFile();
        }
        fputs(log, fp);
        fputs("\n", fp);
        if (isFlush) {
            fflush(fp);
        }
        lineNo++;
    }
    if (isStdout) {
        puts(log);
    }
}

/**
 * Open new file
 */
void LteLog::OpenNewFile()
{
    char buf[MAX_FILE_NAME_LENGTH + 1];
    if (fp) {
        fclose(fp);
    }
    snprintf(buf, MAX_FILE_NAME_LENGTH, "%s.%03d.csv", fname, fileNo);
    buf[MAX_FILE_NAME_LENGTH] = 0;
    fp = fopen(buf, "w");
    if (!fp) {
        printf("FATAL ERROR : fail to open file \"%s\".", buf);
    }
    else {
        fputs("SimTime,LogLevel,NodeType,NodeId,"
              "index,Layer,Category,OppositeRNTI,LogBody\n",
              fp);
    }
    lineNo = 1;
    fileNo++;
}

#if ENABLE_FILTERING

/**
 * Filter log output by layerName
 */
bool LteLog::FilterLayer(const char* layerName)
{
    for (int l = 0; l < NUM_LTE_STRING_LAYER_TYPE; ++l)
    {
        if (strcmp(LTE_STRING_LAYERS[l], layerName) == 0){
            return (layerMask[l] == '1');
        }
    }

    //return true;
    return false;
}

/**
 * Filter log output by time
 */
bool LteLog::FilterTime(clocktype t)
{
    return (t >= outputStartTime && t <= outputEndTime);
}

/**
 * Filter log output by node ID
 */
bool LteLog::FilterNode(int nodeId)
{
    std::set < int > ::const_iterator it;
    it = std::find(outputNodes.begin(), outputNodes.end(), nodeId);
    return (it != outputNodes.end());
}

#ifdef LTE_LIB_LOG
#ifdef LTE_LIB_VALIDATION_LOG
double LteLog::getValidationStatOffset()
{
    return theLteLog.validationStatOffset;
}

const BaseConnectionInfo* LteLog::getConnectionInfo(int* len)
{
    if (theLteLog.validationPatternId >= 0)
    {
        *len = validationPatternProfile
                            [theLteLog.validationPatternId].numConnInfo;
        return validationPatternProfile
                            [theLteLog.validationPatternId].connectionInfo;
    }else
    {
        *len = 0;
        return NULL;
    }
}

bool LteLog::getConnectionInfoConnectingWithMe(
        int myNodeId,
        bool isEnb,
        std::vector < ConnectionInfo > &connectionInfos)
{

    int len;

    const BaseConnectionInfo* bci
        = getConnectionInfo(&len);

    if (bci == NULL)
    {
        return false;
    }

    for (int i = 0; i < len; ++i)
    {
        bool thisisthat =
                isEnb ? (bci[i].nodeIdEnb == myNodeId) :
                        (bci[i].nodeIdUe  == myNodeId) ;
        if (thisisthat)
        {
            ConnectionInfo ci;
            ci.nodeId =
                    isEnb ? bci[i].nodeIdUe : bci[i].nodeIdEnb;
            ci.isEnb = !isEnb;
            ci.baseConnectionInfo = &bci[i];

            connectionInfos.push_back(ci);
        }
    }

    return true;
}

bool LteLog::validationPatternEnabled()
{
    return (theLteLog.validationPatternId >= 0);
}

#endif
#endif
#endif

} // namespace lte
