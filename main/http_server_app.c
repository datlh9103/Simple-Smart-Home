#include "http_server_app.h"

/* Simple HTTP Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include <esp_http_server.h>
#include <driver/gpio.h>
#include "mcpwm_servo_control_example_main.h"
/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static const char *TAG = "example";
static httpd_handle_t server = NULL;
#define BLINK_GPIO 12

// Khởi tạo GPIO cho LED
void configure_led(void) {
    gpio_pad_select_gpio(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

extern const uint8_t login_html_start[] asm("_binary_login_html_start");
extern const uint8_t login_html_end[] asm("_binary_login_html_end");

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

/*--------------------login-----------------------*/
static esp_err_t login_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)login_html_start, login_html_end - login_html_start);
    return ESP_OK;
}

static const httpd_uri_t login_data_smarthome = {
    .uri       = "/login",
    .method    = HTTP_GET,
    .handler   = login_get_handler,
    .user_ctx  = NULL
};
/*----------------------------------------------------*/
/*----------------------login success-----------------*/
static esp_err_t hello_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_data_smarthome = {
    .uri       = "/smarthome",
    .method    = HTTP_GET,
    .handler   = hello_get_handler,
    .user_ctx  = NULL
};
/*----------------------------------------------------*/
/*----------------------led on-----------------*/
static esp_err_t led_on_handler(httpd_req_t *req)
{
    //turn on led
    gpio_set_level(BLINK_GPIO, 1);
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t led_on_uri = {
    .uri       = "/led/on",
    .method    = HTTP_GET,
    .handler   = led_on_handler,
    .user_ctx  = NULL
};
/*----------------------------------------------------*/
/*----------------------led off-----------------*/
static esp_err_t led_off_handler(httpd_req_t *req)
{
    //turn off led
    gpio_set_level(BLINK_GPIO, 0);
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t led_off_uri = {
    .uri       = "/led/off",
    .method    = HTTP_GET,
    .handler   = led_off_handler,
    .user_ctx  = NULL
};
/*----------------------------------------------------*/
/*----------------------open door-----------------*/
static esp_err_t door_open_handler(httpd_req_t *req)
{
    servo(0,90,0);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static const httpd_uri_t door_open_uri = {
    .uri       = "/door/open",
    .method    = HTTP_GET,
    .handler   = door_open_handler,
    .user_ctx  = NULL
};
/*----------------------------------------------------*/
/*----------------------close door-----------------*/
static esp_err_t door_close_handler(httpd_req_t *req)
{
    servo(90,0,1);
    httpd_resp_send(req, NULL, 0);

    return ESP_OK;
}

static const httpd_uri_t door_close_uri = {
    .uri       = "/door/close",
    .method    = HTTP_GET,
    .handler   = door_close_handler,
    .user_ctx  = NULL
};

static esp_err_t data_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    printf("DATA: %s\n", buf);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_data = {
    .uri       = "/data",
    .method    = HTTP_POST,
    .handler   = data_post_handler,
    .user_ctx  = NULL
};


esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/smarthome", req->uri) == 0) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/hello URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &login_data_smarthome);       
        httpd_register_uri_handler(server, &get_data_smarthome);
        httpd_register_uri_handler(server, &post_data);
        httpd_register_uri_handler(server, &led_on_uri);
        httpd_register_uri_handler(server, &led_off_uri);
        httpd_register_uri_handler(server, &door_open_uri);
        httpd_register_uri_handler(server, &door_close_uri);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
        //httpd_register_uri_handler(server, &echo);
    }
    else 
    {
        ESP_LOGI(TAG, "Error starting server!");
    }
    
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}