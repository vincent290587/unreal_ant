//
// Created by vgol on 17/08/2022.
//

#ifndef ANT_TEST_ANT_RECEIVE_CH_H
#define ANT_TEST_ANT_RECEIVE_CH_H

#include "types.h"
#include "ant_receive_defines.h"
#include "dsi_framer_ant.hpp"
//#include "dsi_thread.h"
#include "dsi_serial_generic.hpp"
//#include "dsi_debug.hpp"
#include <stdio.h>
//#include <stdlib.h>
#include <assert.h>
//#include <string.h>

class ANTdevice {
	friend class ANTrxService;

public:
	ANTdevice(sANTrxServiceInit sInit,
	          std::function<void(UCHAR *p_aucData)> callback) {
		ucTransType = sInit.ucTransType;
		ucAntChannel = sInit.ucAntChannel;
		ucDeviceType = sInit.ucDeviceType;
		usDeviceNum = sInit.usDeviceNum;
		usMessagePeriod = sInit.usMessagePeriod;

		msg_callback = callback;
	}

private:

	// Network variables
	UCHAR ucTransType;
	UCHAR ucAntChannel;
	UCHAR ucDeviceType;
	USHORT usDeviceNum;
	USHORT usMessagePeriod;

	std::function<void(UCHAR *p_aucData)> msg_callback;

	void Start(DSIFramerANT* pclMessageObject) {
		BOOL bStatus;

		bStatus = pclMessageObject->AssignChannel(ucAntChannel, PARAMETER_RX_NOT_TX, 0, MESSAGE_TIMEOUT);
		DSIThread_Sleep(100);

		bStatus = pclMessageObject->SetChannelID(ucAntChannel, usDeviceNum, ucDeviceType, ucTransType, MESSAGE_TIMEOUT);
		DSIThread_Sleep(100);

		bStatus = pclMessageObject->SetChannelRFFrequency(ucAntChannel, USER_RADIOFREQ, MESSAGE_TIMEOUT);
		DSIThread_Sleep(100);

		bStatus = pclMessageObject->SetChannelPeriod(ucAntChannel, usMessagePeriod, MESSAGE_TIMEOUT);
		DSIThread_Sleep(100);

		bStatus = pclMessageObject->OpenChannel(ucAntChannel, MESSAGE_TIMEOUT);
		DSIThread_Sleep(100);

		(void)bStatus;
	}
};

class ANTrxService {
public:
	ANTrxService();
	virtual ~ANTrxService();
	BOOL Init();

	void AddSlave(sANTrxServiceInit sInit, std::function<void(UCHAR *p_aucData)> callback) {
		mDevices.push_back(ANTdevice(sInit, callback));
	}

    BOOL TransmitMessage(UCHAR channel, UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE]);

	void Start();
	void Close();

	void PrintUsbDescr();

private:

	// Starts the Message thread.
	static DSI_THREAD_RETURN RunMessageThread(void *pvParameter_);

	// Listens for a response from the module
	void MessageThread();

	// Decodes the received message
	void ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_);

	// Detect transmitter device type: current or legacy
	void DetectDevice(UCHAR &ucDeviceType_, BOOL &bOldToggleBit_, UCHAR &ucToggleAttempts_, BOOL bToggleBit);

	std::vector<ANTdevice> mDevices;

//	// Network variables
//	UCHAR ucTransType;
//	UCHAR ucAntChannel;
//	UCHAR ucDeviceType;
//	USHORT usDeviceNum;
//	USHORT usMessagePeriod;

    BOOL bTXwaiting;
    UCHAR txChannel;
    UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE];

	BOOL bBursting; // Holds whether the bursting phase of the test has started
	BOOL bBroadcasting;
	BOOL bMyDone;
	BOOL bDone;
	UCHAR ucChannelType;
	DSISerialGeneric* pclSerialObject;
	DSIFramerANT* pclMessageObject;
	DSI_THREAD_ID uiDSIThread;
	DSI_CONDITION_VAR condTestDone;
	DSI_MUTEX mutexTestDone;

	BOOL bDisplay;
	BOOL bProcessedData;
};

#endif //ANT_TEST_ANT_RECEIVE_CH_H
