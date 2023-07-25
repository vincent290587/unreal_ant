//
// Created by vgol on 17/08/2022.
//

#include "ant_receive_ch.h"

ANTrxService::ANTrxService() {
	ucChannelType = CHANNEL_TYPE_SLAVE; // always a slave
	pclSerialObject = (DSISerialGeneric*)NULL;
	pclMessageObject = (DSIFramerANT*)NULL;
	uiDSIThread = (DSI_THREAD_ID)NULL;
	bMyDone = FALSE;
	bDone = FALSE;
	bDisplay = TRUE;
	bProcessedData = TRUE;
	bBroadcasting = FALSE;
}

ANTrxService::~ANTrxService()
{
    printf("Closing the Receiver!\n");

	for (auto slave : mDevices)
	{
        if(pclMessageObject)
		    pclMessageObject->CloseChannel(slave.ucAntChannel, MESSAGE_TIMEOUT);
	}

    if(pclSerialObject)
	    pclSerialObject->Close();

	if(pclMessageObject)
		delete pclMessageObject;

	if(pclSerialObject)
		delete pclSerialObject;

    pclSerialObject = (DSISerialGeneric*)NULL;
    pclMessageObject = (DSIFramerANT*)NULL;
    uiDSIThread = (DSI_THREAD_ID)NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Init
//
// Initize the ANTrxService and ANT Library.
//
// ucDeviceNumber_: USB Device Number (0 for first USB stick plugged and so on)
//                  If not specified on command line, 0xFF is passed in as invalid.
////////////////////////////////////////////////////////////////////////////////
BOOL ANTrxService::Init()
{

	BOOL bStatus;
	UCHAR ucDeviceNumber_ = 0;

	// Initialize condition var and mutex
	UCHAR ucCondInit = DSIThread_CondInit(&condTestDone);
	assert(ucCondInit == DSI_THREAD_ENONE);

	UCHAR ucMutexInit = DSIThread_MutexInit(&mutexTestDone);
	assert(ucMutexInit == DSI_THREAD_ENONE);

#if defined(DEBUG_FILE)
	// Enable logging
   DSIDebug::Init();
   DSIDebug::SetDebug(TRUE);
#endif

	// Create Serial object.
	pclSerialObject = new DSISerialGeneric();
	assert(pclSerialObject);

	// Create Framer object.
	pclMessageObject = new DSIFramerANT(pclSerialObject);
	assert(pclMessageObject);

	// Initialize Serial object.
	// The device number depends on how many USB sticks have been
	// plugged into the PC. The first USB stick plugged will be 0
	// the next 1 and so on.
	//
	// The Baud Rate depends on the ANT solution being used. AP1
	// is 50000, all others are 57600
	bStatus = pclSerialObject->Init(USER_BAUDRATE, 0);
	assert(bStatus);

	// Initialize Framer object.
	bStatus = pclMessageObject->Init();
	assert(bStatus);

	// Let Serial know about Framer.
	pclSerialObject->SetCallback(pclMessageObject);

	// Open Serial.
	bStatus = pclSerialObject->Open();

	// If the Open function failed, most likely the device
	// we are trying to access does not exist, or it is connected
	// to another program
	if(!bStatus)
	{
		printf("Failed to connect to device at USB port %d\n", 0);
		return FALSE;
	}

	// NOTE: Will fail if the module is not available.
	// If no device number was specified on the command line,
	// prompt the user for input.

//	if(ucDeviceNumber_ == 0xFF)
//	{
//		printf("USB Device number?\n"); fflush(stdout);
//		char st[1024];
//		fgets(st, sizeof(st), stdin);
//		sscanf(st, "%u", &ucDeviceNumber_);
//	}
	printf("USB Number: %d\n", ucDeviceNumber_);

	// Create message thread.
	uiDSIThread = DSIThread_CreateThread(&ANTrxService::RunMessageThread, this);
	assert(uiDSIThread);

	return TRUE;
}


void ANTrxService::PrintUsbDescr() {

	// Print out information about the device we are connected to
	printf("USB Device Description\n");
	USHORT usDevicePID;
	USHORT usDeviceVID;
	UCHAR aucDeviceDescription[USB_MAX_STRLEN];
	UCHAR aucDeviceSerial[USB_MAX_STRLEN];
	// Retrieve info
	if(pclMessageObject->GetDeviceUSBVID(usDeviceVID))
	{
		printf("  VID: 0x%X\n", usDeviceVID);
	}
	if(pclMessageObject->GetDeviceUSBPID(usDevicePID))
	{
		printf("  PID: 0x%X\n", usDevicePID);
	}
	if(pclMessageObject->GetDeviceUSBInfo(pclSerialObject->GetDeviceNumber(), aucDeviceDescription, aucDeviceSerial, USB_MAX_STRLEN))
	{
		printf("  Product Description: %s\n", aucDeviceDescription);
		printf("  Serial String: %s\n", aucDeviceSerial);
	}
}


////////////////////////////////////////////////////////////////////////////////
// Close
//
// Close connection to USB stick.
//
////////////////////////////////////////////////////////////////////////////////
void ANTrxService::Close()
{
	//Wait for test to be done
	DSIThread_MutexLock(&mutexTestDone);
	bDone = TRUE;

	UCHAR ucWaitResult = DSIThread_CondTimedWait(&condTestDone, &mutexTestDone, 2000);
	assert(ucWaitResult == DSI_THREAD_ENONE);

	DSIThread_MutexUnlock(&mutexTestDone);

	//Destroy mutex and condition var
	DSIThread_MutexDestroy(&mutexTestDone);
	DSIThread_CondDestroy(&condTestDone);

    if (uiDSIThread) {
        (void)DSIThread_DestroyThread(uiDSIThread);
    }

#if defined(DEBUG_FILE)
	DSIDebug::Close();
#endif

}

////////////////////////////////////////////////////////////////////////////////
// RunMessageThread
//
// Callback function that is used to create the thread. This is a static
// function.
//
////////////////////////////////////////////////////////////////////////////////
DSI_THREAD_RETURN ANTrxService::RunMessageThread(void *pvParameter_)
{
	((ANTrxService*) pvParameter_)->MessageThread();
	return NULL;
}

BOOL ANTrxService::TransmitMessage(UCHAR channel, UCHAR tx_buffer[ANT_STANDARD_DATA_PAYLOAD_SIZE])
{
    if (bTXwaiting) {
        return FALSE;
    }
    txChannel = channel;
    memcpy(aucTransmitBuffer, tx_buffer, ANT_STANDARD_DATA_PAYLOAD_SIZE);
    bTXwaiting = true;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// MessageThread
//
// Run message thread
////////////////////////////////////////////////////////////////////////////////
void ANTrxService::MessageThread()
{
	ANT_MESSAGE stMessage;
	USHORT usSize;
	bDone = FALSE;

	while(!bDone)
	{
		if(pclMessageObject->WaitForMessage(1000))
		{
			usSize = pclMessageObject->GetMessage(&stMessage);

			if(bDone)
				break;

			if(usSize == DSI_FRAMER_ERROR)
			{
				// Get the message to clear the error
				usSize = pclMessageObject->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE);
				continue;
			}

			if(usSize != DSI_FRAMER_ERROR && usSize != DSI_FRAMER_TIMEDOUT && usSize != 0)
			{
				//printf("ProcessMessage %u \n", stMessage.aucData[0]);
				ProcessMessage(stMessage, usSize);
			}

            if (bTXwaiting &&
                pclMessageObject->SendBroadcastData(txChannel, aucTransmitBuffer)) { // SendAcknowledgedData
                bTXwaiting = false;
                //printf("SendBroadcastData %u \n", aucTransmitBuffer[0]);
            }
		}
	}

	DSIThread_MutexLock(&mutexTestDone);
	UCHAR ucCondResult = DSIThread_CondSignal(&condTestDone);
	assert(ucCondResult == DSI_THREAD_ENONE);
	DSIThread_MutexUnlock(&mutexTestDone);

}

void ANTrxService::Start() {

	BOOL bStatus = TRUE;

	printf("Resetting module...\n");
	bStatus = pclMessageObject->ResetSystem();
	DSIThread_Sleep(1000);

//	// Request capabilites.
//	ANT_MESSAGE_ITEM stResponse;
//	pclMessageObject->SendRequest(MESG_CAPABILITIES_ID, 0, &stResponse, 0);

	// Start the test by setting network key
	printf("Setting network key...\n");
	UCHAR ucNetKey[8] = USER_NETWORK_KEY;

	bStatus &= pclMessageObject->SetNetworkKey(0, ucNetKey, MESSAGE_TIMEOUT);

	for (auto slave : mDevices) {
		slave.Start(pclMessageObject);
	}
}

////////////////////////////////////////////////////////////////////////////////
// ProcessMessage
//
// Process ALL messages that come from ANT, including event messages.
//
// stMessage: Message struct containing message recieved from ANT
// usSize_:
////////////////////////////////////////////////////////////////////////////////
void ANTrxService::ProcessMessage(ANT_MESSAGE stMessage, USHORT usSize_)
{
	BOOL bStatus;
	BOOL bPrintBuffer = FALSE;
	UCHAR ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages

	// For decoding device type -- legacy or current
	static UCHAR ucDeviceType_ = INVALID_DEVICE;

	switch(stMessage.ucMessageID)
	{
		//RESPONSE MESG
		case MESG_RESPONSE_EVENT_ID:
		{
			//RESPONSE TYPE
			switch(stMessage.aucData[1])
			{
				case MESG_NETWORK_KEY_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error configuring network key: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Network key set.\n");
					printf("Assigning channel...\n");
					//bStatus = pclMessageObject->AssignChannel(ucAntChannel, PARAMETER_RX_NOT_TX, 0, MESSAGE_TIMEOUT);
					break;
				}

				case MESG_ASSIGN_CHANNEL_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error assigning channel: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Channel assigned\n");
					printf("Setting Channel ID...\n");
					//bStatus = pclMessageObject->SetChannelID(ucAntChannel, usDeviceNum, ucDeviceType, ucTransType, MESSAGE_TIMEOUT);
					break;
				}


				case MESG_CHANNEL_ID_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error configuring Channel ID: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Channel ID set\n");
					printf("Setting Radio Frequency %u...\n", USER_RADIOFREQ);
					//bStatus = pclMessageObject->SetChannelRFFrequency(ucAntChannel, USER_RADIOFREQ, MESSAGE_TIMEOUT);
					break;
				}

				case MESG_CHANNEL_RADIO_FREQ_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error configuring Radio Frequency: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Radio Frequency set\n");
					printf("Setting Message Period...\n");
					//bStatus = pclMessageObject->SetChannelPeriod(ucAntChannel, usMessagePeriod, MESSAGE_TIMEOUT);
					break;
				}

				case MESG_CHANNEL_MESG_PERIOD_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error assigning Message Period: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Message period assigned\n");
					printf("Opening channel...\n");
					bBroadcasting = TRUE;
					//bStatus = pclMessageObject->OpenChannel(ucAntChannel, MESSAGE_TIMEOUT);
					break;
				}

				case MESG_OPEN_CHANNEL_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error opening channel: Code 0%d\n", stMessage.aucData[2]);
						bBroadcasting = FALSE;
						break;
					}
					printf("Channel opened\n");
