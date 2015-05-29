namespace cpp  trafodion.wms.thrift
namespace java org.trafodion.wms.thrift.generated

enum Operation {
  OPERATION_BEGIN = 100,
  OPERATION_UPDATE,
  OPERATION_END,
  OPERATION_UPDATE_PARENT_ID,
  OPERATION_CANCEL_CHILDREN
}

enum Action {
  ACTION_CONTINUE = 200,
  ACTION_REJECT,
  ACTION_CANCEL,
  ACTION_KILL,
  ACTION_WARNING,
  ACTION_PRIORITY_LOW,
  ACTION_PRIORITY_MEDIUM,
  ACTION_PRIORITY_HIGH
}

struct KeyValue {
  1: optional bool boolValue
  2: optional byte byteValue
  3: optional i16 shortValue
  4: optional i32 intValue
  5: optional i64 longValue
  6: optional double floatValue
  7: optional string stringValue 
}

// Client Data...set by clients
struct Data {
  1: optional map<string,KeyValue> keyValues
}

// Header...set by APIs
struct Header {
 1: string version,
 2: string clientIpAddress,
 3: i64 clientTimestamp,
 4: string clientUserName,
 5: string clientApplication,
 6: i64 serverLastUpdated
}

//  Workload Request
struct Request {
 1: Header header,
 2: Data data 
}

// Workload response  
struct Response {
  1: Header header
  2: Data data
}

// WmsAdmin 
//  Stream
struct Stream {
 1: string name,
 2: string value,
 3: string comment,
 4: i64 timestamp
}

// StreamResponse 
struct StreamResponse {
  1: list<Stream> streamList
}

//  Rule
struct Rule {
 1: string name,
 2: string value,
 3: string comment,
 4: i64 timestamp
}

// RuleResponse 
struct RuleResponse {
  1: list<Rule> ruleList
}

// Workload 
struct WorkloadResponse {
  1: list<Request> workloadList
}

//
// Exceptions 
//
/**
 * An IOError exception signals that an error occurred 
 * communicating to the WmsServer.  Also used to return
 * more general error conditions.
 */
exception IOError {
  1:string message
}

/**
 * An IllegalArgument exception indicates an illegal or invalid
 * argument was passed into a procedure.
 */
exception IllegalArgument {
  1:string message
}

//
// The WMS Service 
//
service WmsService {
  /** Sends a ping to a server */
  i64 ping(
      1: i64 timestamp
  ) throws (1:IOError io)
  
  /** Sends a workload to a server and returns the response */
  Response writeread(
      1: Request request
  ) throws (1:IOError io, 2:IllegalArgument ia); 
  
}

//
// The WMS Admin Service 
//
service WmsAdminService {
  /** Sends a ping to a server */
  i64 ping(
      1: i64 timestamp
  ) throws (1:IOError io)
  
  /** Add Stream */
  void addStream(
       1: Stream stream
  ) throws (1:IOError io, 2:IllegalArgument ia); 
  
  /** Alter Stream */
  void alterStream(
       1: Stream stream
  ) throws (1:IOError io, 2:IllegalArgument ia); 
  
  /** Delete Stream */
  void deleteStream(
       1: Stream stream
  ) throws (1:IOError io, 2:IllegalArgument ia); 
   
  /** List Streams */
  StreamResponse stream(
  ) throws (1:IOError io); 
  
  /** Add Rule */
  void addRule(
       1: Rule rule
  ) throws (1:IOError io, 2:IllegalArgument ia); 
  
  /** Alter Rule */
  void alterRule(
       1: Rule rule
  ) throws (1:IOError io, 2:IllegalArgument ia); 
  
  /** Delete Rule */
  void deleteRule(
       1: Rule rule
  ) throws (1:IOError io, 2:IllegalArgument ia); 
   
  /** List Rules */
  RuleResponse rule(
  ) throws (1:IOError io); 
  
   /** Lists Workloads */
  WorkloadResponse workload(
  ) throws (1:IOError io); 
 
}