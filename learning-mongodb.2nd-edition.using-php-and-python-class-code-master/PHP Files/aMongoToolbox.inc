<?php
/**
 * aMongoToolBox.inc -- PHP Abstraction Class Extends aMongoConnectors Class
 *
 * This class is invoked by the cUserManager class and, in turn, extends the lower-level mongoDB connector class.
 *
 * The intent of this class is to provide an entry-point for all mongoDB CRUD commands we may issue.  We accomplish
 * this by first successfully connecting/instantiating the connector class (via the constructor) and passing back/thru
 * the results of the connection attempt.
 *
 * Other intended functionality is to directly access mongoDB service for various CRUD commands.
 *
 *
 * @author  mshallop@linux.com
 * @version 1.0
 *
 * HISTORY:
 * ========
 * 11-11-18     mks     Checked into source code control
 *
 */

// load the MongoConnectors abstraction class, wrapping the load in an ParseError trap, until we install an autoloader
try {
    require_once "aMongoConnectors.inc";
} catch (ParseError $p) {
    $msg = sprintf("%s@%d-->", basename(__FILE__), __LINE__);
    echo $msg . 'Parse error encountered!' . PHP_EOL;
    echo $p->getMessage() . PHP_EOL;
    exit(1);
}

const STRING_QUERY = 'query';
const STRING_PROJECTION = 'projection';
const STRING_SORT = 'sort';
const STRING_SKIP = 'skip';
const STRING_LIMIT = 'limit';
const MONGO_BULK_WRITE_RESULTS = '%d documents matched, %d documents updated, %d documents upserted, %d documents inserted, %d documents deleted';


/**
 * Class aMongoToolbox
 *
 * This is the constructor for the class and it's only job is to invoke the aMongoConnectors abstraction class to
 * make/confirm that we've successfully created a resource link to the remote mongoDB service.
 *
 * There are no input parameters to the constructor method and the method returns data implicitly by setting various
 * member variable values.
 *
 *
 * @author  mshallop@linux.com
 * @version 1.0
 *
 * HISTORY:
 * ========
 * 11-11-18     mks     Checked into source code control
 *
 */
abstract class aMongoToolbox extends aMongoConnectors
{
    public function __construct()
    {
        parent::__construct();
        if (!$this->status) {
            echo 'Unable to connect to the mongoDB instance' . PHP_EOL;
            return;
        }
    }


    /**
     * createRecords() -- protected class method
     *
     * There is one input to this method:
     *
     *    -- $_data is an indexed array of associative arrays with each record being a new record that will be
     *       created in the current namespace.  This method does not evaluate the data nor does it validate the
     *       structure of the input parameter as all this should have happened in the parent routines prior to
     *       the invocation of this method.
     *
     * We'll accept an array of records because we're going to use the mongoDB bulkWrite operations -- in other
     * words, the "cost" of saving one record, or 1,000 records, is the same.
     *
     * Instantiate a mongoDB bulkWrite object from the MongoDB-Driver API.
     *
     * Begin by spinning through the indexed $_data array so that we're dealing with a record and using the bulkWrite
     * resource just generated to insert the records.
     *
     * When we're done processing the $_data array, make a call to the executeBulkWrite() method to write all the data
     * to the mongoDB collection.  Write the data as an ordered write so as to preserve any generated timestamps.
     *
     * Store the results of the write as a string using a member variable and, if an exception was not trapped,
     * return a boolean true to the calling client to indicate a success write operation.
     *
     * If an exception was raised, then store the exception information in the error container (member variable) while
     * mirroring the messages to stdout.  Return a Boolean(false) to the calling client.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param array $_data
     * @return bool
     *
     * HISTORY:
     * ========
     * 11-18-18     mks     original coding
     * 11-25-18     mks     injecting GUIDs and timestamp values into every created record
     *
     */
    protected function createRecords(array $_data): bool
    {
        $this->status = false;
        // $_userData is an indexed array of associative arrays and the associative array elements
        // are the: userName, userPassword, and userEmail, (in that order), in each row of the
        // indexed array.  In other words, we can insert 1 or many records with the same function.
        try {
            $bWrite = new MongoDB\Driver\BulkWrite(['ordered' => true]);  // ordered
            foreach ($_data as &$userRecord) {
                $userRecord['token'] = $this->guid();
                $userRecord['created'] = time();
                $bWrite->insert($userRecord);
            }
            return ($this->doBulkWrite($bWrite));
        } catch (MongoDB\Driver\Exception\BulkWriteException |
                 MongoDB\Driver\Exception\RuntimeException | Throwable $e) {
            $msg =  'An exception was trapped!' . PHP_EOL;
            $this->errors[] = $msg;
            echo $msg;
            $msg = $e->getMessage() . PHP_EOL;
            $this->errors[] = $msg;
            echo $msg;
            return false;
        }
    }


