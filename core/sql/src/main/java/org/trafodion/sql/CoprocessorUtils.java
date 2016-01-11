package org.trafodion.sql;

import java.io.IOException;
import java.net.URL;

import org.apache.commons.configuration.Configuration;
import org.apache.commons.configuration.PropertiesConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.log4j.Logger;

public class CoprocessorUtils {
    private static Logger logger = Logger.getLogger(HBaseClient.class.getName());
    private static String[] coprocessors = null;

    static {
        init();
    }

    private static void init() {
        Configuration config = null;
        try {
            String path = System.getenv("MY_SQROOT") + "/etc/trafcoprocess.properties";
            config = new PropertiesConfiguration(path);
        } catch (Exception e) {
            logger.error("error when finding trafcoprocess.properties");
            e.printStackTrace();
        }

        if (config != null) {
            coprocessors = config.getStringArray("coprocessors");
        }
    }

    public static void addCoprocessor(String currentAllClassName, HTableDescriptor desc) throws IOException {
        if (coprocessors == null) {
            return;
        }
        for (String coprocess : coprocessors) {
            if (!currentAllClassName.contains(coprocess)) {
                desc.addCoprocessor(coprocess);
            }
        }
    }

    public static void main(String[] args) {
        System.out.println("================CoprocessorUtils.main======================");
        //init();
        if (coprocessors == null) {
            return;
        }
        for (String coprocess : coprocessors) {
            System.out.println(coprocess);
        }
    }
}
