//
// Created by vgol on 17/08/2022.
//

#ifndef ANT_TEST_ANT_RECEIVE_DEFINES_H
#define ANT_TEST_ANT_RECEIVE_DEFINES_H

typedef struct {
	UCHAR ucAntChannel;
	UCHAR ucTransType;
	UCHAR ucDeviceType;
	USHORT usDeviceNum;
	USHORT usMessagePeriod;
} sANTrxServiceInit;

#define CHANNEL_TYPE_MASTER       (0)
#define CHANNEL_TYPE_SLAVE        (1)
#define CHANNEL_TYPE_INVALID      (2)

//#define ENABLE_EXTENDED_MESSAGES
#define USER_BAUDRATE         (57600)  // For AT3/AP2, use 57600

// Page decoding constants
#define TOGGLE_MASK           ((UCHAR)0x80) // For removing the toggle bit for HRM decoding
#define INVALID_TOGGLE_BIT    ((BOOL)0xFF) // Invalid BOOL, can be initialised because BOOL is just int
#define PAGE_0                ((UCHAR)0x00)
#define PAGE_1                ((UCHAR)0x01)
#define PAGE_2                ((UCHAR)0x02)
#define PAGE_3                ((UCHAR)0x03)
#define PAGE_4                ((UCHAR)0x04)
#define MAX_TOGGLE_ATTEMPTS   ((UCHAR)0x06) // 1 extra to provide some margin
#define LEGACY_DEVICE         ((UCHAR)0x10) // Arbitrarily chosen definition
#define CURRENT_DEVICE        ((UCHAR)0x11) // Arbitrarily chosen definition
#define INVALID_DEVICE        ((UCHAR)0xFF) // Arbitrarily chosen definition

// Channel configuration specs for HRM Receiever (including network config defaults)
#define USER_ANTCHANNEL       (0) // Arbitrarily chosen as default
#define USER_NETWORK_KEY      {0xB9, 0xA5, 0x21, 0xFB, 0xBD, 0x72, 0xC3, 0x45} // ANT+ network key
#define USER_RADIOFREQ        (57) // ANT+ spec

#define MESSAGE_TIMEOUT       (12) // = 12*2.5 = 30 seconds

// Indexes into message recieved from ANT
#define MESSAGE_BUFFER_DATA1_INDEX ((UCHAR) 0)
#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)
#define MESSAGE_BUFFER_DATA3_INDEX ((UCHAR) 2)
#define MESSAGE_BUFFER_DATA4_INDEX ((UCHAR) 3)
#define MESSAGE_BUFFER_DATA5_INDEX ((UCHAR) 4)
#define MESSAGE_BUFFER_DATA6_INDEX ((UCHAR) 5)
#define MESSAGE_BUFFER_DATA7_INDEX ((UCHAR) 6)
#define MESSAGE_BUFFER_DATA8_INDEX ((UCHAR) 7)
#define MESSAGE_BUFFER_DATA9_INDEX ((UCHAR) 8)
#define MESSAGE_BUFFER_DATA10_INDEX ((UCHAR) 9)
#define MESSAGE_BUFFER_DATA11_INDEX ((UCHAR) 10)
#define MESSAGE_BUFFER_DATA12_INDEX ((UCHAR) 11)
#define MESSAGE_BUFFER_DATA13_INDEX ((UCHAR) 12)
#define MESSAGE_BUFFER_DATA14_INDEX ((UCHAR) 13)

#endif //ANT_TEST_ANT_RECEIVE_DEFINES_H
