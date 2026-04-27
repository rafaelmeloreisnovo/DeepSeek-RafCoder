#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_rafcoder_app_MainActivity_nativeMessage(JNIEnv* env, jobject /* this */) {
    std::string msg = "RafCoder Native Core OK (armeabi-v7a + arm64-v8a)";
    return env->NewStringUTF(msg.c_str());
}
