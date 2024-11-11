

#include "gatt_cache_manager.h"

#include <string.h>
#include "ble_gap.h"
#include "ble_conn_state.h"
#include "peer_manager_types.h"
#include "peer_database.h"
#include "id_manager.h"
#include "security_dispatcher.h"
#include "gatts_cache_manager.h"
#include "gattc_cache_manager.h"


/**@brief Macro for verifying that the module is initialized. It will cause the function to return
 *        @ref NRF_ERROR_INVALID_STATE if not.
 */
#define VERIFY_MODULE_INITIALIZED()     \
do                                      \
{                                       \
    if (m_gcm.evt_handler == NULL)      \
    {                                   \
        return NRF_ERROR_INVALID_STATE; \
    }                                   \
} while(0)


/**@brief Macro for verifying that the module is initialized. It will cause the function to return
 *        if not.
 *
 * @param[in] param  The variable to check if is NULL.
 */
#define VERIFY_PARAM_NOT_NULL(param)    \
do                                      \
{                                       \
    if (param == NULL)                  \
    {                                   \
        return NRF_ERROR_NULL;          \
    }                                   \
} while(0)



/**@brief Structure containing the module variable(s) of the GCM module.
 */
typedef struct
{
    gcm_evt_handler_t             evt_handler;                     /**< The event handler to use for outbound GSCM events. */
    ble_conn_state_user_flag_id_t flag_id_local_db_update_pending; /**< Flag ID for flag collection to keep track of which connections need a local DB update procedure. */
    ble_conn_state_user_flag_id_t flag_id_local_db_apply_pending;  /**< Flag ID for flag collection to keep track of which connections need a local DB apply procedure. */
    ble_conn_state_user_flag_id_t flag_id_service_changed_pending; /**< Flag ID for flag collection to keep track of which connections need to be sent a service changed indication. */
} gcm_t;

static gcm_t m_gcm;  /**< Instantiation of module variable(s). */


/**@brief Function for resetting the module variable(s) of the GSCM module.
 *
 * @param[out]  The instance to reset.
 */
static void internal_state_reset(gcm_t * p_gcm)
{
    memset(p_gcm, 0, sizeof(gcm_t));
}


/**@brief Function for checking a write event for whether a CCCD was written during the write
 *        operation.
 *
 * @param[in]  p_write_evt  The parameters of the write event.
 *
 * @return  Whether the write was on a CCCD.
 */
static bool cccd_written(ble_gatts_evt_write_t * p_write_evt)
{
    return (    (p_write_evt->op                     == BLE_GATTS_OP_WRITE_REQ)
             && (p_write_evt->context.type           == BLE_GATTS_ATTR_TYPE_DESC)
             && (p_write_evt->context.desc_uuid.type == BLE_UUID_TYPE_BLE)
             && (p_write_evt->context.desc_uuid.uuid == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG)
           );
}


/**@brief Function for performing the local DB update procedure in an event context, where no return
 *        code can be given.
 *
 * @details This function will do the procedure, and check the result, set a flag if needed, and
 *          send an event if needed.
 *
 * @param[in]  conn_handle  The connection to perform the procedure on.
 */
static void local_db_apply_in_evt(uint16_t conn_handle)
{
    bool set_procedure_as_pending = false;
    ret_code_t err_code;
    gcm_evt_t event;

    if (conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return;
    }

    err_code = gscm_local_db_cache_apply(conn_handle);

    switch(err_code)
    {
        case NRF_SUCCESS:
            event.evt_id                                    = GCM_EVT_LOCAL_DB_CACHE_APPLIED;
            event.peer_id                                   = im_peer_id_get_by_conn_handle(conn_handle);
            event.params.local_db_cache_applied.conn_handle = conn_handle;

            m_gcm.evt_handler(&event);
            break;

        case NRF_ERROR_BUSY:
            set_procedure_as_pending = true;
            break;

        case NRF_ERROR_INVALID_DATA:
            event.evt_id                                        = GCM_EVT_ERROR_LOCAL_DB_CACHE_APPLY;
            event.peer_id                                       = im_peer_id_get_by_conn_handle(conn_handle);
            event.params.error_local_db_cache_apply.conn_handle = conn_handle;

            m_gcm.evt_handler(&event);
            break;

        case BLE_ERROR_INVALID_CONN_HANDLE:
            /* Do nothing */
            break;

        default:
            event.evt_id                              = GCM_EVT_ERROR_UNEXPECTED;
            event.peer_id                             = im_peer_id_get_by_conn_handle(conn_handle);
            event.params.error_unexpected.conn_handle = conn_handle;
            event.params.error_unexpected.error       = err_code;

            m_gcm.evt_handler(&event);
            break;
    }

    ble_conn_state_user_flag_set(conn_handle, m_gcm.flag_id_local_db_apply_pending, set_procedure_as_pending);
}


