// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/macro_utils.h"
#include "azure_c_shared_utility/umock_c_prod.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/agenttime.h" 
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
#include "azure_c_shared_utility/sastoken.h"

#include "iothub_client_authorization.h"

#define DEFAULT_SAS_TOKEN_EXPIRY_TIME_SECS          3600
#define INDEFINITE_TIME                             ((time_t)(-1))

typedef struct IOTHUB_AUTHORIZATION_DATA_TAG
{
    char* device_sas_token;
    char* device_key;
    char* device_id;
    size_t token_expiry_time_sec;
    IOTHUB_CREDENTIAL_TYPE cred_type;
} IOTHUB_AUTHORIZATION_DATA;

static int get_seconds_since_epoch(size_t* seconds)
{
    int result;
    time_t current_time;
    if ((current_time = get_time(NULL)) == INDEFINITE_TIME)
    {
        LogError("Failed getting the current local time (get_time() failed)");
        result = __LINE__;
    }
    else
    {
        *seconds = (size_t)get_difftime(current_time, (time_t)0);
        result = 0;
    }
    return result;
}

IOTHUB_AUTHORIZATION_HANDLE IoTHubClient_Auth_Create(const char* device_key, const char* device_id, const char* device_sas_token)
{
    IOTHUB_AUTHORIZATION_DATA* result;
    /* Codes_SRS_IoTHub_Authorization_07_001: [if device_id is NULL IoTHubClient_Auth_Create, shall return NULL. ] */
    if (device_id == NULL)
    {
        LogError("Invalid Parameter device_id: %p", device_key, device_id);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IoTHub_Authorization_07_002: [IoTHubClient_Auth_Create shall allocate a IOTHUB_AUTHORIZATION_HANDLE that is needed for subsequent calls. ] */
        result = (IOTHUB_AUTHORIZATION_DATA*)malloc(sizeof(IOTHUB_AUTHORIZATION_DATA) );
        if (result == NULL)
        {
            /* Codes_SRS_IoTHub_Authorization_07_019: [ On error IoTHubClient_Auth_Create shall return NULL. ] */
            LogError("Failed allocating IOTHUB_AUTHORIZATION_DATA");
            result = NULL;
        }
        else
        {
            memset(result, 0, sizeof(IOTHUB_AUTHORIZATION_DATA) );
            result->token_expiry_time_sec = DEFAULT_SAS_TOKEN_EXPIRY_TIME_SECS;

            if (device_key != NULL && mallocAndStrcpy_s(&result->device_key, device_key) != 0)
            {
                /* Codes_SRS_IoTHub_Authorization_07_019: [ On error IoTHubClient_Auth_Create shall return NULL. ] */
                LogError("Failed allocating device_key");
                free(result);
                result = NULL;
            }
            else if (mallocAndStrcpy_s(&result->device_id, device_id) != 0)
            {
                /* Codes_SRS_IoTHub_Authorization_07_019: [ On error IoTHubClient_Auth_Create shall return NULL. ] */
                LogError("Failed allocating device_key");
                free(result->device_key);
                free(result);
                result = NULL;
            }
            else
            {
                if (device_key != NULL)
                {
                    /* Codes_SRS_IoTHub_Authorization_07_003: [ IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY if the device_sas_token is NULL. ]*/
                    result->cred_type = IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY;
                }
                else if (device_sas_token != NULL)
                {
                    /* Codes_SRS_IoTHub_Authorization_07_020: [ else IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN. ] */
                    result->cred_type = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
                    if (mallocAndStrcpy_s(&result->device_sas_token, device_sas_token) != 0)
                    {
                        /* Codes_SRS_IoTHub_Authorization_07_019: [ On error IoTHubClient_Auth_Create shall return NULL. ] */
                        LogError("Failed allocating device_key");
                        free(result->device_key);
                        free(result->device_id);
                        free(result);
                        result = NULL;
                    }
                }
                else
                {
                    /* Codes_SRS_IoTHub_Authorization_07_024: [ if device_sas_token and device_key are NULL IoTHubClient_Auth_Create shall set the credential type to IOTHUB_CREDENTIAL_TYPE_UNKNOWN. ] */
                    result->cred_type = IOTHUB_CREDENTIAL_TYPE_UNKNOWN;
                }
            }
        }
    }
    /* Codes_SRS_IoTHub_Authorization_07_004: [ If successful IoTHubClient_Auth_Create shall return a IOTHUB_AUTHORIZATION_HANDLE value. ] */
    return result;
}


