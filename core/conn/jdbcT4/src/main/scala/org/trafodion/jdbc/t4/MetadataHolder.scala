package org.trafodion.jdbc.t4

import java.sql.{ResultSet, Connection, PreparedStatement}

/**
 * Created by kaihua.xu on 2016/5/26.
 */
class MetadataHolder{
  private var preparedStatementMap = Map[Short, PreparedStatement]()
  private val sqlMap = Map(T4DatabaseMetaData.SQL_API_SQLPROCEDURES ->
    """select obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEMA,
       obj.OBJECT_NAME PROCEDURE_NAME, cast(NULL as varchar(10)) R1,cast(NULL as varchar(10)) R2,
       cast(NULL as varchar(10)) R3, cast(NULL as varchar(10)) REMARKS,
       cast(case when routines.UDR_TYPE = 'P' then 1
       when routines.UDR_TYPE = 'F' or routines.UDR_TYPE = 'T'
       then 2 else 0 end as smallint) PROCEDURE_TYPE,
       obj.OBJECT_NAME SPECIFIC_NAME
       from TRAFODION."_MD_".OBJECTS obj
       left join TRAFODION."_MD_".ROUTINES routines on obj.OBJECT_UID = routines.UDR_UID
       where
       (trim(obj.SCHEMA_NAME) LIKE ? )
       and (trim(obj.OBJECT_NAME) LIKE ? )
       and obj.OBJECT_TYPE='UR'
       order by obj.OBJECT_NAME
       FOR READ UNCOMMITTED ACCESS""",
    T4DatabaseMetaData.SQL_API_SQLPROCEDURECOLUMNS->
      """
        select obj.CATALOG_NAME PROCEDURE_CAT, obj.SCHEMA_NAME PROCEDURE_SCHEM,
        |obj.OBJECT_NAME PROCEDURE_NAME, cols.COLUMN_NAME COLUMN_NAME,
        |cast((case when cols.DIRECTION='I' then 1 when cols.DIRECTION='N' then 2 when cols.DIRECTION='O' then 3 else 0 end) as smallint) COLUMN_TYPE,
        |cols.FS_DATA_TYPE DATA_TYPE, cols.SQL_DATA_TYPE TYPE_NAME,
        |cols.COLUMN_PRECISION "PRECISION", cols.COLUMN_SIZE LENGTH, cols.COLUMN_SCALE SCALE,
        |cast(1 as smallint) RADIX, cols.NULLABLE NULLABLE, cast(NULL as varchar(10)) REMARKS,
        |cols.DEFAULT_VALUE COLUMN_DEF, cols.FS_DATA_TYPE SQL_DATA_TYPE, cast(0 as smallint) SQL_DATETIME_SUB,
        |cols.COLUMN_SIZE CHAR_OCTET_LENGTH, cols.COLUMN_NUMBER ORDINAL_POSITION,
        |cols.NULLABLE IS_NULLABLE, cols.COLUMN_NAME SPECIFIC_NAME
        | from TRAFODION."_MD_".OBJECTS obj
        | left join TRAFODION."_MD_".COLUMNS cols on obj.OBJECT_UID=cols.OBJECT_UID
        | where
        | obj.SCHEMA_NAME like ?
        | and obj.OBJECT_NAME like ?
        | and cols.COLUMN_NAME like ?
        | order by cols.COLUMN_NUMBER
        | FOR READ UNCOMMITTED ACCESS
      """.stripMargin)
  def getPStatement(apiId : Short, conn: Connection):PreparedStatement = {
    if( !preparedStatementMap.contains(apiId)){
      preparedStatementMap += (apiId -> conn.prepareStatement(sqlMap(apiId), ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT ))
    }
    preparedStatementMap(apiId)
  }

  def removePStatement(apiId:Short) = {
    if( preparedStatementMap contains apiId){
      try{
        preparedStatementMap(apiId) close()
      }catch{
        case ex: Exception =>
          None
      }
      preparedStatementMap -= apiId
    }
  }

  def show = {
    sqlMap.foreach((e:(Short,String))=>println(e._1+": "+e._2))
  }
}
