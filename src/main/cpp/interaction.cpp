#include "Tox4j.h"


/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSetTyping
 * Signature: (IIZ)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSetTyping
  (JNIEnv *env, jclass, jint instanceNumber, jint, jboolean)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "toxSetTyping");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSendMessage
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSendMessage
  (JNIEnv *env, jclass, jint instanceNumber, jint, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "toxSendMessage");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSendAction
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSendAction
  (JNIEnv *env, jclass, jint instanceNumber, jint, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "toxSendAction");
    });
}