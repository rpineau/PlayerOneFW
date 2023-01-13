#include "x2filterwheel.h"


X2FilterWheel::X2FilterWheel(const char* pszDriverSelection,
				const int& nInstanceIndex,
				SerXInterface					* pSerX, 
				TheSkyXFacadeForDriversInterface	* pTheSkyX, 
				SleeperInterface					* pSleeper,
				BasicIniUtilInterface			* pIniUtil,
				LoggerInterface					* pLogger,
				MutexInterface					* pIOMutex,
				TickCountInterface				* pTickCount)
{
    m_nPrivateMulitInstanceIndex    = nInstanceIndex;
    m_pSerX                            = pSerX;
    m_pTheSkyXForMounts                = pTheSkyX;
    m_pSleeper                        = pSleeper;
    m_pIniUtil                        = pIniUtil;
    m_pLogger                        = pLogger;
    m_pIOMutex                        = pIOMutex;
    m_pTickCount                    = pTickCount;


    int  nErr = PLUGIN_OK;

    m_bLinked = false;
    m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_SERIAL, "0", m_szFilterWheelSerial, 128);
    nErr = m_PlayerOneFW.getFilterWheelHandleFromSerial(m_nFilterWheelHandle, std::string(m_szFilterWheelSerial));
    if(nErr) {
        m_nFilterWheelHandle = 0;
        m_PlayerOneFW.setFilterWheelHandle(-1);
        return;
    }
    m_PlayerOneFW.setFilterWheelSerial(std::string(m_szFilterWheelSerial));
    m_PlayerOneFW.setFilterWheelHandle(m_nFilterWheelHandle);
    m_bMoving = false;
    
}

X2FilterWheel::~X2FilterWheel()
{
	if (m_pSerX)
		delete m_pSerX;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pIOMutex)
		delete m_pIOMutex;
}


int	X2FilterWheel::queryAbstraction(const char* pszName, void** ppVal)
{
    X2MutexLocker ml(GetMutex());

    *ppVal = NULL;

    if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();
    else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
        *ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
    else if (!strcmp(pszName, X2GUIEventInterface_Name))
        *ppVal = dynamic_cast<X2GUIEventInterface*>(this);

    return SB_OK;
}


#pragma mark - LinkInterface

int	X2FilterWheel::establishLink(void)
{
    int nErr;

    X2MutexLocker ml(GetMutex());

    nErr = m_PlayerOneFW.Connect(m_nFilterWheelHandle);
    if(nErr)
        m_bLinked = false;
    else
        m_bLinked = true;

    return nErr;
}

int	X2FilterWheel::terminateLink(void)
{
    X2MutexLocker ml(GetMutex());
    m_PlayerOneFW.Disconnect();
    m_bLinked = false;
    return SB_OK;
}

bool X2FilterWheel::isLinked(void) const
{
    X2FilterWheel* pMe = (X2FilterWheel*)this;
    X2MutexLocker ml(pMe->GetMutex());
    return pMe->m_bLinked;
}

bool X2FilterWheel::isEstablishLinkAbortable(void) const	{

    return false;
}


#pragma mark - AbstractDriverInfo

void	X2FilterWheel::driverInfoDetailedInfo(BasicStringInterface& str) const
{
    str = "X2 Player One Filter Wheel Plug In by Rodolphe Pineau";
}

double	X2FilterWheel::driverInfoVersion(void) const
{
	return PLUGIN_VERSION;
}

void X2FilterWheel::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "X2 Player One Filter Wheel ";
}

void X2FilterWheel::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = "X2 Player One Filter Wheel ";

}

void X2FilterWheel::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    str = "X2 Player One Filter Wheel Plug In";

}

void X2FilterWheel::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        std::string sFirmware;
        m_PlayerOneFW.getFirmwareVersion(sFirmware);
        str = sFirmware.c_str();
    }
    else
        str = "N/A";
}
void X2FilterWheel::deviceInfoModel(BasicStringInterface& str)				
{
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        str = "Player One Filter Wheel ";
    }
    else
        str = "N/A";
}

#pragma mark - UI binding

