//
//  PlayerOneFW.cpp
//  Pegasus Indigo Filter Wheel
//
//  Created by Rodolphe Pineau on 30/5/2022.
//  Copyright © 2022 RTI-Zone. All rights reserved.
//

#include "PlayerOneFW.h"

CPlayerOneFW::CPlayerOneFW()
{
    m_bIsConnected = false;
    m_nCurentFilterSlot = -1;
    m_nTargetFilterSlot = 0;
    m_nFilterWheelCount = 0;
    m_sFWSerial.clear();
    m_sFirmwareVersion.clear();
    m_bBidirectional = true;

#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\PlayerOneFWLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/PlayerOneFWLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/PlayerOneFWLog.txt";
#endif
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CPlayerOneFW] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CPlayerOneFW] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif
    memset((void *)&m_tFilterWheelProperties, 0, sizeof(PWProperties));
    std::vector<PWProperties> tFilterWheelPropertiesList;
    listFilterWheel(tFilterWheelPropertiesList);
}

CPlayerOneFW::~CPlayerOneFW()
{
    if(m_bIsConnected)
        Disconnect();
}

int CPlayerOneFW::Connect(int nHandle)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;
    bool bComplete = false;
    int nbTimeout = 0;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connect Called." << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Trying to connect Filter Wheel with ID " << nHandle<< std::endl;
    m_sLogFile.flush();
#endif

    if(nHandle>=0 && m_sFWSerial.size()!=0) {
        pwError = POAGetPWPropertiesByHandle(nHandle, &m_tFilterWheelProperties);
        if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error getting properties for Filter wheel ID " << nHandle << " serial " << m_sFWSerial << " , Error = " << POAGetPWErrorString(pwError) << std::endl;
            m_sLogFile.flush();
#endif
            return ERR_NORESPONSE;
        }
    }
    else {
        // check if there is at least one camera connected to the system
        if(POAGetPWCount() >= 1) {
            std::vector<PWProperties> tFilterWheelPropertiesList;
            listFilterWheel(tFilterWheelPropertiesList);
            if(tFilterWheelPropertiesList.size()) {
                m_tFilterWheelProperties.Handle = tFilterWheelPropertiesList[0].Handle;
                m_tFilterWheelProperties.PositionCount = tFilterWheelPropertiesList[0].PositionCount;
                strncpy(m_tFilterWheelProperties.Name, tFilterWheelPropertiesList[0].Name, 64);
                strncpy((char *)m_tFilterWheelProperties.SN, tFilterWheelPropertiesList[0].SN, 32);
                m_sFWSerial.assign(tFilterWheelPropertiesList[0].SN);
            }
            else
                return ERR_NODEVICESELECTED;
        }
        else
            return ERR_NODEVICESELECTED;
    }

    // connect
    pwError = POAOpenPW(m_tFilterWheelProperties.Handle);
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error connecting to Filter wheel ID " << nHandle << " serial " << m_sFWSerial << " , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_NORESPONSE;
    }
    m_bIsConnected = true;


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connected." << std::endl;
    m_sLogFile.flush();
#endif

    // if any of this fails we're not properly connected or there is a hardware issue.
    nErr = getFirmwareVersion(m_sFirmwareVersion);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error Getting Firmware : " << nErr << std::endl;
        m_sLogFile.flush();
#endif

        m_bIsConnected = false;
        return FIRMWARE_NOT_SUPPORTED;
    }
    bComplete = false;
    while(!bComplete) {
        nErr = isMoveToComplete(bComplete);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::this_thread::yield();
        nbTimeout++;
        if(nbTimeout>10)  {  // 10 seconds
            POAClosePW(m_tFilterWheelProperties.Handle);
            return ERR_NORESPONSE;
        }
    }

    nErr = setWheelMoveDirection(m_bBidirectional);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error setting wheel direction : " << nErr << std::endl;
        m_sLogFile.flush();
#endif

        m_bIsConnected = false;
        return ERR_DEVICENOTSUPPORTED;
    }

    nErr = getCurrentSlot(m_nCurentFilterSlot);
    if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error Getting current slot : " << nErr << std::endl;
        m_sLogFile.flush();
#endif

        m_bIsConnected = false;
        return ERR_DEVICENOTSUPPORTED;
    }


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connected" << std::endl;
    m_sLogFile.flush();
#endif


    return nErr;
}



void CPlayerOneFW::Disconnect()
{

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Disconnect] Called" << std::endl;
    m_sLogFile.flush();
