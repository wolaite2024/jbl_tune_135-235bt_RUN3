/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file    aes_api.h
  * @brief   aes apis abstracted for encryption related.
  * @details AES encryption APIs which delivers HW/SW implemented reliable and safe AES solution.
  * @author  Abel
  * @date    2017.5.17
  * @version v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   * *************************************************************************************
  */


/*============================================================================*
 *                      Define to prevent recursive inclusion
 *============================================================================*/
#ifndef _AES_API_H_
#define _AES_API_H_

/*============================================================================*
 *                               Header Files
*============================================================================*/
#include <stdint.h>
#include <stdbool.h>

#define AES256 1
#define ECB    1
#define CBC    1
/** @defgroup 87x3e_AES_API AES API Sets
  * @brief API sets for aes encryption implementation
  * @{
  */

/*============================================================================*
  *                                Functions
  *============================================================================*/
/** @defgroup AES_API_Exported_Functions AES API Sets Exported Functions
    * @{
    */
/**
    * @brief  byte swap
    * @param  src          source address
    * @param  dst          destination address
    * @param  len          swap size
    */

void swap_buf(const uint8_t *src, uint8_t *dst, uint16_t len);

/**
    * @brief  128 bit AES ECB encryption on speicified plaintext and keys
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes128_ecb_encrypt(uint8_t plaintext[16], const uint8_t key[16], uint8_t *encrypted);
/**
    * @brief  128 bit AES ECB encryption on speicified plaintext and keys
    * @note   most significant octet of encrypted data corresponds to encypted[0]
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes128_ecb_encrypt_msb2lsb(uint8_t plaintext[16], const uint8_t key[16],
                                uint8_t *encrypted);

/**
    * @brief  128 bit AES ECB decryption on speicified encrypted data and keys
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @return decryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes128_ecb_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output);
/**
    * @brief  128 bit AES ECB decryption on speicified encrypted data and keys
    * @note   most significant octet of encrypted data corresponds to encypted[0]
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @return decryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes128_ecb_decrypt_msb2lsb(uint8_t *input, const uint8_t *key, uint8_t *output);

/**
    * @brief  256 bit AES ECB encryption on speicified plaintext and keys
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    * @attention please swap the first and last 16 bytes of the key before calling the api
    * @code
    * sample code:
    * void aes256_key_swap(uint8_t key[32])
    * {
    *     uint32_t tmp[4];
    *
    *     memcpy(tmp, key, 16);
    *     memcpy(key, key + 16, 16);
    *     memcpy(key + 16, tmp, 16);
    * }
    *
    * uint8_t key_swap_tmp[32];
    * memcpy(key_swap_tmp,key,32);
    * aes256_key_swap(key_swap_tmp);
    * aes256_ecb_encrypt(plaintext,key_swap_tmp,encrypted);
    *
    * @endcode
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes256_ecb_encrypt(uint8_t plaintext[16], const uint8_t key[32], uint8_t *encrypted);
/**
    * @brief  256 bit AES ECB encryption on speicified plaintext and keys
    * @note   most significant octet of encrypted data corresponds to encypted[0]
    * @attention please swap the first and last 16 bytes of the key before calling the api
    * @code
    * sample code:
    * void aes256_key_swap(uint8_t key[32])
    * {
    *     uint32_t tmp[4];
    *
    *     memcpy(tmp, key, 16);
    *     memcpy(key, key + 16, 16);
    *     memcpy(key + 16, tmp, 16);
    * }
    *
    * uint8_t key_swap_tmp[32];
    * memcpy(key_swap_tmp,key,32);
    * aes256_key_swap(key_swap_tmp);
    * aes256_ecb_encrypt_msb2lsb(plaintext,key_swap_tmp,encrypted);
    *
    * @endcode
    * @param  plaintext    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  encrypted    output buffer to store encrypted data
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes256_ecb_encrypt_msb2lsb(uint8_t plaintext[16], const uint8_t key[32],
                                uint8_t *encrypted);
/**
    * @brief  256 bit AES ECB decryption on speicified encrypted data and keys
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    * @attention please swap the first and last 16 bytes of the key before calling the api
    * @code
    * sample code:
    * void aes256_key_swap(uint8_t key[32])
    * {
    *     uint32_t tmp[4];
    *
    *     memcpy(tmp, key, 16);
    *     memcpy(key, key + 16, 16);
    *     memcpy(key + 16, tmp, 16);
    * }
    *
    * uint8_t key_swap_tmp[32];
    * memcpy(key_swap_tmp,key,32);
    * aes256_key_swap(key_swap_tmp);
    * aes256_ecb_decrypt(input,key_swap_tmp,output);
    *
    * @endcode
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @return decryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes256_ecb_decrypt(uint8_t *input, const uint8_t *key, uint8_t *output);
/**
    * @brief  256 bit AES ECB decryption on speicified encrypted data and keys
    * @note   most significant octet of encrypted data corresponds to encypted[0]
    * @attention please swap the first and last 16 bytes of the key before calling the api
    * @code
    * sample code:
    * void aes256_key_swap(uint8_t key[32])
    * {
    *     uint32_t tmp[4];
    *
    *     memcpy(tmp, key, 16);
    *     memcpy(key, key + 16, 16);
    *     memcpy(key + 16, tmp, 16);
    * }
    *
    * uint8_t key_swap_tmp[32];
    * memcpy(key_swap_tmp,key,32);
    * aes256_key_swap(key_swap_tmp);
    * aes256_ecb_decrypt_msb2lsb(input,key_swap_tmp,output);
    *
    * @endcode
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @return decryption results
    * @retval true      successful
    * @retval false     fail
    */
bool aes256_ecb_decrypt_msb2lsb(uint8_t *input, const uint8_t *key, uint8_t *output);

/** @} */ /* End of group AES_API_Exported_Functions */

/** @} */ /* End of group 87x3e_AES_API */


#endif