#if defined (ENABLE_EXTENDED_MESSAGES)
					printf("Enabling extended messages...\n");
					pclMessageObject->RxExtMesgsEnable(TRUE);
#endif
					break;
				}

				case MESG_RX_EXT_MESGS_ENABLE_ID:
				{
					if(stMessage.aucData[2] == INVALID_MESSAGE)
					{
						printf("Extended messages not supported in this ANT product\n");
						break;
					}
					else if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error enabling extended messages: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Extended messages enabled\n");
					break;
				}

				case MESG_UNASSIGN_CHANNEL_ID:
				{
					if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error unassigning channel: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					printf("Channel unassigned\n");
					printf("Press enter to exit\n");
					bMyDone = TRUE;
					break;
				}

				case MESG_CLOSE_CHANNEL_ID:
				{
					if(stMessage.aucData[2] == CHANNEL_IN_WRONG_STATE)
					{
						// We get here if we tried to close the channel after the search timeout (slave)
						printf("Channel is already closed\n");
						//printf("Unassigning channel...\n");
						//bStatus = pclMessageObject->UnAssignChannel(ucAntChannel, MESSAGE_TIMEOUT);
						break;
					}
					else if(stMessage.aucData[2] != RESPONSE_NO_ERROR)
					{
						printf("Error closing channel: Code 0%d\n", stMessage.aucData[2]);
						break;
					}
					// If this message was successful, wait for EVENT_CHANNEL_CLOSED to confirm channel is closed
					break;
				}

				case MESG_REQUEST_ID:
				{
					if(stMessage.aucData[2] == INVALID_MESSAGE)
					{
						printf("Requested message not supported in this ANT product\n");
					}
					break;
				}

				case MESG_EVENT_ID:
				{
					switch(stMessage.aucData[2])
					{
						case EVENT_CHANNEL_CLOSED:
						{
							printf("Channel %u Closed\n", stMessage.aucData[0]);
							//printf("Unassigning channel...\n");
							//bStatus = pclMessageObject->UnAssignChannel(ucAntChannel, MESSAGE_TIMEOUT);
							printf("Re-opening channel %u ...\n", stMessage.aucData[0]);
							bStatus = pclMessageObject->OpenChannel(stMessage.aucData[0], MESSAGE_TIMEOUT);
							break;
						}

						case EVENT_RX_SEARCH_TIMEOUT:
						{
							printf("Search Timeout\n");
							break;
						}
						case EVENT_RX_FAIL:
						{
							printf("Rx Fail\n");
							break;
						}

						case EVENT_RX_FAIL_GO_TO_SEARCH:
						{
							printf("Go to Search\n");
							break;
						}
						case EVENT_CHANNEL_COLLISION:
						{
							printf("Channel Collision\n");
							break;
						}

						default:
						{
							printf("Unhandled channel event: 0x%X\n", stMessage.aucData[2]);
							break;
						}

					}

					break;
				}

				default:
				{
					printf("Unhandled response 0%d to message 0x%X\n", stMessage.aucData[2], stMessage.aucData[1]);
					break;
				}
			}
			break;
		}

		case MESG_STARTUP_MESG_ID:
		{
			printf("RESET Complete, reason: ");

			UCHAR ucReason = stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX];

			if(ucReason == RESET_POR)
				printf("RESET_POR");
			if(ucReason & RESET_SUSPEND)
				printf("RESET_SUSPEND ");
			if(ucReason & RESET_SYNC)
				printf("RESET_SYNC ");
			if(ucReason & RESET_CMD)
				printf("RESET_CMD ");
			if(ucReason & RESET_WDT)
				printf("RESET_WDT ");
			if(ucReason & RESET_RST)
				printf("RESET_RST ");
			printf("\n");

			break;
		}

		case MESG_CAPABILITIES_ID:
		{
			printf("CAPABILITIES:\n");
			printf("   Max ANT Channels: %d\n",stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
			printf("   Max ANT Networks: %d\n",stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX]);

			UCHAR ucStandardOptions = stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX];
			UCHAR ucAdvanced = stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
			UCHAR ucAdvanced2 = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];

			printf("Standard Options:\n");
			if( ucStandardOptions & CAPABILITIES_NO_RX_CHANNELS )
				printf("CAPABILITIES_NO_RX_CHANNELS\n");
			if( ucStandardOptions & CAPABILITIES_NO_TX_CHANNELS )
				printf("CAPABILITIES_NO_TX_CHANNELS\n");
			if( ucStandardOptions & CAPABILITIES_NO_RX_MESSAGES )
				printf("CAPABILITIES_NO_RX_MESSAGES\n");
			if( ucStandardOptions & CAPABILITIES_NO_TX_MESSAGES )
				printf("CAPABILITIES_NO_TX_MESSAGES\n");
			if( ucStandardOptions & CAPABILITIES_NO_ACKD_MESSAGES )
				printf("CAPABILITIES_NO_ACKD_MESSAGES\n");
			if( ucStandardOptions & CAPABILITIES_NO_BURST_TRANSFER )
				printf("CAPABILITIES_NO_BURST_TRANSFER\n");

			printf("Advanced Options:\n");
			if( ucAdvanced & CAPABILITIES_OVERUN_UNDERRUN )
				printf("CAPABILITIES_OVERUN_UNDERRUN\n");
			if( ucAdvanced & CAPABILITIES_NETWORK_ENABLED )
				printf("CAPABILITIES_NETWORK_ENABLED\n");
			if( ucAdvanced & CAPABILITIES_AP1_VERSION_2 )
				printf("CAPABILITIES_AP1_VERSION_2\n");
			if( ucAdvanced & CAPABILITIES_SERIAL_NUMBER_ENABLED )
				printf("CAPABILITIES_SERIAL_NUMBER_ENABLED\n");
			if( ucAdvanced & CAPABILITIES_PER_CHANNEL_TX_POWER_ENABLED )
				printf("CAPABILITIES_PER_CHANNEL_TX_POWER_ENABLED\n");
			if( ucAdvanced & CAPABILITIES_LOW_PRIORITY_SEARCH_ENABLED )
				printf("CAPABILITIES_LOW_PRIORITY_SEARCH_ENABLED\n");
			if( ucAdvanced & CAPABILITIES_SCRIPT_ENABLED )
				printf("CAPABILITIES_SCRIPT_ENABLED\n");
			if( ucAdvanced & CAPABILITIES_SEARCH_LIST_ENABLED )
				printf("CAPABILITIES_SEARCH_LIST_ENABLED\n");

			if(usSize_ > 4)
			{
				printf("Advanced 2 Options 1:\n");
				if( ucAdvanced2 & CAPABILITIES_LED_ENABLED )
					printf("CAPABILITIES_LED_ENABLED\n");
				if( ucAdvanced2 & CAPABILITIES_EXT_MESSAGE_ENABLED )
					printf("CAPABILITIES_EXT_MESSAGE_ENABLED\n");
				if( ucAdvanced2 & CAPABILITIES_SCAN_MODE_ENABLED )
					printf("CAPABILITIES_SCAN_MODE_ENABLED\n");
				if( ucAdvanced2 & CAPABILITIES_RESERVED )
					printf("CAPABILITIES_RESERVED\n");
				if( ucAdvanced2 & CAPABILITIES_PROX_SEARCH_ENABLED )
					printf("CAPABILITIES_PROX_SEARCH_ENABLED\n");
				if( ucAdvanced2 & CAPABILITIES_EXT_ASSIGN_ENABLED )
					printf("CAPABILITIES_EXT_ASSIGN_ENABLED\n");
				if( ucAdvanced2 & CAPABILITIES_FS_ANTFS_ENABLED)
					printf("CAPABILITIES_FREE_1\n");
				if( ucAdvanced2 & CAPABILITIES_FIT1_ENABLED )
					printf("CAPABILITIES_FIT1_ENABLED\n");
			}
			break;
		}
		case MESG_CHANNEL_STATUS_ID:
		{
			printf("Got Status\n");

			char astrStatus[][32] = {  "STATUS_UNASSIGNED_CHANNEL",
			                           "STATUS_ASSIGNED_CHANNEL",
			                           "STATUS_SEARCHING_CHANNEL",
			                           "STATUS_TRACKING_CHANNEL"   };

			UCHAR ucStatusByte = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] & STATUS_CHANNEL_STATE_MASK; // MUST MASK OFF THE RESERVED BITS
			printf("STATUS: %s\n",astrStatus[ucStatusByte]);
			break;
		}
		case MESG_CHANNEL_ID_ID:
		{
			// Channel ID of the device that we just recieved a message from.
			USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX] << 8);
			UCHAR ucDeviceType__ =  stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
			UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];

			printf("CHANNEL ID: (%d/%d/%d)\n", usDeviceNumber, ucDeviceType__, ucTransmissionType);
			break;
		}
		case MESG_VERSION_ID:
		{
			printf("VERSION: %s\n", (char*) &stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
			break;
		}

		case MESG_ACKNOWLEDGED_DATA_ID:
		case MESG_BROADCAST_DATA_ID:
			// Burst not supported by ANT+ HRM
		{
			if (ucDeviceType_ == INVALID_DEVICE)
			{
				// Detecting current vs. legacy transmitter, only if not detected yet
				static BOOL bOldToggleBit = INVALID_TOGGLE_BIT;
				static UCHAR ucToggleAttempts = 0; // The number of attempts made so far
				BOOL bToggleBit = stMessage.aucData[ucDataOffset + 0] >> 7;
				DetectDevice(ucDeviceType_, bOldToggleBit, ucToggleAttempts, bToggleBit);
			}

			if ((bProcessedData) && (ucDeviceType_ == INVALID_DEVICE))
			{
				// The user wants to see processed data, but device type is not detected yet
				break;
			}

			// The flagged and unflagged data messages have the same
			// message ID. Therefore, we need to check the size to
			// verify if a flag is present at the end of a message.
			// To enable flagged messages, must call ANT_RxExtMesgsEnable first.
			if(usSize_ > MESG_DATA_SIZE)
			{
				UCHAR ucFlag = stMessage.aucData[MESSAGE_BUFFER_DATA10_INDEX];

				if(bDisplay && ucFlag & ANT_EXT_MESG_BITFIELD_DEVICE_ID)
				{
					// Channel ID of the device that we just recieved a message from.
					USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA11_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA12_INDEX] << 8);
					UCHAR ucDeviceType__ =  stMessage.aucData[MESSAGE_BUFFER_DATA13_INDEX];
					UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA14_INDEX];

					printf("Chan ID(%d/%d/%d) - ", usDeviceNumber, ucDeviceType__, ucTransmissionType);
				}
			}

			// Display recieved message
			bPrintBuffer = TRUE;
			ucDataOffset = MESSAGE_BUFFER_DATA2_INDEX;   // For most data messages

			for (auto slave : mDevices) {
				if (stMessage.aucData[0] == slave.ucAntChannel) {
					slave.msg_callback(&stMessage.aucData[ucDataOffset]);
				}
			}

