#include <tox/core.h>
#include "Tox4j.h"

template<typename T> void unused(T const &) { }


void
throw_tox_exception(JNIEnv *env, char const *method, char const *code)
{
    std::string className = "im/tox/tox4j/api/exceptions/Tox";
    className += method;
    className += "Exception";
    jclass exClass = env->FindClass(className.c_str());
    assert(exClass);

    std::string enumName = className + "$Code";
    jclass enumClass = env->FindClass(enumName.c_str());
    assert(enumClass);

    std::string valueOfSig = "(Ljava/lang/String;)L" + enumName + ";";
    jmethodID valueOf = env->GetStaticMethodID(enumClass, "valueOf", valueOfSig.c_str());
    assert(valueOf);

    jobject enumCode = env->CallStaticObjectMethod(enumClass, valueOf, env->NewStringUTF(code));
    assert(enumCode);

    std::string constructorName = "(L" + enumName + ";)V";
    jmethodID constructor = env->GetMethodID(exClass, "<init>", constructorName.c_str());
    assert(constructor);

    jobject exception = env->NewObject(exClass, constructor, enumCode);
    assert(exception);

    env->Throw((jthrowable) exception);
}


/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    destroyAll
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_destroyAll
  (JNIEnv *, jclass)
{
    std::unique_lock<std::mutex> lock(ToxInstances::self.mutex);
    ToxInstances::self.destroyAll();
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxNew
 * Signature: (ZZILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxNew
  (JNIEnv *env, jclass, jboolean ipv6Enabled, jboolean udpEnabled, jint proxyType, jstring proxyAddress, jint proxyPort)
{
    auto opts = Tox_Options();
    opts.ipv6_enabled = ipv6Enabled;
    opts.udp_enabled = udpEnabled;
    opts.proxy_type = (TOX_PROXY_TYPE) proxyType; // TODO: validate

    std::vector<char> proxy_address;
    if (proxyAddress == nullptr) {
        opts.proxy_address = nullptr;
    } else {
        proxy_address = UTFChars(env, proxyAddress);
    }
    opts.proxy_port = proxyPort;

    TOX_ERR_NEW error;
    std::unique_ptr<Tox, ToxDeleter> tox(tox_new(&opts, &error));
    switch (error) {
        case TOX_ERR_NEW_OK: {
            assert(tox != nullptr);
            // We can create the new instance outside ToxInstances' critical section.
            Tox4jStruct instance {
                std::move(tox),
                std::unique_ptr<ToxEvents>(new ToxEvents),
                std::unique_ptr<std::mutex>(new std::mutex)
            };

            // This lock guards the instance manager.
            std::lock_guard<std::mutex> lock(ToxInstances::self.mutex);
            return ToxInstances::self.add(std::move(instance));
        }
        case TOX_ERR_NEW_MALLOC: {
            throw_tox_exception(env, "New", "MALLOC");
            return 0;
        }
        case TOX_ERR_NEW_PORT_ALLOC: {
            throw_tox_exception(env, "New", "PORT_ALLOC");
            return 0;
        }
        case TOX_ERR_NEW_PROXY_BAD_HOST: {
            throw_tox_exception(env, "New", "PROXY_BAD_HOST");
            return 0;
        }
        case TOX_ERR_NEW_PROXY_BAD_PORT: {
            throw_tox_exception(env, "New", "PROXY_BAD_PORT");
            return 0;
        }
        case TOX_ERR_NEW_PROXY_NOT_FOUND: {
            throw_tox_exception(env, "New", "PROXY_NOT_FOUND");
            return 0;
        }
    }

    throw_illegal_state_exception(env, error, "Unknown error code");
    return 0;
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxKill
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxKill
  (JNIEnv *env, jclass, jint instanceNumber)
{
    std::lock_guard<std::mutex> lock(ToxInstances::self.mutex);

    if (instanceNumber < 0) {
        throw_illegal_state_exception(env, instanceNumber, "Tox instance out of range");
        return;
    }

    if (!ToxInstances::self.isValid(instanceNumber)) {
        throw_tox_killed_exception(env, instanceNumber, "close called on invalid instance");
        return;
    }

    // After this move, the pointers in instance_vector[instance_number] will all be nullptr...
    Tox4jStruct dying(ToxInstances::self.remove(instanceNumber));

    // ... so that this check will fail, if the function is called twice on the same instance.
    if (!dying.tox) {
        throw_tox_killed_exception(env, instanceNumber, "close called on already closed instance");
        return;
    }

    std::lock_guard<std::mutex> ilock(*dying.mutex);
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    finalize
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_finalize
  (JNIEnv *env, jclass, jint instanceNumber)
{
    if (instanceNumber == 0) {
        // This can happen when an exception is thrown from the constructor, giving this object an invalid state,
        // containing instanceNumber = 0.
        return;
    }

    if (ToxInstances::self.empty()) {
        throw_illegal_state_exception(env, instanceNumber, "Tox instance manager is empty");
        return;
    }

    std::lock_guard<std::mutex> lock(ToxInstances::self.mutex);
    if (!ToxInstances::self.isValid(instanceNumber)) {
        throw_illegal_state_exception(env, instanceNumber,
            "Tox instance out of range (max: " + std::to_string(ToxInstances::self.size() - 1) + ")");
        return;
    }

    // An instance should never be on this list twice.
    if (ToxInstances::self.isFree(instanceNumber)) {
        throw_illegal_state_exception(env, instanceNumber, "Tox instance already on free list");
        return;
    }

    ToxInstances::self.setFree(instanceNumber);
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSave
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSave
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        std::vector<uint8_t> buffer(tox_save_size(tox));
        tox_save(tox, buffer.data());

        return toByteArray(env, buffer);
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxLoad
 * Signature: (I[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxLoad
  (JNIEnv *env, jclass, jint instanceNumber, jbyteArray data)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        ByteArray bytes(env, data);
        TOX_ERR_LOAD error;
        tox_load(tox, bytes.data(), bytes.length(), &error);
        switch (error) {
            case TOX_ERR_LOAD_OK:
                return;
            case TOX_ERR_LOAD_NULL:
                throw_tox_exception(env, "Load", "NULL");
                return;
            case TOX_ERR_LOAD_ENCRYPTED:
                throw_tox_exception(env, "Load", "ENCRYPTED");
                return;
            case TOX_ERR_LOAD_BAD_FORMAT:
                throw_tox_exception(env, "Load", "BAD_FORMAT");
                return;
        }

        throw_illegal_state_exception(env, error, "Unknown error code");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxBootstrap
 * Signature: (ILjava/lang/String;I[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxBootstrap
  (JNIEnv *env, jclass, jint instanceNumber, jstring address, jint port, jbyteArray public_key)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        TOX_ERR_BOOTSTRAP error;
        tox_bootstrap(tox, UTFChars(env, address), port, ByteArray(env, public_key), &error);
        switch (error) {
            case TOX_ERR_BOOTSTRAP_OK:
                return;
            case TOX_ERR_BOOTSTRAP_NULL:
                throw_tox_exception(env, "Bootstrap", "NULL");
                return;
            case TOX_ERR_BOOTSTRAP_BAD_ADDRESS:
                throw_tox_exception(env, "Bootstrap", "BAD_ADDRESS");
                return;
            case TOX_ERR_BOOTSTRAP_BAD_PORT:
                throw_tox_exception(env, "Bootstrap", "BAD_PORT");
                return;
        }

        throw_illegal_state_exception(env, error, "Unknown error code");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetPort
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetPort
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return 0;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxIterationTime
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxIterationTime
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        return tox_iteration_time(tox);
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxIteration
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxIteration
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        tox_iteration(tox);

        std::vector<char> buffer(events.ByteSize());
        events.SerializeToArray(buffer.data(), buffer.size());
        events.Clear();

        return toByteArray(env, buffer);
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetSelfClientId
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetSelfClientId
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return nullptr;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetSecretKey
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetSecretKey
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return nullptr;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSetNospam
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSetNospam
  (JNIEnv *env, jclass, jint instanceNumber, jint)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetNospam
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetNospam
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return 0;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetAddress
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetAddress
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        std::vector<uint8_t> address(TOX_ADDRESS_SIZE);
        tox_get_address(tox, address.data());

        return toByteArray(env, address);
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSetName
 * Signature: (I[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSetName
  (JNIEnv *env, jclass, jint instanceNumber, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetName
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetName
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return nullptr;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSetStatusMessage
 * Signature: (I[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSetStatusMessage
  (JNIEnv *env, jclass, jint instanceNumber, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetStatusMessage
 * Signature: (I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetStatusMessage
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return nullptr;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSetStatus
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSetStatus
  (JNIEnv *env, jclass, jint instanceNumber, jint)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetStatus
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetStatus
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return 0;
    });
}


static uint32_t
handle_tox_add_friend_result(JNIEnv *env, jint instanceNumber, uint32_t friend_number, TOX_ERR_ADD_FRIEND error)
{
    switch (error) {
        case TOX_ERR_ADD_FRIEND_OK:
            return friend_number;

        case TOX_ERR_ADD_FRIEND_NULL:
            throw_tox_exception(env, "AddFriend", "NULL");
            return 0;
        case TOX_ERR_ADD_FRIEND_TOO_LONG:
            throw_tox_exception(env, "AddFriend", "TOO_LONG");
            return 0;
        case TOX_ERR_ADD_FRIEND_NO_MESSAGE:
            throw_tox_exception(env, "AddFriend", "NO_MESSAGE");
            return 0;
        case TOX_ERR_ADD_FRIEND_OWN_KEY:
            throw_tox_exception(env, "AddFriend", "OWN_KEY");
            return 0;
        case TOX_ERR_ADD_FRIEND_ALREADY_SENT:
            throw_tox_exception(env, "AddFriend", "ALREADY_SENT");
            return 0;
        case TOX_ERR_ADD_FRIEND_BAD_CHECKSUM:
            throw_tox_exception(env, "AddFriend", "BAD_CHECKSUM");
            return 0;
        case TOX_ERR_ADD_FRIEND_SET_NEW_NOSPAM:
            throw_tox_exception(env, "AddFriend", "SET_NEW_NOSPAM");
            return 0;
        case TOX_ERR_ADD_FRIEND_MALLOC:
            throw_tox_exception(env, "AddFriend", "MALLOC");
            return 0;
    }

    throw_illegal_state_exception(env, error, "Unknown error code");
    return 0;
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxAddFriend
 * Signature: (I[B[B)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxAddFriend
  (JNIEnv *env, jclass, jint instanceNumber, jbyteArray address, jbyteArray message)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        TOX_ERR_ADD_FRIEND error;
        ByteArray messageBytes(env, message);
        uint32_t friend_number = tox_add_friend(tox, ByteArray(env, address), messageBytes.data(), messageBytes.length(), &error);
        return handle_tox_add_friend_result(env, instanceNumber, friend_number, error);
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxAddFriendNorequest
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxAddFriendNorequest
  (JNIEnv *env, jclass, jint instanceNumber, jbyteArray clientId)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        TOX_ERR_ADD_FRIEND error;
        uint32_t friend_number = tox_add_friend_norequest(tox, ByteArray(env, clientId), &error);
        return handle_tox_add_friend_result(env, instanceNumber, friend_number, error);
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxDeleteFriend
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxDeleteFriend
  (JNIEnv *env, jclass, jint instanceNumber, jint)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetFriendNumber
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetFriendNumber
  (JNIEnv *env, jclass, jint instanceNumber, jbyteArray clientId)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        TOX_ERR_GET_FRIEND_NUMBER error;
        uint32_t friend_number = tox_get_friend_number(tox, ByteArray(env, clientId), &error);
        switch (error) {
            case TOX_ERR_GET_FRIEND_NUMBER_OK:
                return friend_number;
            case TOX_ERR_GET_FRIEND_NUMBER_NULL:
                throw_tox_exception(env, "GetFriendNumber", "NULL");
                return 0u;
            case TOX_ERR_GET_FRIEND_NUMBER_NOT_FOUND:
                throw_tox_exception(env, "GetFriendNumber", "NOT_FOUND");
                return 0u;
        }

        throw_illegal_state_exception(env, error, "Unknown error code");
        return 0u;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetFriendClientId
 * Signature: (II)[B
 */
JNIEXPORT jbyteArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetFriendClientId
  (JNIEnv *env, jclass, jint instanceNumber, jint friendNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        TOX_ERR_GET_CLIENT_ID error;
        std::vector<uint8_t> buffer(TOX_CLIENT_ID_SIZE);
        jbyteArray result = nullptr;
        tox_get_friend_client_id(tox, friendNumber, buffer.data(), &error);
        switch (error) {
            case TOX_ERR_GET_CLIENT_ID_OK:
                result = toByteArray(env, buffer);
                break;
            case TOX_ERR_GET_CLIENT_ID_NULL:
                throw_tox_exception(env, "GetClientId", "NULL");
                break;
            case TOX_ERR_GET_CLIENT_ID_NOT_FOUND:
                throw_tox_exception(env, "GetClientId", "NOT_FOUND");
                break;
        }
        return result;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxFriendExists
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxFriendExists
  (JNIEnv *env, jclass, jint instanceNumber, jint)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return false;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxGetFriendList
 * Signature: (I)[I
 */
JNIEXPORT jintArray JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxGetFriendList
  (JNIEnv *env, jclass, jint instanceNumber)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return nullptr;
    });
}

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
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
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
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
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
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxFileControl
 * Signature: (IIBI)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxFileControl
  (JNIEnv *env, jclass, jint instanceNumber, jint, jbyte, jint)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxFileSend
 * Signature: (IIIJ[B)B
 */
JNIEXPORT jbyte JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxFileSend
  (JNIEnv *env, jclass, jint instanceNumber, jint, jint, jlong, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
        return 0;
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSendLossyPacket
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSendLossyPacket
  (JNIEnv *env, jclass, jint instanceNumber, jint, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}

/*
 * Class:     im_tox_tox4j_v2_ToxCoreImpl
 * Method:    toxSendLosslessPacket
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL Java_im_tox_tox4j_v2_ToxCoreImpl_toxSendLosslessPacket
  (JNIEnv *env, jclass, jint instanceNumber, jint, jbyteArray)
{
    return with_instance(env, instanceNumber, [=](Tox *tox, ToxEvents &events) {
        unused(tox);
        unused(events);
        unused(tox_callback_lossless_packet);
        throw_unsupported_operation_exception(env, instanceNumber, "tox_callback_lossless_packet");
    });
}