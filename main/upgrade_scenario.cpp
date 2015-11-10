// Copyright (c) 2001-2013, SCALABLE Network Technologies, Inc.  All Rights Reserved.
//                          600 Corporate Pointe
//                          Suite 1200
//                          Culver City, CA 90230
//                          info@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the QualNet
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

//
// This file generate the utility tool to be used for upgrading
// scenarios created by older QualNet versions to the latest one.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "api.h"

int main(int argc, char **argv) 
{
    char srcFilePath[BIG_STRING_LENGTH] = {0};
    char destFilePath[BIG_STRING_LENGTH] = {0};
    static BOOL versionPrinted = FALSE;

    if(argc == 3)
    {
        strcpy(srcFilePath, argv[1]);
        strcpy(destFilePath, argv[2]);
    }
    else if(argc == 2)
    {
        strcpy(srcFilePath, argv[1]);
        strcpy(destFilePath, srcFilePath);
    }
    else if(argc == 1 || argc > 3)
    {
        printf("\n Error: Source config file name is missing!\n\n"
            " Syntax:\n\n"
            "     upgrade_scenario <source config file name> [<dest config file name>]\n\n"
            " Example: \n"
            "     upgrade_scenario.exe myscenario.config myscenario_new.config\n"
            "   Or\n"
            "     upgrade_scenario.exe myscenario.config \n\n"
            " When <dest config file name> is not specified, the upgraded config file will\n"
            " have the name as <source config file name>. Original scenario configuration\n"
            " file will be renamed as <source config file name>.old.\n\n");
        exit(0);
    }

    NodeInput nodeInput;
    //Allocates spaces, get ready for accepting data
    IO_InitializeNodeInput(&nodeInput, true); 
    IO_ReadNodeInputEx(&nodeInput, srcFilePath, TRUE);

    if(argc == 2){
        char changedSrcFileName[BIG_STRING_LENGTH] = {0};
        strcpy(changedSrcFileName, srcFilePath);
        strcat(changedSrcFileName, ".old");
        rename(srcFilePath, changedSrcFileName);
    }
    
    FILE *fd = fopen(destFilePath, "w");
    if(!fd){
        char buf[BIG_STRING_LENGTH] = {0};
        sprintf(buf, "Unable to write file: %s", destFilePath);
        ERROR_ReportError("Unable to write file: ");
        return -1;
    }

    for(int i=nodeInput.numFiles + 1; i < nodeInput.numLines; i++)
    {
        char outPutString[BIG_STRING_LENGTH] = {0};

        if(nodeInput.inputStrings[i][0] == '#')
        {
            strcat(outPutString, nodeInput.inputStrings[i]);
            strcat(outPutString, "\n");
        
            if(i>0 && i < nodeInput.numLines-1)
            {
                // Add Formating
                if(nodeInput.inputStrings[i-1][0] == '#'
                    && nodeInput.inputStrings[i+1][0] != '#')
                {
                    strcat(outPutString, "\n");
                }
                else if(nodeInput.inputStrings[i+1][0] != '#')
                {
                    char tempOutPutString[BIG_STRING_LENGTH] = {0};
                    strcat(tempOutPutString, "\n");
                    strcat(tempOutPutString, outPutString);
                    strcpy(outPutString, tempOutPutString);
                }
            }
        }
        else
        {
            if(!versionPrinted && nodeInput.variableNames[0]
                                        && nodeInput.values[0])
            {
                double version = atof(nodeInput.values[0]);
                if(version<5.0)
                {
                    fprintf(fd,
                            "\n\n%s %s\n\n",
                            nodeInput.variableNames[0],
                            "5.0");
                }
                else{
                    fprintf(fd, 
                            "\n\n%s %s\n\n",
                            nodeInput.variableNames[0],
                            nodeInput.values[0]);
                }
                versionPrinted = TRUE;
            }
        
            if(nodeInput.qualifiers[i])
            {
                strcat(outPutString, "[");
                strcat(outPutString, nodeInput.qualifiers[i]);
                strcat(outPutString, "] ");
            }
            
            if(nodeInput.variableNames[i])
            {
                if((strstr(nodeInput.variableNames[i],"SUBNET") 
                    && strlen(nodeInput.variableNames[i])
                                                == strlen("SUBNET"))
                || (strstr(nodeInput.variableNames[i],"LINK")
                    && strlen(nodeInput.variableNames[i]) == strlen("LINK")))
                {
                    strcat(outPutString, "\n");
                }
                strcat(outPutString, nodeInput.variableNames[i]);
            }
            
            if(nodeInput.instanceIds[i] != -1)
            {
                char idx[BIG_STRING_LENGTH] = {0};
                sprintf(idx, "%d", nodeInput.instanceIds[i]);
                strcat (outPutString, "[");
                strcat (outPutString, idx);
                strcat (outPutString, "] ");
            }
            
            if(nodeInput.values[i])
            {
                strcat(outPutString, " ");
                strcat(outPutString, nodeInput.values[i]);
                strcat(outPutString, "\n");
            }
        }    
        
        char versionDelim[] = " ";
        char* versionNextPtr = NULL;
        char firstItem[BIG_STRING_LENGTH] = {0};
        char lastItem [BIG_STRING_LENGTH] = {0};
        char* versionTokenPtr = NULL;
        char versionIoTokenStr[BIG_STRING_LENGTH] = {0};
        versionTokenPtr = IO_GetDelimitedToken(versionIoTokenStr, outPutString, versionDelim, &versionNextPtr);\

        if(versionTokenPtr && strcmp(versionTokenPtr, "VERSION") == 0)
        {
            continue;
        }
        
        fprintf(fd, "%s", outPutString);
    }

    // Copy All the file information at last in the configuration file.
    for(int i=1; i<nodeInput.numFiles + 1; i++)
    {
        char outPutString[BIG_STRING_LENGTH] = {0};
        if(nodeInput.inputStrings[i][0] == '#')
        {
            strcat(outPutString, nodeInput.inputStrings[i]);
            strcat(outPutString, "\n");
        }
        else
        {

            if(i>0)
            {
                if(nodeInput.inputStrings[i-1][0] == '#')
                    strcat(outPutString, "\n");
            }

            if(nodeInput.qualifiers[i])
            {
                strcat(outPutString, "[");
                strcat(outPutString, nodeInput.qualifiers[i]);
                strcat(outPutString, "] ");
            }

            if(nodeInput.variableNames[i])
            {
                strcat(outPutString, nodeInput.variableNames[i]);
            }

            if(nodeInput.instanceIds[i] != -1)
            {
                char idx[BIG_STRING_LENGTH] = {0};
                sprintf(idx, "%d", nodeInput.instanceIds[i]);
                strcat(outPutString, "[");
                strcat(outPutString, idx);
                strcat(outPutString, "] ");
            }

            if(nodeInput.values[i])
            {
                strcat(outPutString, " ");
                strcat(outPutString, nodeInput.values[i]);
                strcat(outPutString, "\n");
            }
        }    

        fprintf(fd, "%s", outPutString);
    }

    fclose(fd);
    if(argc == 2){
        printf("\nSuccess: %s Updated\n", destFilePath);
    }
    else  if(argc == 3){
        printf("\nSuccess: %s Created\n", destFilePath);
    }
    return 0;
}
