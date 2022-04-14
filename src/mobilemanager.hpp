#ifndef MOBILE_MANAGER_HPP
#define MOBILE_MANAGER_HPP

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

extern "C" {
    #include <libimobiledevice/libimobiledevice.h>
    #include <libimobiledevice/lockdown.h>
    #include <libimobiledevice/service.h>
    #include <libimobiledevice/mobile_image_mounter.h>
    #include <libimobiledevice/afc.h>
    #include <plist/plist.h>
}

#include "endianness.hpp"

#define TOOL_NAME "mobilemanager"
#define DT_SIMULATELOCATION_SERVICE "com.apple.dt.simulatelocation"

static const char PKG_PATH[] = "PublicStaging";
static const char PATH_PREFIX[] = "/private/var/mobile/Media";

typedef enum {
	DISK_IMAGE_UPLOAD_TYPE_AFC,
	DISK_IMAGE_UPLOAD_TYPE_UPLOAD_IMAGE
} disk_image_upload_type_t;


class MobileManager
{
public:
    MobileManager(const char* udid, int use_network, const char* cfg_file);
    ~MobileManager();

public:
    bool check_device(const char* udid);
    bool connect_device();
    void print_device_info();
    void update_location_range(double lng1, double lng2, double lat1, double lat2);
    bool simulate_location(bool reset, double* lng, double* lat);
    char* get_last_error_message();

    bool mount_image();
private:
    bool is_connected();
    double random_range(float start, float end);
    void get_random_location(double* wgs_lat, double* wgs_lng);
    void load_config(const char* cfg_file);
    void set_error_message(const char* msg);
private:
    idevice_t m_device;
    lockdownd_client_t m_client;

    const char* m_udid;

    int m_use_network;
    char error_message[512] = {0};
};

#endif  // MOBILE_MANAGER_HPP

