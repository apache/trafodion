package org.apache.hadoop.hbase.regionserver.transactional;

import java.util.Collection;
import java.util.List;

import org.apache.hadoop.hbase.DoNotRetryIOException;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValue.Type;
import org.apache.hadoop.hbase.client.Delete;

/**
 * Used to fail-fast when Delete.addColumn is used instead of Delete.addColumns. The transactional layer does not yet
 * support the deletion of a single row.cell.version.
 */
public class SingleVersionDeleteNotSupported extends DoNotRetryIOException {

    private static final long serialVersionUID = 1297446454511704139L;

    /**
     * default constructor
     */
    public SingleVersionDeleteNotSupported() {
        super(TransactionalRegionServer.class.getName()
                + " does not support single version deletes. Use Delete.addColumns()"
                + " to remove all versions of the given row, column.");
    }

    /**
     * Validate that a single-version delete is not used. Having to do this for now because transactional delete
     * mechansim will currently treat DeleteColumn the same as Delete which could cause confusion.
     */
    public static void validateDelete(final Delete delete) throws SingleVersionDeleteNotSupported {
        Collection<List<KeyValue>> values = delete.getFamilyMap().values();
        for (List<KeyValue> value : values) {
            for (KeyValue kv : value) {
                if (Type.Delete.getCode() == kv.getType()) {
                    throw new SingleVersionDeleteNotSupported();
                }
            }
        }
    }
}
