// The PROJECTTEAM procedure accepts a project code and returns the
// employee number, first name, last name, and location of the employees
// assigned to that project.
//
// See http://trafodion.apache.org/docs/spj_guide/index.html#projectteam-procedure
// for more documentation.
public static void projectTeam( int projectCode
			      , ResultSet[] members
			      ) throws SQLException
{
   Connection conn =
      DriverManager.getConnection( "jdbc:default:connection" ) ;

   PreparedStatement getMembers = 
      conn.prepareStatement( "SELECT E.empnum, E.first_name, E.last_name, D.location " 
			   + "FROM trafodion.persnl.employee E, trafodion.persnl.dept D, trafodion.persnl.project P "
			   + "WHERE P.projcode = ? " 
			   + "  AND P.empnum = E.empnum " 
			   + "  AND E.deptnum = D.deptnum "
			   ) ; 

    getMembers.setInt( 1, projectCode ) ;
    members[0] = getMembers.executeQuery() ;

} 