//			if(bDisplay)
//			{
//				if(stMessage.ucMessageID == MESG_ACKNOWLEDGED_DATA_ID )
//					printf("Acked Rx:(%d): ", stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
//
//				else if (stMessage.ucMessageID == MESG_BROADCAST_DATA_ID)
//					printf("Rx:(%d): ", stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
//
//				// Burst is not supported by ANT+ HRM
//			}
			break;

		}

		case MESG_EXT_BROADCAST_DATA_ID:
		case MESG_EXT_ACKNOWLEDGED_DATA_ID:
		{
			if (ucDeviceType_ == INVALID_DEVICE)
			{
				// Detecting current vs. legacy transmitter, only if not detected yet
				static BOOL bOldToggleBit = INVALID_TOGGLE_BIT;
				static UCHAR ucToggleAttempts = 0; // The number of attempts made so far
				BOOL bToggleBit = stMessage.aucData[ucDataOffset + 0] >> 7;
				DetectDevice(ucDeviceType_, bOldToggleBit, ucToggleAttempts, bToggleBit);
			}

			if ((bProcessedData) && (ucDeviceType_ == INVALID_DEVICE))
			{
				// The user wants to see processed data, but device type is not detected yet
				break;
			}

			// The "extended" part of this message is the 4-byte channel
			// id of the device that we recieved this message from. This message
			// is only available on the AT3. The AP2 uses flagged versions of the
			// data messages as shown above.

			// Channel ID of the device that we just recieved a message from.
			USHORT usDeviceNumber = stMessage.aucData[MESSAGE_BUFFER_DATA2_INDEX] | (stMessage.aucData[MESSAGE_BUFFER_DATA3_INDEX] << 8);
			UCHAR ucDeviceType__ =  stMessage.aucData[MESSAGE_BUFFER_DATA4_INDEX];
			UCHAR ucTransmissionType = stMessage.aucData[MESSAGE_BUFFER_DATA5_INDEX];

			bPrintBuffer = TRUE;
			ucDataOffset = MESSAGE_BUFFER_DATA6_INDEX;   // For most data messages

			for (auto slave : mDevices) {
				if (stMessage.aucData[0] == slave.ucAntChannel) {
					slave.msg_callback(&stMessage.aucData[ucDataOffset]);
				}
			}

//			if(bDisplay)
//			{
//				// Display the channel id
//				printf("Chan ID(%d/%d/%d) ", usDeviceNumber, ucDeviceType__, ucTransmissionType );
//
//				if(stMessage.ucMessageID == MESG_EXT_ACKNOWLEDGED_DATA_ID)
//					printf("- Acked Rx:(%d): ", stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
//
//				else if(stMessage.ucMessageID == MESG_EXT_BROADCAST_DATA_ID)
//					printf("- Rx:(%d): ", stMessage.aucData[MESSAGE_BUFFER_DATA1_INDEX]);
//
//				// Burst not supported by ANT+ HRM
//			}

			break;
		}

		default:
		{
			break;
		}
	}

