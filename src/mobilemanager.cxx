#include "mobilemanager.hpp"
#include "utils.hpp"
#include "transform.hpp"
#include <fstream>
#include <string.h>

// Default
double _lat[] = {30.290670, 30.291925};
double _lng[] = {120.067904, 120.069884};

MobileManager::MobileManager(const char* udid, int use_network, const char* cfg_file)
    : m_device(nullptr), m_use_network(use_network), m_udid(udid)
{
    load_config(cfg_file);  
}

MobileManager::~MobileManager()
{
    if (m_client) {
        lockdownd_client_free(m_client);
    }

    if (m_device) 
    {
        idevice_free(m_device);
    }
}

char* MobileManager::get_last_error_message()
{
    return error_message;
}

void MobileManager::set_error_message(const char* msg)
{
    sprintf(error_message, "%s", msg);
}


void MobileManager::update_location_range(double lng1, double lng2, double lat1, double lat2)
{
    _lat[0] = lat1;
    _lat[1] = lat2;
    _lng[0] = lng1;
    _lng[1] = lng2;

    // TOFIX: Do you need save the parameters to the file.
}

void MobileManager::load_config(const char* cfg_file)
{
    if (cfg_file) {
        printf("cfg_file: %s\n", cfg_file);
        std::ifstream ifs;
        ifs.open(cfg_file, std::ios_base::in);
        if (!ifs.is_open()) {
            printf("ERROR: open config file [%s] failed.\nIt will be use default values.\n", cfg_file);
            return;
        }

        char buf[1024] = {0};
        char key[20];
        float values;
        while (ifs.getline(buf, sizeof(buf)))
        {
            sscanf(buf, "%s = %f", key, &values);
            if (strcmp(key, "lat_start") == 0) {
                _lat[0] = values;
            } else if (strcmp(key, "lat_end") == 0) {
                _lat[1] = values;
            } else if (strcmp(key, "lng_start") == 0) {
                _lng[0] = values;
            } else if (strcmp(key, "lng_end") == 0) {
                _lng[1] = values;
            }
        }
        ifs.close();
    }
}

bool MobileManager::check_device(const char* udid)
{
    if (IDEVICE_E_SUCCESS != idevice_new_with_options(&m_device, udid, m_use_network ? IDEVICE_LOOKUP_NETWORK : IDEVICE_LOOKUP_USBMUX))
    {
        if (udid) {
            set_error_message("未找到设备。");
        } else {
            set_error_message("未找到设备。");
        }

        return false;
    }

    return true;
}

bool MobileManager::is_connected()
{
    if (!m_client) {
        set_error_message("请先连接设备。");
        return false;
    }
    return true;  
}

bool MobileManager::connect_device()
{
    if (!m_device) {
        set_error_message("设备未初始化。");
        return false;
    }

    lockdownd_error_t err = lockdownd_client_new_with_handshake(m_device, &m_client, TOOL_NAME);
    if (LOCKDOWN_E_SUCCESS != err) 
    {
        char msg[512] = {0};
        sprintf(msg, "无法连接到lockdown: %s (%d)", lockdownd_strerror(err), err);    
        set_error_message(msg);
        return false;
    }

    return true;
}

void MobileManager::print_device_info()
{
    if (!is_connected()) return;

    const char *key = NULL;
    plist_t node = NULL;

    if (LOCKDOWN_E_SUCCESS == lockdownd_get_value(m_client, NULL, key, &node))
    {
        if (node) 
        {
            plist_dict_iter it = NULL;
            char* key = NULL;
            plist_t subnode = NULL;

            fprintf(stdout, "------------- Device info -------------\n");
            plist_dict_new_iter(node, &it);
            plist_dict_next_item(node, it, &key, &subnode);
            while (subnode)
            {
                if (is_key_wants(key))
                {
                    fprintf(stdout, "%s", key);
                    if (plist_get_node_type(subnode) == PLIST_ARRAY)
                        fprintf(stdout, "[%d]: ", plist_array_get_size(subnode));
                    else
                        fprintf(stdout, ": ");
                    free(key);
                    key = NULL;
                    plist_node_print_to_stream(subnode, 0, stdout);

                }

                plist_dict_next_item(node, it, &key, &subnode);
            }
            free(it);
            fprintf(stdout, "---------------------------------------\n");
        }
    }
}