    /**
     * updateRecords() -- protected method
     *
     * This method requires three input parameters, all of which are arrays:
     *
     * $_filter  -- this is the query filter and defines the record scope for the update
     * $_data    -- this is the update data as an indexed-array of associative arrays
     * $_options -- controls for the upsert and multi options
     *
     * The update filter defines the record(s) to be updated and is in the format of a mongodb query.
     * e.g.:  [ 'y' => 1 ]   ; all records where the column "y" has a value of 1
     *
     * The data payload is an associative array containing column names, as keys, and the values to be
     * assigned to the named columns.
     *
     * Options is also an associative array which has, at most, two keys: "multi" and "upsert".  (Any other
     * keys will be discarded) and controls the update scope (1 or many) and the upsert (if not found, insert)
     * option.
     *
     * The method returns a boolean value which indicates if the update operation/request successfully completed
     * or not -- if so, then we'll also return information about the number of records affected by the update
     * request.
     *
     *
     * @author mike@linux.com
     * @version 1.0
     *
     * @param array $_filter
     * @param array $_data
     * @param array $_options
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 12-02-18     mks     original coding
     *
     */
    protected function updateRecords(array $_filter, array $_data, array $_options = null): bool
    {

        if (!empty($_options)) {
            foreach ($_options as $key => $value) {
                if ($key != 'multi' and $key != 'upsert') {
                    $this->errors[] = 'Invalid update option: ' . $key;
                    return false;
                }
            }
        }

        try {
            $bwo = new MongoDB\Driver\BulkWrite();   // bwo = bulk write object
            // inject updated timestamp into the record
            $_data['updated'] = time();
            // transform the key-value pairs in $_data to use mongoDB $set for the update command:
            $updateData = [ "\$set" => $_data];
            $bwo->update($_filter, $updateData, $_options);
            return($this->doBulkWrite($bwo));
        } catch (MongoDB\Driver\Exception\InvalidArgumentException |
                 MongoDB\Driver\Exception\ConnectionException |
                 MongoDB\Driver\Exception\RuntimeException |
                 MongoDB\Driver\Exception\Exception | Throwable $e) {
            $this->errors[] = 'Trapped mongo exception on update!' . PHP_EOL;
            $this->errors[] = $e->getMessage();
            return false;
        }
    }


    /**
     * deleteRecords() -- public method
     *
     * This method requires the following input parameters:
     *
     * $_query -- this is an array containing a proper mongo query filter
     * $_limit -- optional (defaults to false) value that determines if we delete all the records identified by the
     *            filter (true) or only the first record found (false).
     *
     * The method returns a boolean param back to the calling client to indicate that the delete operation successfully
     * completed.
     *
     * Note that this method will return a Boolean(true) of the filter returns no records -- the calling client must
     * check the values stored in the $info member variable to confirm that records were actually removed from the
     * collection.
     *
     * todo: support for soft-delete
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param array $_query
     * @param bool $_limit
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 12-17-18     mks     original coding
     *
     */
    public function deleteRecords(array $_query, bool $_limit = false): bool
    {
        // normally, we'd put validation code here to ensure that the column names match the schema
        // and that types per column are consistent.

        try {
            $bwo = new MongoDB\Driver\BulkWrite();
            $bwo->delete($_query, ['limit' => $_limit]);
            return ($this->doBulkWrite($bwo));
        } catch (\MongoDB\Driver\Exception\InvalidArgumentException | Throwable $t) {
            $this->errors[] = 'Trapped mongo exception on delete!' . PHP_EOL;
            $this->errors[] = $t->getMessage();
            return false;
        }
    }