#if 0
	// If we received a data message, display its contents here.
	if(bPrintBuffer)
	{
		if(bDisplay)
		{
			// If the user wants to see the decoded information
			if (bProcessedData)
			{
				// HR data, common to all data pages and device types

				// Merge the 2 bytes to form the HRM event time
				USHORT usEventTime = ((USHORT)stMessage.aucData[ucDataOffset + 5] << 8) +
				                     (USHORT)stMessage.aucData[ucDataOffset + 4];
				UCHAR ucHR = stMessage.aucData[ucDataOffset + 7];
				UCHAR ucBeatCount = stMessage.aucData[ucDataOffset + 6];

				printf("HR: %d , Beat Count: %d , Beat Event Time: %d\n", ucHR, ucBeatCount, usEventTime);

				if (ucDeviceType == CURRENT_DEVICE)
				{
					// Current device => Page numbers are enabled
					UCHAR ucPageNum = stMessage.aucData[ucDataOffset + 0] & ~TOGGLE_MASK;

					// Page specific data
					switch(ucPageNum) // Removing the toggle bit
					{

						case PAGE_0:
						{
							// Contains only common data
							break;
						}

						case PAGE_1:
						{
							// Decoding cumulative operating time
							ULONG ulOperatingTime_ = ((ULONG)stMessage.aucData[ucDataOffset + 3] << 16) +
							                        ((ULONG)stMessage.aucData[ucDataOffset + 2] << 8) +
							                        ((ULONG)stMessage.aucData[ucDataOffset + 1]);
							ulOperatingTime_ = 2*ulOperatingTime_;
							printf("Transmitter operating Time: %lu seconds\n", ulOperatingTime_);
							break;
						}

						case PAGE_2:
						{
							// Decoding Manufacturer ID & serial number
							UCHAR ucManufacturerID = stMessage.aucData[ucDataOffset + 1];
							USHORT usSerialNum = ((USHORT)stMessage.aucData[ucDataOffset + 3] << 8) +
							                     (USHORT)stMessage.aucData[ucDataOffset + 2];
							printf("Transmitter manufacturer ID : %d , ", ucManufacturerID);
							printf("Ser. #: %d\n", usSerialNum);
							break;
						}

						case PAGE_3:
						{
							// Decoding Hardware, software version and model number
							UCHAR ucHWVer = stMessage.aucData[ucDataOffset + 1];
							UCHAR ucSWVer = stMessage.aucData[ucDataOffset + 2];
							UCHAR ucModelNum = stMessage.aucData[ucDataOffset + 3];
							printf("Transmitter HW ver: %d , ", ucHWVer);
							printf("SF ver: %d , ", ucSWVer);
							printf("Model #: %d\n", ucModelNum);
							break;
						}

						case PAGE_4:
						{
							// Decoding R-R Interval Measurements
							static UCHAR ucPreviousBeatCount;
							USHORT usPreviousEventTime = ((USHORT)stMessage.aucData[ucDataOffset + 3] << 8) +
							                             (USHORT)stMessage.aucData[ucDataOffset + 2];
							if (ucBeatCount - ucPreviousBeatCount == 1)// Ensure that there is only one beat between time intervals
							{
								USHORT usR_R_Interval = usEventTime - usPreviousEventTime; // Subracting the event times gives the R-R interval
								// Converting the timebase from 1/1024 of a second to milliseconds
								USHORT usR_R_Interval_ms = (((ULONG) usR_R_Interval) * (ULONG) 1000) / (ULONG) 1024;
								printf("R-R Interval: %d ms\n", usR_R_Interval_ms);
							}
							ucPreviousBeatCount = ucBeatCount;
							break;
						}
						default:
						{
							// This should never be the case for current ANT+ HRM transmitter
							// More pages may be implemented in future
							printf("Unable to recognise the page number. Please check for an updated version of this app.\n");
						}

					}

				}

			}

				// If the user wants to see the raw bytes received
			else
			{
				printf("[%02x],[%02x],[%02x],[%02x],[%02x],[%02x],[%02x],[%02x]\n",
				       stMessage.aucData[ucDataOffset + 0],
				       stMessage.aucData[ucDataOffset + 1],
				       stMessage.aucData[ucDataOffset + 2],
				       stMessage.aucData[ucDataOffset + 3],
				       stMessage.aucData[ucDataOffset + 4],
				       stMessage.aucData[ucDataOffset + 5],
				       stMessage.aucData[ucDataOffset + 6],
				       stMessage.aucData[ucDataOffset + 7]);
			}
		}
		else
		{
			static int iIndex = 0;
			static char ac[] = {'|','/','-','\\'};
			printf("Rx: %c\r",ac[iIndex++]); fflush(stdout);
			iIndex &= 3;

		}
	}