double MobileManager::random_range(float start, float end)
{
    return start + rand() / double(RAND_MAX / (end - start));
}

void MobileManager::get_random_location(double* wgs_lat, double* wgs_lng)
{
    double lat = random_range(_lat[0], _lat[1]);
	double lng = random_range(_lng[0], _lng[1]);

    gcj2wgs(lat, lng, wgs_lat, wgs_lng);
}

bool MobileManager::simulate_location(bool reset, double* lng, double* lat)
{
    if (!is_connected()) return false;

    srand((unsigned)time(NULL));

    lockdownd_service_descriptor_t svc = NULL;
    lockdownd_error_t err = lockdownd_start_service(m_client, DT_SIMULATELOCATION_SERVICE, &svc);
    if (LOCKDOWN_E_SUCCESS != err)
    {
        char msg[512] = {0};
        sprintf(msg, "无法开启位置模拟服务，请确认开发镜像已挂载: %s (%d)", lockdownd_strerror(err), err); 
        set_error_message(msg);

        //printf("ERROR: Could not start the simulatelocation service: %s\nMake sure a developer disk image is mounted!\n", lockdownd_strerror(err));
        //printf("INFO: Auto mounting developer disk image....\n");
        if (!mount_image()) {
            return false;
        } else {
            // Try again
            err = lockdownd_start_service(m_client, DT_SIMULATELOCATION_SERVICE, &svc);
            if (LOCKDOWN_E_SUCCESS != err)
            {
                sprintf(msg, "无法开启位置模拟服务: %s (%d)", lockdownd_strerror(err), err);    
                set_error_message(msg);

                return false;
            }
        }
    }

    service_client_t service = NULL;
	service_error_t serr = service_client_new(m_device, svc, &service);

    uint32_t l;
	uint32_t s = 0;

	l = htobe32(0);
	serr = service_send(service, (const char*)&l, 4, &s);
	if (!reset) {
        double wgs_lat, wgs_lng;
        get_random_location(&wgs_lat, &wgs_lng);

        char lat_str[15] = {0};
		sprintf(lat_str, "%6f", wgs_lat);
        char lng_str[15] = {0};
		sprintf(lng_str, "%6f", wgs_lng);

        *lng = wgs_lng;
        *lat = wgs_lat;

        uint32_t latlen = strlen(lat_str);
        uint32_t longlen = strlen(lng_str);

		int len = 4 + latlen + 4 + longlen;
		char *buf = (char *)malloc(len);
		
		l = htobe32(latlen);
		memcpy(buf, &l, 4);
		memcpy(buf + 4, lat_str, latlen);
		
		l = htobe32(longlen);
		memcpy(buf + 4 + latlen, &l, 4);
		memcpy(buf + 4 + latlen + 4, lng_str, longlen);

		s = 0;
		serr = service_send(service, buf, len, &s);
	}

    return serr == SERVICE_E_SUCCESS;
}

static ssize_t mim_upload_cb(void* buf, size_t size, void* userdata)
{
	return fread(buf, 1, size, (FILE*)userdata);
}