/**@brief Function for performing the local DB apply procedure in an event context, where no return
 *        code can be given.
 *
 * @details This function will do the procedure, and check the result, set a flag if needed, and
 *          send an event if needed.
 *
 * @param[in]  conn_handle  The connection to perform the procedure on.
 */
static void local_db_update_in_evt(uint16_t conn_handle)
{
    gcm_evt_t event;
    bool set_procedure_as_pending = false;
    ret_code_t err_code = gscm_local_db_cache_update(conn_handle);

    switch(err_code)
    {
        case NRF_SUCCESS:
            event.evt_id                                    = GCM_EVT_LOCAL_DB_CACHE_UPDATED;
            event.params.local_db_cache_applied.conn_handle = conn_handle;
            event.peer_id                                   = im_peer_id_get_by_conn_handle(conn_handle);

            m_gcm.evt_handler(&event);
            break;

        case BLE_ERROR_INVALID_CONN_HANDLE:
            /* Do nothing */
            break;

        case NRF_ERROR_BUSY:
            set_procedure_as_pending = true;
            break;

        case NRF_ERROR_DATA_SIZE:
            event.evt_id = GCM_EVT_ERROR_DATA_SIZE;
            event.params.error_data_size.conn_handle = conn_handle;
            event.peer_id = im_peer_id_get_by_conn_handle(conn_handle);

            m_gcm.evt_handler(&event);
            break;

        case NRF_ERROR_NO_MEM:
            event.evt_id = GCM_EVT_ERROR_STORAGE_FULL;
            event.params.error_no_mem.conn_handle = conn_handle;
            event.peer_id = im_peer_id_get_by_conn_handle(conn_handle);

            m_gcm.evt_handler(&event);
            break;

        default:
            event.evt_id                              = GCM_EVT_ERROR_UNEXPECTED;
            event.peer_id                             = im_peer_id_get_by_conn_handle(conn_handle);
            event.params.error_unexpected.conn_handle = conn_handle;
            event.params.error_unexpected.error       = err_code;

            m_gcm.evt_handler(&event);
            break;
    }

    ble_conn_state_user_flag_set(conn_handle, m_gcm.flag_id_local_db_update_pending, set_procedure_as_pending);
}


/**@brief Function for sending a service changed indication in an event context, where no return
 *        code can be given.
 *
 * @details This function will do the procedure, and check the result, set a flag if needed, and
 *          send an event if needed.
 *
 * @param[in]  conn_handle  The connection to perform the procedure on.
 */
static void service_changed_send_in_evt(uint16_t conn_handle)
{
    gcm_evt_t event;
    ret_code_t err_code = gscm_service_changed_ind_send(conn_handle);

    switch(err_code)
    {
        case NRF_SUCCESS:
            /* Do nothing */
            break;

        case BLE_ERROR_INVALID_CONN_HANDLE:
            /* Do nothing */
            break;

        case NRF_ERROR_BUSY:
            /* Do nothing */
            break;

        case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
            local_db_apply_in_evt(conn_handle);
            break;

        default:
            event.evt_id = GCM_EVT_ERROR_UNEXPECTED;
            event.params.error_unexpected.conn_handle = conn_handle;
            event.params.error_unexpected.error = err_code;
            event.peer_id = im_peer_id_get_by_conn_handle(conn_handle);

            m_gcm.evt_handler(&event);
            break;
    }

    ble_conn_state_user_flag_set(conn_handle, m_gcm.flag_id_service_changed_pending, true);
}


/**@brief Callback function for events from the GATT Cache Server Manager module.
 *
 * @param[in]  p_event  The event from the GATT Cache Server Manager module.
 */
static void gscm_evt_handler(gscm_evt_t const * p_event)
{
    gcm_evt_t event;
    switch (p_event->evt_id)
    {
        case GSCM_EVT_LOCAL_DB_CACHE_STORED:
            event.evt_id = GCM_EVT_LOCAL_DB_CACHE_STORED;
            event.peer_id = p_event->peer_id;

            m_gcm.evt_handler(&event);
            local_db_apply_in_evt(im_conn_handle_get(p_event->peer_id));
            break;
        case GSCM_EVT_LOCAL_DB_CACHE_UPDATED:
            event.evt_id = GCM_EVT_LOCAL_DB_CACHE_UPDATED;
            event.peer_id = p_event->peer_id;
            event.params.local_db_cache_updated.conn_handle = p_event->params.local_db_cache_updated.conn_handle;

            m_gcm.evt_handler(&event);
            break;
        case GSCM_EVT_SC_STATE_STORED:
            if (p_event->params.sc_state_stored.state)
            {
                uint16_t conn_handle = im_conn_handle_get(p_event->peer_id);
                if (conn_handle != BLE_CONN_HANDLE_INVALID)
                {
                    ble_conn_state_user_flag_set(conn_handle, m_gcm.flag_id_service_changed_pending, true);
                }
            }
            break;
    }
}


