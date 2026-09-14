
#include "mqtt_client.h"
#include "esp_log.h"
#include "mqtt.h"

static esp_mqtt_client_handle_t client;
static uint8_t connected = 0;
static const char *TAG = "mqtt";
static dtMQTT_State MqttState;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

typedef struct sub_descriptor
{
    uint8_t topic[32];
    uint8_t data[8];
} dtSubDesc;

static dtSubDesc SubDesc[3];
static uint8_t SubLimit;

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        connected = 1;
        MqttState = MQTT_State_Connected;
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
        //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        //ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        //ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        connected = 0;
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MqttState = MQTT_State_Disconnected;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        {
            uint8_t i;
            for(i = 0; i < (sizeof(SubDesc)/sizeof(SubDesc[0])); i++)
            {
                if(SubDesc[i].topic[0] != 0)
                {   
                    if(0 == memcmp((char*)SubDesc[i].topic, event->topic, event->topic_len) && (strlen((char*)SubDesc[i].topic) == event->topic_len))
                    {
                        memcpy(SubDesc[i].data, event->data, event->data_len);
                        SubDesc[i].data[event->data_len] = 0;
                        printf("Subscription has been found\n");
                    }
                }
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

dtMQTT_State MQTT_GetState(void)
{
    return MqttState;
}

void mqtt_init(const uint8_t *const broker_url)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = (char*)broker_url, };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if(MqttState == MQTT_State_Uninitialized) MqttState = MQTT_State_Disconnected;
}

void mqtt_app_start(void)
{
    MqttState = MQTT_State_Connecting;
    esp_mqtt_client_start(client);
}

void mqtt_stop(void)
{
    connected = 0;
    MqttState = MQTT_State_Disconnecting;
    esp_mqtt_client_disconnect(client);
    while(MqttState != MQTT_State_Disconnected)
    {
        vTaskDelay((10 / portTICK_PERIOD_MS));
    }
    SubLimit = 0;
    ESP_LOGI(TAG, "MQTT has disconnected ... stopping");
    esp_mqtt_client_stop(client);
}

uint8_t mqtt_isConnected(void)
{
    return connected;
}

void mqtt_reconnect(void)
{
    MqttState = MQTT_State_Connecting;
    esp_mqtt_client_reconnect(client);
}

uint32_t mqtt_publish(const char *topic, const char *data, uint32_t len, uint8_t qos, uint8_t retain)
{
    uint32_t result = esp_mqtt_client_publish(client, topic, data, len, qos, retain);
    ESP_LOGI(TAG, "sent publish result: %d", result);
    return result;
}

void mqtt_subscribe(uint8_t *topic)
{
    strcpy((char*)(SubDesc[SubLimit].topic), (char*)topic);
    esp_mqtt_client_subscribe_single(client, (char*)SubDesc[SubLimit].topic, 2);
    SubLimit++;
}

uint8_t mqtt_get_data(uint8_t *topic, uint8_t *data)
{
    uint8_t ret = 1;
    uint8_t i = 0;
    while((i < SubLimit) && (0 != strcmp((char*)SubDesc[i].topic, (char*)topic)))
    {
        i ++;
    }
    if(i < SubLimit)
    {
        if(SubDesc[i].data[0] != 0)
        {
            if(data != nullptr)
            {
                ret = 0;
                strcpy((char*)data, (char*)&SubDesc[i].data[0]);
                SubDesc[i].data[0] = 0;
            }
        }
    }
    return ret;
}