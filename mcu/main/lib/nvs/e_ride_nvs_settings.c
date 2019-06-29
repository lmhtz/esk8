#include <esk8_nvs.h>

#include <nvs.h>
#include <nvs_flash.h>


static nvs_handle_t esk8_nvs_handle = 0;


esk8_err_t esk8_nvs_init()
{
    esp_err_t errCode = nvs_flash_init();

    if (errCode == ESP_ERR_NVS_NO_FREE_PAGES || errCode == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        errCode = nvs_flash_erase();
        if (errCode)
            return ESK8_NVS_NOT_AVAILABLE;

        errCode = nvs_flash_init();
        if (errCode)
            return ESK8_NVS_NOT_AVAILABLE;
    }

    errCode = nvs_open(ESK8_NVS_STORAGE_NAME, NVS_READWRITE, &esk8_nvs_handle);
    if (errCode)
        return ESK8_NVS_NOT_AVAILABLE;

    return ESK8_SUCCESS;
}


esk8_err_t esk8_nvs_settings_get(

    esk8_nvs_settings_t* out_settings

)
{
    if (esk8_nvs_handle == 0)
        return ESK8_NVS_NOT_INIT;

    esp_err_t errCode;
    size_t sttgs_size   = sizeof(esk8_nvs_settings_t);
    size_t sttgs_size_r = 0;

    nvs_get_blob(esk8_nvs_handle,
        ESK8_NVS_STORAGE_KEY_SETTINGS, NULL, &sttgs_size_r);

    if (sttgs_size != sttgs_size_r)
        return ESK8_NVS_WRONG_SIZE;

    errCode = nvs_get_blob(esk8_nvs_handle,
        ESK8_NVS_STORAGE_KEY_SETTINGS, (void*)out_settings, &sttgs_size_r);

    if (errCode)
        return ESK8_NVS_NOT_AVAILABLE;

    return ESK8_SUCCESS;
}


esk8_err_t esk8_nvs_settings_set(

    esk8_nvs_settings_t* settings

)
{
    if (esk8_nvs_handle == 0)
        return ESK8_NVS_NOT_INIT;

    esp_err_t errCode;
    size_t sttgs_size = sizeof(esk8_nvs_settings_t);

    errCode = nvs_set_blob(esk8_nvs_handle,
        ESK8_NVS_STORAGE_KEY_SETTINGS, (void*)settings, sttgs_size);

    if (!errCode)
        return ESK8_SUCCESS;

    switch (errCode)
    {
        case ESP_ERR_NVS_INVALID_HANDLE:
            return ESK8_NVS_NOT_INIT;
        case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
            return ESK8_NVS_FULL;
        default:
            return ESK8_NVS_NOT_AVAILABLE;
    }
}