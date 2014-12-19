package im.tox.tox4j.spec.api;

/**
 * Represents an input to a function.
 *
 * @param <T>    The type of the input parameter.
 */
public interface Input<T> extends Value<T> {
    /**
     * When created in a constructor (before finalising the object layout), a member can be created from the parameter,
     * storing a copy of the parameter. Further constraints on either the input or the member will not affect each
     * other.
     *
     * @return the new member.
     */
    Member<T> toMember();
}