/**@brief Callback function for events from the GATT Cache Client Manager module.
 *
 * @param[in]  p_event  The event from the GATT Cache Client Manager module.
 */
static void gccm_evt_handler(gccm_evt_t const * p_event)
{

}


/**@brief Callback function for events from the Identity Manager module.
 *
 * @param[in]  p_event  The event from the Identity Manager module.
 */
static void im_evt_handler(im_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case IM_EVT_BONDED_PEER_CONNECTED:
            local_db_apply_in_evt(p_event->conn_handle);
            if (gscm_service_changed_ind_needed(p_event->conn_handle))
            {
                ble_conn_state_user_flag_set(p_event->conn_handle, m_gcm.flag_id_service_changed_pending, true);
            }
            break;
        default:
            break;
    }
}


/**@brief Callback function for events from the Security Dispatcher module.
 *
 * @param[in]  p_event  The event from the Security Dispatcher module.
 */
static void smd_evt_handler(smd_evt_t const * p_event)
{
    switch (p_event->evt_id)
    {
        case SMD_EVT_BONDING_INFO_STORED:
            local_db_update_in_evt(p_event->conn_handle);
            break;
        default:
            break;
    }
}


ret_code_t gcm_init(gcm_evt_handler_t evt_handler)
{
    VERIFY_PARAM_NOT_NULL(evt_handler);

    ret_code_t err_code;

    err_code = gscm_init(gscm_evt_handler);
    if (err_code != NRF_SUCCESS) {return err_code;}

    err_code = gccm_init(gccm_evt_handler);
    if (err_code != NRF_SUCCESS) {return err_code;}

    internal_state_reset(&m_gcm);
    m_gcm.evt_handler = evt_handler;

    err_code = im_register(im_evt_handler);
    if (err_code != NRF_SUCCESS) {return err_code;}

    err_code = smd_register(smd_evt_handler);
    if (err_code != NRF_SUCCESS) {return err_code;}


    m_gcm.flag_id_local_db_update_pending = ble_conn_state_user_flag_acquire();
    m_gcm.flag_id_local_db_apply_pending  = ble_conn_state_user_flag_acquire();
    m_gcm.flag_id_service_changed_pending = ble_conn_state_user_flag_acquire();

    if  ((m_gcm.flag_id_local_db_update_pending  == BLE_CONN_STATE_USER_FLAG_INVALID)
      || (m_gcm.flag_id_local_db_apply_pending   == BLE_CONN_STATE_USER_FLAG_INVALID)
      || (m_gcm.flag_id_service_changed_pending  == BLE_CONN_STATE_USER_FLAG_INVALID))
    {
        err_code = NRF_ERROR_INTERNAL;
    }

    return err_code;
}


/**@brief Function for performing the Local DB apply procedure if it is pending on any connections.
 */
static void apply_pending_flags_check(void)
{
    sdk_mapped_flags_t apply_pending_flags;

    apply_pending_flags = ble_conn_state_user_flag_collection(m_gcm.flag_id_local_db_apply_pending);
    if (sdk_mapped_flags_any_set(apply_pending_flags))
    {
        sdk_mapped_flags_key_list_t conn_handle_list;
        conn_handle_list = ble_conn_state_conn_handles();

        for (int i = 0; i < conn_handle_list.len; i++)
        {
            if (ble_conn_state_user_flag_get(conn_handle_list.flag_keys[i], m_gcm.flag_id_local_db_apply_pending))
            {
                local_db_apply_in_evt(conn_handle_list.flag_keys[i]);
            }
        }
    }
}


/**@brief Function for performing the Local DB update procedure if it is pending on any connections.
 */
static void update_pending_flags_check(void)
{
    sdk_mapped_flags_t update_pending_flags;

    update_pending_flags = ble_conn_state_user_flag_collection(m_gcm.flag_id_local_db_update_pending);
    if (sdk_mapped_flags_any_set(update_pending_flags))
    {
        sdk_mapped_flags_key_list_t conn_handle_list;
        conn_handle_list = ble_conn_state_conn_handles();

        for (int i = 0; i < conn_handle_list.len; i++)
        {
            if (ble_conn_state_user_flag_get(conn_handle_list.flag_keys[i], m_gcm.flag_id_local_db_update_pending))
            {
                local_db_update_in_evt(conn_handle_list.flag_keys[i]);
            }
        }
    }
}


