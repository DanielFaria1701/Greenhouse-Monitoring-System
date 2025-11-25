/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/

#define DEBUG 1 //DEBUG_NONE 0

#include "contiki-conf.h"
#include "rpl/rpl-private.h"
#include "mqtt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"

#if USE_TUNSLIP6
#include "net/netstack.h"
#include "dev/slip.h"
#include "net/ip/uip-debug.h"

#if WEBSERVER
#include "httpd-simple.h"
#endif

#endif

#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"

#if CONTIKI_TARGET_ZOUL
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"
#include "dev/tsl256x.h"
#include "dev/dht22.h"
#else
#include "dev/adxl345.h"
#include "dev/i2cmaster.h"
#include "dev/tmp102.h"
#endif

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/*
 * MQTT broker address
 */
static const char *broker_ip = MQTT_DEMO_BROKER_IP_ADDR;
/*---------------------------------------------------------------------------*/
/*
 * A timeout used when waiting for something to happen (e.g. to connect or to
 * disconnect)
 */
#define STATE_MACHINE_PERIODIC     (CLOCK_SECOND >> 1)
/*---------------------------------------------------------------------------*/
/* Provide visible feedback via LEDS during various states */
/* When connecting to broker */
#define CONNECTING_LED_DURATION    (CLOCK_SECOND >> 2)

/* Each time we try to publish */
#define PUBLISH_LED_ON_DURATION    (CLOCK_SECOND)
#define DEVICE_LED_ON_DURATION     (CLOCK_SECOND * 2)
/*---------------------------------------------------------------------------*/
/* Connections and reconnections */
#define RETRY_FOREVER              0xFF
#define RECONNECT_INTERVAL         (CLOCK_SECOND * 2)

/*
 * Number of times to try reconnecting to the broker.
 * Can be a limited number (e.g. 3, 10 etc) or can be set to RETRY_FOREVER
 */
#define RECONNECT_ATTEMPTS         RETRY_FOREVER
#define CONNECTION_STABLE_TIME     (CLOCK_SECOND * 5)
/*---------------------------------------------------------------------------*/
static struct timer connection_life;
static uint8_t connect_attempt;
/*---------------------------------------------------------------------------*/
/* Various states */
static uint8_t state;

#define STATE_INIT                    0
#define STATE_REGISTERED              1
#define STATE_CONNECTING              2
#define STATE_CONNECTED               3
#define STATE_PUBLISHING              4
#define STATE_DISCONNECTED            5
#define STATE_NEWCONFIG               6
#define STATE_COMPARE                 7
#define STATE_CONFIG_ERROR         0xFE
#define STATE_ERROR                0xFF
/*---------------------------------------------------------------------------*/
#define CONFIG_EVENT_TYPE_ID_LEN     24
#define CONFIG_CMD_TYPE_LEN          24
#define CONFIG_IP_ADDR_STR_LEN       32
/*---------------------------------------------------------------------------*/
/* A timeout used when waiting to connect to a network */
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
#define NO_NET_LED_DURATION         (NET_CONNECT_PERIODIC >> 1)
/*---------------------------------------------------------------------------*/
PROCESS_NAME(mqtt_publisher_process);
AUTOSTART_PROCESSES(&mqtt_publisher_process);
/*---------------------------------------------------------------------------*/
/**
 * \brief Data structure declaration for the MQTT client configuration
 */
typedef struct mqtt_client_config {
  char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
  char broker_ip[CONFIG_IP_ADDR_STR_LEN];
  char cmd_type[CONFIG_CMD_TYPE_LEN];
  clock_time_t pub_interval;
  uint16_t broker_port;
} mqtt_client_config_t;
/*---------------------------------------------------------------------------*/
/*
 * Buffers for Client ID and Topic.
 * Make sure they are large enough to hold the entire respective string
 */
