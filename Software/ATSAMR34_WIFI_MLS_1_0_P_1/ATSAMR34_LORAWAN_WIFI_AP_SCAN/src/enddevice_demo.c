/**
* \file  enddevice_demo.c
*
* \brief LORAWAN Demo Application
*		
*
* Copyright (c) 2018 Microchip Technology Inc. and its subsidiaries. 
*
* \asf_license_start
*
* \page License
*
* Subject to your compliance with these terms, you may use Microchip
* software and any derivatives exclusively with Microchip products. 
* It is your responsibility to comply with third party license terms applicable 
* to your use of third party software (including open source software) that 
* may accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, 
* WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, 
* INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, 
* AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE 
* LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL 
* LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE 
* SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE 
* POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY 
* RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY, 
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*
* \asf_license_stop
*
*/
/*
* Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
*/

/**
* \mainpage
* \section preface Preface
* The EndDevice_Demo_application available in Atmel Studio, 
* is used to send the temperature sensor data through the 
* LoRaWAN network to the network server.
* <P>• This example provides option to user to configure regional band in run time.</P>
* <P>• Using this example application, LoRaWAN Functionalities such as Joining, Sending data
* and changing end device class is demonstrated.</P>
* <P>• This example showcases sleep functionality of LoRaWAN Stack and the Hardware.</P>
* <P>• This example demonstrates storing stack parameters in NVM using PDS. </P>
*/

/****************************** INCLUDES **************************************/
#include "asf.h"
#include "lorawan.h"
#include "system_task_manager.h"
#include "enddevice_demo.h"
#include "conf_app.h"
#include "sio2host.h"
#include "resources.h"
#include "delay.h"
#include "sw_timer.h"
#include "LED.h"
#include "pmm.h"
#include "radio_driver_hal.h"
#include "conf_pmm.h"
#include "conf_sio2host.h"
#include "pds_interface.h"

#if (CERT_APP == 1)
#include "conf_certification.h"
#include "enddevice_cert.h"
#endif
#if (EDBG_EUI_READ == 1)
#include "edbg_eui.h"
#endif
#include "atomic.h"
#include <stdint.h>

// WiFi
#include "asf.h"
#include "main.h"
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"

/******************************** MACROS ***************************************/
// WiFi
#define STRING_EOL    "\r\n"
#define STRING_HEADER "-- WINC1500 AP scan example --"STRING_EOL \
"-- "BOARD_NAME " --"STRING_EOL	\
"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL

/************************** GLOBAL VARIABLES ***********************************/
// WiFi
static uint8_t u8ScanResultIdx = 0 ;	// Index of scan list to request scan result
static uint8_t u8buffer[BUFFER_LEN] ;	// can store up to 7 entries. 1 entry = RSSI [1Byte], BSSID [6Bytes]
static uint8_t u8bufferLen = 0 ;		// buffer length
static uint8_t u8bufferIdx = 0 ;		// buffer index
static uint8_t u8bufferEntry = 0 ;		// buffer entry, 1 entry contains: RSSI and BSSID

static bool joined = false;
//static float cel_val;
//static float fahren_val;
//static char temp_sen_str[25];
//static uint8_t data_len = 0;
bool certAppEnabled = false;

static uint8_t on = LON;
static uint8_t off = LOFF;
static uint8_t toggle = LTOGGLE;

static volatile uint8_t appTaskFlags = 0x00u;
/* Default Regional band start delay time */
volatile static uint8_t count = 5;

static uint8_t rxchar[11];
static bool startReceiving = false;
extern uint8_t demoTimerId;
extern uint8_t lTimerId;
static AppTaskState_t appTaskState;

static const char* bandStrings[] =
{
    "FactoryDefaultReset",
#if (EU_BAND == 1)
    "EU868",
#endif
#if (NA_BAND == 1)
    "NA915",
#endif
#if (AU_BAND == 1)
    "AU915",
#endif
#if (AS_BAND == 1)
    "AS923",
#endif
#if (JPN_BAND == 1)
    "JPN923",
#endif
#if (KR_BAND == 1)
    "KR920",
#endif
#if (IND_BAND == 1)
    "IND865",
#endif
    "Clear PDS",
    "Reset Board"
};


uint8_t bandTable[] =
{
    0xFF,
#if (EU_BAND == 1)
    ISM_EU868,
#endif
#if (NA_BAND == 1)
    ISM_NA915,
#endif
#if (AU_BAND == 1)
    ISM_AU915,
#endif
#if (AS_BAND == 1)
    ISM_THAI923,
#endif
#if (JPN_BAND == 1)
    ISM_JPN923,
#endif
#if (KR_BAND == 1)
    ISM_KR920,
#endif
#if (IND_BAND == 1)
    ISM_IND865,
#endif
    0xFF,
    0xFF
};

/*ABP Join Parameters */
static uint32_t demoDevAddr = DEMO_DEVICE_ADDRESS;
static uint8_t demoNwksKey[16] = DEMO_NETWORK_SESSION_KEY;
static uint8_t demoAppsKey[16] = DEMO_APPLICATION_SESSION_KEY;
/* OTAA join parameters */
static uint8_t demoDevEui[8] = DEMO_DEVICE_EUI;
static uint8_t demoAppEui[8] = DEMO_APPLICATION_EUI;
static uint8_t demoAppKey[16] = DEMO_APPLICATION_KEY;

static LorawanSendReq_t lorawanSendReq;
static char serialBuffer;

/* Muticast Parameters */
static bool demoMcastEnable = DEMO_APP_MCAST_ENABLE;
static uint32_t demoMcastDevAddr = DEMO_APP_MCAST_GROUP_ADDRESS;
static uint8_t demoMcastNwksKey[16] = DEMO_APP_MCAST_NWK_SESSION_KEY;
static uint8_t demoMcastAppsKey[16] = DEMO_APP_MCAST_APP_SESSION_KEY;
/************************** EXTERN VARIABLES ***********************************/
extern bool button_pressed;
extern bool factory_reset;
extern bool bandSelected;
extern uint32_t longPress;

