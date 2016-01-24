/* 
 * File:   newmain.c
 * Author: vasily
 *
 * Created on January 23, 2016, 11:07 PM
 */

// LOCAL_CFLAGS += -fPIE
// LOCAL_LDFLAGS += -fPIE -pie -llog

//on init
//    export LD_PRELOAD /system/lib/libhookril.so

//#define MTK_RIL

#include <android/log.h>
#include "telephony/ril.h"
#include <dlfcn.h>
#include <string.h>

#define  LOG_TAG    "hookril"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

void inner_RIL_RequestFunc(int request, void *data, size_t datalen, RIL_Token t);
RIL_RadioState inner_RIL_RadioStateRequest(
#ifdef MTK_RIL 
RILId rid, int *sim_status
#endif
);

RIL_RadioFunctions* orig_RIL_RadioFunctions;
//RIL_RequestFunc orig_RIL_RequestFunc;
//RIL_RadioStateRequest orig_RIL_RadioStateRequest;


/****************************   Base functions   ****************************/

void RIL_register (const RIL_RadioFunctions *callbacks) {
    LOGD("Entered to %s", "RIL_register.");
    void (*orig_RIL_register)(const RIL_RadioFunctions *callbacks);
    orig_RIL_register = dlsym(RTLD_NEXT, "RIL_register");
//    orig_RIL_RequestFunc = callbacks->onRequest;
//    orig_RIL_RadioStateRequest = callbacks->onStateRequest;
    orig_RIL_RadioFunctions = callbacks;
    RIL_RadioFunctions* new_RIL_RadioFunctions = (RIL_RadioFunctions*)malloc(sizeof(RIL_RadioFunctions));
    memcpy(new_RIL_RadioFunctions, callbacks, sizeof(RIL_RadioFunctions));
    new_RIL_RadioFunctions->onRequest = &inner_RIL_RequestFunc;
    new_RIL_RadioFunctions->onStateRequest = &inner_RIL_RadioStateRequest;
    orig_RIL_register(new_RIL_RadioFunctions);
}

void RIL_onRequestComplete(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
    LOGD("Entered to %s", "RIL_onRequestComplete.");
    void (*org_RIL_onRequestComplete)(RIL_Token t, RIL_Errno e, void *response, size_t responselen);
    org_RIL_onRequestComplete = dlsym(RTLD_NEXT, "RIL_onRequestComplete");
    org_RIL_onRequestComplete(t, e, response, responselen);
}

void RIL_onUnsolicitedResponse(int unsolResponse, const void *data, size_t datalen
#ifdef MTK_RIL
, RILId id
#endif
) {
    LOGD("Entered to %s", "RIL_onUnsolicitedResponse.");
    void (*org_RIL_onUnsolicitedResponse)(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime
#ifdef MTK_RIL
, RILId id
#endif
    );
    org_RIL_onUnsolicitedResponse = dlsym(RTLD_NEXT, "RIL_onUnsolicitedResponse");
    org_RIL_onUnsolicitedResponse(unsolResponse, data, datalen
#ifdef MTK_RIL
, id
#endif
    );
}

void RIL_requestTimedCallback(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime) {
    LOGD("Entered to %s", "RIL_requestTimedCallback.");
    void (*org_RIL_requestTimedCallback)(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime);
    org_RIL_requestTimedCallback = dlsym(RTLD_NEXT, "RIL_requestTimedCallback");
    org_RIL_requestTimedCallback(callback, param, relativeTime);
}


/****************************   Vendor functions   ****************************/

void inner_RIL_RequestFunc(int request, void *data, size_t datalen, RIL_Token t) {
    LOGD("Entered to %s", "RIL_RequestFunc.");
    orig_RIL_RadioFunctions->onRequest(request, data, datalen, t);
}

RIL_RadioState inner_RIL_RadioStateRequest(
#ifdef MTK_RIL 
RILId rid, int *sim_status
#endif
) {
    LOGD("Entered to %s", "RIL_RadioStateRequest.");
    return orig_RIL_RadioFunctions->onStateRequest(
#ifdef MTK_RIL 
rid, sim_status
#endif
    );
}