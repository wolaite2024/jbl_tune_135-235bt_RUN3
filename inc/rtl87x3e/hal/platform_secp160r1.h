/**
 * \file _PLATFORM_SECP160R1_H_
 *
 * \brief micro_ecc Platform abstraction layer
 *
 */
#ifndef _PLATFORM_SECP160R1_H_
#define _PLATFORM_SECP160R1_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    SECP160R1 = 0,
    SECP256R1,
    SECP256K1,/*to do*/
    UECC_ALGORITHE_NUM
} T_UECC_ALGORITHM_TYPE;

int platform_uecc_compute_public_key(uint8_t *aes_key, uint8_t *public_key, uint8_t *r,
                                     T_UECC_ALGORITHM_TYPE uecc_type);


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PLATFORM_SECP160R1_H_ */
