package im.tox.tox4j.spec.api;

/**
 * Represents a random value of any type. The value set can be constrained through the interface methods.
 *
 * @param <T>    Type of the value.
 */
public interface Value<T> {
    Value<T> range(T from, T to);

    Value<T> length(int length);
}
