#ifndef ANDROID_AUDIO_ASSERT_H
#define ANDROID_AUDIO_ASSERT_H

#ifndef DB_OPT_DEFAULT
#define DB_OPT_DEFAULT (0)
#endif

#define ASSERT(exp) \
    do { \
        if (!(exp)) { \
            ALOGE("ASSERT("#exp") fail: \""  __FILE__ "\", %uL", __LINE__); \
        } \
    } while(0)

#define WARNING(string) \
    do { \
        ALOGW("WARNING("string") fail: \""  __FILE__ "\", %uL", __LINE__); \
    } while(0)



#endif // end of ANDROID_AUDIO_ASSERT_H
