//
// Created by vince on 26/12/2022.
//

#ifndef ANT_TEST_UE5_LIB_H
#define ANT_TEST_UE5_LIB_H

#include <functional>

#if defined(__cplusplus)
extern "C" {
#endif // defined C++

/*
 * Library functions in the order they should be used
 */

int ue5_lib__startupAntPlusLib(void);

int ue5_lib__addDeviceID(unsigned short usDeviceNum,
                unsigned char ucDeviceType,
                unsigned short usMessagePeriod,
                std::function<void(unsigned char *p_aucData)> callback);

int ue5_lib__sendBytes(unsigned char ucChannel, unsigned char *p_aucData);

int ue5_lib__startANT(void);

int ue5_lib__endAntPlusLib(void);

#if defined(__cplusplus)
}
#endif // defined C++

#endif //ANT_TEST_UE5_LIB_H