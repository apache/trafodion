import java.io.IOException;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import org.apache.hadoop.hbase.util.Bytes;
import com.google.protobuf.ByteString;

public class HelloWorld {

    /* increment/deincrement for positive value */
    /* copied from org.apache.hadoop.hbase.util.Bytes code - as this method is a private there */
    private static byte [] binaryIncrementPos(byte [] value, long amount) {
	long amo = amount;
	int sign = 1;
	if (amount < 0) {
	    amo = -amount;
	    sign = -1;
	}
	for(int i=0;i<value.length;i++) {
	    int cur = ((int)amo % 256) * sign;
	    amo = (amo >> 8);
	    int val = value[value.length-i-1] & 0x0ff;
	    int total = val + cur;
	    if(total > 255) {
		amo += sign;
		total %= 256;
	    } else if (total < 0) {
		amo -= sign;
	    }
	    value[value.length-i-1] = (byte)total;
	    if (amo == 0) return value;
	}
	return value;
    }

    public static void main(String[] args) {

	System.out.println("Hello World!");

	String lp_string1 = new String("aa");

	byte [] lv_byte = Bytes.toBytes("abc");

	lp_string1 = null;
	System.out.println("Byte Array: " + new String(lv_byte) + " length: " + lv_byte.length);
	byte [] lv_byte_minus1 = binaryIncrementPos(lv_byte, -1);
	System.out.println("Byte Array: " + new String(lv_byte_minus1) + " length: " + lv_byte_minus1.length);

	byte lv_oneb = lv_byte[lv_byte.length - 1];
	System.out.println("Byte : " + lv_oneb);
	--lv_oneb ;
	System.out.println("Byte : " + lv_oneb);
	lv_byte[lv_byte.length - 1] = lv_oneb;
	System.out.println("Byte Array: " + new String(lv_byte) + " length: " + lv_byte.length);

	try {
	    String lp_string2 = new String(lp_string1);
	}
	catch (java.lang.NullPointerException lp_exc) {
	    System.out.println("lp_string1 is null");
	}

    }
    
}
