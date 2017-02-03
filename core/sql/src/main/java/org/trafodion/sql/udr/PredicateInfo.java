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

package org.trafodion.sql.udr;
import java.nio.ByteBuffer;


// TO BE DONE (1) 3 serialize methods and (2) EvaluationCode is stored as an emum and not 
// as an int. So it is not a bit flag.

/**
   *  A predicate to be evaluated on a table
   *
   *  These could be different kinds of predicates, like an equals
   *  predicate, a non-equals predicate or more complex cases.
  */

public abstract class PredicateInfo extends TMUDRSerializableObject
{
    /**
     *  Info on whether a table-valued input or output column is used
     *
     *  <p> Note that these are not necessarily exclusive, a predicate might
     *  be evaluated in multiple places, although that should not be
     *  common and is not yet allowed.
     */
    public enum EvaluationCode
    {
        /** Not yet determined where predicate is evaluated. */
        UNKNOWN_EVAL        (0),   
        /**  Predicate is evaluated on the UDF result, in Trafodion code. This is the default. */
        EVALUATE_ON_RESULT  (0x1), 
        /** Predicate is evaluated inside the code provided by the UDF writer. */
        EVALUATE_IN_UDF     (0x2),
        /** Predicate should be evaluated in a table-valued input before the data reaches the UDF. */
        EVALUATE_IN_CHILD   (0x4);

        private final int code_;
        
        EvaluationCode(int val) {
            code_ = val;
        }
          
        public int getEvaluationCode() {
            return code_;
        }
        
        public static EvaluationCode getEnum(int x) {
          switch (x) {
          case 0:
            return UNKNOWN_EVAL;
          case 0x1:
            return EVALUATE_ON_RESULT;
          case 0x2:
            return EVALUATE_IN_UDF;
          case 0x4:
            return EVALUATE_IN_CHILD;
          default:
            return UNKNOWN_EVAL;
          }
        }
    };

    /** Operator of a relational (comparison) predicate */
    public enum PredOperator
    {
        /** Operator not yet determined */
        UNKNOWN_OP,   
        /** Equals predicate (col = val) */
        EQUAL,    
        /** Not equals predicate (col <> val) */
        NOT_EQUAL,    
        /** Less than predicate (col <) */
        LESS,  
        /** Less or equals predicate (col <=) */
        LESS_EQUAL,
        /** Greater than predicate (col >) */
        GREATER,   
        /** Greater or equals predicate (col >=) */
        GREATER_EQUAL, 
        /** IN predicate (col IN) */
        IN,       
        /** NOT IN predicate (col NOT IN) */ 
        NOT_IN ; 
   
       private static PredOperator[] allValues = values();
       public static PredOperator fromOrdinal(int i) {
         return allValues[i];
       }
    };

    /**
     *  Get evaluation code for a predicate.
     *  @return Evaluation code.
     */
    public EvaluationCode getEvaluationCode() {
        return evalCode_ ;
    }
    
    /**
     *  Get operator code for a predicate.
     *  @return Operator code.
     */
    public PredOperator getOperator() {
        return operator_ ;
    }

    /**
     *  Check whether this predicate is a comparison predicate.
     *
     *  <p> Use this method to determine  whether it is safe to cast the object
     *  to class ComparisonPredicateInfo.
     
     *  @return true if predcate i is a comparison predicate, false otherwise.
     */
    public boolean isAComparisonPredicate() {
        switch (operator_)
        {
        case EQUAL:
        case NOT_EQUAL:
        case LESS:
        case LESS_EQUAL:
        case GREATER:
        case GREATER_EQUAL:
            return true;
            
        default:
            return false;
        }
    }
    
    // UDR writers can ignore these methods
    PredicateInfo(TMUDRSerializableObject.TMUDRObjectType t) {
        super(t, getCurrentVersion());
        evalCode_ = EvaluationCode.UNKNOWN_EVAL;
        operator_ = PredOperator.UNKNOWN_OP;
    }
    
    void setOperator(PredOperator op) {
        operator_ = op;
    }
    
    void setEvaluationCode(EvaluationCode c) {
        evalCode_ = c ;
    }

    abstract String toString(TableInfo ti) throws UDRException;
    
    static short getCurrentVersion() { return 1; }

    @Override
    int serializedLength() throws UDRException{
      return super.serializedLength() + 2 * serializedLengthOfInt();
    }

    @Override
    int serialize(ByteBuffer outputBuffer) throws UDRException{

      int origPos = outputBuffer.position();

      super.serialize(outputBuffer);

      serializeInt(evalCode_.getEvaluationCode(),
                   outputBuffer);

      serializeInt(operator_.ordinal(),
                   outputBuffer);

      // validate length in derived classes
      int bytesSerialized = outputBuffer.position() - origPos;

      return bytesSerialized;
    }

    @Override
    int deserialize(ByteBuffer inputBuffer) throws UDRException {
     int origPos = inputBuffer.position();

      super.deserialize(inputBuffer);

      int op = 0;

      op = deserializeInt(inputBuffer);

      evalCode_ = EvaluationCode.getEnum(op);

      op = deserializeInt(inputBuffer);

      operator_ = PredOperator.fromOrdinal(op);

      // validate operator type and length in derived classes
      int bytesDeserialized = inputBuffer.position() - origPos;

      return bytesDeserialized;
    }
    
    private EvaluationCode evalCode_;
    private PredOperator operator_;
    
}