bool MobileManager::mount_image()
{
    if (!is_connected()) return false;
    
    plist_t pver = NULL;
	char *product_version = NULL;
	lockdownd_get_value(m_client, NULL, "ProductVersion", &pver);
	if (pver && plist_get_node_type(pver) == PLIST_STRING) {
		plist_get_string_val(pver, &product_version);
	}
    
    disk_image_upload_type_t disk_image_upload_type = DISK_IMAGE_UPLOAD_TYPE_AFC;
	int product_version_major = 0;
	int product_version_minor = 0;
	if (product_version) {
		if (sscanf(product_version, "%d.%d.%*d", &product_version_major, &product_version_minor) == 2) {
			if (product_version_major >= 7)
				disk_image_upload_type = DISK_IMAGE_UPLOAD_TYPE_UPLOAD_IMAGE;
		}
	}

    lockdownd_service_descriptor_t service = NULL;
    lockdownd_start_service(m_client, "com.apple.mobile.mobile_image_mounter", &service);
    if (!service || service->port == 0) {
		set_error_message("无法开启mobile_image_mounter服务");
		return false;
	}

    mobile_image_mounter_client_t mim = NULL;
	if (mobile_image_mounter_new(m_device, service, &mim) != MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
        set_error_message("无法连接到mobile_image_mounter服务");
		return false;
	}

    if (service) {
        lockdownd_service_descriptor_free(service);
        service = NULL;
    }

    struct stat fst;
    afc_client_t afc = NULL;

    if (disk_image_upload_type == DISK_IMAGE_UPLOAD_TYPE_AFC) 
    {
        if ((lockdownd_start_service(m_client, "com.apple.afc", &service) != LOCKDOWN_E_SUCCESS) || !service || !service->port) 
        {
            set_error_message("无法启动com.apple.afc服务");
            return false;
        }

        if (afc_client_new(m_device, service, &afc) != AFC_E_SUCCESS) {
            set_error_message("无法连接到AFC服务");
            return false;
        }

        if (service) {
            lockdownd_service_descriptor_free(service);
            service = NULL;
        }
    }

    char image_path[256] = { 0 };
    sprintf(image_path, "./drivers/%d.%d/%s", product_version_major, product_version_minor, "DeveloperDiskImage.dmg");

    if (stat(image_path, &fst) != 0) {
        set_error_message("获取镜像文件状态失败。");
        return false;
    }

    char image_sig_path[256] = { 0 };
    sprintf(image_sig_path, "./drivers/%d.%d/%s", product_version_major, product_version_minor, "DeveloperDiskImage.dmg.signature");

    size_t image_size = fst.st_size;
    if (stat(image_sig_path, &fst) != 0) {
        set_error_message("获取签名文件状态失败。");
        return false;
    }
    
    static const char *imagetype = NULL;
    mobile_image_mounter_error_t err = MOBILE_IMAGE_MOUNTER_E_UNKNOWN_ERROR;
    char sig[8192];
    size_t sig_length = 0;
    FILE *f = fopen(image_sig_path, "rb");
    if (!f) {
        // fprintf(stderr, "Error opening signature file '%s': %s\n", image_sig_path, strerror(errno));
        set_error_message("打开签名文件失败。");
        return false;
    }

    sig_length = fread(sig, 1, sizeof(sig), f);
    fclose(f);
    if (sig_length == 0) {
        // fprintf(stderr, "Could not read signature from file '%s'\n", image_sig_path);
        set_error_message("读取签名文件失败。");
        return false;
    }

    f = fopen(image_path, "rb");
    if (!f) {
        // fprintf(stderr, "Error opening image file '%s': %s\n", image_path, strerror(errno));
        set_error_message("打开镜像文件失败。");
        return false;
    }

    char *targetname = NULL;
    if (asprintf(&targetname, "%s/%s", PKG_PATH, "staging.dimage") < 0) {
        // fprintf(stderr, "Out of memory!?\n");
        set_error_message("内存不足！");
        return false;
    }
    char *mountname = NULL;
    if (asprintf(&mountname, "%s/%s", PATH_PREFIX, targetname) < 0) {
        // fprintf(stderr, "Out of memory!?\n");
        set_error_message("内存不足！");
        return false;
    }

    if (!imagetype) {
        imagetype = "Developer";
    }

    switch(disk_image_upload_type) {
        case DISK_IMAGE_UPLOAD_TYPE_UPLOAD_IMAGE:
            printf("Uploading %s\n", image_path);
            err = mobile_image_mounter_upload_image(mim, imagetype, image_size, sig, sig_length, mim_upload_cb, f);
            break;
        case DISK_IMAGE_UPLOAD_TYPE_AFC:
        default:
            printf("Uploading %s --> afc:///%s\n", image_path, targetname);
            char **strs = NULL;
            if (afc_get_file_info(afc, PKG_PATH, &strs) != AFC_E_SUCCESS) {
                if (afc_make_directory(afc, PKG_PATH) != AFC_E_SUCCESS) {
                    fprintf(stderr, "WARNING: Could not create directory '%s' on device!\n", PKG_PATH);
                }
            }
            if (strs) {
                int i = 0;
                while (strs[i]) {
                    free(strs[i]);
                    i++;
                }
                free(strs);
            }

            uint64_t af = 0;
            if ((afc_file_open(afc, targetname, AFC_FOPEN_WRONLY, &af) !=
                    AFC_E_SUCCESS) || !af) {
                fclose(f);
                // fprintf(stderr, "afc_file_open on '%s' failed!\n", targetname);
                set_error_message("afc_file_open失败！");
                return false;
            }

            char buf[8192];
            size_t amount = 0;
            do {
                amount = fread(buf, 1, sizeof(buf), f);
                if (amount > 0) {
                    uint32_t written, total = 0;
                    while (total < amount) {
                        written = 0;
                        if (afc_file_write(afc, af, buf + total, amount - total, &written) !=
                            AFC_E_SUCCESS) {
                            // fprintf(stderr, "AFC Write error!\n");
                            set_error_message("AFC写错误！");
                            break;
                        }
                        total += written;
                    }
                    if (total != amount) {
                        // fprintf(stderr, "Error: wrote only %d of %d\n", total,
                        //         (unsigned int)amount);
                        set_error_message("AFC写入不完整错误！");
                        afc_file_close(afc, af);
                        fclose(f);
                        return false;
                    }
                }
            }
            while (amount > 0);

            afc_file_close(afc, af);
            break;
    }

    fclose(f);

    if (err != MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
        if (err == MOBILE_IMAGE_MOUNTER_E_DEVICE_LOCKED) {
            // printf("ERROR: Device is locked, can't mount. Unlock device and try again.\n");
            set_error_message("设备已锁屏，无法挂载。请解锁后重试。");
        } else {
            // printf("ERROR: Unknown error occurred, can't mount.\n");
            set_error_message("未知错误，无法挂载。");
        }
        return false;
    }
    // printf("done.\n");

    plist_t result = NULL;
    int res = -1;

    // printf("Mounting...\n");
    err = mobile_image_mounter_mount_image(mim, mountname, sig, sig_length, imagetype, &result);
    if (err == MOBILE_IMAGE_MOUNTER_E_SUCCESS) {
        if (result) {
            plist_t node = plist_dict_get_item(result, "Status");
            if (node) {
                char *status = NULL;
                plist_get_string_val(node, &status);
                if (status) {
                    if (!strcmp(status, "Complete")) {
                        printf("Done.\n");
                        res = 0;
                    } else {
                        printf("unexpected status value:\n");
                        plist_node_print_to_stream(result, 0, stdout);
                    }
                    free(status);
                } else {
                    printf("unexpected result:\n");
                    plist_node_print_to_stream(result, 0, stdout);
                }
            }
            node = plist_dict_get_item(result, "Error");
            if (node) {
                char *error = NULL;
                plist_get_string_val(node, &error);
                if (error) {
                    printf("Error: %s\n", error);
                    free(error);
                } else {
                    printf("unexpected result:\n");
                    plist_node_print_to_stream(result, 0, stdout);
                }

            } else {
                plist_node_print_to_stream(result, 0, stdout);
            }
        }
        return true;
    } else {
        char msg[512] = {0};
        sprintf(msg, "挂载镜像失败，错误码: %d", err);    
        set_error_message(msg);
        return false;
    }    
}
