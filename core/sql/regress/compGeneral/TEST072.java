
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

public class TEST072 {

        public static void testMoreResultSet(ResultSet[] rs0,ResultSet[] rs1,ResultSet[] rs2,ResultSet[] rs3,ResultSet[] rs4) {
            try {
                    Connection conn =   DriverManager.getConnection( "jdbc:default:connection" );
                    System.out.println("*******************");
                    PreparedStatement pstmt0 =
                                    conn.prepareStatement("select * from qa_jdbc_statement.testGetMoreResults");
                    rs0[0] = pstmt0.executeQuery();
                    PreparedStatement pstmt1 =
                                    conn.prepareStatement("select count(*) from qa_jdbc_statement.testGetMoreResults");
                    rs1[0]= pstmt1.executeQuery();
                    PreparedStatement pstmt2 =
                                    conn.prepareStatement("select c_char from qa_jdbc_statement.testGetMoreResults");
                    rs2[0]= pstmt2.executeQuery();
                    PreparedStatement pstmt3 = conn.prepareStatement("select c_integer from qa_jdbc_statement.testGetMoreResults");
                    rs3[0]= pstmt3.executeQuery();
                    PreparedStatement pstmt4 = conn.prepareStatement("select c_integer from qa_jdbc_statement.testGetMoreResults where c_integer<10");
                    rs4[0]= pstmt4.executeQuery();
            } catch (SQLException e) {
                    e.printStackTrace();
            }
        }
}

