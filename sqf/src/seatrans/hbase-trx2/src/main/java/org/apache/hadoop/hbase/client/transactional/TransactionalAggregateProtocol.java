package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.util.List;

import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.coprocessor.AggregationClient;
import org.apache.hadoop.hbase.coprocessor.AggregateProtocol;
import org.apache.hadoop.hbase.coprocessor.ColumnInterpreter;
import org.apache.hadoop.hbase.util.Pair;

public interface TransactionalAggregateProtocol extends AggregateProtocol {

	/**
	 * Gives the maximum for a given combination of column qualifier and column
	 * family, in the given row range as defined in the Scan object. In its
	 * current implementation, it takes one column family and one column
	 * qualifier (if provided). In case of null column qualifier, maximum value
	 * for the entire column family will be returned.
	 * 
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return max value as mentioned above
	 * @throws IOException
	 */
	<T, S> T getMax(Long transactionId, ColumnInterpreter<T, S> ci, Scan scan)
			throws IOException;

	/**
	 * Gives the minimum for a given combination of column qualifier and column
	 * family, in the given row range as defined in the Scan object. In its
	 * current implementation, it takes one column family and one column
	 * qualifier (if provided). In case of null column qualifier, minimum value
	 * for the entire column family will be returned.
	 * 
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return min as mentioned above
	 * @throws IOException
	 */
	<T, S> T getMin(Long transactionId, ColumnInterpreter<T, S> ci, Scan scan)
			throws IOException;

	/**
	 * Gives the sum for a given combination of column qualifier and column
	 * family, in the given row range as defined in the Scan object. In its
	 * current implementation, it takes one column family and one column
	 * qualifier (if provided). In case of null column qualifier, sum for the
	 * entire column family will be returned.
	 * 
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return sum of values as defined by the column interpreter
	 * @throws IOException
	 */
	<T, S> S getSum(Long transactionId, ColumnInterpreter<T, S> ci, Scan scan)
			throws IOException;

	/**
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return Row count for the given column family and column qualifier, in
	 *         the given row range as defined in the Scan object.
	 * @throws IOException
	 */
	<T, S> long getRowNum(Long transactionId, ColumnInterpreter<T, S> ci,
			Scan scan) throws IOException;

	/**
	 * Gives a Pair with first object as Sum and second object as row count,
	 * computed for a given combination of column qualifier and column family in
	 * the given row range as defined in the Scan object. In its current
	 * implementation, it takes one column family and one column qualifier (if
	 * provided). In case of null column qualifier, an aggregate sum over all
	 * the entire column family will be returned.
	 * <p>
	 * The average is computed in
	 * {@link AggregationClient#avg(byte[], ColumnInterpreter, Scan)} by
	 * processing results from all regions, so its "ok" to pass sum and a Long
	 * type.
	 * 
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return Average
	 * @throws IOException
	 */
	<T, S> Pair<S, Long> getAvg(Long transactionId, ColumnInterpreter<T, S> ci,
			Scan scan) throws IOException;

	/**
	 * Gives a Pair with first object a List containing Sum and sum of squares,
	 * and the second object as row count. It is computed for a given
	 * combination of column qualifier and column family in the given row range
	 * as defined in the Scan object. In its current implementation, it takes
	 * one column family and one column qualifier (if provided). The idea is get
	 * the value of variance first: the average of the squares less the square
	 * of the average a standard deviation is square root of variance.
	 * 
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return STD
	 * @throws IOException
	 */
	<T, S> Pair<List<S>, Long> getStd(Long transactionId,
			ColumnInterpreter<T, S> ci, Scan scan) throws IOException;

	/**
	 * Gives a List containing sum of values and sum of weights. It is computed
	 * for the combination of column family and column qualifier(s) in the given
	 * row range as defined in the Scan object. In its current implementation,
	 * it takes one column family and two column qualifiers. The first qualifier
	 * is for values column and the second qualifier (optional) is for weight
	 * column.
	 * 
	 * @param transactionId
	 * @param ci
	 * @param scan
	 * @return Pair
	 * @throws IOException
	 */
	<T, S> List<S> getMedian(Long transactionId, ColumnInterpreter<T, S> ci,
			Scan scan) throws IOException;

}