#endif

    if(m_bIsConnected) {
        POAClosePW(m_tFilterWheelProperties.Handle);
        m_nCurentFilterSlot = -1;
        m_nTargetFilterSlot = 0;
        m_nFilterWheelCount = 0;
        m_sFWSerial.clear();
        m_sFirmwareVersion.clear();
    }
    m_bIsConnected = false;
}


#pragma mark - Filter Wheel info commands

int CPlayerOneFW::getFirmwareVersion(std::string &sVersion)
{
    int nErr = 0;
    std::stringstream ssTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFirmwareVersion] Called" << std::endl;
    m_sLogFile.flush();
#endif

    ssTmp << " API V" << POAGetPWAPIVer() << ", SDK "<<POAGetPWSDKVer();

    sVersion.assign(ssTmp.str());
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFirmwareVersion] Firmware : " << sVersion << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}


#pragma mark - Filter Wheel move commands

int CPlayerOneFW::moveToFilterIndex(int nTargetPosition)
{
    int nErr = 0;
    PWErrors pwError;

    std::stringstream ssTmp;
    std::string sResp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [moveToFilterIndex] Moving to filter  : " << nTargetPosition << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [moveToFilterIndex] m_nCurentFilterSlot      : " << m_nCurentFilterSlot << std::endl;
    m_sLogFile.flush();
#endif

    pwError = POAGotoPosition(m_tFilterWheelProperties.Handle, nTargetPosition); // goto position
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [moveToFilterIndex] Error moving filter wheel , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }

    m_nTargetFilterSlot = nTargetPosition;

    return nErr;
}

int CPlayerOneFW::isMoveToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;
    PWState pw_state = PW_STATE_OPENED;

    int nFilterSlot = 0;

    bComplete = false;

    pwError = POAGetPWState(m_tFilterWheelProperties.Handle, &pw_state);
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isMoveToComplete] Error getting filter wheel status , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }

    if(pw_state == PW_STATE_MOVING) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isMoveToComplete] bComplete : " << (bComplete?"Yes":"No") << std::endl;
        m_sLogFile.flush();
#endif
        return nErr;
    }

    pwError = POAGetCurrentPosition(m_tFilterWheelProperties.Handle, &nFilterSlot);
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isMoveToComplete] Error getting filter wheel position , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }


    if(nFilterSlot == m_nTargetFilterSlot) {
        bComplete = true;
        m_nCurentFilterSlot = nFilterSlot;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [isMoveToComplete] bComplete : " << (bComplete?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}


#pragma mark - filters and device params functions
int CPlayerOneFW::getFilterCount(int &nCount)
{
    nCount = m_tFilterWheelProperties.PositionCount;
    return PLUGIN_OK;
}


int CPlayerOneFW::getCurrentSlot(int &nSlot)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;

    pwError = POAGetCurrentPosition(m_tFilterWheelProperties.Handle, &nSlot);
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCurrentSlot] Error getting filter wheel position , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }
    return nErr;
}


int CPlayerOneFW::listFilterWheel(std::vector<PWProperties> &tFilterWheelPropertiesList)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;
    PWProperties   tFilterWheelProperties;

    tFilterWheelPropertiesList.clear();
    if(!m_bIsConnected) {
        // list filter wheel connected to the system
        m_nFilterWheelCount = POAGetPWCount();

        for (int i = 0; i < m_nFilterWheelCount; i++)
        {
            pwError = POAGetPWProperties(i, &m_tFilterWheelProperties);
            if (pwError == PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listFilterWheel] Name          : " <<  m_tFilterWheelProperties.Name << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listFilterWheel] Handle        : " << m_tFilterWheelProperties.Handle<< std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listFilterWheel] SN            : " << m_tFilterWheelProperties.SN << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listFilterWheel] PositionCount : " << m_tFilterWheelProperties.PositionCount << std::endl;
                m_sLogFile.flush();
#endif
                tFilterWheelProperties.Handle = m_tFilterWheelProperties.Handle;
                tFilterWheelProperties.PositionCount = m_tFilterWheelProperties.PositionCount;
                strncpy(tFilterWheelProperties.Name, m_tFilterWheelProperties.Name, 64);
                strncpy((char *)tFilterWheelProperties.SN, m_tFilterWheelProperties.SN, 32);

                tFilterWheelPropertiesList.push_back(tFilterWheelProperties);
            }
        }
    }
    else {
        tFilterWheelProperties.Handle = m_tFilterWheelProperties.Handle;
        tFilterWheelProperties.PositionCount = m_tFilterWheelProperties.PositionCount;
        strncpy(tFilterWheelProperties.Name, m_tFilterWheelProperties.Name, 64);
        strncpy((char *)tFilterWheelProperties.SN, m_tFilterWheelProperties.SN, 32);

        tFilterWheelPropertiesList.push_back(tFilterWheelProperties);
    }

    return nErr;
}

