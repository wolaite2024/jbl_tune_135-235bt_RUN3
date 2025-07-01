/*******************************************************************************
 * RTK Error codes
 *
 * Copyright 2017 RealTek.com, Inc. or its affiliates. All Rights Reserved.
 *******************************************************************************
 */

/**
 * @file
 *
 * @brief This file contains the error codes used by the Amazon Firmware Platform modules.
 */

#ifndef RTK_ERROR_H
#define RTK_ERROR_H

#include <stdint.h>

/**
 * @name RTK Error codes
 * @anchor RTK_error_codes
 */
/**@{*/

/** @brief Success */
#define RTK_SUCCESS             0

/** @brief File is already open */
#define RTK_EBUSY               1

/** @brief Too many open files */
#define RTK_EMFILE              2

/** @brief Read only file system */
#define RTK_EROFS               3

/** @brief No space left on device */
#define RTK_ENOSPC              4

/** @brief No such file or directory */
#define RTK_ENOENT              5

/** @brief Bad file descriptor */
#define RTK_EBADF               6

/** @brief File name too long */
#define RTK_ENAMETOOLONG        7

/** @brief Math argument out of domain of func to Invalid argument(10)**/
#define RTK_EDOM                8

/** @brief Value too large for defined data type to Invalid argument(10)**/
#define RTK_EOVERFLOW           9

/** @brief Invalid argument */
#define RTK_EINVAL              10

/** @brief I/O Error */
#define RTK_EIO                 11

/** @brief Bad address */
#define RTK_EFAULT              12

/** @brief Not Implemented */
#define RTK_ENOTIMPL            13

/** @brief index out of range to Out of memory(16)**/
#define RTK_EIDRNG              14

/** @brief Message too long to Out of memory(16)**/
#define RTK_EMSGSIZE            15

/** @brief Out of memory */
#define RTK_ENOMEM              16

/** @brief Permission denied */
#define RTK_EPERM               17

/** @brief No such device */
#define RTK_ENODEV              18

/** @brief Timed Out */
#define RTK_ETIMEOUT            19

/** @brief no ISR support */
#define RTK_ENOISR              20

/** @brief No message of desired type to Internal Error(28)**/
#define RTK_ENOMSG              21

/** @brief Invalid request code to Internal Error(28)**/
#define RTK_EBADRQC             22

/** @brief Device not a stream to Internal Error(28)**/
#define RTK_ENOSTR              23

/** @brief No data available to Internal Error(28)**/
#define RTK_ENODATA             24

/** @brief Protocol not supported  to Internal Error(28)**/
#define RTK_EPROTONOSUPPORT     25

/** @brief Protocol not available  to Internal Error(28)**/
#define RTK_ENOPROTOOPT         26

/** @brief No buffer space available to Internal Error(28)**/
#define RTK_ENOBUFS             27

/** @brief Internal Error */
#define RTK_EINTRL              28

/** @brief Math result not representable to Unknown Failure(255)**/
#define RTK_ERANGE              29

/** @brief Host is down to Unknown Failure(255)**/
#define RTK_EHOSTDOWN           30

/** @brief Slave is down to Unknown Failure(255)**/
#define RTK_ESLAVEDOWN          31

/** @brief Unknown Failure to Unknown Failure(255)*/
#define RTK_EUFAIL              255

/**@}*/

/**
 * @brief Return amzn error code for rtk error code.
 */
int32_t rtk_err_rtk2amzn(int32_t rtx_error);

/**
 * @brief Return error string given negative error code.
 */
const char *rtk_strerror(int32_t err);

#endif /* RTK_ERROR_H */