    /**
     * doBulkWrite() -- private helper function
     *
     * This function requires a single input parameter - that's the mongo bulk-write object created in
     * the calling method and containing info specific to the operation.
     *
     * This method calls the mongo API function to execute the stored bulk-write directives while wrapped
     * in exception handling.
     *
     * The method will return a Boolean(true) if the query successfully executes and we populate the class
     * member with information about the bulk operation.  (records written, updated, etc.)
     *
     * If an exception is trapped, then we'll populate the error container and return a Boolean(false).
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param \MongoDB\Driver\BulkWrite $_bwo
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 12-16-18     mks     original coding
     *
     */
    private function doBulkWrite(MongoDB\Driver\BulkWrite $_bwo): bool
    {
        try {
            $result = $this->mdbRes->executeBulkWrite($this->namespace, $_bwo);
            $this->info[] = sprintf(MONGO_BULK_WRITE_RESULTS, $result->getMatchedCount(),
                $result->getModifiedCount(), $result->getUpsertedCount(),
                $result->getInsertedCount(), $result->getDeletedCount());
            return true;
        } catch (MongoDB\Driver\Exception\InvalidArgumentException |
                 MongoDB\Driver\Exception\ConnectionException |
                 MongoDB\Driver\Exception\RuntimeException | Throwable $e) {
            $this->errors[] = 'Trapped mongo exception on update!' . PHP_EOL;
            $this->errors[] = $e->getMessage();
            return false;
        }
    }


    /**
     * fetchRecords() -- protected method
     *
     * This method requires a single input parameter, an associative array who's top-level keys are named containers
     * for various parts of the query:
     *
     * STRING_QUERY -- contains the literal query, as an associative array, to be passed directly to mongodb
     * STRING_PROJECTION -- contains the associative array of fields that are filtered in the query
     * STRING_SORT -- the mongo sort command if we want the return data set sorted
     * STRING_SKIP -- the number of records to skip (optional) in the query to support pagination
     * STRING_LIMIT -- the number of records to limit (optional) in the return data to support pagination
     *
     * There's no error-checking/data validation beyond the most rudimentary.  We basically build the query
     * by extracting the named parts from the data payload (input parameter) and then assign those pieces
     * to their proper places.  (e.g.: query is passed to the query object as itself, everything else is
     * bundled as an option to the query proper.)
     *
     * Data is returned as an associative array and is assigned to the member variable $data.
     *
     * The method explicitly returns a boolean (based on the class status value) indicating if the query
     * successfully executed or not - it does not - indicate the status of the data itself.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param array $_data
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 11-25-18     mks     original coding
     *
     */
    protected function fetchRecords(array $_data): bool
    {
        $this->status = false;

        if (!array_key_exists(STRING_QUERY, $_data)) {
            $this->errors[] = 'A query structure is required for fetching data' . PHP_EOL;
            return false;
        }

        // set up the query
        $query = $_data[STRING_QUERY];
        $options = null;
        if (array_key_exists(STRING_PROJECTION, $_data))
            $options[STRING_PROJECTION] = $_data[STRING_PROJECTION];
        if (array_key_exists(STRING_SORT, $_data))
            $options[STRING_SORT] = $_data[STRING_SORT];
         if (array_key_exists(STRING_SKIP, $_data))
             $options[STRING_SKIP] = $_data[STRING_SKIP];
         if (array_key_exists(STRING_LIMIT, $_data))
             $options[STRING_LIMIT] = $_data[STRING_LIMIT];

        try {
            $queryObject = new MongoDB\Driver\Query($query, $options);
            /** @var MongoDB\Driver\Cursor $cursor */
            $cursor = $this->mdbRes->executeQuery($this->namespace, $queryObject);
            $cursor->setTypeMap(['root' => 'array', 'document' => 'array', 'array' => 'array']);
            $this->data = $cursor->toArray();
            $this->status = true;
        } catch (MongoDB\Driver\Exception\InvalidArgumentException |
                 MongoDB\Driver\Exception\ConnectionException |
                 MongoDB\Driver\Exception\AuthenticationException |
                 MongoDB\Driver\Exception\RuntimeException |
                 MongoDB\Driver\Exception\Exception |
                 Throwable $t) {
            $this->errors[] = 'caught exception in: ' . __METHOD__ . '@' . __LINE__ . PHP_EOL;
            $this->errors[] = $t->getMessage();
        }
        return $this->status;
    }
}
