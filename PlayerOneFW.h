//
//  PlayerOneFW.h
//  Pegasus Indigo Filter Wheel
//
//  Created by Rodolphe Pineau on 30/5/2022.
//  Copyright © 2022 RTI-Zone. All rights reserved.
//

#ifndef PlayerOneFW_h
#define PlayerOneFW_h
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <time.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

// C++ includes
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <thread>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <algorithm>


#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"

#include "PlayerOnePW.h"

//#define PLUGIN_DEBUG 2
#define PLUGIN_VERSION      1.02


#define SERIAL_BUFFER_SIZE 1024
#define MAX_TIMEOUT 1000
#define ERR_PARSE   1

#define MAX_READ_WAIT_TIMEOUT 25
#define NB_RX_WAIT 10

enum PlayerOneFWFilterWheelErrors {PLUGIN_OK=0, PLUGIN_NOT_CONNECTED, PLUGIN_CANT_CONNECT, PLUGIN_BAD_CMD_RESPONSE, PLUGIN_COMMAND_FAILED, PLUGIN_COMMAND_TIMEOUT};

class CPlayerOneFW
{
public:
    CPlayerOneFW();
    ~CPlayerOneFW();

    int             Connect(int nHandle);
    void            Disconnect(void);
    bool            IsConnected(void) { return m_bIsConnected; };

    int             listFilterWheel(std::vector<PWProperties> &tFilterWheelPropertiesList);
    int             getFilterWheelHandleFromSerial(int &nHandle, std::string sSerial);
    int             getFilterWheelSerialFromHandle(int nHandle, std::string &sSerial);
    
    // Filter Wheel commands
    int             getFirmwareVersion(std::string &sVersion);
    int             getStatus();
    
    int             moveToFilterIndex(int nTargetPosition);
    int             isMoveToComplete(bool &bComplete);

    int             getFilterCount(int &nCount);
    int             getCurrentSlot(int &nSlot);

    void            setFilterWheelHandle(int nFilterWheelHandle);
    void            setFilterWheelSerial(std::string sFilterWheelSerial);

    int             getWheelMoveDirection(bool &bBidirectional);
    int             setWheelMoveDirection(bool bBidirectional);

#ifdef PLUGIN_DEBUG
    void  log(const std::string sLogLine);
#endif

protected:

    bool            m_bIsConnected;
    bool            m_bBidirectional;
    int             m_nFilterWheelCount;
    PWProperties    m_tFilterWheelProperties;

    std::string     m_sFirmwareVersion;
    std::string     m_sFWSerial;

    int             m_nCurentFilterSlot;
    int             m_nTargetFilterSlot;

    
#ifdef PLUGIN_DEBUG
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
#endif

};
#endif /*PlayerOneFW_h */
