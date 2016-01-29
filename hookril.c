
//#define MTK_RIL

#define LIB_PATH_PROPERTY_NEW   "rild.libpath"
#define LIB_PATH_PROPERTY_ORIG   "rild.libpath_orig"

#include "telephony/ril.h"
#include <dlfcn.h>
#include <string.h>
#include <sys/system_properties.h>
#include <stdlib.h>

#include "logger.h"
#include "transmitter.h"

void inner_RIL_RequestFunc(int request, void *data, size_t datalen, RIL_Token t);
RIL_RadioState inner_RIL_RadioStateRequest(
#ifdef MTK_RIL 
RILId rid, int *sim_status
#endif
);
void RIL_onRequestComplete(RIL_Token t, RIL_Errno e, void *response, size_t responselen);
void RIL_onUnsolicitedResponse(int unsolResponse, const void *data, size_t datalen
#ifdef MTK_RIL
, RILId id
#endif
);
void RIL_onRequestTimedCallback(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime);
#ifdef MTK_RIL
void RIL_onRequestProxyTimedCallback(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime, int proxyId);
RILChannelId RIL_onQueryMyChannelId(RIL_Token t);
int RIL_onQueryMyProxyIdByThread();
#endif


struct RIL_Env orig_RIL_Env;
RIL_RadioFunctions orig_RIL_RadioFunctions;
//RIL_RequestFunc orig_RIL_RequestFunc;
//RIL_RadioStateRequest orig_RIL_RadioStateRequest;


struct RIL_Env s_rilEnv = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponse,
    RIL_onRequestTimedCallback
#ifdef MTK_RIL
    , RIL_onRequestProxyTimedCallback,
    RIL_onQueryMyChannelId,
    RIL_onQueryMyProxyIdByThread
#endif
};
RIL_RadioFunctions base_RIL_RadioFunctions;
//const char* (*requestToString)(int request);

/****************************   Lib functions   ****************************/

const RIL_RadioFunctions* RIL_Init(const struct RIL_Env *env, int argc, char **argv) {
    char rilLibPath[PROP_VALUE_MAX];
    
    SLOGD("Start hookril init\n");
    
    if ( 0 == __system_property_get(LIB_PATH_PROPERTY_ORIG, rilLibPath)) {
        SLOGE("No vendor so");
        return NULL;
    }

    void* dlHandle = dlopen(rilLibPath, RTLD_NOW);

    if (dlHandle == NULL) {
        LOGE("dlopen failed: %s", dlerror());
        return NULL;
    }

    SLOGD("Try to find vendor library init function");
    const RIL_RadioFunctions *(*rilInit)(const struct RIL_Env *, int, char **);
    rilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))dlsym(dlHandle, "RIL_Init");
    
    if (rilInit == NULL) {
        LOGE("RIL_Init not defined or exported in %s", rilLibPath);
        return NULL;
    }
    SLOGD("Vendor library init function found");
    
    memcpy(&orig_RIL_Env, env, sizeof(struct RIL_Env));
    
    SLOGD("Try to call vendor library init function");
    const RIL_RadioFunctions *funcs;
    funcs = rilInit(&s_rilEnv, argc, argv);
    if(funcs == NULL) {
        SLOGE("RIL_Init vendor function call error");
        return NULL;
    }
    SLOGD("Vendor library init function pass");
    
//#ifdef MTK_RIL
//    void* librilmtkHandle = dlopen("/system/lib/librilmtk.so", RTLD_NOW);
//    requestToString = (const char* (*)(int))dlsym(librilmtkHandle, "requestToString");
//#else
//    void* librilmtkHandle = dlopen("/system/lib/libril.so", RTLD_NOW);
//    requestToString = (const char* (*)(int))dlsym(librilmtkHandle, "requestToString");
//#endif
//    if(requestToString == NULL) {
//        LOGE("requestToString function symbol looking error");
//        return NULL;
//    }
    
    if(initSocket() != 0) {
        SLOGE("Transmitter initializations fail");
        return NULL;
    }
    
    memcpy(&orig_RIL_RadioFunctions, funcs, sizeof(RIL_RadioFunctions));
    memcpy(&base_RIL_RadioFunctions, &orig_RIL_RadioFunctions, sizeof(RIL_RadioFunctions));
    base_RIL_RadioFunctions.onRequest = &inner_RIL_RequestFunc;
    base_RIL_RadioFunctions.onStateRequest = &inner_RIL_RadioStateRequest;
    return &base_RIL_RadioFunctions;
}

