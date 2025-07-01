/*
 * Copyright (c) 2018, Realsil Semiconductor Corporation. All rights reserved.
 */

#include "trace.h"
#include "gfps.h"
#include "ftl.h"
#include "app_gfps_personalized_name.h"

#if GFPS_PERSONALIZED_NAME_SUPPORT
typedef struct
{
    uint8_t is_valid;
    uint8_t stored_personalized_name_len;
    uint8_t reserved[2];
    uint8_t stored_personalized_name[GFPS_PERSONALIZED_NAME_MAX_LEN];
} T_PERSONALIZED_NAME;

T_PERSONALIZED_NAME personalized_name;

void app_gfps_personalized_name_set_default(void)
{
    personalized_name.is_valid = false;
    personalized_name.stored_personalized_name_len = 0;

    ftl_save_to_storage(&personalized_name, PERSONALIZED_NAME_FLASH_OFFSET,
                        sizeof(personalized_name));
}

void app_gfps_personalized_name_init(void)
{
    uint32_t read_result = ftl_load_from_storage(&personalized_name, PERSONALIZED_NAME_FLASH_OFFSET,
                                                 sizeof(personalized_name));

    if (read_result == ENOF || personalized_name.is_valid == false)
    {
//        APP_PRINT_INFO0("app_gfps_personalized_name_init: initialize");
        app_gfps_personalized_name_set_default();
    }
    else
    {
        APP_PRINT_TRACE3("app_gfps_personalized_name_init: personalized_name in ftl %b, is_valid %d, stored_personalized_name_len %d",
                         TRACE_BINARY(personalized_name.stored_personalized_name_len,
                                      personalized_name.stored_personalized_name),
                         personalized_name.is_valid, personalized_name.stored_personalized_name_len);
    }
}

/*personalzied name store */
T_APP_GFPS_PERSONALIZED_NAME_RESULT app_gfps_personalized_name_store(uint8_t
                                                                     *stored_personalized_name,
                                                                     uint8_t stored_personalized_name_len)
{
    T_APP_GFPS_PERSONALIZED_NAME_RESULT result = APP_GFPS_PERSONALIZED_NAME_SUCCESS;

    if (stored_personalized_name == NULL || stored_personalized_name_len == 0)
    {
        result = APP_GFPS_PERSONALIZED_NAME_POINTER_NULL;
        return result;
    }

    if (ftl_load_from_storage(&personalized_name, PERSONALIZED_NAME_FLASH_OFFSET,
                              sizeof(personalized_name)))
    {
        personalized_name.is_valid = false;
    }

    if (personalized_name.is_valid &&
        (stored_personalized_name_len == personalized_name.stored_personalized_name_len) &&
        memcmp(personalized_name.stored_personalized_name, stored_personalized_name,
               personalized_name.stored_personalized_name_len) == 0)
    {
        APP_PRINT_INFO0("app_gfps_personalized_name_store: this name has already stored in ftl");
    }
    else
    {
        if (stored_personalized_name_len > GFPS_PERSONALIZED_NAME_MAX_LEN)
        {
            personalized_name.stored_personalized_name_len = GFPS_PERSONALIZED_NAME_MAX_LEN;
        }
        else
        {
            personalized_name.stored_personalized_name_len = stored_personalized_name_len;
        }

        memcpy(personalized_name.stored_personalized_name, stored_personalized_name,
               personalized_name.stored_personalized_name_len);

        if (!personalized_name.is_valid)
        {
            personalized_name.is_valid = true;
        }

        if (ftl_save_to_storage(&personalized_name, PERSONALIZED_NAME_FLASH_OFFSET,
                                sizeof(personalized_name)))
        {
            result  = APP_GFPS_PERSONALIZED_NAME_SAVE_FAIL;
            return result;
        };
    }

    APP_PRINT_TRACE3("app_gfps_personalized_name_store: new personalized_name in ftl %b, stored_personalized_name_len %d, is_valid %d",
                     TRACE_BINARY(personalized_name.stored_personalized_name_len,
                                  personalized_name.stored_personalized_name), personalized_name.stored_personalized_name_len,
                     personalized_name.is_valid);
    return result;
}

T_APP_GFPS_PERSONALIZED_NAME_RESULT app_gfps_personalized_name_read(uint8_t
                                                                    *stored_personalized_name,
                                                                    uint8_t *stored_personalized_name_len)
{
    T_APP_GFPS_PERSONALIZED_NAME_RESULT result = APP_GFPS_PERSONALIZED_NAME_SUCCESS;

    if (stored_personalized_name == NULL || stored_personalized_name_len == NULL)
    {
        result = APP_GFPS_PERSONALIZED_NAME_POINTER_NULL;
        return result;
    }

    if (ftl_load_from_storage(&personalized_name, PERSONALIZED_NAME_FLASH_OFFSET,
                              sizeof(personalized_name)))
    {
        result = APP_GFPS_PERSONALIZED_NAME_LOAD_FAIL;
        return result;
    }

    if (personalized_name.is_valid && personalized_name.stored_personalized_name_len != 0)
    {
        *stored_personalized_name_len = personalized_name.stored_personalized_name_len;
        memcpy(stored_personalized_name, personalized_name.stored_personalized_name,
               *stored_personalized_name_len);

        APP_PRINT_TRACE3("app_gfps_personalized_name_read: personalized_name in ftl %b, stored_personalized_name_len %d, is_valid %d",
                         TRACE_BINARY(*stored_personalized_name_len, stored_personalized_name),
                         personalized_name.stored_personalized_name_len, personalized_name.is_valid);
    }
    else
    {
        result = APP_GFPS_PERSONALIZED_NAME_INVALID;
        return result;
    }

    return result;
}

void app_gfps_personalized_name_send(uint8_t conn_id, uint8_t service_id)
{
    T_APP_GFPS_PERSONALIZED_NAME_RESULT result;
    bool is_send = false;

    uint8_t  stored_personalized_name[GFPS_PERSONALIZED_NAME_MAX_LEN] = {0};
    uint8_t stored_personalized_name_len = 0;

    result = app_gfps_personalized_name_read(stored_personalized_name, &stored_personalized_name_len);

    if (result == APP_GFPS_PERSONALIZED_NAME_SUCCESS)
    {
        is_send = gfps_send_personalized_name(stored_personalized_name, stored_personalized_name_len,
                                              conn_id,
                                              service_id);
    }
    else
    {
        is_send = gfps_send_personalized_name(NULL, 0, conn_id,
                                              service_id);
    }

    APP_PRINT_INFO1(" app_gfps_personalized_name_send: is_send =  %d", is_send);
}

void app_gfps_personalized_name_clear(void)
{
    app_gfps_personalized_name_set_default();
}
#endif
