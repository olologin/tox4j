package im.tox.tox4j.spec.api;

import im.tox.tox4j.ToxOptions;

import java.io.Closeable;

public interface ObjectState<ObjectT> extends Closeable {
    <T> Input input(String name, Class<T> clazz);

    <T> Member<T> member(String name, Class<T> clazz);

    ObjectState<ObjectT> open();

    void close();
}
