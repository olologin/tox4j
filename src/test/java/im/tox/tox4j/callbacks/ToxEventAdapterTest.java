package im.tox.tox4j.callbacks;

import im.tox.tox4j.enums.ToxFileControl;
import im.tox.tox4j.enums.ToxFileKind;
import im.tox.tox4j.enums.ToxStatus;
import org.junit.Test;

import static org.junit.Assert.*;

public class ToxEventAdapterTest {

    private final ToxEventListener listener = new ToxEventAdapter();

    @Test
    public void testConnectionStatus() throws Exception {
        listener.connectionStatus(false);
    }

    @Test
    public void testFileControl() throws Exception {
        listener.fileControl(0, 0, ToxFileControl.RESUME);
    }

    @Test
    public void testFileReceive() throws Exception {
        listener.fileReceive(0, 0, ToxFileKind.DATA, 0, null);
    }

    @Test
    public void testFileReceiveChunk() throws Exception {
        listener.fileReceiveChunk(0, 0, 0, null);
    }

    @Test
    public void testFileRequestChunk() throws Exception {
        listener.fileRequestChunk(0, 0, 0, 0);
    }

    @Test
    public void testFriendAction() throws Exception {
        listener.friendAction(0, 0, null);
    }

    @Test
    public void testFriendConnected() throws Exception {
        listener.friendConnected(0, false);
    }

    @Test
    public void testFriendMessage() throws Exception {
        listener.friendMessage(0, 0, null);
    }

    @Test
    public void testFriendName() throws Exception {
        listener.friendName(0, null);
    }

    @Test
    public void testFriendRequest() throws Exception {
        listener.friendRequest(null, 0, null);
    }

    @Test
    public void testFriendStatus() throws Exception {
        listener.friendStatus(0, ToxStatus.NONE);
    }

    @Test
    public void testFriendStatusMessage() throws Exception {
        listener.friendStatusMessage(0, null);
    }

    @Test
    public void testFriendTyping() throws Exception {
        listener.friendTyping(0, false);
    }

    @Test
    public void testFriendLosslessPacket() throws Exception {
        listener.friendLosslessPacket(0, null);
    }

    @Test
    public void testFriendLossyPacket() throws Exception {
        listener.friendLossyPacket(0, null);
    }

    @Test
    public void testReadReceipt() throws Exception {
        listener.readReceipt(0, 0);
    }
}