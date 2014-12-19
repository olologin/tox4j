package im.tox.tox4j;

import im.tox.tox4j.callbacks.*;
import im.tox.tox4j.enums.ToxFileControl;
import im.tox.tox4j.enums.ToxFileKind;
import im.tox.tox4j.enums.ToxStatus;
import im.tox.tox4j.exceptions.*;

/**
 * This class provides a formal specification of the semantics defined by the {@link ToxCore} interface. A transition
 * function will either complete fully or have no effect, so exceptions thrown in Java code means there was no change to
 * the program state.
 */
public final class ToxCoreSpecification implements ToxCore {

    public ToxCoreSpecification() {

    }

    @Override
    public void close() {

	}

    @Override
    public byte[] save() {
        return new byte[0];
    }

    @Override
    public void load(byte[] data) throws ToxLoadException {

    }

    @Override
    public void bootstrap(String address, int port, byte[] public_key) throws ToxBootstrapException {

    }

    @Override
    public void callbackConnectionStatus(ConnectionStatusCallback callback) {

    }

    @Override
    public int getPort() throws ToxGetPortException {
        return 0;
    }

    @Override
    public byte[] getDhtId() {
        return new byte[0];
    }

    @Override
    public int iterationInterval() {
        return 0;
    }

    @Override
    public void iteration() {

    }

    @Override
    public byte[] getClientId() {
        return new byte[0];
    }

    @Override
    public byte[] getPrivateKey() {
        return new byte[0];
    }

    @Override
    public void setNospam(int noSpam) {

    }

    @Override
    public int getNospam() {
        return 0;
    }

    @Override
    public byte[] getAddress() {
        return new byte[0];
    }

    @Override
    public void setName(byte[] name) throws ToxSetInfoException {

    }

    @Override
    public byte[] getName() {
        return new byte[0];
    }

    @Override
    public void setStatusMessage(byte[] message) throws ToxSetInfoException {

    }

    @Override
    public byte[] getStatusMessage() {
        return new byte[0];
    }

    @Override
    public void setStatus(ToxStatus status) {

    }

    @Override
    public ToxStatus getStatus() {
        return null;
    }

    @Override
    public int addFriend(byte[] address, byte[] message) throws ToxFriendAddException {
        return 0;
    }

    @Override
    public int addFriendNoRequest(byte[] clientId) throws ToxFriendAddException {
        return 0;
    }

    @Override
    public void deleteFriend(int friendNumber) throws ToxFriendDeleteException {

    }

    @Override
    public int getFriendByClientId(byte[] clientId) throws ToxFriendByClientIdException {
        return 0;
    }

    @Override
    public byte[] getClientId(int friendNumber) throws ToxFriendGetClientIdException {
        return new byte[0];
    }

    @Override
    public boolean friendExists(int friendNumber) {
        return false;
    }

    @Override
    public int[] getFriendList() {
        return new int[0];
    }

    @Override
    public void callbackFriendName(FriendNameCallback callback) {

    }

    @Override
    public void callbackFriendStatusMessage(FriendStatusMessageCallback callback) {

    }

    @Override
    public void callbackFriendStatus(FriendStatusCallback callback) {

    }

    @Override
    public void callbackFriendConnected(FriendConnectedCallback callback) {

    }

    @Override
    public void callbackFriendTyping(FriendTypingCallback callback) {

    }

    @Override
    public void setTyping(int friendNumber, boolean typing) throws ToxSetTypingException {

    }

    @Override
    public int sendMessage(int friendNumber, byte[] message) throws ToxSendMessageException {
        return 0;
    }

    @Override
    public int sendAction(int friendNumber, byte[] action) throws ToxSendMessageException {
        return 0;
    }

    @Override
    public void callbackReadReceipt(ReadReceiptCallback callback) {

    }

    @Override
    public void callbackFriendRequest(FriendRequestCallback callback) {

    }

    @Override
    public void callbackFriendMessage(FriendMessageCallback callback) {

    }

    @Override
    public void callbackFriendAction(FriendActionCallback callback) {

    }

    @Override
    public void fileControl(int friendNumber, int fileNumber, ToxFileControl control) throws ToxFileControlException {

    }

    @Override
    public void callbackFileControl(FileControlCallback callback) {

    }

    @Override
    public int fileSend(int friendNumber, ToxFileKind kind, long fileSize, byte[] filename) throws ToxFileSendException {
        return 0;
    }

    @Override
    public void fileSendChunk(int friendNumber, int fileNumber, byte[] data) throws ToxFileSendChunkException {

    }

    @Override
    public void callbackFileRequestChunk(FileRequestChunkCallback callback) {

    }

    @Override
    public void callbackFileReceive(FileReceiveCallback callback) {

    }

    @Override
    public void callbackFileReceiveChunk(FileReceiveChunkCallback callback) {

    }

    @Override
    public void sendLossyPacket(int friendNumber, byte[] data) throws ToxSendCustomPacketException {

    }

    @Override
    public void callbackFriendLossyPacket(FriendLossyPacketCallback callback) {

    }

    @Override
    public void sendLosslessPacket(int friendNumber, byte[] data) throws ToxSendCustomPacketException {

    }

    @Override
    public void callbackFriendLosslessPacket(FriendLosslessPacketCallback callback) {

    }

    @Override
    public void callback(ToxEventListener handler) {

    }

}
