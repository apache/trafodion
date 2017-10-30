import static org.junit.Assert.assertTrue;

import java.sql.Connection;
import java.sql.SQLException;

import org.junit.Test;

public class TestWrap {

    @Test
    public void testIsWrapFor() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            boolean result = conn.isWrapperFor(Connection.class);
            assertTrue("It is wrapper for this interface", result);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }

    @Test(expected = SQLException.class)
    public void testConnectIsClose() throws SQLException {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            conn.close();
        } catch (SQLException e) {
            e.printStackTrace();
        }
        conn.isWrapperFor(Connection.class) ;
    }

    @Test
    public void testUnwrap() {
        Connection conn = null;
        try {
            System.out.println("Connecting to database...");
            conn = Utils.getUserConnection();
            boolean result = conn.unwrap(Connection.class) instanceof Connection;
            assertTrue("It is unwrape for this interface", result);
            result = conn.unwrap(Connection.class) instanceof TestWrap;
            assertTrue("It is unwrape for this interface", !result);
        } catch (SQLException e) {
            e.printStackTrace();
        }
    }
}