int CPlayerOneFW::getFilterWheelHandleFromSerial(int &nHandle, std::string sSerial)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;
    PWProperties   tFilterWheelProperties;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelHandleFromSerial] sSerial = " << sSerial << std::endl;
    m_sLogFile.flush();

#endif
    nHandle = -1;

    int nFWCount = POAGetPWCount();
    for (int i = 0; i < nFWCount; i++)
    {
        pwError = POAGetPWProperties(i, &tFilterWheelProperties);
        if (pwError == PW_OK)
        {
            if(sSerial==tFilterWheelProperties.SN) {
                nHandle = tFilterWheelProperties.Handle;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelHandleFromSerial] found handle = " << nHandle << std::endl;
                m_sLogFile.flush();
#endif
                break;
            }
        }
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    if(nHandle>=0) {
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelHandleFromSerial] handle  = " << nHandle <<" SN: "<< sSerial << std::endl;
    }
    else {
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraIdFromSerial] camera id  not found for SN: "<< sSerial << std::endl;
    }
    m_sLogFile.flush();
#endif
    if(nHandle<0)
        nErr = ERR_NODEVICESELECTED;
    return nErr;
}

int CPlayerOneFW::getFilterWheelSerialFromHandle(int nHandle, std::string &sSerial)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;
    PWProperties   tFilterWheelProperties;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelSerialFromHandle] Handle = " << nHandle << std::endl;
    m_sLogFile.flush();
#endif

    if(nHandle<0)
        return ERR_NODEVICESELECTED;

    sSerial.clear();
    int nFWCount = POAGetPWCount();
    for (int i = 0; i < nFWCount; i++)
    {
        pwError = POAGetPWProperties(i, &tFilterWheelProperties);
        if (pwError == PW_OK)
        {
            if(nHandle==tFilterWheelProperties.Handle) {
                sSerial = tFilterWheelProperties.SN;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelSerialFromHandle] sSerial = " << sSerial << std::endl;
                m_sLogFile.flush();
#endif
                break;
            }
        }
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    if(sSerial.size()) {
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelSerialFromHandle] Handle  = " << nHandle <<" SN: "<< sSerial << std::endl;
    }
    else {
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFilterWheelSerialFromHandle] filter wheel handle not found for id: "<< nHandle << std::endl;
    }
    m_sLogFile.flush();
#endif
    return nErr;
}

void CPlayerOneFW::setFilterWheelHandle(int nFilterWheelHandle)
{
    m_tFilterWheelProperties.Handle = nFilterWheelHandle;
}

void CPlayerOneFW::setFilterWheelSerial(std::string sFilterWheelSerial)
{
    strncpy(m_tFilterWheelProperties.SN,sFilterWheelSerial.c_str(), 32);
}


int CPlayerOneFW::getWheelMoveDirection(bool &bBidirectional)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;
    int nIsOneWay;

    pwError = POAGetOneWay(m_tFilterWheelProperties.Handle, &nIsOneWay);
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWheelMoveDirection] Error getting  wheel direction , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }
    if(!nIsOneWay)
        bBidirectional = true;
    else
        bBidirectional = false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWheelMoveDirection] bBidirectional : " << (bBidirectional?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CPlayerOneFW::setWheelMoveDirection(bool bBidirectional)
{
    int nErr = PLUGIN_OK;
    PWErrors pwError;

    m_bBidirectional = bBidirectional;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWheelMoveDirection] setting  wheel direction , Bidrectional = " << (m_bBidirectional?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    if(!m_bIsConnected)
        return ERR_NOLINK;

    pwError = POASetOneWay(m_tFilterWheelProperties.Handle, bBidirectional?0:1);
    if(pwError != PW_OK) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWheelMoveDirection] Error setting  wheel direction , Error = " << POAGetPWErrorString(pwError) << std::endl;
        m_sLogFile.flush();
#endif
        nErr = ERR_CMDFAILED;
    }

    return nErr;
}


#ifdef PLUGIN_DEBUG

void CPlayerOneFW::log(const std::string sLogLine)
{
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [log] " << sLogLine << std::endl;
    m_sLogFile.flush();

}


const std::string CPlayerOneFW::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#endif