/****************************   Base functions   ****************************/

void RIL_onRequestComplete(RIL_Token t, RIL_Errno e, void *response, size_t responselen) {
    SLOGD("Entered to RIL_onRequestComplete.");
    orig_RIL_Env.OnRequestComplete(t, e, response, responselen);
    
    if(!isTransmittionActive())
        return;

    int32_t token = *(int32_t*)t;
    TransData* transData = malloc(sizeof(TransDataHeader) + responselen);
    transData->header.funcIdentifier = 2;
    transData->header.command = -1;
    transData->header.token = token;
    transData->header.datalen = responselen;
    memcpy(transData->data, response, responselen);
    putNextData(transData);
}

void RIL_onUnsolicitedResponse(int unsolResponse, const void *data, size_t datalen
#ifdef MTK_RIL
, RILId id
#endif
) {
    SLOGD("Entered to RIL_onUnsolicitedResponse.");
    orig_RIL_Env.OnUnsolicitedResponse(unsolResponse, data, datalen
#ifdef MTK_RIL
, id
#endif
    );
    
    if(!isTransmittionActive())
        return;

    TransData* transData = malloc(sizeof(TransDataHeader) + datalen);
    transData->header.funcIdentifier = 3;
    transData->header.command = unsolResponse;
    transData->header.token = -1;
    transData->header.datalen = datalen;
    memcpy(transData->data, data, datalen);
    putNextData(transData);
}

void RIL_onRequestTimedCallback(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime) {
    SLOGD("Entered to RIL_requestTimedCallback.");
    orig_RIL_Env.RequestTimedCallback(callback, param, relativeTime);
}

#ifdef MTK_RIL
void RIL_onRequestProxyTimedCallback(RIL_TimedCallback callback, void *param, const struct timeval *relativeTime, int proxyId) {
    sLOGD("Entered to RIL_onRequestProxyTimedCallback.");
    orig_RIL_Env.RequestProxyTimedCallback(callback, param, relativeTime, proxyId);
}

RILChannelId RIL_onQueryMyChannelId(RIL_Token t) {
    sLOGD("Entered to RIL_onQueryMyChannelId.");
    return orig_RIL_Env.QueryMyChannelId(t);
}

int RIL_onQueryMyProxyIdByThread() {
    sLOGD("Entered to RIL_onQueryMyProxyIdByThread.");
    return orig_RIL_Env.QueryMyProxyIdByThread();
}
#endif

/****************************   Vendor functions   ****************************/

void inner_RIL_RequestFunc(int request, void *data, size_t datalen, RIL_Token t) {
    SLOGD("Entered to RIL_RequestFunc.");
    orig_RIL_RadioFunctions.onRequest(request, data, datalen, t);

    if(!isTransmittionActive())
        return;

    int32_t token = *(int32_t*)t;
    TransData* transData = malloc(sizeof(TransDataHeader) + datalen);
    transData->header.funcIdentifier = 1;
    transData->header.command = request;
    transData->header.token = token;
    transData->header.datalen = datalen;
    memcpy(transData->data, data, datalen);
    putNextData(transData);
}

RIL_RadioState inner_RIL_RadioStateRequest(
#ifdef MTK_RIL 
RILId rid, int *sim_status
#endif
) {
    RIL_RadioState state = orig_RIL_RadioFunctions.onStateRequest(
#ifdef MTK_RIL 
rid, sim_status
#endif
    );
    LOGD("Entered to RIL_RadioStateRequest. State is %d", state);
    
    if(!isTransmittionActive())
        return state;

    int32_t token = state;
    TransData* transData = malloc(sizeof(TransDataHeader));
    transData->header.funcIdentifier = 4;
    transData->header.command = -1;
    transData->header.token = token;
    transData->header.datalen = 0;
    putNextData(transData);

    return state;
}