static void sendData(void);
static void appPostTask(AppTaskIds_t id);
static SYSTEM_TaskStatus_t (*appTaskHandlers[])(void);
static void demoTimerCb(void * cnt);
static void lTimerCb(void * data);
static SYSTEM_TaskStatus_t displayTask(void);
static SYSTEM_TaskStatus_t processTask(void);
static void processRunDemoCertApp(void);
static void processRunRestoreBand(void);
static void processJoinAndSend(void);
static void processRunDemoApp(void);
static void displayRunDemoCertApp(void);
static void displayRunRestoreBand(void);
static void displayJoinAndSend(void);
static void displayRunDemoApp(void);
#if (CERT_APP == 1)
static void runCertApp(void);
#endif
#ifdef CONF_PMM_ENABLE
static void appWakeup(uint32_t sleptDuration);
static void app_resources_uninit(void);
#endif
static void dev_eui_read(void);
/************************** FUNCTION PROTOTYPES ********************************/
SYSTEM_TaskStatus_t APP_TaskHandler(void);
static float convert_celsius_to_fahrenheit(float cel_val);
// WiFi
void power_down_wifi(void) ;
/*********************************************************************//*
 \brief      Function that processes the Rx data
 \param[in]  data - Rx data payload
 \param[in]  dataLen - The number of Rx bytes
 ************************************************************************/
static void demo_handle_evt_rx_data(void *appHandle, appCbParams_t *appdata);

/***************************** FUNCTIONS ***************************************/

static SYSTEM_TaskStatus_t (*appTaskHandlers[APP_TASKS_COUNT])(void) = {
    /* In the order of descending priority */
    displayTask,
    processTask
};

