#ifndef LOGGER_H
#define	LOGGER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <android/log.h>
#define  LOG_TAG    "hookril"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  SLOGI(STR)  __android_log_write(ANDROID_LOG_INFO,LOG_TAG,STR)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  SLOGW(STR)  __android_log_write(ANDROID_LOG_WARN,LOG_TAG,STR)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  SLOGE(STR)  __android_log_write(ANDROID_LOG_ERROR,LOG_TAG,STR)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  SLOGD(STR)  __android_log_write(ANDROID_LOG_DEBUG,LOG_TAG,STR)
#ifdef	__cplusplus
}
#endif

#endif	/* LOGGER_H */

