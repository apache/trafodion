
package org.apache.hadoop.hbase.regionserver.transactional;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;


class CompressionContext {
  final Dictionary regionDict;
  final Dictionary tableDict;
  final Dictionary familyDict;
  final Dictionary qualifierDict;
  final Dictionary rowDict;

  public CompressionContext(Class<? extends Dictionary> dictType)
  throws SecurityException, NoSuchMethodException, InstantiationException,
      IllegalAccessException, InvocationTargetException {
    Constructor<? extends Dictionary> dictConstructor =
        dictType.getConstructor();
    regionDict = dictConstructor.newInstance();
    tableDict = dictConstructor.newInstance();
    familyDict = dictConstructor.newInstance();
    qualifierDict = dictConstructor.newInstance();
    rowDict = dictConstructor.newInstance();
  }

  void clear() {
    regionDict.clear();
    tableDict.clear();
    familyDict.clear();
    qualifierDict.clear();
    rowDict.clear();
  }
}
