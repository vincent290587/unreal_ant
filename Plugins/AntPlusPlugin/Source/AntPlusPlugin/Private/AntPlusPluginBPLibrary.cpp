#include "AntPlusPluginBPLibrary.h"
#include "AntPlusPlugin.h"
#include "ue5_lib.h"

static unsigned short power1 = 0u;

/**
 * Implemented for ANT+ FEC device to get the power produced by the user
 *
 * @param p_aucData
 */
static void _pw1_callback(unsigned char *p_aucData) {

    unsigned char ucPageNum = p_aucData[0];

    // printf("Receiving FEC page %u \n", ucPageNum);

    // Page specific data
    switch(ucPageNum) // Removing the toggle bit
    {
        case 25:
        {
            power1 = p_aucData[5] + ((p_aucData[6] & 0b00001111) << 8u);
            // printf("FEC power %u received\n", power1);
        } break;

        default:

            break;

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define PW_PERIOD_COUNTS 8182u
#define PW_DEVICE_TYPE   0x0Bu

#define FEC_DEVICE_TYPE            0x11u               ///< Device type reserved for ANT+ Bicycle Power.
#define FEC_ANTPLUS_RF_FREQ        0x39u               ///< Frequency, decimal 57 (2457 MHz).
#define FEC_MSG_PERIOD             8192u               ///< Message period, decimal 8182 (4 Hz).

UAntPlusPluginBPLibrary::UAntPlusPluginBPLibrary(const FObjectInitializer& ObjectInitializer)
        : Super(ObjectInitializer)
{

}

int UAntPlusPluginBPLibrary::connectPowerMeterDeviceID(int devID)
{
    ue5_lib__addDeviceID(devID,
                                FEC_DEVICE_TYPE,
                                FEC_MSG_PERIOD,
                                _pw1_callback);


    return ue5_lib__startANT();
}

int UAntPlusPluginBPLibrary::getRawPower(void)
{
    return power1;
}

#define UCHAR                                      unsigned char
#define ANT_STANDARD_DATA_PAYLOAD_SIZE             ((UCHAR)8)

// Indexes into message recieved from ANT
#define MESSAGE_BUFFER_DATA1_INDEX ((UCHAR) 0)
#define MESSAGE_BUFFER_DATA2_INDEX ((UCHAR) 1)
#define MESSAGE_BUFFER_DATA3_INDEX ((UCHAR) 2)
#define MESSAGE_BUFFER_DATA4_INDEX ((UCHAR) 3)
#define MESSAGE_BUFFER_DATA5_INDEX ((UCHAR) 4)
#define MESSAGE_BUFFER_DATA6_INDEX ((UCHAR) 5)
#define MESSAGE_BUFFER_DATA7_INDEX ((UCHAR) 6)
#define MESSAGE_BUFFER_DATA8_INDEX ((UCHAR) 7)

void UAntPlusPluginBPLibrary::sendBytes(int page, int byte1, int byte2, int byte3, int byte4, int byte5, int byte6, int byte7)
{
    unsigned char aucData[8];

    aucData[0] = (unsigned char)page;
    aucData[1] = (unsigned char)byte1;
    aucData[2] = (unsigned char)byte2;
    aucData[3] = (unsigned char)byte3;
    aucData[4] = (unsigned char)byte4;
    aucData[5] = (unsigned char)byte5;
    aucData[6] = (unsigned char)byte6;
    aucData[7] = (unsigned char)byte7;

    ue5_lib__sendBytes(0, aucData);
}

#define ANT_FEC_PAGE49_TARGET_POWER_LSB     (0.25f)

#define ANT_FEC_PAGE51_SLOPE_LSB       (1.f/100.f)
#define ANT_FEC_PAGE51_ROLL_RES_LSB    (5.f/100000.f)

void UAntPlusPluginBPLibrary::setFECPage49(float targetPower)
{
    UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE];

    unsigned short usPower = (unsigned short)(targetPower / ANT_FEC_PAGE49_TARGET_POWER_LSB);

    aucTransmitBuffer[MESSAGE_BUFFER_DATA1_INDEX] = 49u; // page
    aucTransmitBuffer[MESSAGE_BUFFER_DATA7_INDEX] = usPower & 0xFF; // power part 2
    aucTransmitBuffer[MESSAGE_BUFFER_DATA8_INDEX] = (usPower & 0xFF00) >> 8; // power part 1

    ue5_lib__sendBytes(0 /* channel 0 forced in current version */, aucTransmitBuffer);
}

void UAntPlusPluginBPLibrary::setFECPage51(float targetSlope, float targetResistance)
{
    UCHAR aucTransmitBuffer[ANT_STANDARD_DATA_PAYLOAD_SIZE];

    float slope = 5.f; // 5%
    unsigned short usSlope = (unsigned short)(targetSlope / ANT_FEC_PAGE51_SLOPE_LSB);
    float roll_res = 0.005f; // 0.005
    unsigned char usroll_res = (unsigned char)(targetResistance / ANT_FEC_PAGE51_ROLL_RES_LSB);

    aucTransmitBuffer[MESSAGE_BUFFER_DATA1_INDEX] = 51u; // page
    aucTransmitBuffer[MESSAGE_BUFFER_DATA6_INDEX] = usSlope & 0xFF; // slope part 1 (LSB is 0.5 W)
    aucTransmitBuffer[MESSAGE_BUFFER_DATA7_INDEX] = (usSlope & 0xFF00) >> 8; // slope part 2
    aucTransmitBuffer[MESSAGE_BUFFER_DATA8_INDEX] = usroll_res; // rolling res

    ue5_lib__sendBytes(0 /* channel 0 forced in current version */, aucTransmitBuffer);

}