void IoTHubClient_Auth_Destroy(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    /* Codes_SRS_IoTHub_Authorization_07_005: [ if handle is NULL IoTHubClient_Auth_Destroy shall do nothing. ] */
    if (handle != NULL)
    {
        /* Codes_SRS_IoTHub_Authorization_07_006: [ IoTHubClient_Auth_Destroy shall free all resources associated with the IOTHUB_AUTHORIZATION_HANDLE handle. ] */
        free(handle->device_key);
        free(handle->device_id);
        free(handle->device_sas_token);
        free(handle);
    }
}

IOTHUB_CREDENTIAL_TYPE IoTHubClient_Auth_Set_x509_Type(IOTHUB_AUTHORIZATION_HANDLE handle, bool enable_x509)
{
    IOTHUB_CREDENTIAL_TYPE result;
    if (handle != NULL)
    {
        if (enable_x509)
        {
            result = handle->cred_type = IOTHUB_CREDENTIAL_TYPE_X509;
        }
        else
        {
            if (handle->device_sas_token == NULL)
            {
                result = handle->cred_type = IOTHUB_CREDENTIAL_TYPE_DEVICE_KEY;
            }
            else if (handle->device_key == NULL)
            {
                result = handle->cred_type = IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN;
            }
            else
            {
                result = handle->cred_type = IOTHUB_CREDENTIAL_TYPE_UNKNOWN;
            }
        }
    }
    else
    {
        result = IOTHUB_CREDENTIAL_TYPE_UNKNOWN;
    }
    return result;
}

IOTHUB_CREDENTIAL_TYPE IoTHubClient_Auth_Get_Credential_Type(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    IOTHUB_CREDENTIAL_TYPE result;
    /* Codes_SRS_IoTHub_Authorization_07_007: [ if handle is NULL IoTHub_Auth_Get_Credential_Type shall return IOTHUB_CREDENTIAL_TYPE_UNKNOWN. ] */
    if (handle == NULL)
    {
        LogError("Invalid Parameter handle: %p", handle);
        result = IOTHUB_CREDENTIAL_TYPE_UNKNOWN;
    }
    else
    {
        /* Codes_SRS_IoTHub_Authorization_07_008: [ IoTHub_Auth_Get_Credential_Type shall return the credential type that is set upon creation. ] */
        result = handle->cred_type;
    }
    return result;
}

char* IoTHubClient_Auth_Get_SasToken(IOTHUB_AUTHORIZATION_HANDLE handle, const char* scope, size_t expire_time)
{
    char* result;
    /* Codes_SRS_IoTHub_Authorization_07_009: [ if handle or scope are NULL, IoTHubClient_Auth_Get_SasToken shall return NULL. ] */
    if (handle == NULL)
    {
        LogError("Invalid Parameter handle: %p", handle);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IoTHub_Authorization_07_021: [If the device_sas_token is NOT NULL IoTHubClient_Auth_Get_SasToken shall return a copy of the device_sas_token. ] */
        if (handle->device_sas_token != NULL)
        {
            if (mallocAndStrcpy_s(&result, handle->device_sas_token) != 0)
            {
                LogError("failure allocating sas token", scope);
                result = NULL;
            }
        }
        /* Codes_SRS_IoTHub_Authorization_07_009: [ if handle or scope are NULL, IoTHubClient_Auth_Get_SasToken shall return NULL. ] */
        else if (scope == NULL)
        {
            LogError("Invalid Parameter scope: %p", scope);
            result = NULL;
        }
        else
        {
            const char* key_name = "";
            STRING_HANDLE sas_token;
            size_t sec_since_epoch;

            /* Codes_SRS_IoTHub_Authorization_07_010: [ IoTHubClient_Auth_Get_ConnString shall construct the expiration time using the expire_time. ] */
            if (get_seconds_since_epoch(&sec_since_epoch) != 0)
            {
                /* Codes_SRS_IoTHub_Authorization_07_020: [ If any error is encountered IoTHubClient_Auth_Get_ConnString shall return NULL. ] */
                LogError("failure getting seconds from epoch");
                result = NULL;
            }
            else 
            {
                /* Codes_SRS_IoTHub_Authorization_07_011: [ IoTHubClient_Auth_Get_ConnString shall call SASToken_CreateString to construct the sas token. ] */
                size_t expiry_time = sec_since_epoch+expire_time;
                if ( (sas_token = SASToken_CreateString(handle->device_key, scope, key_name, expiry_time)) == NULL)
                {
                    /* Codes_SRS_IoTHub_Authorization_07_020: [ If any error is encountered IoTHubClient_Auth_Get_ConnString shall return NULL. ] */
                    LogError("Failed creating sas_token");
                    result = NULL;
                }
                else
                {
                    /* Codes_SRS_IoTHub_Authorization_07_012: [ On success IoTHubClient_Auth_Get_ConnString shall allocate and return the sas token in a char*. ] */
                    if (mallocAndStrcpy_s(&result, STRING_c_str(sas_token) ) != 0)
                    {
                        /* Codes_SRS_IoTHub_Authorization_07_020: [ If any error is encountered IoTHubClient_Auth_Get_ConnString shall return NULL. ] */
                        LogError("Failed copying result");
                        result = NULL;
                    }
                    STRING_delete(sas_token);
                }
            }
        }
    }
    return result;
}

