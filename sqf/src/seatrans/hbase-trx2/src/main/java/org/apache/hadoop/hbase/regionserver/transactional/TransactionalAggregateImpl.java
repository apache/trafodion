package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.NavigableSet;

import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.client.transactional.TransactionalAggregateProtocol;
import org.apache.hadoop.hbase.coprocessor.AggregateImplementation;
import org.apache.hadoop.hbase.coprocessor.ColumnInterpreter;
import org.apache.hadoop.hbase.coprocessor.RegionCoprocessorEnvironment;
import org.apache.hadoop.hbase.filter.FirstKeyOnlyFilter;
import org.apache.hadoop.hbase.regionserver.InternalScanner;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.Pair;

public class TransactionalAggregateImpl extends AggregateImplementation
		implements TransactionalAggregateProtocol {

	@Override
	public <T, S> long getRowNum(Long transactionId,
			ColumnInterpreter<T, S> ci, Scan scan) throws IOException {
		long counter = 0l;
		List<KeyValue> results = new ArrayList<KeyValue>();
		byte[] colFamily = scan.getFamilies()[0];
		byte[] qualifier = scan.getFamilyMap().get(colFamily).pollFirst();
		if (scan.getFilter() == null && qualifier == null)
			scan.setFilter(new FirstKeyOnlyFilter());

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		try {
			boolean hasMoreRows = false;
			do {
				hasMoreRows = scanner.next(results);
				if (results.size() > 0) {
					counter++;
				}
				results.clear();
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		log.debug("Row counter, txid: " + transactionId + ", from this region is "
				+ ((RegionCoprocessorEnvironment) getEnvironment()).getRegion()
						.getRegionNameAsString() + ": " + counter);
		return counter;
	}

	@Override
	public <T, S> T getMax(Long transactionId, ColumnInterpreter<T, S> ci,
			Scan scan) throws IOException {
		T temp;
		T max = null;

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		List<KeyValue> results = new ArrayList<KeyValue>();
		byte[] colFamily = scan.getFamilies()[0];
		byte[] qualifier = scan.getFamilyMap().get(colFamily).pollFirst();
		// qualifier can be null.
		try {
			boolean hasMoreRows = false;
			do {
				hasMoreRows = scanner.next(results);
				for (KeyValue kv : results) {
					log.trace(Bytes.toString(colFamily) + ","
							+ Bytes.toString(colFamily) + ""
							+ Bytes.toInt(kv.getValue()) + "," + max);
					temp = ci.getValue(colFamily, qualifier, kv);
					max = (max == null || (temp != null && ci
							.compare(temp, max) > 0)) ? temp : max;
				}
				results.clear();
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		log.debug("Maximum, txid: " + transactionId + ", from this region is "
				+ ((RegionCoprocessorEnvironment) getEnvironment()).getRegion()
						.getRegionNameAsString() + ": " + max);
		return max;
	}

	@Override
	public <T, S> T getMin(Long transactionId, ColumnInterpreter<T, S> ci,
			Scan scan) throws IOException {
		T min = null;
		T temp;

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		List<KeyValue> results = new ArrayList<KeyValue>();
		byte[] colFamily = scan.getFamilies()[0];
		byte[] qualifier = scan.getFamilyMap().get(colFamily).pollFirst();
		try {
			boolean hasMoreRows = false;
			do {
				hasMoreRows = scanner.next(results);
				for (KeyValue kv : results) {
					temp = ci.getValue(colFamily, qualifier, kv);
					min = (min == null || (temp != null && ci
							.compare(temp, min) < 0)) ? temp : min;
				}
				results.clear();
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		log.debug("Minimum, txid: " + transactionId + ", from this region is "
				+ ((RegionCoprocessorEnvironment) getEnvironment()).getRegion()
						.getRegionNameAsString() + ": " + min);
		return min;
	}

	@Override
	public <T, S> S getSum(Long transactionId, ColumnInterpreter<T, S> ci,
			Scan scan) throws IOException {
		long sum = 0l;
		S sumVal = null;
		T temp;

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		byte[] colFamily = scan.getFamilies()[0];
		byte[] qualifier = scan.getFamilyMap().get(colFamily).pollFirst();
		List<KeyValue> results = new ArrayList<KeyValue>();
		try {
			boolean hasMoreRows = false;
			do {
				hasMoreRows = scanner.next(results);
				for (KeyValue kv : results) {
					temp = ci.getValue(colFamily, qualifier, kv);
					if (temp != null)
						sumVal = ci.add(sumVal, ci.castToReturnType(temp));
				}
				results.clear();
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		log.debug("Sum, txid: " + transactionId + ", from this region is "
				+ ((RegionCoprocessorEnvironment) getEnvironment()).getRegion()
						.getRegionNameAsString() + ": " + sum);
		return sumVal;
	}

	@Override
	public <T, S> Pair<S, Long> getAvg(Long transactionId,
			ColumnInterpreter<T, S> ci, Scan scan) throws IOException {
		S sumVal = null;
		Long rowCountVal = 0l;

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		byte[] colFamily = scan.getFamilies()[0];
		byte[] qualifier = scan.getFamilyMap().get(colFamily).pollFirst();
		List<KeyValue> results = new ArrayList<KeyValue>();
		boolean hasMoreRows = false;
		try {
			do {
				results.clear();
				hasMoreRows = scanner.next(results);
				for (KeyValue kv : results) {
					sumVal = ci.add(sumVal, ci.castToReturnType(ci.getValue(
							colFamily, qualifier, kv)));
				}
				rowCountVal++;
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		Pair<S, Long> pair = new Pair<S, Long>(sumVal, rowCountVal);
		return pair;
	}

	@Override
	public <T, S> Pair<List<S>, Long> getStd(Long transactionId,
			ColumnInterpreter<T, S> ci, Scan scan) throws IOException {
		S sumVal = null, sumSqVal = null, tempVal = null;
		long rowCountVal = 0l;

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		byte[] colFamily = scan.getFamilies()[0];
		byte[] qualifier = scan.getFamilyMap().get(colFamily).pollFirst();
		List<KeyValue> results = new ArrayList<KeyValue>();

		boolean hasMoreRows = false;
		try {
			do {
				tempVal = null;
				hasMoreRows = scanner.next(results);
				for (KeyValue kv : results) {
					tempVal = ci.add(tempVal, ci.castToReturnType(ci.getValue(
							colFamily, qualifier, kv)));
				}
				results.clear();
				sumVal = ci.add(sumVal, tempVal);
				sumSqVal = ci.add(sumSqVal, ci.multiply(tempVal, tempVal));
				rowCountVal++;
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		List<S> l = new ArrayList<S>();
		l.add(sumVal);
		l.add(sumSqVal);
		Pair<List<S>, Long> p = new Pair<List<S>, Long>(l, rowCountVal);
		return p;
	}

	@Override
	public <T, S> List<S> getMedian(Long transactionId,
			ColumnInterpreter<T, S> ci, Scan scan) throws IOException {
		S sumVal = null, sumWeights = null, tempVal = null, tempWeight = null;

		InternalScanner scanner;
		if (transactionId != 0) {
		    log.debug("txid: " + transactionId + " Using the transactional scanner");
		    scanner = ((TransactionalRegion) ((RegionCoprocessorEnvironment) getEnvironment())
			       .getRegion()).getScanner(transactionId, scan);
		}
		else {
		    scanner = ((RegionCoprocessorEnvironment) getEnvironment()).getRegion().getScanner(scan);
		}

		byte[] colFamily = scan.getFamilies()[0];
		NavigableSet<byte[]> quals = scan.getFamilyMap().get(colFamily);
		byte[] valQualifier = quals.pollFirst();
		// if weighted median is requested, get qualifier for the weight column
		byte[] weightQualifier = quals.size() > 1 ? quals.pollLast() : null;
		List<KeyValue> results = new ArrayList<KeyValue>();

		boolean hasMoreRows = false;
		try {
			do {
				tempVal = null;
				tempWeight = null;
				hasMoreRows = scanner.next(results);
				for (KeyValue kv : results) {
					tempVal = ci.add(tempVal, ci.castToReturnType(ci.getValue(
							colFamily, valQualifier, kv)));
					if (weightQualifier != null) {
						tempWeight = ci.add(tempWeight, ci.castToReturnType(ci
								.getValue(colFamily, weightQualifier, kv)));
					}
				}
				results.clear();
				sumVal = ci.add(sumVal, tempVal);
				sumWeights = ci.add(sumWeights, tempWeight);
			} while (hasMoreRows);
		} finally {
			scanner.close();
		}
		List<S> l = new ArrayList<S>();
		l.add(sumVal);
		l.add(sumWeights == null ? ci.castToReturnType(ci.getMinValue())
				: sumWeights);
		return l;
	}

}
