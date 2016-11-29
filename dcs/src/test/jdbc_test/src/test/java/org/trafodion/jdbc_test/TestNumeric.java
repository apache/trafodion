import static org.junit.Assert.assertEquals;

import java.math.BigDecimal;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import org.junit.Test;

public class TestNumeric {

    @Test
    public void JDBCNumeric() throws SQLException {
        Connection conn = null;
        Statement stmt = null;
        PreparedStatement prepStmt = null;
        ResultSet rs = null;
        String sql = "upsert using load into numeric_tbl values (?,?);";
        try {
            conn = Utils.getUserConnection();
            stmt = conn.createStatement();
            stmt.executeUpdate("set schema " + Utils.catalog + "." + Utils.schema);
            stmt.executeUpdate("create table if not exists numeric_tbl (c0 int not null, c1 numeric(20,0))");
            stmt.executeUpdate("delete from numeric_tbl");

            prepStmt = conn.prepareStatement(sql);
            for (int i = 0; i < 1000; i++) {
                prepStmt.setInt(1, i);
                prepStmt.setBigDecimal(2, new BigDecimal(-1));
                prepStmt.addBatch();
            }
            prepStmt.executeBatch();

            rs = stmt.executeQuery("select count(*) from numeric_tbl where c1=-1;");
            int result = 0;
            while (rs.next()) {
                result = rs.getInt(1);
            }
            rs.close();
            assertEquals("Rows returned count should be 1000", 1000, result);
        } catch (SQLException e) {
            e.printStackTrace();
        } finally {
            stmt.close();
            prepStmt.close();
            conn.close();
        }

    }
}