const char* IoTHubClient_Auth_Get_DeviceId(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_IoTHub_Authorization_07_013: [ if handle is NULL, IoTHubClient_Auth_Get_DeviceId shall return NULL. ] */
        LogError("Invalid Parameter handle: %p", handle);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IoTHub_Authorization_07_014: [ IoTHubClient_Auth_Get_DeviceId shall return the device_id specified upon creation. ] */
        result = handle->device_id;
    }
    return result;
}

const char* IoTHubClient_Auth_Get_DeviceKey(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    const char* result;
    if (handle == NULL)
    {
        /* Codes_SRS_IoTHub_Authorization_07_022: [ if handle is NULL, IoTHubClient_Auth_Get_DeviceKey shall return NULL. ] */
        LogError("Invalid Parameter handle: %p", handle);
        result = NULL;
    }
    else
    {
        /* Codes_SRS_IoTHub_Authorization_07_023: [ IoTHubClient_Auth_Get_DeviceKey shall return the device_key specified upon creation. ] */
        result = handle->device_key;
    }
    return result;
}

SAS_TOKEN_STATUS IoTHubClient_Auth_Is_SasToken_Valid(IOTHUB_AUTHORIZATION_HANDLE handle)
{
    SAS_TOKEN_STATUS result;
    if (handle == NULL)
    {
        /* Codes_SRS_IoTHub_Authorization_07_015: [ if handle is NULL, IoTHubClient_Auth_Is_SasToken_Valid shall return false. ] */
        LogError("Invalid Parameter handle: %p", handle);
        result = SAS_TOKEN_STATUS_FAILED;
    }
    else
    {
        if (handle->cred_type == IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN)
        {
            if (handle->device_sas_token == NULL)
            {
                /* Codes_SRS_IoTHub_Authorization_07_017: [ If the sas_token is NULL IoTHubClient_Auth_Is_SasToken_Valid shall return false. ] */
                LogError("Failure: device_sas_toke is NULL");
                result = SAS_TOKEN_STATUS_FAILED;
            }
            else
            {
                /* Codes_SRS_IoTHub_Authorization_07_018: [ otherwise IoTHubClient_Auth_Is_SasToken_Valid shall return the value returned by SASToken_Validate. ] */
                STRING_HANDLE strSasToken = STRING_construct(handle->device_sas_token);
                if (strSasToken != NULL)
                {
                    if (!SASToken_Validate(strSasToken))
                    {
                        result = SAS_TOKEN_STATUS_INVALID;
                    }
                    else
                    {
                        result = SAS_TOKEN_STATUS_VALID;
                    }
                    STRING_delete(strSasToken);
                }
                else
                {
                    LogError("Failure constructing SAS Token");
                    result = SAS_TOKEN_STATUS_FAILED;
                }
            }
        }
        else
        {
            /* Codes_SRS_IoTHub_Authorization_07_016: [ if credential type is not IOTHUB_CREDENTIAL_TYPE_SAS_TOKEN IoTHubClient_Auth_Is_SasToken_Valid shall return true. ] */
            result = true;
        }
    }
    return result;
}