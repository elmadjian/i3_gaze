#include <tobii/tobii.h>
#include <tobii/tobii_streams.h>
#include <stdio.h>
#include <assert.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>


using namespace std;
  
static void gaze_point_callback(tobii_gaze_point_t const *gaze_point, void *user_data) {
   if (gaze_point->validity == TOBII_VALIDITY_VALID) {
        float x = gaze_point->position_xy[0];
        float y = gaze_point->position_xy[1];
        string buffer = "g~"+to_string(x) + '~' + to_string(y) +'\n';
        const char *cstr = buffer.c_str();
        char *msg = (char *) user_data;
        strcpy(msg, cstr);
    }
}

static void head_pose_callback(tobii_head_pose_t const *head_pose, void *user_data) {
    if (head_pose->position_validity == TOBII_VALIDITY_VALID) {
        float xyz[3];
        float x = head_pose->position_xyz[0];
        float y = head_pose->position_xyz[1];
        float z = head_pose->position_xyz[2];
        for (int i = 0; i < 3; i++) {
            if (head_pose->rotation_validity_xyz[i] == TOBII_VALIDITY_VALID)
                xyz[i] = head_pose->rotation_xyz[i];
        }
        string buffer = "h~"+to_string(x)+'~'+to_string(y)+'~'+to_string(z);
        buffer += "~"+to_string(xyz[0])+'~'+to_string(xyz[1])+'~'+to_string(xyz[2])+'\n';
        const char *cstr = buffer.c_str();
        char *msg = (char *) user_data;
        strcpy(msg, cstr);
    }
}

static void url_receiver(char const *url, void *user_data) {
    char *buffer = (char *) user_data;
    if (*buffer != '\0') return; // only keep first value

    if (strlen(url) < 256)
        strcpy(buffer, url);
}

int main() {
    struct sockaddr_in server;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server.sin_family = AF_INET;
    server.sin_port = htons(9998);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t len = sizeof(server);

    char gaze_msg[64];
    char head_msg[64];
    char url[256] = {0};
    tobii_api_t *api;
    tobii_error_t error = tobii_api_create(&api, NULL, NULL);
    assert(error == TOBII_ERROR_NO_ERROR);

    error = tobii_enumerate_local_device_urls(api, url_receiver, url);
    assert (error == TOBII_ERROR_NO_ERROR && *url != '\0');

    tobii_device_t *device;
    error = tobii_device_create(api, url, &device);
    assert(error == TOBII_ERROR_NO_ERROR);

    error = tobii_gaze_point_subscribe(device, gaze_point_callback, gaze_msg);
    assert(error == TOBII_ERROR_NO_ERROR);
    
    error = tobii_head_pose_subscribe(device, head_pose_callback, head_msg);
    assert(error == TOBII_ERROR_NO_ERROR);   

    
    while (true) {
        error = tobii_wait_for_callbacks(1, &device);
        assert(error == TOBII_ERROR_NO_ERROR || error == TOBII_ERROR_TIMED_OUT);

        error = tobii_device_process_callbacks(device);
        assert(error == TOBII_ERROR_NO_ERROR);

        sendto(sockfd, gaze_msg, sizeof(gaze_msg), 0, (struct sockaddr *)&server, len);
        sendto(sockfd, head_msg, sizeof(head_msg), 0, (struct sockaddr *)&server, len);
    }

    error = tobii_gaze_point_unsubscribe(device);
    assert(error == TOBII_ERROR_NO_ERROR);

    error = tobii_device_destroy(device);
    assert(error == TOBII_ERROR_NO_ERROR);

    error = tobii_api_destroy(api);
    assert(error == TOBII_ERROR_NO_ERROR);
    return 0;
}