static char client_id[BUFFER_SIZE];
static char pub_topic[BUFFER_SIZE];
static char sub_topic[BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
/*
 * The main MQTT buffers.
 * We will need to increase if we start publishing more data.
 */
static struct mqtt_connection conn;
static char app_buffer[APP_BUFFER_SIZE];
/*---------------------------------------------------------------------------*/
static struct mqtt_message *msg_ptr = 0;
static struct etimer publish_periodic_timer;
static struct ctimer ct;
static char *buf_ptr;
static uint16_t seq_nr_value = 0;

static bool aquecedor_ligado = false;
static bool ventilador_ligado = false;
static bool desumidificador_ligado = false;
static bool humidificador_ligado = false;
static bool dar_luz_ligado = false;
static bool tapar_luz_ligado = false;

/*---------------------------------------------------------------------------*/
static mqtt_client_config_t conf;

#if USE_TUNSLIP6
static uip_ipaddr_t prefix;
static uint8_t prefix_set;
#endif

/*---------------------------------------------------------------------------*/
PROCESS(mqtt_publisher_process, "DEEC MQTT Publisher Demo");
/*---------------------------------------------------------------------------*/
int
ipaddr_sprintf(char *buf, uint8_t buf_len, const uip_ipaddr_t *addr)
{
  uint16_t a;
  uint8_t len = 0;
  int i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) {
        len += snprintf(&buf[len], buf_len - len, "::");
      }
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
        len += snprintf(&buf[len], buf_len - len, ":");
      }
      len += snprintf(&buf[len], buf_len - len, "%x", a);
    }
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static void
publish_led_off(void *d)
{
  leds_off(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/
static void
pub_handler(const char *topic, uint16_t topic_len, const uint8_t *chunk, uint16_t chunk_len)
{
    char temperatura[20];
    char humidade[20];
    char luz[20];
    char aux1[5];
    char aux2[5];
    char aux3[5];
    char um[2] = "1";
    char msg_str[chunk_len + 1];
    strncpy(msg_str, (const char *)chunk, chunk_len);
    msg_str[chunk_len] = '\0';

    /* Extract and store Temperature, Humidity, and Light */
    char *temp_ptr = strstr(msg_str, "\"Temperature\":\"");
    char *humid_ptr = strstr(msg_str, "\"Humidity\":\"");
    char *light_ptr = strstr(msg_str, "\"Light\":\"");
    char *aquecedor_ptr = strstr(msg_str, "\"Aquecedor\":\"");
    char *ventilador_ptr = strstr(msg_str, "\"Ventilador\":\"");
    char *desumidificador_ptr = strstr(msg_str, "\"Desumidificador\":\"");
    char *humidificador_ptr = strstr(msg_str, "\"Humidificador\":\"");
    char *dar_luz_ptr = strstr(msg_str, "\"Dar_Luz\":\"");
    char *tapar_luz_ptr = strstr(msg_str, "\"Tapar_Luz\":\"");
    char *id_ptr = strstr(msg_str, "\"ID\":\"");

    printf("Pub Handler: topic='%s' (len=%u), chunk_len=%u\n", topic, topic_len, chunk_len);

    /* If we don't like the length, ignore */
    if (topic_len != 11) {
        printf("Incorrect topic or chunk len. Ignored\n");
        return;
    }

    if (id_ptr != NULL) {
        id_ptr += strlen("\"ID\":\"");
        char *end_ptr = strchr(id_ptr, '\"');
        if (end_ptr != NULL) {
            size_t id_len = end_ptr - id_ptr;
            strncpy(aux1, id_ptr, id_len);
            aux1[id_len] = '\0';
            if (strcmp(aux1, um) != 0) {
                printf("ID not equal to 1. Ignored\n");
                return;
            }
        }
    } else {
        printf("ID not found in chunk. Ignored\n");
        return;
    }

    if (aquecedor_ptr != NULL && ventilador_ptr != NULL && desumidificador_ptr != NULL && humidificador_ptr != NULL && dar_luz_ptr != NULL && tapar_luz_ptr != NULL ) {
      if (aquecedor_ligado) {
        aquecedor_ptr += strlen("\"Aquecedor\":\"");
        char *end_ptr = strchr(aquecedor_ptr, '\"');
        if (end_ptr != NULL) {
            size_t aquecedor_len = end_ptr - aquecedor_ptr;
            strncpy(aux1, aquecedor_ptr, aquecedor_len);
            aux1[aquecedor_len] = '\0';
            if (strcmp(aux1, um) == 0) {
                printf("O aquecedor já está ligado\n");
                aquecedor_ligado = true;
                ventilador_ligado = false;
            } else {
                ventilador_ptr += strlen("\"Ventilador\":\"");
                char *end_ptr = strchr(ventilador_ptr, '\"');
                if (end_ptr != NULL) {
                    size_t ventilador_len = end_ptr - ventilador_ptr;
                    strncpy(aux1, ventilador_ptr, ventilador_len);
                    aux1[ventilador_len] = '\0';
                    if (strcmp(aux1, um) == 0) {
                        printf("Ligar Ventilador e Desligar Aquecedor\n");
                        aquecedor_ligado = false;
                        ventilador_ligado = true;
                        leds_on(LEDS_BLUE);
                        ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                    } else {
                        printf("Desligar Aquecedor\n");
                        aquecedor_ligado = false;
                    }
                }
              }
        }
      } else if(ventilador_ligado){
        ventilador_ptr += strlen("\"Ventilador\":\"");
        char *end_ptr = strchr(ventilador_ptr, '\"');
        if (end_ptr != NULL){
          size_t ventilador_len = end_ptr - ventilador_ptr;
          strncpy(aux1, ventilador_ptr, ventilador_len);
          aux1[ventilador_len] = '\0';
          if (strcmp(aux1, um) == 0){
              printf("O ventilador já está ligado\n");
              aquecedor_ligado = false;
              ventilador_ligado = true;            
            } else {
              aquecedor_ptr += strlen("\"Aquecedor\":\"");
              char *end_ptr = strchr(aquecedor_ptr, '\"');
              if (end_ptr != NULL){
                size_t aquecedor_len = end_ptr - aquecedor_ptr;
                strncpy(aux1, aquecedor_ptr, aquecedor_len);
                aux1[aquecedor_len] = '\0';
                if (strcmp(aux1, um) == 0){
                  printf("Ligar Aquecedor e Desligar Ventilador\n");
                  aquecedor_ligado = true;
                  ventilador_ligado = false;
                  leds_on(LEDS_BLUE);
                  ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                } else {
                    printf("Desligar Ventilador\n");
                    ventilador_ligado = false;                  
                }
              }
            }
        }
      } else {
        aquecedor_ptr += strlen("\"Aquecedor\":\"");
        char *end_ptr = strchr(aquecedor_ptr, '\"');
        if (end_ptr != NULL) {
            size_t aquecedor_len = end_ptr - aquecedor_ptr;
            strncpy(aux1, aquecedor_ptr, aquecedor_len);
            aux1[aquecedor_len] = '\0';
            if (strcmp(aux1, um) == 0) {
                printf("Ligar Aquecedor\n");
                aquecedor_ligado = true;
                ventilador_ligado = false;
                leds_on(LEDS_RED);
                ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
            } else {
                ventilador_ptr += strlen("\"Ventilador\":\"");
                char *end_ptr = strchr(ventilador_ptr, '\"');
                if (end_ptr != NULL) {
                    size_t ventilador_len = end_ptr - ventilador_ptr;
                    strncpy(aux1, ventilador_ptr, ventilador_len);
                    aux1[ventilador_len] = '\0';
                    if (strcmp(aux1, um) == 0) {
                        printf("Ligar Ventilador\n");
                        aquecedor_ligado = false;
                        ventilador_ligado = true;
                        leds_on(LEDS_BLUE);
                        ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                    } else {
                        if (!aquecedor_ligado && !ventilador_ligado)
                            printf("O aquecedor e o ventilador já estão desligados\n");
                    }
                }
            }
        }

      }

      if (desumidificador_ligado) {
        desumidificador_ptr += strlen("\"Desumidificador\":\"");
        char *end_ptr = strchr(desumidificador_ptr, '\"');
        if (end_ptr != NULL) {
            size_t desumidificador_len = end_ptr - desumidificador_ptr;
            strncpy(aux2, desumidificador_ptr, desumidificador_len);
            aux2[desumidificador_len] = '\0';
            if (strcmp(aux2, um) == 0) {
                printf("O desumidificador já está ligado\n");
                desumidificador_ligado = true;
                humidificador_ligado = false;
            } else {
                humidificador_ptr += strlen("\"Humidificador\":\"");
                char *end_ptr = strchr(humidificador_ptr, '\"');
                if (end_ptr != NULL) {
                    size_t humidificador_len = end_ptr - humidificador_ptr;
                    strncpy(aux2, humidificador_ptr, humidificador_len);
                    aux2[humidificador_len] = '\0';
                    if (strcmp(aux2, um) == 0) {
                        printf("Ligar Humidificador e Desligar Desumidificador\n");
                        desumidificador_ligado = false;
                        humidificador_ligado = true;
                        leds_on(LEDS_PURPLE);
                        ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                    } else {
                        printf("Desligar Desumidificador\n");
                        desumidificador_ligado = false;
                    }
                }
              }
        }
      } else if(humidificador_ligado){
        humidificador_ptr += strlen("\"Humidificador\":\"");
        char *end_ptr = strchr(humidificador_ptr, '\"');
        if (end_ptr != NULL){
          size_t humidificador_len = end_ptr - humidificador_ptr;
          strncpy(aux2, humidificador_ptr, humidificador_len);
          aux2[humidificador_len] = '\0';
          if (strcmp(aux2, um) == 0){
              printf("O humidificador já está ligado\n");
              desumidificador_ligado = false;
              humidificador_ligado = true;            
            } else {
              desumidificador_ptr += strlen("\"Desumidificador\":\"");
              char *end_ptr = strchr(desumidificador_ptr, '\"');
              if (end_ptr != NULL){
                size_t desumidificador_len = end_ptr - desumidificador_ptr;
                strncpy(aux2, desumidificador_ptr, desumidificador_len);
                aux2[desumidificador_len] = '\0';
                if (strcmp(aux2, um) == 0){
                  printf("Ligar Desumidificador e Desligar Humidificador\n");
                  desumidificador_ligado = true;
                  humidificador_ligado = false;
                  leds_on(LEDS_YELLOW);
                  ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                } else {
                    printf("Desligar Humidificador\n");
                    humidificador_ligado = false;                  
                }
              }
            }
        }
      } else {
        desumidificador_ptr += strlen("\"Desumidificador\":\"");
        char *end_ptr = strchr(desumidificador_ptr, '\"');
        if (end_ptr != NULL) {
            size_t desumidificador_len = end_ptr - desumidificador_ptr;
            strncpy(aux2, desumidificador_ptr, desumidificador_len);
            aux2[desumidificador_len] = '\0';
            if (strcmp(aux2, um) == 0) {
                printf("Ligar Desumidificador\n");
                desumidificador_ligado = true;
                humidificador_ligado = false;
                leds_on(LEDS_YELLOW);
                ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
            } else {
                humidificador_ptr += strlen("\"Humidificador\":\"");
                char *end_ptr = strchr(humidificador_ptr, '\"');
                if (end_ptr != NULL) {
                    size_t humidificador_len = end_ptr - humidificador_ptr;
                    strncpy(aux2, humidificador_ptr, humidificador_len);
                    aux2[humidificador_len] = '\0';
                    if (strcmp(aux2, um) == 0) {
                        printf("Ligar Humidificador\n");
                        desumidificador_ligado = false;
                        humidificador_ligado = true;
                        leds_on(LEDS_PURPLE);
                        ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                    } else {
                        if (!desumidificador_ligado && !humidificador_ligado)
                            printf("O desumidificador e o humidificador já estão desligados\n");
                    }
                }
            }
        }

      }

      if (dar_luz_ligado) {
        dar_luz_ptr += strlen("\"Dar_Luz\":\"");
        char *end_ptr = strchr(dar_luz_ptr, '\"');
        if (end_ptr != NULL) {
            size_t dar_luz_len = end_ptr - dar_luz_ptr;
            strncpy(aux3, dar_luz_ptr, dar_luz_len);
            aux3[dar_luz_len] = '\0';
            if (strcmp(aux3, um) == 0) {
                printf("O dar_luz já está ligado\n");
                dar_luz_ligado = true;
                tapar_luz_ligado = false;
            } else {
                tapar_luz_ptr += strlen("\"Tapar_Luz\":\"");
                char *end_ptr = strchr(tapar_luz_ptr, '\"');
                if (end_ptr != NULL) {
                    size_t tapar_luz_len = end_ptr - tapar_luz_ptr;
                    strncpy(aux3, tapar_luz_ptr, tapar_luz_len);
                    aux3[tapar_luz_len] = '\0';
                    if (strcmp(aux3, um) == 0) {
                        printf("Ligar Tapar_Luz e Desligar Dar_Luz\n");
                        dar_luz_ligado = false;
                        tapar_luz_ligado = true;
                        leds_on(LEDS_ALL);
                        ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                    } else {
                        printf("Desligar Dar_Luz\n");
                        dar_luz_ligado = false;
                    }
                }
              }
        }
      } else if(tapar_luz_ligado){
        tapar_luz_ptr += strlen("\"Tapar_Luz\":\"");
        char *end_ptr = strchr(tapar_luz_ptr, '\"');
        if (end_ptr != NULL){
          size_t tapar_luz_len = end_ptr - tapar_luz_ptr;
          strncpy(aux3, tapar_luz_ptr, tapar_luz_len);
          aux3[tapar_luz_len] = '\0';
          if (strcmp(aux3, um) == 0){
              printf("O tapar_luz já está ligado\n");
              dar_luz_ligado = false;
              tapar_luz_ligado = true;            
            } else {
              dar_luz_ptr += strlen("\"Dar_Luz\":\"");
              char *end_ptr = strchr(dar_luz_ptr, '\"');
              if (end_ptr != NULL){
                size_t dar_luz_len = end_ptr - dar_luz_ptr;
                strncpy(aux3, dar_luz_ptr, dar_luz_len);
                aux3[dar_luz_len] = '\0';
                if (strcmp(aux3, um) == 0){
                  printf("Ligar Dar_Luz e Desligar Tapar_Luz\n");
                  dar_luz_ligado = true;
                  tapar_luz_ligado = false;
                  leds_on(LEDS_GREEN);
                  ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                } else {
                    printf("Desligar Tapar_Luz\n");
                    tapar_luz_ligado = false;                  
                }
              }
            }
        }
      } else {
        dar_luz_ptr += strlen("\"Dar_Luz\":\"");
        char *end_ptr = strchr(dar_luz_ptr, '\"');
        if (end_ptr != NULL) {
            size_t dar_luz_len = end_ptr - dar_luz_ptr;
            strncpy(aux3, dar_luz_ptr, dar_luz_len);
            aux3[dar_luz_len] = '\0';
            if (strcmp(aux3, um) == 0) {
                printf("Ligar Dar_Luz\n");
                dar_luz_ligado = true;
                tapar_luz_ligado = false;
                leds_on(LEDS_GREEN);
                ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
            } else {
                tapar_luz_ptr += strlen("\"Tapar_Luz\":\"");
                char *end_ptr = strchr(tapar_luz_ptr, '\"');
                if (end_ptr != NULL) {
                    size_t tapar_luz_len = end_ptr - tapar_luz_ptr;
                    strncpy(aux3, tapar_luz_ptr, tapar_luz_len);
                    aux3[tapar_luz_len] = '\0';
                    if (strcmp(aux3, um) == 0) {
                        printf("Ligar Tapar_Luz\n");
                        dar_luz_ligado = false;
                        tapar_luz_ligado = true;
                        leds_on(LEDS_ALL);
                        ctimer_set(&ct, DEVICE_LED_ON_DURATION, publish_led_off, NULL);
                    } else {
                        if (!dar_luz_ligado && !tapar_luz_ligado)
                            printf("O dar_luz e o tapar_luz já estão desligados\n");
                    }
                }
            }
        }

      }

        
      } else {
        printf("Devices State not found in chunk.\n");
        strcpy(aux1, "");
        strcpy(aux2, "");
        strcpy(aux3, "");
    }

    if (temp_ptr != NULL && humid_ptr != NULL && light_ptr != NULL) {
        // Extract Temperature
        temp_ptr += strlen("\"Temperature\":\"");
        char *end_ptr = strchr(temp_ptr, '\"');
        if (end_ptr != NULL) {
            size_t temp_len = end_ptr - temp_ptr;
            strncpy(temperatura, temp_ptr, temp_len);
            temperatura[temp_len] = '\0';
            printf("Valor de Temperatura: %s\n", temperatura);
        }

        // Extract Humidity
        humid_ptr += strlen("\"Humidity\":\"");
        end_ptr = strchr(humid_ptr, '\"');
        if (end_ptr != NULL) {      
            size_t humid_len = end_ptr - humid_ptr;
            strncpy(humidade, humid_ptr, humid_len);
            humidade[humid_len] = '\0';
            printf("Valor de Humidade: %s\n", humidade);    
        }

        // Extract Light
        light_ptr += strlen("\"Light\":\"");
        end_ptr = strchr(light_ptr, '\"');
        if (end_ptr != NULL) {
            size_t light_len = end_ptr - light_ptr;
            strncpy(luz, light_ptr, light_len);
            luz[light_len] = '\0';
            printf("Valor de Luz: %s\n", luz);
        }
    } else {
        printf("Temperature, Humidity, or Light not found in chunk.\n");
        // Set valores para vazio ou valor padrão caso necessário
        strcpy(temperatura, "");
        strcpy(humidade, "");
        strcpy(luz, "");
    }
}
/*---------------------------------------------------------------------------*/
static void
mqtt_event(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
  switch(event) {
  case MQTT_EVENT_CONNECTED: {
    printf("APP - Application has a MQTT connection\n");
    timer_set(&connection_life, CONNECTION_STABLE_TIME);
    state = STATE_CONNECTED;
    break;
  }
  case MQTT_EVENT_DISCONNECTED: {
    printf("APP - MQTT Disconnect. Reason %u\n", *((mqtt_event_t *)data));

    state = STATE_DISCONNECTED;
    process_poll(&mqtt_publisher_process);
    break;
  }
  case MQTT_EVENT_PUBLISH: {
    msg_ptr = data;

    /* Implement first_flag in publish message? */
    if(msg_ptr->first_chunk) {
      msg_ptr->first_chunk = 0;
      printf("APP - Application received a publish on topic '%s'. Payload "
          "size is %i bytes. Content:\n\n",
          msg_ptr->topic, msg_ptr->payload_length);
    }

    pub_handler(msg_ptr->topic, strlen(msg_ptr->topic), msg_ptr->payload_chunk,
                msg_ptr->payload_length);
    break;
  }
  case MQTT_EVENT_SUBACK: {
    printf("APP - Application is subscribed to topic successfully\n");
    break;
  }
  case MQTT_EVENT_UNSUBACK: {
    printf("APP - Application is unsubscribed to topic successfully\n");
    break;
  }
  case MQTT_EVENT_PUBACK: {
    printf("APP - Publishing complete.\n");
    break;
  }
  default:
    printf("APP - Application got a unhandled MQTT event: %i\n", event);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static int
construct_pub_topic(void)
{
  int len = snprintf(pub_topic, BUFFER_SIZE, "%s",
                     conf.event_type_id);
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Pub Topic too large: %d, Buffer %d\n", len, BUFFER_SIZE);
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
construct_sub_topic(void)
{
  int len = snprintf(sub_topic, BUFFER_SIZE, "%s",
                     conf.cmd_type);
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Sub Topic too large: %d, Buffer %d\n", len, BUFFER_SIZE);
    return 0;
  }

  printf("Subscription topic %s\n", sub_topic);

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
construct_client_id(void)
{
  int len = snprintf(client_id, BUFFER_SIZE, "d:%02x%02x%02x%02x%02x%02x",
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  /* len < 0: Error. Len >= BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= BUFFER_SIZE) {
    printf("Client ID: %d, Buffer %d\n", len, BUFFER_SIZE);
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
update_config(void)
{
  if(construct_client_id() == 0) {
    /* Fatal error. Client ID larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  if(construct_sub_topic() == 0) {
    /* Fatal error. Topic larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  if(construct_pub_topic() == 0) {
    /* Fatal error. Topic larger than the buffer */
    state = STATE_CONFIG_ERROR;
    return;
  }

  /* Reset the counter */
  seq_nr_value = 0;

  state = STATE_INIT;

  /*
   * Schedule next timer event ASAP
   * If we entered an error state then we won't do anything when it fires.
   * Since the error at this stage is a config error, we will only exit this
   * error state if we get a new config.
   */
  etimer_set(&publish_periodic_timer, 0);

  return;
}
/*---------------------------------------------------------------------------*/
static int
init_config()
{
  /* Populate configuration with default values */
  memset(&conf, 0, sizeof(mqtt_client_config_t));
  memcpy(conf.event_type_id, DEFAULT_STATUS_EVENT_TOPIC, strlen(DEFAULT_STATUS_EVENT_TOPIC));
  memcpy(conf.broker_ip, broker_ip, strlen(broker_ip));
  memcpy(conf.cmd_type, DEFAULT_LEDS_CMD_TOPIC, strlen(DEFAULT_LEDS_CMD_TOPIC));

  conf.broker_port = DEFAULT_BROKER_PORT;
  conf.pub_interval = DEFAULT_PUBLISH_INTERVAL;

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
subscribe(void)
{
  /* Publish MQTT topic in IBM quickstart format */
  mqtt_status_t status;

  status = mqtt_subscribe(&conn, NULL, sub_topic, MQTT_QOS_LEVEL_0);

  printf("APP - Subscribing to %s\n", sub_topic);
  if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
    printf("APP - Tried to subscribe but command queue was full!\n");
  }
}
/*---------------------------------------------------------------------------*/
long long current_timestamp_ms() {
    struct timeval te;
    gettimeofday(&te, NULL); // Obter o tempo atual
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // Calcular milissegundos
    return milliseconds;
}
/*---------------------------------------------------------------------------*/
static void
publish(void)
{
  int len;
  float aux1, aux2;
  uint16_t light;
  int remaining = APP_BUFFER_SIZE;
  int16_t temperature, humidity;
  //bool helpt = false, helph = false, helpl = false;
  //char timestamp[40];

  seq_nr_value++;
  buf_ptr = app_buffer;

  //get_current_time(timestamp, sizeof(timestamp));

  len = snprintf(buf_ptr, remaining,
                 "{"
                 //"\"d\":{"
                 "\"APP\":\"0\","
                 "\"ID\": \"1\"");

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  remaining -= len;
  buf_ptr += len;

if(dht22_read_all(&temperature, &humidity) != DHT22_ERROR) {
  len = snprintf(buf_ptr, remaining, ",\"Temperature\":\"%02d.%02d\"", temperature / 10, temperature % 10);

  remaining -= len;
  buf_ptr += len;

  len = snprintf(buf_ptr, remaining, ",\"Humidity\":\"%02d.%02d\"", humidity / 10, humidity % 10);

  remaining -= len;
  buf_ptr += len;

  light = adc_zoul.value(ZOUL_SENSORS_ADC3);

  len = snprintf(buf_ptr, remaining, ",\"Light\":\"%u\"", light);
 
  remaining -= len;
  buf_ptr += len;
  } else {
      printf("Failed to read the sensor\n");
    }

  //len = snprintf(buf_ptr, remaining, "}");

  if(len < 0 || len >= remaining) {
    printf("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }
  
  mqtt_publish(&conn, NULL, pub_topic, (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

  printf("APP - Publish to %s: %s\n", pub_topic, app_buffer);


}
/*---------------------------------------------------------------------------*/
static void
connect_to_broker(void)
{
  /* Connect to MQTT server */
  mqtt_connect(&conn, conf.broker_ip, conf.broker_port,
               conf.pub_interval * 3);

  state = STATE_CONNECTING;
}
/*---------------------------------------------------------------------------*/
static void
state_machine(void)
{
  switch(state) {
  case STATE_INIT:
    /* If we have just been configured register MQTT connection */
    mqtt_register(&conn, &mqtt_publisher_process, client_id, mqtt_event,
                  MAX_TCP_SEGMENT_SIZE);

    conn.auto_reconnect = 0;
    connect_attempt = 1;

    state = STATE_REGISTERED;
    printf("Init\n");

    /* Notice there is no "break" here, it will continue to the
     * STATE_REGISTERED
     */
  case STATE_REGISTERED:
    if(uip_ds6_get_global(ADDR_PREFERRED) != NULL) {
      /* Registered and with a public IP. Connect */
      printf("Registered. Connect attempt %u\n", connect_attempt);
      connect_to_broker();

    } else {
      leds_on(LEDS_GREEN);
      ctimer_set(&ct, NO_NET_LED_DURATION, publish_led_off, NULL);
    }
    etimer_set(&publish_periodic_timer, NET_CONNECT_PERIODIC);
    return;
    break;

  case STATE_CONNECTING:
    leds_on(LEDS_GREEN);
    ctimer_set(&ct, CONNECTING_LED_DURATION, publish_led_off, NULL);
    /* Not connected yet. Wait */
    printf("Connecting (%u)\n", connect_attempt);
    break;

  case STATE_CONNECTED:
    /* Notice there's no "break" here, it will continue to subscribe */

  case STATE_PUBLISHING:
    /* If the timer expired, the connection is stable. */
    if(timer_expired(&connection_life)) {
      /*
       * Intentionally using 0 here instead of 1: We want RECONNECT_ATTEMPTS
       * attempts if we disconnect after a successful connect
       */
      connect_attempt = 0;
    }

    if(mqtt_ready(&conn) && conn.out_buffer_sent) {
      /* Connected. Publish */
      if(state == STATE_CONNECTED) {
        subscribe();
        state = STATE_PUBLISHING;

      } else {
        leds_on(LEDS_GREEN);
        printf("Publishing\n");
        ctimer_set(&ct, PUBLISH_LED_ON_DURATION, publish_led_off, NULL);
        publish();
      }
      etimer_set(&publish_periodic_timer, conf.pub_interval);

      /* Return here so we don't end up rescheduling the timer */
      return;

    } else {
      /*
       * Our publish timer fired, but some MQTT packet is already in flight
       * (either not sent at all, or sent but not fully ACKd).
       *
       * This can mean that we have lost connectivity to our broker or that
       * simply there is some network delay. In both cases, we refuse to
       * trigger a new message and we wait for TCP to either ACK the entire
       * packet after retries, or to timeout and notify us.
       */
      printf("Publishing... (MQTT state=%d, q=%u)\n", conn.state,
          conn.out_queue_full);
    }
    break;

  case STATE_DISCONNECTED:
    printf("Disconnected\n");
    if(connect_attempt < RECONNECT_ATTEMPTS ||
       RECONNECT_ATTEMPTS == RETRY_FOREVER) {
      /* Disconnect and backoff */
      clock_time_t interval;
      mqtt_disconnect(&conn);
      connect_attempt++;

      interval = connect_attempt < 3 ? RECONNECT_INTERVAL << connect_attempt :
        RECONNECT_INTERVAL << 3;

      printf("Disconnected. Attempt %u in %lu ticks\n", connect_attempt, interval);

      etimer_set(&publish_periodic_timer, interval);

      state = STATE_REGISTERED;
      return;

    } else {
      /* Max reconnect attempts reached. Enter error state */
      state = STATE_ERROR;
      printf("Aborting connection after %u attempts\n", connect_attempt - 1);
    }
    break;

  case STATE_CONFIG_ERROR:
    /* Idle away. The only way out is a new config */
    printf("Bad configuration.\n");
    return;

  case STATE_ERROR:
  default:
    leds_on(LEDS_GREEN);
    printf("Default case: State=0x%02x\n", state);
    return;
  }

  /* If we didn't return so far, reschedule ourselves */
  etimer_set(&publish_periodic_timer, STATE_MACHINE_PERIODIC);
}

#if USE_TUNSLIP6
/*---------------------------------------------------------------------------*/
static void print_local_addresses(void)
{
  int i;
  uint8_t state;

  printf("Server IPv6 addresses:\n");
  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
      printf(" ");
      uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
      printf("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
void request_prefix(void)
{
  /* mess up uip_buf with a dirty request... */
  uip_buf[0] = '?';
  uip_buf[1] = 'P';
  uip_len = 2;
  slip_send();
  uip_clear_buf();
}
/*---------------------------------------------------------------------------*/
void set_prefix_64(uip_ipaddr_t *prefix_64)
{
  rpl_dag_t *dag;
  uip_ipaddr_t ipaddr;
  memcpy(&prefix, prefix_64, 16);
  memcpy(&ipaddr, prefix_64, 16);
  prefix_set = 1;
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

  dag = rpl_set_root(RPL_DEFAULT_INSTANCE, &ipaddr);
  if(dag != NULL) {
    rpl_set_prefix(dag, &prefix, 64);
    PRINTF("created a new RPL dag\n");
  }
}
#endif

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(mqtt_publisher_process, ev, data)
{
#if USE_TUNSLIP6
  static struct etimer et;
#endif

  PROCESS_BEGIN();

#if USE_TUNSLIP6
/* While waiting for the prefix to be sent through the SLIP connection, the future
 * border router can join an existing DAG as a parent or child, or acquire a default
 * router that will later take precedence over the SLIP fallback interface.
 * Prevent that by turning the radio off until we are initialized as a DAG root.
 */
  prefix_set = 0;
  NETSTACK_MAC.off(0);
#endif

  printf("DEEC MQTT Publisher Demo Process\n");

#if USE_TUNSLIP6


#if WEBSERVER
  httpd_init();
#endif

  /* Request prefix until it has been received */
  while(!prefix_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_prefix();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  /* Now turn the radio on, but disable radio duty cycling.
   * Since we are the DAG root, reception delays would constrain mesh throughput.
   */
  NETSTACK_MAC.off(1);

  print_local_addresses();
#endif


  if(init_config() != 1) {
    PROCESS_EXIT();
  }

#if CONTIKI_TARGET_ZOUL
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);
  SENSORS_ACTIVATE(dht22);
#else /* Default is Z1 */
  SENSORS_ACTIVATE(adxl345);
  SENSORS_ACTIVATE(tmp102);
#endif

  update_config();

  while(1) {
    PROCESS_YIELD();

#if USE_TUNSLIP6
    if (ev == sensors_event && data == &button_sensor) {
      PRINTF("Initiating SLIP Connection Global Repair\n");
      rpl_repair_root(RPL_DEFAULT_INSTANCE);
    }
#endif

    if((ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) ||
       ev == PROCESS_EVENT_POLL) {
      state_machine();
    }
  }

  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
