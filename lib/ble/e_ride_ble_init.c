#include <e_ride_config.h>
#include <e_ride_err.h>
#include <e_ride_ble.h>

#include <esp_log.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>
#include <esp_gatts_api.h>
#include <nvs_flash.h>

#include <string.h>
#include <stddef.h>


static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
//  .peer_addr          = {0},
//  .peer_addr_type     = 0,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = NULL,
};


/**
 * Global one and only BLE allocation.
 */
static e_ride_ble_t bleHandler = {0};


/**
 * 
 */
void e_ride_gatts_event_hndlr(

    esp_gatts_cb_event_t event,
    esp_gatt_if_t gatts_if,
    esp_ble_gatts_cb_param_t *param

);


/**
 * 
 */
void e_ride_gap_event_hndlr(

    esp_gap_ble_cb_event_t  event,
    esp_ble_gap_cb_param_t* param

);


/**
 *
 */
e_ride_ble_app_t* e_ride_ble_get_app_from_if(

    uint16_t gatts_if

)
{
    for (int i=0; i<bleHandler.appNum; i++)
        if (bleHandler.p_appList[i]->app_appHndl == gatts_if)
            return bleHandler.p_appList[i];

    return NULL;
}


e_ride_err_t e_ride_ble_init(

    e_ride_ble_config_t*    bleCnfg

)
{
     /**
     * NOTE: (b.covas) NVS Has to be initialized
     * for BLE to work. If NVS was already initialized,
     * nvs_flash_init() will still return OK.
     */
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    esp_bt_controller_config_t bt_Cnfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(    ret                                                         );
    ESP_ERROR_CHECK(    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)       );
    ESP_ERROR_CHECK(    esp_bt_controller_init(&bt_Cnfg)                            );
    ESP_ERROR_CHECK(    esp_bt_controller_enable(ESP_BT_MODE_BLE)                   );
 
    ESP_ERROR_CHECK(    esp_bluedroid_init()                                        );
    ESP_ERROR_CHECK(    esp_bluedroid_enable()                                      );

    ESP_ERROR_CHECK(    esp_ble_gap_register_callback(e_ride_gap_event_hndlr)       );
    ESP_ERROR_CHECK(    esp_ble_gap_set_device_name(E_RIDE_BLE_DEV_NAME)            );
    ESP_ERROR_CHECK(    esp_ble_gatts_register_callback(e_ride_gatts_event_hndlr)   );
    ESP_ERROR_CHECK(    esp_ble_gap_config_adv_data(&adv_data)                      );

    return E_RIDE_SUCCESS;
}


e_ride_err_t e_ride_ble_close()
{
    if (!bleHandler.p_appList)
        return E_RIDE_SUCCESS;

    for (uint16_t i=0; i<bleHandler.appNum; i++)
        esp_ble_gatts_app_unregister(i);

    free(bleHandler.p_appList);

    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    return E_RIDE_SUCCESS;
}


e_ride_err_t e_ride_ble_register_apps(

    uint16_t           numApps,
    e_ride_ble_app_t** p_appList

)
{
    static size_t currAppId = 0;

    if (!numApps || !p_appList)
        return E_RIDE_ERR_INVALID_PARAM;

    if (currAppId > 0)
        /**
         * This function was already called.
         * We don't want that. It can only
         * be called once.
         */
        return E_RIDE_BLE_INIT_REINIT;

    /**
     * Allocate app list.
     */
    bleHandler.p_appList = calloc(numApps, sizeof(void*));
    if (!bleHandler.p_appList)
        return E_RIDE_ERR_OOM;

    bleHandler.appNum = numApps;

    /**
     * Copy all the app's addresses to the bleHandler.
     * They are assumed to last for the time ble is
     * running.
     * */
    memcpy((void*)bleHandler.p_appList, (void*)p_appList, sizeof(e_ride_ble_app_t*) * numApps);

    /**
     * Register the apps with ble.
     */
    for (size_t i = 0; i<numApps; i++)
        ESP_ERROR_CHECK(esp_ble_gatts_app_register(currAppId++));

    return E_RIDE_SUCCESS;
}


void e_ride_gatts_event_hndlr(

    esp_gatts_cb_event_t event,
    esp_gatt_if_t gatts_if,
    esp_ble_gatts_cb_param_t* param

)
{
    uint16_t appId;
    e_ride_ble_app_t*  bleApp;
    e_ride_ble_notif_t bleNotif = {
        .event = event,
        .param = param
    };

    switch (event)
    {
        /**
         * Register an app.
         */
        case ESP_GATTS_REG_EVT:
        {
            appId = param->reg.app_id;

            /**
             * The app id is the index.
             * Check if some some reason
             * the index is unexpected.
             */
            if (appId > bleHandler.appNum)
            {
                ESP_LOGE(__func__, "App id was %d, but there are %d apps.", appId, bleHandler.appNum);
                return;
            }

            /**
             * The handler is the interface associated
             * with the app.
             */
            bleApp = bleHandler.p_appList[appId];
            bleApp->app_appHndl = gatts_if;

            /*               | Service Handler         | Char, char value and char descriptor handle.   */
            /*               V                         V One of each per BLE char.                      */
            int numHandles = 1 + bleApp->app_numChar * 3;
            esp_ble_gatts_create_service(
                gatts_if, bleApp->app_serviceId_p, numHandles);

        }
        break;

        /**
         * Service was created. Start it, and then
         * create it's BLE chars.
         */
        case ESP_GATTS_CREATE_EVT:
        {
            esp_ble_gatts_start_service(param->create.service_handle);

            bleApp = e_ride_ble_get_app_from_if(gatts_if);

            if (!bleApp)
            {
                ESP_LOGE(__func__, "No app for gatts interface %d", gatts_if);
                return;
            }

            /**
             * Add every BLE char.
             */
            e_ride_ble_char_t* bleChar_p;
            for (int i=0; i<bleApp->app_numChar; i++)
            {
                bleChar_p = bleApp->app_charList_p[i];

                ESP_LOGD(__func__, "Adding char %d from app '%s'", i, bleApp->app_serviceName);

                ESP_ERROR_CHECK(esp_ble_gatts_add_char(
                    param->create.service_handle,
                    &bleChar_p->char_uuid,
                    bleChar_p->char_perm,
                    bleChar_p->char_prop,
                    &bleChar_p->char_val,
                    &bleChar_p->char_ctrl));
            }

        }
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
            break;

        default:
            break;
    }

    /**
     * Find which app this event is meant to,
     * and jump to it's event function.
     */
    bleApp = e_ride_ble_get_app_from_if(gatts_if);
    if (bleApp && bleApp->app_evtFunc)
        bleApp->app_evtFunc(&bleNotif);
}


void e_ride_gap_event_hndlr(

    esp_gap_ble_cb_event_t  event,
    esp_ble_gap_cb_param_t* param

)
{
    switch (event)
    {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_ERROR_CHECK(esp_ble_gap_start_advertising(&adv_params));
            break;
        default:
            break;
    }
}