/**@brief Function for sending service changed indications if it is pending on any connections.
 */
static void service_changed_pending_flags_check(void)
{
    sdk_mapped_flags_t service_changed_pending_flags;

    service_changed_pending_flags = ble_conn_state_user_flag_collection(m_gcm.flag_id_service_changed_pending);
    if (sdk_mapped_flags_any_set(service_changed_pending_flags))
    {
        sdk_mapped_flags_key_list_t conn_handle_list;
        conn_handle_list = ble_conn_state_conn_handles();

        for (int i = 0; i < conn_handle_list.len; i++)
        {
            if (ble_conn_state_user_flag_get(conn_handle_list.flag_keys[i], m_gcm.flag_id_service_changed_pending))
            {
                service_changed_send_in_evt(conn_handle_list.flag_keys[i]);
            }
        }
    }
}


/**@brief Callback function for BLE events from the SoftDevice.
 *
 * @param[in]  p_ble_evt  The BLE event from the SoftDevice.
 */
void gcm_ble_evt_handler(ble_evt_t * p_ble_evt)
{
    switch(p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            local_db_apply_in_evt(p_ble_evt->evt.gatts_evt.conn_handle);
            break;

        case BLE_GATTS_EVT_SC_CONFIRM:
            gscm_peer_was_notified_of_db_change(im_peer_id_get_by_conn_handle(p_ble_evt->evt.gatts_evt.conn_handle));
            ble_conn_state_user_flag_set(p_ble_evt->evt.gatts_evt.conn_handle, m_gcm.flag_id_service_changed_pending, false);
            break;

        case BLE_GATTS_EVT_WRITE:
            if (cccd_written(&p_ble_evt->evt.gatts_evt.params.write))
            {
                local_db_update_in_evt(p_ble_evt->evt.gatts_evt.conn_handle);
            }
            break;
    }

    apply_pending_flags_check();
    update_pending_flags_check();
    service_changed_pending_flags_check();
}


ret_code_t gcm_remote_db_store(pm_peer_id_t peer_id, pm_peer_data_remote_gatt_db_t * p_remote_db)
{
    VERIFY_MODULE_INITIALIZED();

    return gccm_remote_db_store(peer_id, p_remote_db);
}


ret_code_t gcm_remote_db_retrieve(pm_peer_id_t peer_id, pm_peer_data_remote_gatt_db_t * p_remote_db)
{
    VERIFY_MODULE_INITIALIZED();
    VERIFY_PARAM_NOT_NULL(p_remote_db);

    return gccm_remote_db_retrieve(peer_id, p_remote_db);
}


ret_code_t gcm_local_db_cache_update(uint16_t conn_handle)
{
    VERIFY_MODULE_INITIALIZED();

    ret_code_t err_code = gscm_local_db_cache_update(conn_handle);
    bool set_procedure_as_pending = false;

    if (err_code == NRF_ERROR_BUSY)
    {
        set_procedure_as_pending = true;
        err_code = NRF_SUCCESS;
    }

    ble_conn_state_user_flag_set(conn_handle, m_gcm.flag_id_local_db_update_pending, set_procedure_as_pending);

    return err_code;
}


ret_code_t gcm_local_db_cache_set(pm_peer_id_t peer_id, pm_peer_data_local_gatt_db_t * p_local_db)
{
    VERIFY_MODULE_INITIALIZED();

    return gscm_local_db_cache_set(peer_id, p_local_db);
}


ret_code_t gcm_local_db_cache_get(pm_peer_id_t peer_id, pm_peer_data_local_gatt_db_t * p_local_db)
{
    VERIFY_MODULE_INITIALIZED();

    return gscm_local_db_cache_get(peer_id, p_local_db);
}


void gcm_local_database_has_changed(void)
{
    gscm_local_database_has_changed();

    sdk_mapped_flags_key_list_t conn_handles = ble_conn_state_conn_handles();

    for (uint16_t i = 0; i < conn_handles.len; i++)
    {
        if (im_peer_id_get_by_conn_handle(conn_handles.flag_keys[i]) == PM_PEER_ID_INVALID)
        {
            ble_conn_state_user_flag_set(conn_handles.flag_keys[i], m_gcm.flag_id_service_changed_pending, true);
        }
    }

    service_changed_pending_flags_check();
}