#endif

	return;
}

////////////////////////////////////////////////////////////////////////////////
// DetectDevice
//
// Helper member for detecting the device (type) -- legacy or current.
//
////////////////////////////////////////////////////////////////////////////////
void ANTrxService::DetectDevice(UCHAR &ucDeviceType_, BOOL &bOldToggleBit_, UCHAR &ucToggleAttempts_, BOOL bToggleBit)
{
	if (ucToggleAttempts_ == 0)
	{
		printf("Detecting transmitter device type...\n");
	}

	if (ucToggleAttempts_ < MAX_TOGGLE_ATTEMPTS)
	{
		// Try to see if the ms bit of the ms byte toggles this time
		// Check MAX_TOGGLE_ATTEMPTS number of times
		if ((bToggleBit != bOldToggleBit_) && (bOldToggleBit_ != INVALID_TOGGLE_BIT))
		{
			ucDeviceType_ = CURRENT_DEVICE; // Toggle Bit toggled => Current device
			printf("Current Device detected\n");
		}
		else
		{
			bOldToggleBit_ = bToggleBit;
			ucToggleAttempts_++;
		}
	}

	if ((ucToggleAttempts_ >= MAX_TOGGLE_ATTEMPTS) && (ucDeviceType_ == INVALID_DEVICE))
	{
		// Toggle Bit didn't toggle for max # of attempts => Legacy device
		ucDeviceType_ = LEGACY_DEVICE;
		printf("Legacy Device detected\n");
	}
}

