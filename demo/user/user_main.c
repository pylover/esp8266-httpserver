
// Internal 
#include "user_config.h"
#include "partition.h"
#include "wifi.h"
#include "params.h" 
#include "debug.h"
#include "fotabtn.h"

// SDK
#include <ets_sys.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <driver/uart.h>
#include <upgrade.h>
#include <c_types.h>
#include <ip_addr.h> 
#include <espconn.h>


static Params params;
static struct mdns_info mdns;


static ICACHE_FLASH_ATTR 
void _mdns_init() {
	struct ip_info ipconfig;
	wifi_set_broadcast_if(STATIONAP_MODE);
	wifi_get_ip_info(STATION_IF, &ipconfig);
	mdns.ipAddr = ipconfig.ip.addr; //ESP8266 Station IP
	mdns.host_name = params.device_name;
	mdns.server_name = "ESPWebAdmin";
	mdns.server_port = 80;
	mdns.txt_data[0] = "version = 0.1.0";
	espconn_mdns_init(&mdns);
}


void wifi_connect_cb(uint8_t status) {
    if(status == STATION_GOT_IP) {
		_mdns_init();
        INFO("Wifi connected.\r\n");
    } else {
		espconn_mdns_close();
        INFO("Wifi disconnected.\r\n");
    }
}


void user_init(void) {
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	bool ok = params_load(&params);
	if (!ok) {
		ERROR("Cannot load Params\r\n");
#if !WIFI_ENABLE_SOFTAP
		return;
#endif
		if(!params_defaults(&params)) {
			ERROR("Cannot save params\r\n");
			return;
		}
	}

	INFO("\r\nParams: name: %s, ssid: %s psk: %s ap-psk: %s\r\n",
			params.device_name,
			params.station_ssid, 
			params.station_psk,
			params.ap_psk
		);
    
    fotabtn_init();

#if WIFI_ENABLE_SOFTAP
    wifi_start(STATIONAP_MODE, &params, wifi_connect_cb);
#else
    wifi_start(STATION_MODE, &params, wifi_connect_cb);
#endif
    INFO("System started ...\r\n");
    INFO("Hello World!\r\n");
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}

