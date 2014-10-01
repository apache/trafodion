package org.apache.hadoop.hbase.client.transactional;

public interface TransactionalReturn {
  /** Status code representing a transaction that can be committed. */
  final int COMMIT_OK = 1;
  /** Status code representing a read-only transaction that can be committed. */
  final int COMMIT_OK_READ_ONLY = 2;
  /** Status code representing a transaction that cannot be committed. */
  final int COMMIT_UNSUCCESSFUL = 3;
  /** Status code representing a transaction that cannot be committed due to conflict. */
  final int COMMIT_CONFLICT = 5;
}
