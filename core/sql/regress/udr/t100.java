/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/

import java.math.*;
import java.sql.*;
import java.io.*;
import java.lang.reflect.*;

class t100
{
    //
    // IN, OUT
    //
    public static void T100_io_nn(BigDecimal IN1, BigDecimal[] OUT2)
    {
        BigDecimal one = new BigDecimal(1);
        OUT2[0] = IN1.add(one);
    }
    public static void T100_io_ee(BigDecimal IN1, BigDecimal[] OUT2)
    {
        BigDecimal two = new BigDecimal(2);
        OUT2[0] = IN1.divide(two, BigDecimal.ROUND_HALF_UP);
    }
    public static void T100_io_yy(short IN1, short[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_ss(short IN1, short[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_ii(int IN1, int[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_ll(long IN1, long[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_ff(double IN1, double[] OUT2)
    {
        OUT2[0] = IN1 / 10;
    }
    public static void T100_io_rr(float IN1, float[] OUT2)
    {
        OUT2[0] = IN1 / 10;
    }
    public static void T100_io_ff_2(Double IN1, Double[] OUT2)
    {
        OUT2[0] = new Double(IN1.doubleValue()/10.0);
    }
    public static void T100_io_pp(double IN1, double[] OUT2)
    {
        OUT2[0] = IN1 / 10;
    }
    public static void T100_io_pp_2(Double IN1, Double[] OUT2)
    {
        OUT2[0] = new Double(IN1.doubleValue()/10.0);
    }
    public static void T100_io_cc(String IN1, String[] OUT2)
    {
        StringBuffer b = new StringBuffer(IN1);
        b.reverse();
        OUT2[0] = b.toString();
    }
    public static void T100_io_dd(Date IN1, Date[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_tt(Time IN1, Time[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_mm(Timestamp IN1, Timestamp[] OUT2)
    {
        OUT2[0] = IN1;
    }
    public static void T100_io_mmm(Timestamp IN1, Timestamp[] OUT2)
    {
        OUT2[0] = IN1;
    }

    //
    // IN, INOUT
    //
    public static void T100_ix_if(int IN1, float[] OUT2)
    {
        OUT2[0] = IN1 + OUT2[0];
    }

    //
    // Many types in one method
    //
    public static void T100_iiiiio_vinrdv(String IN1, int IN2,
                                          BigDecimal IN3, float IN4,
                                          Date IN5, String[] OUT6)
    {
        String result = new String("");
        result += "[" + IN1 + "]";
        result += "[" + IN2 + "]";
        result += "[" + IN3.toString() + "]";
        result += "[" + IN4 + "]";
        result += "[" + IN5.toString() + "]";
        OUT6[0] = result;
    }

    //
    // IN, INOUT, OUT
    //
    public static void T100_ixo_ccc(String IN1, String[] INOUT2, String[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_vvv(String IN1, String[] INOUT2, String[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_iio_vvv(String IN1, String IN2, String[] OUT3)
    {
        // This method supports a test case where we DROP a procedure
        // then create another with same name, different SQL signature
        OUT3[0] = "OK";
    }
    public static void T100_ixo_iii(int IN1, int[] INOUT2, int[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_sss(short IN1, short[] INOUT2, short[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_lll(long IN1, long[] INOUT2, long[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_eee(BigDecimal IN1, BigDecimal[] INOUT2, BigDecimal[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_nnn(BigDecimal IN1, BigDecimal[] INOUT2, BigDecimal[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_fff(float IN1, float[] INOUT2, float[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_ggg(double IN1, double[] INOUT2, double[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_rrr(float IN1, float[] INOUT2, float[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_ppp(double IN1, double[] INOUT2, double[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_ddd(java.sql.Date IN1, java.sql.Date[] INOUT2, java.sql.Date[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_ttt(java.sql.Time IN1, java.sql.Time[] INOUT2, java.sql.Time[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }
    public static void T100_ixo_mmm(java.sql.Timestamp IN1, java.sql.Timestamp[] INOUT2, java.sql.Timestamp[] OUT3)
    {
        OUT3[0] = INOUT2[0];
        INOUT2[0] = IN1;
    }

    public static void T100_NAME_This_name_is_128_chars_long(int IN1, int[] INOUT1, int[] OUT1)
    {
	OUT1[0] = INOUT1[0];
	INOUT1[0] = IN1;
    }

    // No parameters, the simplest test
    public static void T100_none()
    {}

    // Just one parameter, could test some boundary conditions
    public static void T100_one(int IN1)
    {
	int x = IN1;
    }

    //
    // All parameters have the same mode
    //
    public static void T100_iii_iii(int IN1, int IN2, int IN3)
    {
    }
    public static void T100_xxx_iii(int[] INOUT1, int[] INOUT2, int[] INOUT3)
    {
        INOUT1[0] = -INOUT1[0];
        INOUT2[0] = -INOUT2[0];
        INOUT3[0] = -INOUT3[0];
    }
    public static void T100_ooo_iii(int[] OUT1, int[] OUT2, int[] OUT3)
    {
        OUT1[0] = 2;
        OUT2[0] = 4;
        OUT3[0] = 8;
    }

    // Lots of parameters
    public static void T100_many(int IN1, double IN2,
			    int[] OUT1, double[] OUT2,
			    int[] INOUT1, double[] INOUT2,
			    java.sql.Timestamp IN3, java.sql.Date IN4,
			    java.sql.Timestamp[] OUT3, java.sql.Date[] OUT4,
			    java.sql.Timestamp[] INOUT3, java.sql.Date[] INOUT4,
			    String IN5, String IN6,
			    String[] OUT5, String[] OUT6,
			    String[] INOUT5, String[] INOUT6,
			    java.math.BigDecimal IN7, long IN8,
			    java.math.BigDecimal[] OUT7, long[] OUT8,
			    java.math.BigDecimal[] INOUT7, long[] INOUT8,
			    float IN9, short IN10,
			    float[] OUT9, short[] OUT10,
			    float[] INOUT9, short[] INOUT10,
			    // 11- 20
			    int IN010, double IN20,
			    int[] OUT010, double[] OUT20,
			    int[] INOUT010, double[] INOUT20,
			    java.sql.Timestamp IN30, java.sql.Date IN40,
			    java.sql.Timestamp[] OUT30, java.sql.Date[] OUT40,
			    java.sql.Timestamp[] INOUT30, java.sql.Date[] INOUT40,
			    String IN50, String IN60,
			    String[] OUT50, String[] OUT60,
			    String[] INOUT50, String[] INOUT60,
			    java.math.BigDecimal IN70, long IN80,
			    java.math.BigDecimal[] OUT70, long[] OUT80,
			    java.math.BigDecimal[] INOUT70, long[] INOUT80,
			    float IN90, short IN100,
			    float[] OUT90, short[] OUT100,
			    float[] INOUT90, short[] INOUT100,
			    // 21 - 30
			    int IN0100, double IN200,
			    int[] OUT0100, double[] OUT200,
			    int[] INOUT0100, double[] INOUT200,
			    java.sql.Timestamp IN300, java.sql.Date IN400,
			    java.sql.Timestamp[] OUT300, java.sql.Date[] OUT400,
			    java.sql.Timestamp[] INOUT300, java.sql.Date[] INOUT400,
			    String IN500, String IN600,
			    String[] OUT500, String[] OUT600,
			    String[] INOUT500, String[] INOUT600,
			    java.math.BigDecimal IN700, long IN800,
			    java.math.BigDecimal[] OUT700, long[] OUT800,
			    java.math.BigDecimal[] INOUT700, long[] INOUT800,
			    float IN900, short IN1000,
			    float[] OUT900, short[] OUT1000,
			    float[] INOUT900, short[] INOUT1000,
			    // 31 - 40
			    int IN01000, double IN2000,
			    int[] OUT01000, double[] OUT2000,
			    int[] INOUT01000, double[] INOUT2000,
			    java.sql.Timestamp IN3000, java.sql.Date IN4000,
			    java.sql.Timestamp[] OUT3000, java.sql.Date[] OUT4000,
			    java.sql.Timestamp[] INOUT3000, java.sql.Date[] INOUT4000,
			    String IN5000, String IN6000,
			    String[] OUT5000, String[] OUT6000,
			    String[] INOUT5000, String[] INOUT6000,
			    java.math.BigDecimal IN7000, long IN8000,
			    java.math.BigDecimal[] OUT7000, long[] OUT8000,
			    java.math.BigDecimal[] INOUT7000, long[] INOUT8000,
			    float IN9000, short IN10000,
			    float[] OUT9000, short[] OUT10000,
			    float[] INOUT9000, short[] INOUT10000 )
    {
	// 1 - 10
	OUT1[0] = IN1;
	INOUT1[0] = OUT1[0];

	OUT2[0] = IN2;
	INOUT2[0] = OUT2[0];

	OUT3[0] = IN3;
	INOUT3[0] = OUT3[0];

	OUT4[0] = IN4;
	INOUT4[0] = OUT4[0];

	OUT5[0] = IN5;
	INOUT5[0] = OUT5[0];

	OUT6[0] = IN6;
	INOUT6[0] = OUT6[0];

	OUT7[0] = IN7;
	INOUT7[0] = OUT7[0];

	OUT8[0] = IN8;
	INOUT8[0] = OUT8[0];

	OUT9[0] = IN9;
	INOUT9[0] = OUT9[0];

	OUT10[0] = IN10;
	INOUT10[0] = OUT10[0];

	// 11 - 20
	OUT010[0] = IN010;
	INOUT010[0] = OUT010[0];

	OUT20[0] = IN20;
	INOUT20[0] = OUT20[0];

	OUT30[0] = IN30;
	INOUT30[0] = OUT30[0];

	OUT40[0] = IN40;
	INOUT40[0] = OUT40[0];

	OUT50[0] = IN50;
	INOUT50[0] = OUT50[0];

	OUT60[0] = IN60;
	INOUT60[0] = OUT60[0];

	OUT70[0] = IN70;
	INOUT70[0] = OUT70[0];

	OUT80[0] = IN80;
	INOUT80[0] = OUT80[0];

	OUT90[0] = IN90;
	INOUT90[0] = OUT90[0];

	OUT100[0] = IN100;
	INOUT100[0] = OUT100[0];

	// 21 - 30
	OUT0100[0] = IN0100;
	INOUT0100[0] = OUT0100[0];

	OUT200[0] = IN200;
	INOUT200[0] = OUT200[0];

	OUT300[0] = IN300;
	INOUT300[0] = OUT300[0];

	OUT400[0] = IN400;
	INOUT400[0] = OUT400[0];

	OUT500[0] = IN500;
	INOUT500[0] = OUT500[0];

	OUT600[0] = IN600;
	INOUT600[0] = OUT600[0];

	OUT700[0] = IN700;
	INOUT700[0] = OUT700[0];

	OUT800[0] = IN800;
	INOUT800[0] = OUT800[0];

	OUT900[0] = IN900;
	INOUT900[0] = OUT900[0];

	OUT1000[0] = IN1000;
	INOUT1000[0] = OUT1000[0];

	// 31 - 40
	OUT01000[0] = IN01000;
	INOUT01000[0] = OUT01000[0];

	OUT2000[0] = IN2000;
	INOUT2000[0] = OUT2000[0];

	OUT3000[0] = IN3000;
	INOUT3000[0] = OUT3000[0];

	OUT4000[0] = IN4000;
	INOUT4000[0] = OUT4000[0];

	OUT5000[0] = IN5000;
	INOUT5000[0] = OUT5000[0];

	OUT6000[0] = IN6000;
	INOUT6000[0] = OUT6000[0];

	OUT7000[0] = IN7000;
	INOUT7000[0] = OUT7000[0];

	OUT8000[0] = IN8000;
	INOUT8000[0] = OUT8000[0];

	OUT9000[0] = IN9000;
	INOUT9000[0] = OUT9000[0];

	OUT10000[0] = IN10000;
	INOUT10000[0] = OUT10000[0];
    }

    public static void simple( int IN1, int IN2 )
    {}

    public static void T100_readIntFromFile(String fileName,
                                            int[] i,
                                            String[] status)
    {
      try
      {
        fileName = fileName.trim();
        BufferedReader r = new BufferedReader(new FileReader(fileName));
        String data = r.readLine();
        Integer I = new Integer(data);
        i[0] = I.intValue();
        status[0] = "OK";
      }
      catch (Exception e)
      {
        status[0] = "ERROR: " + e.toString();
      }

    }

    public static void T100_writeIntToFile(String fileName,
                                           int i,
                                           String[] status)
    {
      try
      {
        fileName = fileName.trim();
        FileOutputStream f = new FileOutputStream(fileName);
        PrintWriter w = new PrintWriter(f);
        w.println(i);
        w.close();
        f.close();
        status[0] = "OK";
      }
      catch (Exception e)
      {
        status[0] = "ERROR: " + e.toString();
      }
    }

    public static void alldtypes ( String in1, String in2,
				   float in3, java.math.BigDecimal in4,
				   short in5, int in6,
				   long in7, double in8,
				   float in9, double in10,
				   java.sql.Date in11, java.sql.Time in12,
				   java.sql.Timestamp in13
				   )
    {
    }

    public static void alldtypes2 (

      String IN1, int IN2, String IN3, String IN4,
      BigDecimal IN5, short IN6, Date IN7, Time IN8,
      Timestamp IN9, long IN10, double IN11,
      float IN12, double IN13,
      BigDecimal IN14, BigDecimal IN15,
      BigDecimal IN16, BigDecimal IN17,

      String[] OUT1, int[] OUT2, String[] OUT3, String[] OUT4,
      BigDecimal[] OUT5, short[] OUT6, Date[] OUT7, Time[] OUT8,
      Timestamp[] OUT9, long[] OUT10, double[] OUT11,
      float[] OUT12, double[] OUT13,
      BigDecimal[] OUT14, BigDecimal[] OUT15,
      BigDecimal[] OUT16, BigDecimal[] OUT17

      )
    {
        OUT1[0] = IN1;
        OUT2[0] = IN2;
        OUT3[0] = IN3;
        OUT4[0] = IN4;
        OUT5[0] = IN5;
        OUT6[0] = IN6;
        OUT7[0] = IN7;
        OUT8[0] = IN8;
        OUT9[0] = IN9;
        OUT10[0] = IN10;
        OUT11[0] = IN11;
        OUT12[0] = IN12;
        OUT13[0] = IN13;
        OUT14[0] = IN14;
        OUT15[0] = IN15;
        OUT16[0] = IN16;
        OUT17[0] = IN17;
    }

  public static void divide(int a, int b, int[] c)
  {
    c[0] = a / b;
  }

  public static boolean isWindows()
  {
    String os = System.getProperty("os.name");
    os = os.toUpperCase();
    if (os.startsWith("WINDOWS"))
      return true;
    return false;
  }

  public static void lmGateway(String action, String[] status)
    throws Throwable
  {
    try {
      Class[] formalTypes = new Class[2];
      formalTypes[0] = action.getClass();
      formalTypes[1] = status.getClass();

      Object[] args = new Object[2];
      args[0] = action;
      args[1] = status;

      // Due to changes (many library methods have been retrofitted with
      // generic declarations including several in Class) in JDK 1.6, the
      // following declaration reports the following warnings:
      // Note: t100.java uses unchecked or unsafe operations.
      // Note: Recompile with -Xlint:unchecked for details.
      //
      // To avoid these warning, change the declaration as follows:
      // Class<?> lmClass = Class.forName("com.tandem.sqlmx.LmUtility");
      //
      // Since the above declaration produce errors on Java version prior
      // to JDK 1.6, restore the previous code and update the expected file
      // for linux with the above warnings.

      Class lmClass = Class.forName("com.tandem.sqlmx.LmUtility");
      Object o = lmClass.newInstance();
      Method m = lmClass.getMethod("utils", formalTypes);
      Object txName = m.invoke(o, args);
    }
    catch (InvocationTargetException e) {
      throw e.getTargetException();
    }
  }

  public static void main(String args[]) throws Throwable
  {
    System.out.println("Hello, world!");

    String cmd = "insert into " + args[0].trim() + ".t100sq values(-9)";
    // String cmd = "insert into t100sq values(-9)";
    if (!isWindows())
    {
      Connection conn = DriverManager.getConnection("jdbc:default:connection");
      System.out.println("Acquired connection");
      conn.createStatement().executeUpdate(cmd);
      System.out.println("Executed INSERT");
      conn.close();
      System.out.println("Closed connection");
    }
    else
    {
      System.out.println("Acquired connection");
      String action = "ExecSql " + cmd;
      String[] status = new String[1];
      lmGateway(action, status);
      System.out.println("Executed INSERT");
      System.out.println("Closed connection");
    }
  }
}