int X2FilterWheel::execModalSettingsDialog()
{
    int nErr = SB_OK;
    bool bPressedOK = false;
    bool bFWFound = false;
    int nFilterWheelIndex = 0;
    int i=0;
    std::stringstream sTmpBuf;
    std::string fName;


    if(m_bLinked) {
        nErr = doPlayerOneFWFeatureConfig();
        return nErr;
    }


    X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*                    ui = uiutil.X2UI();
    X2GUIExchangeInterface*            dx = NULL;//Comes after ui is loaded

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("PlayerOneFWSelect.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;

    std::vector<PWProperties> tFilterWheelPropertiesList;

    X2MutexLocker ml(GetMutex());
    //Intialize the user interface
    m_PlayerOneFW.listFilterWheel(tFilterWheelPropertiesList);
    if(!tFilterWheelPropertiesList.size()) {
        sTmpBuf << "No Filter Wheel found";
        dx->comboBoxAppendString("comboBox",sTmpBuf.str().c_str());
        dx->setCurrentIndex("comboBox",0);
    }
    else {
        bFWFound = true;
        nFilterWheelIndex = 0;
        for(i=0; i< tFilterWheelPropertiesList.size(); i++) {
            //Populate the combo box and set the current index (selection)
            sTmpBuf<<tFilterWheelPropertiesList[i].Name<<" "<< tFilterWheelPropertiesList[i].SN;
            dx->comboBoxAppendString("comboBox",sTmpBuf.str().c_str());
            if(tFilterWheelPropertiesList[i].Handle == m_nFilterWheelHandle)
                nFilterWheelIndex = i;
            std::stringstream().swap(sTmpBuf);
        }
        dx->setCurrentIndex("comboBox",nFilterWheelIndex);
    }

    m_nCurrentDialog = SELECT;

    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK)
    {
        if(bFWFound) {
            int nFilterWheel;
            std::string sFilterWheelSerial;
            //filter wheel
            nFilterWheel = dx->currentIndex("comboBox");
            m_PlayerOneFW.setFilterWheelHandle(tFilterWheelPropertiesList[nFilterWheel].Handle);
            m_nFilterWheelHandle = tFilterWheelPropertiesList[nFilterWheel].Handle;
            m_PlayerOneFW.getFilterWheelSerialFromHandle(m_nFilterWheelHandle, sFilterWheelSerial);
            m_PlayerOneFW.setFilterWheelSerial(sFilterWheelSerial);
            // store filter wheelserial
            m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_SERIAL, sFilterWheelSerial.c_str());
        }
    }
    return nErr;

}

void X2FilterWheel::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr = SB_OK;

    if(!m_bLinked)
        return;

    if (!strcmp(pszEvent, "on_timer")) {
    }
}

int X2FilterWheel::doPlayerOneFWFeatureConfig()
{
    int nErr = SB_OK;
    bool bPressedOK = false;

    X2GUIExchangeInterface* dx = NULL;
    X2ModalUIUtil           uiutil(this, GetTheSkyXFacadeForDrivers());
    X2GUIInterface*         ui = uiutil.X2UI();

    if (NULL == ui)
        return ERR_POINTER;

    if ((nErr = ui->loadUserInterface("PlayerOneFW.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
        return nErr;

    if (NULL == (dx = uiutil.X2DX()))
        return ERR_POINTER;


    if(m_bLinked){
    }

    m_nCurrentDialog = SETTINGS;
    //Display the user interface
    if ((nErr = ui->exec(bPressedOK)))
        return nErr;

    //Retreive values from the user interface
    if (bPressedOK) {
        if(m_bLinked) {
            // m_PlayerOneFW.setDirectionMode(...);
        }

        // save the values to persistent storage
        //nErr |= m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_HOME_ON_PARK, m_bHomeOnPark);
        //nErr |= m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_HOME_ON_UNPARK, m_bHomeOnUnpark);
        //nErr |= m_pIniUtil->writeInt(PARENT_KEY, CHILD_KEY_LOG_RAIN_STATUS, m_bLogRainStatus);

    }
    return nErr;
}

#pragma mark - FilterWheelMoveToInterface

int	X2FilterWheel::filterCount(int& nCount)
{
    int nErr = SB_OK;
    X2MutexLocker ml(GetMutex());
    nErr = m_PlayerOneFW.getFilterCount(nCount);
    if(nErr) {
        nErr = ERR_CMDFAILED;
    }
    return nErr;
}

int	X2FilterWheel::defaultFilterName(const int& nIndex, BasicStringInterface& strFilterNameOut)
{
	X2MutexLocker ml(GetMutex());
    switch(nIndex) {
        case 0:
            strFilterNameOut = "L";
            break;

        case 1:
            strFilterNameOut = "R";
            break;

        case 2:
            strFilterNameOut = "G";
            break;

        case 3:
            strFilterNameOut = "B";
            break;

        case 4:
            strFilterNameOut = "Ha";
            break;

        case 5:
            strFilterNameOut = "O-III";
            break;

        case 6:
            strFilterNameOut = "S-II";
            break;

        default:
            strFilterNameOut = "Unknown";
            break;

    }
    return SB_OK;
}

int	X2FilterWheel::startFilterWheelMoveTo(const int& nTargetPosition)
{
    int nErr = SB_OK;
    
    if(m_bLinked) {
        X2MutexLocker ml(GetMutex());
        nErr = m_PlayerOneFW.moveToFilterIndex(nTargetPosition);
        if(nErr)
            nErr = ERR_CMDFAILED;
    }
    m_bMoving = true;
    return nErr;
}

int	X2FilterWheel::isCompleteFilterWheelMoveTo(bool& bComplete) const
{
    int nErr = SB_OK;

    if(m_bLinked) {
        X2FilterWheel* pMe = (X2FilterWheel*)this;
        X2MutexLocker ml(pMe->GetMutex());
        if(m_bMoving) {
            nErr = pMe->m_PlayerOneFW.isMoveToComplete(bComplete);
            if(nErr)
                nErr = ERR_CMDFAILED;
            if(bComplete)
                pMe->m_bMoving =  false;
        }
    }
    return nErr;
}

int	X2FilterWheel::endFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}

int	X2FilterWheel::abortFilterWheelMoveTo(void)
{
	X2MutexLocker ml(GetMutex());
	return SB_OK;
}