/*********************************************************************//**
\brief    Calls appropriate functions based on state variables
*************************************************************************/
static SYSTEM_TaskStatus_t displayTask(void)
{
	switch(appTaskState)
	{
		case RESTORE_BAND_STATE:
			displayRunRestoreBand();
			break;
		case DEMO_CERT_APP_STATE:
			displayRunDemoCertApp();
			break;
		case DEMO_APP_STATE:
			displayRunDemoApp();
			break;
		case JOIN_SEND_STATE:
			displayJoinAndSend();
			break;
		default:
			printf("Error STATE Entered\r\n");
			break;
	}
	
	return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief    Pulls the data from UART when activated
*************************************************************************/
void serial_data_handler(void)
{
	int rxChar;
	char serialData;
	/* verify if there was any character received*/
	if (startReceiving == true)
	{
		if((-1) != (rxChar = sio2host_getchar_nowait()))
		{
			serialData = (char)rxChar;
			if((serialData != '\r') && (serialData != '\n') && (serialData != '\b'))
			{
				startReceiving = false;
  			    serialBuffer = rxChar;
			    appPostTask(PROCESS_TASK_HANDLER);
				printf("\r\n");			
			}
		}
	}
}

/*********************************************************************//**
\brief    Calls appropriate functions based on state variables
*************************************************************************/
static SYSTEM_TaskStatus_t processTask(void)
{
	switch(appTaskState)
	{
		case RESTORE_BAND_STATE:
			processRunRestoreBand();
			break;
		case DEMO_CERT_APP_STATE:
			processRunDemoCertApp();
			break;
		case DEMO_APP_STATE:
			processRunDemoApp();
			break;
		case JOIN_SEND_STATE:
			processJoinAndSend();
			break;
		default:
			printf("Error STATE Entered\r\n");
			break;
	}
	
	return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//**
\brief    Activates demo application or certification application
*************************************************************************/
static void processRunDemoCertApp(void)
{
	if(serialBuffer == '1')
	{
		appTaskState = DEMO_APP_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
	#if (CERT_APP == 1)
	else if(serialBuffer == '2')
	{
		runCertApp();
	}
	#endif
	else
	{
		printf("Please enter a valid choice\r\n");
		appTaskState = DEMO_CERT_APP_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
}

/*********************************************************************//**
\brief    Restores the previous band and runs
*************************************************************************/
static void processRunRestoreBand(void)
{
	StackRetStatus_t status = LORAWAN_SUCCESS;
	uint8_t prevBand = 0xff;
	uint8_t choice = 0xff;
	
	PDS_RestoreAll();
	LORAWAN_GetAttr(ISMBAND,NULL,&prevBand);
	for (uint32_t i = 0; i < sizeof(bandTable)-1; i++)
	{
		if(bandTable[i] == prevBand)
		{
			choice = i;
			break;
		}
	}
	if(choice >0 && choice < sizeof(bandTable)-1)
	{
		status = LORAWAN_Reset(bandTable[choice]);
	}
	if(status == LORAWAN_SUCCESS && choice < sizeof(bandTable)-1)
	{
		uint32_t joinStatus = 0;
		PDS_RestoreAll();
		LORAWAN_GetAttr(LORAWAN_STATUS,NULL, &joinStatus);
		printf("\r\nPDS_RestorationStatus: Success\r\n" );
		if(joinStatus & LORAWAN_NW_JOINED)
		{
			joined = true;
			printf("joinStatus: Joined\r\n");
		}
		else
		{
			joined = false;
			printf("JoinStatus : Denied\r\n");
			set_LED_data(LED_AMBER,&on);
			SYSTEM_PostTask(APP_TASK_ID);
		}
		printf("Band: %s\r\n",bandStrings[choice]);

		print_application_config();
		appTaskState = JOIN_SEND_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
	else
	{
		printf("Restoration failed\r\n");
		appTaskState = DEMO_APP_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
}

/*********************************************************************//**
\brief    Sends Join request or Data to the network
*************************************************************************/
static void processJoinAndSend(void)
{
	StackRetStatus_t status = LORAWAN_SUCCESS;
	if(serialBuffer == '1')
	{
		status = LORAWAN_Join(DEMO_APP_ACTIVATION_TYPE);
		if (LORAWAN_SUCCESS == (StackRetStatus_t)status)
		{
			set_LED_data(LED_GREEN,&on);
			printf("\nJoin Request Sent\n\r");

		}
		else
		{
			set_LED_data(LED_AMBER,&on);
			print_stack_status(status);
			appTaskState = JOIN_SEND_STATE;
			appPostTask(DISPLAY_TASK_HANDLER);
		}
	}
	else if(serialBuffer == '2' && joined == true)
	{
#ifdef WIFI_POWER_DOWN_ENABLE
		printf("\n> Powering Up WINC1500\r\n") ;
		wifi_init() ;
#endif
		// Configure Scan Parameters
		tstrM2MScanOption strScanOptions ;
		strScanOptions.u8NumOfSlot = 1 ; //M2M_SCAN_DEFAULT_NUM_SLOTS;
		strScanOptions.u8ProbesPerSlot = M2M_SCAN_DEFAULT_NUM_PROBE ;
		strScanOptions.u8SlotTime = M2M_SCAN_DEFAULT_SLOT_TIME ;
		strScanOptions.s8RssiThresh = -60 ;
		m2m_wifi_set_scan_options(&strScanOptions) ;
#if (DEMO_AP_SCAN_MODE == PASSIVE_SCAN)
		printf("\n>Starting passive scan ...\r\n") ;
		m2m_wifi_request_scan_passive(DEMO_CHANNEL_TO_SCAN, DEMO_PASSIVE_SCAN_TIMEOUT) ;
#else
		printf("\n>Starting active scan ...\r\n") ;
		m2m_wifi_request_scan(DEMO_CHANNEL_TO_SCAN) ;
#endif
	}
	else if(serialBuffer == '2' && !joined)
	{
		set_LED_data(LED_AMBER,&on);
		printf("Device not joined to the network\r\n");
		appTaskState = JOIN_SEND_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
#ifdef CONF_PMM_ENABLE
	else if(serialBuffer == '3')
	{
		static bool deviceResetsForWakeup = false;
		PMM_SleepReq_t sleepReq;
		/* Put the application to sleep */
		sleepReq.sleepTimeMs = DEMO_CONF_DEFAULT_APP_SLEEP_TIME_MS;
		sleepReq.pmmWakeupCallback = appWakeup;
		sleepReq.sleep_mode = CONF_PMM_SLEEPMODE_WHEN_IDLE;
		if (CONF_PMM_SLEEPMODE_WHEN_IDLE == SLEEP_MODE_STANDBY)
		{
			deviceResetsForWakeup = false;
		}
		if (true == LORAWAN_ReadyToSleep(deviceResetsForWakeup))
		{
			app_resources_uninit();
			if (PMM_SLEEP_REQ_DENIED == PMM_Sleep(&sleepReq))
			{
				HAL_Radio_resources_init();
				sio2host_init();
				appTaskState = JOIN_SEND_STATE;
				appPostTask(DISPLAY_TASK_HANDLER);
				printf("\r\nsleep_not_ok\r\n");	
			}
		}
		else
		{
			printf("\r\nsleep_not_ok\r\n");
			appTaskState = JOIN_SEND_STATE;
			appPostTask(DISPLAY_TASK_HANDLER);
		}
	}
	else if(serialBuffer == '4')
#else
	else if(serialBuffer == '3')
#endif
	{
		appTaskState = DEMO_APP_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
	else
	{
		set_LED_data(LED_AMBER,&on);
		printf("Invalid choice entered\r\n");
		appTaskState = JOIN_SEND_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
}

/*********************************************************************//**
\brief    Runs the Demo application
*************************************************************************/
static void processRunDemoApp(void)
{
	uint8_t num = serialBuffer - '0';
	if(num == sizeof(bandTable)-1)
	{
		NVIC_SystemReset();
	}
	else if(num == sizeof(bandTable)-2)
	{
		PDS_DeleteAll();
		appTaskState = DEMO_APP_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
	else if(num >0 && num < sizeof(bandTable) -2)
	{
		LORAWAN_Reset(bandTable[num]);
		mote_set_parameters(bandTable[num],num);
		set_LED_data(LED_GREEN,&on);
	}
	else
	{
		printf("Not a valid regional band choice\r\n");
		appTaskState = DEMO_APP_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
	}
}

/*********************************************************************//**
\brief    Displays and activates LED's to choose between Demo
		  and Certification application
*************************************************************************/
static void displayRunDemoCertApp(void)
{
	//sio2host_rx(rxchar,10);
	set_LED_data(LED_AMBER,&off);
	set_LED_data(LED_GREEN,&off);
	printf("1. Demo application\r\n");
	#if (CERT_APP == 1)
	printf("2. Certification application\r\n");
	#endif
	printf("\r\n Select Application : ");
	startReceiving = true;
}

/*********************************************************************//**
\brief    Activates LED's to indicate restoring of band
*************************************************************************/
static void displayRunRestoreBand(void)
{
	//sio2host_rx(rxchar,10);
	set_LED_data(LED_AMBER,&off);
	set_LED_data(LED_GREEN,&off);
	appPostTask(PROCESS_TASK_HANDLER);
}

/*********************************************************************//**
\brief    Displays and activates LED's for joining to a network
		  and sending data to a network
*************************************************************************/
static void displayJoinAndSend(void)
{
    printf("\r\n1. Send Join Request\r\n");
    printf("2. Scan AP and send Data\r\n");
#ifdef CONF_PMM_ENABLE
    printf("3. Sleep\r\n");
    printf("4. Main Menu\r\n");
#else
    printf("3. Main Menu\r\n");
#endif /* CONF_PMM_ENABLE */
    printf("\r\nEnter your choice: ");
    set_LED_data(LED_AMBER,&off);
    set_LED_data(LED_GREEN,&off);	
	startReceiving = true;
}

/*********************************************************************//**
\brief    Displays and activates LED's for selecting Demo application
*************************************************************************/
static void displayRunDemoApp(void)
{
	uint8_t i = 0;
	
    set_LED_data(LED_AMBER,&off);
    set_LED_data(LED_GREEN,&off);

    printf("\r\nPlease select one of the band given below\r\n");
    for(i = 1;i < sizeof(bandTable); i++)
    {
	    printf("%d. %s\r\n",i,bandStrings[i]);
    }

    printf("Select Regional Band : ");
	startReceiving = true;
}

/*********************************************************************//**
\brief    Initialization the Demo application
*************************************************************************/
void mote_demo_init(void)
{
    bool status = false;
    /* Initialize the resources */
    resource_init();
	/* Read DEV EUI from EDBG */
	dev_eui_read();
	startReceiving = false;
    /* Initialize the LORAWAN Stack */
    LORAWAN_Init(demo_appdata_callback, demo_joindata_callback);
    printf("\n\n\r*******************************************************\n\r");
    printf("\n\rMicrochip LoRaWAN Stack %s\r\n",STACK_VER);
    printf("\r\nInit - Successful\r\n");

    status = PDS_IsRestorable();
    if(status)
    {
        static uint8_t prevBand = 0xFF;
        uint8_t prevChoice = 0xFF;
        PDS_RestoreAll();
        LORAWAN_GetAttr(ISMBAND,NULL,&prevBand);
        for (uint32_t i = 0; i < sizeof(bandTable) -1; i++)
        {
            if(bandTable[i] == prevBand)
            {
                prevChoice = i;
                break;
            }
        }
        memset(rxchar,0,sizeof(rxchar));
        sio2host_rx(rxchar,10);
        printf ("Last configured Regional band %s\r\n",bandStrings[prevChoice]);
        printf("Press any key to change band\r\n Continuing in %s in ", bandStrings[prevChoice]);

        SwTimerStart(demoTimerId,MS_TO_US(1000),SW_TIMEOUT_RELATIVE,(void *)demoTimerCb,NULL);
    }
    else
    {
		appTaskState = DEMO_CERT_APP_STATE;
        appPostTask(DISPLAY_TASK_HANDLER);
    }
}

void power_down_wifi(void)
{
	printf("\n> Powering Down WINC1500\r\n") ;
	m2m_wifi_deinit(NULL) ;
	nm_bsp_deinit() ;
}

/*** wifi_event_cb **************************************************************
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_DHCP_CONF](@ref M2M_WIFI_REQ_DHCP_CONF)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type.
********************************************************************************/
static void wifi_event_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) 
	{
		case M2M_WIFI_RESP_SCAN_DONE:
		{
			tstrM2mScanDone *pstrInfo = (tstrM2mScanDone *)pvMsg ;
			if(pstrInfo->s8ScanState == M2M_SUCCESS)
			{
				// reset counters
				memset(u8buffer, 0, sizeof(u8buffer)) ;
				u8bufferLen = 0 ;
				u8bufferIdx = 0 ;
				u8bufferEntry = 0 ;
				u8ScanResultIdx = 0 ;
				if (pstrInfo->u8NumofCh >= 1)
				{
					printf("Number of AP found %d\r\n", pstrInfo->u8NumofCh) ;
					m2m_wifi_req_scan_result(u8ScanResultIdx) ;
					u8ScanResultIdx ++ ;
				}
				else
				{
					printf("No AP Found") ;
#ifdef WIFI_POWER_DOWN_ENABLE
					power_down_wifi() ;
#endif
					u8buffer[u8bufferIdx++] = pstrInfo->u8NumofCh ;
					u8bufferLen = u8bufferIdx ;
					// transmit 1Byte
					sendData() ;
				}
			}
			else
			{
				printf("(ERR) Scan fail with error <%d>\n",pstrInfo->s8ScanState) ;
			}
			break ;
		}
		case M2M_WIFI_RESP_SCAN_RESULT:
		{
			tstrM2mWifiscanResult *pstrScanResult =(tstrM2mWifiscanResult*)pvMsg ;
			uint8 u8NumFoundAPs = m2m_wifi_get_num_ap_found() ;
			printf("%02d RSSI:%d AUTHTYPE:%d CHANNEL:%02d BSSID:%02X:%02X:%02X:%02X:%02X:%02X SSID:%s\n",
			pstrScanResult->u8index,
			pstrScanResult->s8rssi,
			pstrScanResult->u8AuthType,
			pstrScanResult->u8ch,
			pstrScanResult->au8BSSID[0], pstrScanResult->au8BSSID[1], pstrScanResult->au8BSSID[2],
			pstrScanResult->au8BSSID[3], pstrScanResult->au8BSSID[4], pstrScanResult->au8BSSID[5],
			pstrScanResult->au8SSID) ;

			if (u8bufferEntry < ENTRY_NUMBER)
			{
				u8bufferEntry++ ;
				// store n ENTRY_NUMBER in the buffer, 1 entry = RSSI[1-Byte], BSSID[6-Bytes]
				u8buffer[u8bufferIdx++] = pstrScanResult->s8rssi ;	// signed to unsigned (eg. -53dBm -> 0xCB => (0xFF-0xCB+1)
				u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[0] ;
				u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[1] ;
				u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[2] ;
				u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[3] ;
				u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[4] ;
				u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[5] ;
				u8bufferLen = u8bufferIdx ;
			}
			else
			{
				// new entry is coming
				uint8_t u8new_rssi = pstrScanResult->s8rssi ;
				// search for the lowest RSSI value in the current buffer
				u8bufferIdx = 0 ;
				uint8_t u8min_rssi_pos = 0 ;
				uint8_t u8min_rssi_val = u8buffer[u8bufferIdx] ;
				for (uint8_t i = 1; i < ENTRY_NUMBER; i++)
				{
					u8bufferIdx += 7 ;
					if (u8buffer[u8bufferIdx] < u8min_rssi_val)
					{
						u8min_rssi_val = u8buffer[u8bufferIdx] ;
						u8min_rssi_pos = u8bufferIdx ;
					}
				}
				if (u8new_rssi > u8min_rssi_val)
				{
					// new entry will replace the entry with worst signal
					u8bufferIdx = u8min_rssi_pos ;
					u8buffer[u8bufferIdx++] = pstrScanResult->s8rssi ;	// signed to unsigned (eg. -53dBm -> 0xCB => (0xFF-0xCB+1)
					u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[0] ;
					u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[1] ;
					u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[2] ;
					u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[3] ;
					u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[4] ;
					u8buffer[u8bufferIdx++] = pstrScanResult->au8BSSID[5] ;					
				}
			}	

			if(u8ScanResultIdx < u8NumFoundAPs)
			{
				// Read the next scan result
				m2m_wifi_req_scan_result(u8ScanResultIdx) ;
				u8ScanResultIdx ++ ;
			}
			else
			{
				printf(">End of scan") ;
#ifdef WIFI_POWER_DOWN_ENABLE
				power_down_wifi() ;
#endif
				if(u8bufferEntry==1)printf("\n%d best entry loaded in the payload", u8bufferEntry) ;
				else				printf("\n%d best entries loaded in the payload", u8bufferEntry) ;
				sendData() ;
			}
			break ;
		}
	}
}

/*********************************************************************//**
\brief    Initialize the WiFi device
*************************************************************************/
void wifi_init(void)
{
	/* Initialize WiFi */
	tstrWifiInitParam param ;
	int8_t ret ;
//	printf(STRING_HEADER) ;
	// Initialize the BSP.
	nm_bsp_init();
	// Initialize Wi-Fi parameters structure.
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));
	// Initialize Wi-Fi driver with data and status callbacks.
	param.pfAppWifiCb = wifi_event_cb ;
	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret)
	{
		printf("main: m2m_wifi_init call error!(%d)\r\n", ret) ;
		while (1){} ;
	}
}

/*********************************************************************//*
 \brief      Function that processes the Rx data
 \param[in]  data - Rx data payload
 \param[in]  dataLen - The number of Rx bytes
 ************************************************************************/
static void demo_handle_evt_rx_data(void *appHandle, appCbParams_t *appdata)
{
    uint8_t *pData = appdata->param.rxData.pData;
    uint8_t dataLength = appdata->param.rxData.dataLength;
    uint32_t devAddress = appdata->param.rxData.devAddr;

    //Successful transmission
    if((dataLength > 0U) && (NULL != pData))
    {
        printf("*** Received DL Data ***\n\r");
        printf("\nFrame Received at port %d\n\r",pData[0]);
        printf("\nFrame Length - %d\n\r",dataLength);
        printf("\nAddress - 0x%lx\n\r", devAddress);
        printf ("\nPayload: ");
        for (uint8_t i =0; i<dataLength - 1; i++)
        {
            printf("%x",pData[i+1]);
        }
        printf("\r\n*************************\r\n");
    }
    else
    {
        printf("Received ACK for Confirmed data\r\n");
    }
}

/*********************************************************************//**
\brief Callback function for the ending of Bidirectional communication of
       Application data
 *************************************************************************/
void demo_appdata_callback(void *appHandle, appCbParams_t *appdata)
{
    StackRetStatus_t status = LORAWAN_INVALID_REQUEST;

    if (LORAWAN_EVT_RX_DATA_AVAILABLE == appdata->evt)
    {
        status = appdata->param.rxData.status;
        switch(status)
        {
            case LORAWAN_SUCCESS:
            {
                demo_handle_evt_rx_data(appHandle, appdata);
            }
            break;
            case LORAWAN_RADIO_NO_DATA:
            {
                printf("\n\rRADIO_NO_DATA \n\r");
            }
            break;
            case LORAWAN_RADIO_DATA_SIZE:
                printf("\n\rRADIO_DATA_SIZE \n\r");
            break;
            case LORAWAN_RADIO_INVALID_REQ:
                printf("\n\rRADIO_INVALID_REQ \n\r");
            break;
            case LORAWAN_RADIO_BUSY:
                printf("\n\rRADIO_BUSY \n\r");
            break;
            case LORAWAN_RADIO_OUT_OF_RANGE:
                printf("\n\rRADIO_OUT_OF_RANGE \n\r");
            break;
            case LORAWAN_RADIO_UNSUPPORTED_ATTR:
                printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
            break;
            case LORAWAN_RADIO_CHANNEL_BUSY:
                printf("\n\rRADIO_CHANNEL_BUSY \n\r");
            break;
            case LORAWAN_NWK_NOT_JOINED:
                printf("\n\rNWK_NOT_JOINED \n\r");
            break;
            case LORAWAN_INVALID_PARAMETER:
                printf("\n\rINVALID_PARAMETER \n\r");
            break;
            case LORAWAN_KEYS_NOT_INITIALIZED:
                printf("\n\rKEYS_NOT_INITIALIZED \n\r");
            break;
            case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
                printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
            break;
            case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
                printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
            break;
            case LORAWAN_INVALID_BUFFER_LENGTH:
                printf("\n\rINVALID_BUFFER_LENGTH \n\r");
            break;
            case LORAWAN_MAC_PAUSED :
                printf("\n\rMAC_PAUSED  \n\r");
            break;
            case LORAWAN_NO_CHANNELS_FOUND:
                printf("\n\rNO_CHANNELS_FOUND \n\r");
            break;
            case LORAWAN_BUSY:
                printf("\n\rBUSY\n\r");
            break;
            case LORAWAN_NO_ACK:
                printf("\n\rNO_ACK \n\r");
            break;
            case LORAWAN_NWK_JOIN_IN_PROGRESS:
                printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
            break;
            case LORAWAN_RESOURCE_UNAVAILABLE:
                printf("\n\rRESOURCE_UNAVAILABLE \n\r");
            break;
            case LORAWAN_INVALID_REQUEST:
                printf("\n\rINVALID_REQUEST \n\r");
            break;
            case LORAWAN_FCNTR_ERROR:
                printf("\n\rFCNTR_ERROR \n\r");
            break;
            case LORAWAN_MIC_ERROR:
                printf("\n\rMIC_ERROR \n\r");
            break;
            case LORAWAN_INVALID_MTYPE:
                printf("\n\rINVALID_MTYPE \n\r");
            break;
            case LORAWAN_MCAST_HDR_INVALID:
                printf("\n\rMCAST_HDR_INVALID \n\r");
            break;
            default:
                printf("UNKNOWN ERROR\n\r");
            break;
        }
    }
    else if(LORAWAN_EVT_TRANSACTION_COMPLETE == appdata->evt)
    {
        switch(status = appdata->param.transCmpl.status)
        {
            case LORAWAN_SUCCESS:
            {
                printf("Transmission Success\r\n");
            }
            break;
            case LORAWAN_RADIO_SUCCESS:
            {
                printf("Transmission Success\r\n");
            }
            break;
            case LORAWAN_RADIO_NO_DATA:
            {
                printf("\n\rRADIO_NO_DATA \n\r");
            }
            break;
            case LORAWAN_RADIO_DATA_SIZE:
                printf("\n\rRADIO_DATA_SIZE \n\r");
            break;
            case LORAWAN_RADIO_INVALID_REQ:
                printf("\n\rRADIO_INVALID_REQ \n\r");
            break;
            case LORAWAN_RADIO_BUSY:
                printf("\n\rRADIO_BUSY \n\r");
            break;
            case LORAWAN_TX_TIMEOUT:
                printf("\nTx Timeout\n\r");
            break;
            case LORAWAN_RADIO_OUT_OF_RANGE:
                printf("\n\rRADIO_OUT_OF_RANGE \n\r");
            break;
            case LORAWAN_RADIO_UNSUPPORTED_ATTR:
                printf("\n\rRADIO_UNSUPPORTED_ATTR \n\r");
            break;
            case LORAWAN_RADIO_CHANNEL_BUSY:
                printf("\n\rRADIO_CHANNEL_BUSY \n\r");
            break;
            case LORAWAN_NWK_NOT_JOINED:
                printf("\n\rNWK_NOT_JOINED \n\r");
            break;
            case LORAWAN_INVALID_PARAMETER:
                printf("\n\rINVALID_PARAMETER \n\r");
            break;
            case LORAWAN_KEYS_NOT_INITIALIZED:
                printf("\n\rKEYS_NOT_INITIALIZED \n\r");
            break;
            case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
                printf("\n\rSILENT_IMMEDIATELY_ACTIVE\n\r");
            break;
            case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
                printf("\n\rFCNTR_ERROR_REJOIN_NEEDED \n\r");
            break;
            case LORAWAN_INVALID_BUFFER_LENGTH:
                printf("\n\rINVALID_BUFFER_LENGTH \n\r");
            break;
            case LORAWAN_MAC_PAUSED :
                printf("\n\rMAC_PAUSED  \n\r");
            break;
            case LORAWAN_NO_CHANNELS_FOUND:
                printf("\n\rNO_CHANNELS_FOUND \n\r");
            break;
            case LORAWAN_BUSY:
                printf("\n\rBUSY\n\r");
            break;
            case LORAWAN_NO_ACK:
                printf("\n\rNO_ACK \n\r");
            break;
            case LORAWAN_NWK_JOIN_IN_PROGRESS:
                printf("\n\rALREADY JOINING IS IN PROGRESS \n\r");
            break;
            case LORAWAN_RESOURCE_UNAVAILABLE:
                printf("\n\rRESOURCE_UNAVAILABLE \n\r");
            break;
            case LORAWAN_INVALID_REQUEST:
                printf("\n\rINVALID_REQUEST \n\r");
            break;
            case LORAWAN_FCNTR_ERROR:
                printf("\n\rFCNTR_ERROR \n\r");
            break;
            case LORAWAN_MIC_ERROR:
                printf("\n\rMIC_ERROR \n\r");
            break;
            case LORAWAN_INVALID_MTYPE:
                printf("\n\rINVALID_MTYPE \n\r");
            break;
            case LORAWAN_MCAST_HDR_INVALID:
                printf("\n\rMCAST_HDR_INVALID \n\r");
            break;
            default:
                printf("\n\rUNKNOWN ERROR\n\r");
            break;

                    }
        printf("\n\r*************************************************\n\r");
    }

    SwTimerStop(lTimerId);
    set_LED_data(LED_GREEN,&off);
    if(status != LORAWAN_SUCCESS)
    {
        set_LED_data(LED_AMBER,&on);
    }
	appTaskState = JOIN_SEND_STATE;
    appPostTask(DISPLAY_TASK_HANDLER);
}

/*********************************************************************//*
\brief Callback function for the ending of Activation procedure
 ************************************************************************/
void demo_joindata_callback(bool status)
{
    /* This is called every time the join process is finished */
    set_LED_data(LED_GREEN,&off);
    if(true == status)
    {
        uint32_t devAddress;
        bool mcastEnabled;

        joined = true;
        printf("\nJoining Successful\n\r");
        LORAWAN_GetAttr(DEV_ADDR, NULL, &devAddress);
        LORAWAN_GetAttr(MCAST_ENABLE, NULL, &mcastEnabled);

        if (devAddress != DEMO_APP_MCAST_GROUP_ADDRESS)
        {
            printf("\nDevAddr: 0x%lx\n\r", devAddress);
        }
        else if ((devAddress == DEMO_APP_MCAST_GROUP_ADDRESS) && (true == mcastEnabled))
        {
            printf("\nAddress conflict between Device Address and Multicast group address\n\r");
        }
        print_application_config();
        set_LED_data(LED_GREEN,&on);
    }
    else
    {
        joined = false;
        set_LED_data(LED_AMBER,&on);
        printf("\nJoining Denied\n\r");
    }
    printf("\n\r*******************************************************\n\r");
    PDS_StoreAll();
	
	appTaskState = JOIN_SEND_STATE;
    appPostTask(DISPLAY_TASK_HANDLER);
}

void lTimerCb(void *data)
{
    SwTimerStart(lTimerId,MS_TO_US(100),SW_TIMEOUT_RELATIVE,(void *)lTimerCb,NULL);
    set_LED_data(LED_GREEN,&toggle);
}

/*********************************************************************//*
 \brief      Function to send data from end device to application server
  ************************************************************************/
void sendData(void)
{
	int status = -1 ;

	printf("\nPayload length: %d", u8bufferLen) ;
	printf("\nPayload: ") ;
	print_array(u8buffer, u8bufferLen) ;

	lorawanSendReq.buffer = &u8buffer ;
    lorawanSendReq.bufferLength = u8bufferLen ;
    lorawanSendReq.confirmed = DEMO_APP_TRANSMISSION_TYPE ;
    lorawanSendReq.port = DEMO_APP_FPORT ;
    status = LORAWAN_Send(&lorawanSendReq) ;
    if (LORAWAN_SUCCESS == status)
    {
	    printf("\nTx Data Sent \r\n") ;
	    set_LED_data(LED_GREEN,&on) ;
		SwTimerStart(lTimerId,MS_TO_US(100),SW_TIMEOUT_RELATIVE,(void *)lTimerCb,NULL) ;
    }
    else
    {
	    print_stack_status(status) ;
	    appTaskState = JOIN_SEND_STATE ;
	    appPostTask(DISPLAY_TASK_HANDLER) ;
    }
}

#ifdef CONF_PMM_ENABLE
static void appWakeup(uint32_t sleptDuration)
{
    HAL_Radio_resources_init();
    sio2host_init();
	appTaskState = JOIN_SEND_STATE;
    appPostTask(DISPLAY_TASK_HANDLER);
    printf("\r\nsleep_ok %ld ms\r\n", sleptDuration);
}
#endif

#ifdef CONF_PMM_ENABLE
static void app_resources_uninit(void)
{
    /* Disable USART TX and RX Pins */
    struct port_config pin_conf;
    port_get_config_defaults(&pin_conf);
    pin_conf.powersave  = true;
#ifdef HOST_SERCOM_PAD0_PIN
    port_pin_set_config(HOST_SERCOM_PAD0_PIN, &pin_conf);
#endif
#ifdef HOST_SERCOM_PAD1_PIN
    port_pin_set_config(HOST_SERCOM_PAD1_PIN, &pin_conf);
#endif
    /* Disable UART module */
    sio2host_deinit();
    /* Disable Transceiver SPI Module */
    HAL_RadioDeInit();
}
#endif


#if (CERT_APP == 1)
/*********************************************************************//*
 \brief      Function to runs certification application.
 ************************************************************************/
static void  runCertApp(void)
{
    certAppEnabled = true;
    cert_app_init();
}
#endif
/*********************************************************************//*
 \brief      Timer callback for demo application.
             Used during the initial 5 sec wait period.
 \param[in]  cnt - not used
 ************************************************************************/
void demoTimerCb(void * cnt)
{
    uint8_t i = 10;
    int8_t rxdata = 0;
    printf("%d..",count);
    count--;
	startReceiving = false;
    sio2host_rx(rxchar,10);
    for(i = 0;i<=10;i++)
    {
        if(rxchar[i] != 13 && rxchar[i] != 10)
        {
            rxdata = rxchar[i];
            break;
        }
    }
    if(!count)
    {
        printf("\r\n");
    }
    /* No input so far. start timer till expiry */
    if(count > 0 && (!rxdata))
    {
        SwTimerStart(demoTimerId,MS_TO_US(1000),SW_TIMEOUT_RELATIVE,(void *)demoTimerCb,NULL);
    }
    /* user did not press any input */
    else if(count == 0 && (!rxdata))
    {
		appTaskState = RESTORE_BAND_STATE;
        appPostTask(DISPLAY_TASK_HANDLER);
    }
    /* User pressed a key */
    else if(rxdata)
    {
        printf("\r\n");
		appTaskState = DEMO_CERT_APP_STATE;
        appPostTask(DISPLAY_TASK_HANDLER);
    }

}

/*********************************************************************//*
 \brief      App Post Task
 \param[in]  Id of the application to be posted
 ************************************************************************/

void appPostTask(AppTaskIds_t id)
{
    ATOMIC_SECTION_ENTER
    appTaskFlags |= (1 << id);
    ATOMIC_SECTION_EXIT

    /* Also post a APP task to the system */
    SYSTEM_PostTask(APP_TASK_ID);
}

/*********************************************************************//*
 \brief      Application Task Handler
 ************************************************************************/

SYSTEM_TaskStatus_t APP_TaskHandler(void)
{

    if (appTaskFlags)
    {
        for (uint16_t taskId = 0; taskId < APP_TASKS_COUNT; taskId++)
        {
            if ((1 << taskId) & (appTaskFlags))
            {
                ATOMIC_SECTION_ENTER
                appTaskFlags &= ~(1 << taskId);
                ATOMIC_SECTION_EXIT

                appTaskHandlers[taskId]();

                if (appTaskFlags)
                {
                    SYSTEM_PostTask(APP_TASK_ID);
                }

                break;
            }
        }
    }

    return SYSTEM_TASK_SUCCESS;
}

/*********************************************************************//*
 \brief      Set join parameters function
 \param[in]  activation type - notifies the activation type (OTAA/ABP)
 \return     LORAWAN_SUCCESS, if successfully set the join parameters
             LORAWAN_INVALID_PARAMETER, otherwise
 ************************************************************************/
StackRetStatus_t set_join_parameters(ActivationType_t activation_type)
{
    StackRetStatus_t status;

    printf("\n********************Join Parameters********************\n\r");

    if(ACTIVATION_BY_PERSONALIZATION == activation_type)
    {
        status = LORAWAN_SetAttr (DEV_ADDR, &demoDevAddr);
        if (LORAWAN_SUCCESS == status)
        {
            status = LORAWAN_SetAttr (APPS_KEY, demoAppsKey);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nAppSessionKey : ");
            print_array((uint8_t *)&demoAppsKey, sizeof(demoAppsKey));
            status = LORAWAN_SetAttr (NWKS_KEY, demoNwksKey);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nNwkSessionKey : ");
            print_array((uint8_t *)&demoNwksKey, sizeof(demoNwksKey));
        }

    }
    else
    {
        status = LORAWAN_SetAttr (DEV_EUI, demoDevEui);
        if (LORAWAN_SUCCESS == status)
        {
            printf("\nDevEUI : ");
            print_array((uint8_t *)&demoDevEui, sizeof(demoDevEui));
            status = LORAWAN_SetAttr (APP_EUI, demoAppEui);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nAppEUI : ");
            print_array((uint8_t *)&demoAppEui, sizeof(demoAppEui));
            status = LORAWAN_SetAttr (APP_KEY, demoAppKey);
        }

        if (LORAWAN_SUCCESS == status)
        {
            printf("\nAppKey : ");
            print_array((uint8_t *)&demoAppKey, sizeof(demoAppKey));
        }
    }
    return status;
}

/*********************************************************************//*
 \brief      Function to Initialize the device type
 \param[in]  ed_class - notifies the device class (CLASS_A/CLASS_B/CLASS_C)
 \return     LORAWAN_SUCCESS, if successfully set the device class
             LORAWAN_INVALID_PARAMETER, otherwise
 ************************************************************************/
StackRetStatus_t set_device_type(EdClass_t ed_class)
{
    StackRetStatus_t status = LORAWAN_SUCCESS;

    status = LORAWAN_SetAttr(EDCLASS, &ed_class);

    if((LORAWAN_SUCCESS == status) && ((CLASS_C | CLASS_B) & ed_class) && (true == DEMO_APP_MCAST_ENABLE))
    {
        set_multicast_params();
    }

    return status;
}

/*********************************************************************//*
 \brief      Function to Initialize the Multicast parameters
 ************************************************************************/
void set_multicast_params (void)
{
    StackRetStatus_t status;

    printf("\n***************Multicast Parameters********************\n\r");

    status = LORAWAN_SetAttr(MCAST_APPS_KEY, &demoMcastAppsKey);
    if (status == LORAWAN_SUCCESS)
    {
        printf("\nMcastAppSessionKey : ");
        print_array((uint8_t *)&demoMcastAppsKey, sizeof(demoMcastAppsKey));
        status = LORAWAN_SetAttr(MCAST_NWKS_KEY, &demoMcastNwksKey);
    }

    if(status == LORAWAN_SUCCESS)
    {
        printf("\nMcastNwkSessionKey : ");
        print_array((uint8_t *)&demoMcastNwksKey, sizeof(demoMcastNwksKey));
        status = LORAWAN_SetAttr(MCAST_GROUP_ADDR, &demoMcastDevAddr);
    }

    if (status == LORAWAN_SUCCESS)
    {
        printf("\nMcastGroupAddr : 0x%lx\n\r", demoMcastDevAddr);
        status = LORAWAN_SetAttr(MCAST_ENABLE, &demoMcastEnable);
    }
    else
    {
        printf("\nMcastGroupAddrStatus : Failed\n\r");
    }

    if (status == LORAWAN_SUCCESS)
    {
        printf("\nMulticastStatus : Enabled\n\r");
    }
    else
    {
        printf("\nMulticastStatus : Failed\n\r");
    }
}


/***********************************************************************
 \brief      Function to Initialize set default parameters
 \param[in]  void
 \return     LORAWAN_SUCCESS, if successfully set all the parameters
             LORAWAN_INVALID_PARAMETER, otherwise
 ************************************************************************/
StackRetStatus_t mote_set_parameters(IsmBand_t ismBand, const uint16_t index)
{
    StackRetStatus_t status;

    LORAWAN_Reset(ismBand);
#if (NA_BAND == 1 || AU_BAND == 1)
    if ((ismBand == ISM_NA915) || (ismBand == ISM_AU915))
    {
        #define MAX_NA_CHANNELS 72
        #define MAX_SUBBAND_CHANNELS 8

        ChannelParameters_t ch_params;

        uint8_t allowed_min_125khz_ch,allowed_max_125khz_ch,allowed_500khz_channel;

        allowed_min_125khz_ch = (SUBBAND-1)*MAX_SUBBAND_CHANNELS;

        allowed_max_125khz_ch = ((SUBBAND-1)*MAX_SUBBAND_CHANNELS) + 7 ;

        allowed_500khz_channel = SUBBAND+63;

        for (ch_params.channelId = 0; ch_params.channelId < MAX_NA_CHANNELS; ch_params.channelId++)
        {
            if((ch_params.channelId >= allowed_min_125khz_ch) && (ch_params.channelId <= allowed_max_125khz_ch))
            {
                ch_params.channelAttr.status = true;
            }
            else if(ch_params.channelId == allowed_500khz_channel)
            {
                ch_params.channelAttr.status = true;
            }
            else
            {
                ch_params.channelAttr.status = false;
            }

            LORAWAN_SetAttr(CH_PARAM_STATUS, &ch_params);
        }
    }
#endif


    /* Initialize the join parameters for Demo application */
    status = set_join_parameters(DEMO_APP_ACTIVATION_TYPE);

    if (LORAWAN_SUCCESS != status)
    {
        printf("\nJoin parameters initialization failed\n\r");
        return status;
    }

    /* Set the device type */
    status = set_device_type(DEMO_APP_ENDDEVICE_CLASS);

    if (LORAWAN_SUCCESS != status)
    {
        printf("\nUnsupported Device Type\n\r");
        return status;
    }


    /* Send Join request for Demo application */
    status = LORAWAN_Join(DEMO_APP_ACTIVATION_TYPE);

    if (LORAWAN_SUCCESS == status && index < sizeof(bandTable))
    {
        printf("\nJoin Request Sent for %s\n\r",bandStrings[index]);
    }
    else
    {
        print_stack_status(status);
		appTaskState = JOIN_SEND_STATE;
		appPostTask(DISPLAY_TASK_HANDLER);
    }

    return status;
}

/*********************************************************************//*
 \brief      Function to Print array of characters
 \param[in]  *array  - Pointer of the array to be printed
 \param[in]   length - Length of the array
 ************************************************************************/
void print_array (uint8_t *array, uint8_t length)
{
    printf("0x");
    for (uint8_t i =0; i < length; i++)
    {
        printf("%02x", *array);
        array++;
    }
    printf("\n\r");
}

/*********************************************************************//*
 \brief      Function to Print application configuration
 ************************************************************************/
void  print_application_config (void)
{
    EdClass_t edClass;
    printf("\n***************Application Configuration***************\n\r");
    LORAWAN_GetAttr(EDCLASS, NULL, &edClass);
    printf("\nDevType : ");

    if(edClass == CLASS_A)
    {
        printf("CLASS A\n\r");
    }
    else if(edClass == CLASS_C)
    {
        printf("CLASS C\n\r");
    }

    printf("\nActivationType : ");

    if(DEMO_APP_ACTIVATION_TYPE == OVER_THE_AIR_ACTIVATION)
    {
        printf("OTAA\n\r");
    }
    else if(DEMO_APP_ACTIVATION_TYPE == ACTIVATION_BY_PERSONALIZATION)
    {
        printf("ABP\n\r");
    }

    printf("\nTransmission Type - ");

    if(DEMO_APP_TRANSMISSION_TYPE == CONFIRMED)
    {
        printf("CONFIRMED\n\r");
    }
    else if(DEMO_APP_TRANSMISSION_TYPE == UNCONFIRMED)
    {
        printf("UNCONFIRMED\n\r");
    }

    printf("\nFPort - %d\n\r", DEMO_APP_FPORT);

    printf("\n*******************************************************\n\r");
}

/*********************************************************************//*
 \brief      Function to Print stack return status
 \param[in]  status - Status from the stack
 ************************************************************************/
void print_stack_status(StackRetStatus_t status)
{
    switch(status)
    {
        case LORAWAN_SUCCESS:
             printf("\nlorawan_success\n\r");
        break;

        case LORAWAN_BUSY:
             printf("\nlorawan_state : stack_Busy\n\r");
        break;

        case LORAWAN_NWK_NOT_JOINED:
            printf("\ndevice_not_joined_to_network\n\r");
        break;

        case LORAWAN_INVALID_PARAMETER:
            printf("\ninvalid_parameter\n\r");
        break;

        case LORAWAN_KEYS_NOT_INITIALIZED:
            printf("\nkeys_not_initialized\n\r");
        break;

        case LORAWAN_SILENT_IMMEDIATELY_ACTIVE:
            printf("\nsilent_immediately_active\n\r");
        break;

        case LORAWAN_FCNTR_ERROR_REJOIN_NEEDED:
            printf("\nframecounter_error_rejoin_needed\n\r");
        break;

        case LORAWAN_INVALID_BUFFER_LENGTH:
            printf("\ninvalid_buffer_length\n\r");
        break;

        case LORAWAN_MAC_PAUSED:
            printf("\nMAC_paused\n\r");
        break;

        case LORAWAN_NO_CHANNELS_FOUND:
            printf("\nno_free_channels_found\n\r");
        break;

        case LORAWAN_INVALID_REQUEST:
            printf("\nrequest_invalid\n\r");
        break;
        case LORAWAN_NWK_JOIN_IN_PROGRESS:
            printf("\nprev_join_request_in_progress\n\r");
        break;
        default:
           printf("\nrequest_failed %d\n\r",status);
        break;
    }
}

/*********************************************************************//*
 \brief      Function to convert Celsius value to Fahrenheit
 \param[in]  cel_val   - Temperature value in Celsius
 \param[out] fauren_val- Temperature value in Fahrenheit
 ************************************************************************/
static float convert_celsius_to_fahrenheit(float celsius_val)
{
    float fauren_val;
    /* T(°F) = T(°C) × 9/5 + 32 */
    fauren_val = (((celsius_val * 9)/5) + 32);

    return fauren_val;

}

/*********************************************************************//*
 \brief      Reads the DEV EUI if it is flashed in EDBG MCU
 ************************************************************************/
static void dev_eui_read(void)
{
#if (EDBG_EUI_READ == 1)
	uint8_t invalidEDBGDevEui[8];
	uint8_t EDBGDevEUI[8];
	edbg_eui_read_eui64((uint8_t *)&EDBGDevEUI);
	memset(&invalidEDBGDevEui, 0xFF, sizeof(invalidEDBGDevEui));
	/* If EDBG doesnot have DEV EUI, the read value will be of all 0xFF, 
	   Set devEUI in conf_app.h in that case */
	if(0 != memcmp(&EDBGDevEUI, &invalidEDBGDevEui, sizeof(demoDevEui)))
	{
		/* Set EUI addr in EDBG if there */
		memcpy(demoDevEui, EDBGDevEUI, sizeof(demoDevEui));
	}
#endif
}


/* eof demo_app.c */
