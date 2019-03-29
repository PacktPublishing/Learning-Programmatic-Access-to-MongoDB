<?php
/**
 * Class aMongoConnectors
 *
 * This is an abstraction class who's sole purpose in life is to provide us with a resource (connection) to a mongoDB
 * service, regardless of that services' location or configuration.
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

// lpt: trouble with your mongoDB connection?  Enable this next line to get very verbose debugging info...
//ini_set('mongodb.debug', 'stderr');

abstract class aMongoConnectors
{
    /** @var MongoDB\Driver\Manager $mdbRes */
    public $mdbRes = null;
    public $status = false;
    public $errors = null;
    public $info = null;
    public $data = null;
    protected $user;
    protected $password;
    protected $uri;
    protected $port;
    protected $authDB;
    protected $replSetName;
    protected $replSetMembers;
    protected $RP;                  // read-preference class object
    protected $namespace;           // combination of the database . collection names
    protected $database;            // name of the remote database
    protected $table;               // name of the remote collection/table
    protected $ssl;                 // ssl option block for creating a context stream



    /**
     * aMongoConnectors constructor.
     *
     * This is the constructor for the abstraction class -- the class accepts no input parameters and, as a constructor,
     * returns no value.  All data is implicitly passed by populating class members.
     *
     * Based on which member variables are populated determines how we'll connect to the mongoDB service.
     *
     * For example, if a user name, password, and authentication database is received, we know we have to make an RBAC
     * connection requiring validation of the provided credentials.   If the ssl sub-array is populated, we know that
     * we'll make the connection request over TLS.
     *
     * Basically, then, this method consists of several conditional checks to define how (as well as where) we make
     * the connection request.
     *
     * Connecting to the resource is exception wrapped and any unsuccessful attempt will cause the class member $status
     * variable to be set to Boolean(false) as well as recording diagnostic and error information.
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
    public function __construct()
    {
        /** @var MongoDB\Driver\Manager $manager */
        $manager = null;
        $haveRBAC = false;
        $haveSSL = false;
        $sslParam1 = '&ssl=true';
        $sslParam2 = '?ssl=true';
        $mongoOptions = [];
        $context = null;
        $mongoURIPort = $this->uri . ':' . $this->port;
        $mDSN = 'mongodb://';
        try {
            if (!empty($this->user)) {
                // check and ensure that we have a password and a authDB
                if (empty($this->password) or empty($this->authDB)) {
                    $this->errors[] = 'Error - missing required data';
                    $this->errors[] = 'Mongo RBAC requires a username, password and authentication database';
                    return;
                } else {
                    $mongoDSN = $mDSN . $this->user . ':' . $this->password . '@';
                    $haveRBAC = true;
                }
            } else {
                $mongoDSN = $mDSN;
            }

            if (isset($this->ssl) and !empty($this->ssl) and is_array($this->ssl)) {
                $haveSSL = true;
                $context = stream_context_create($this->ssl);
            }

            if (isset($this->replSetName) and isset($this->replSetMembers) and is_array($this->replSetMembers)) {
                foreach ($this->replSetMembers as $replSetNode)
                    $mongoDSN .= $replSetNode . ',';
                $mongoDSN = rtrim($mongoDSN, ',');
                if (!empty($this->database))
                    $mongoDSN .= '/' . $this->database . '?replicaSet=' . $this->replSetName;
                else
                    $mongoDSN .= '/?replicaSet=' . $this->replSetName;
                if ($haveSSL) $mongoDSN .= $sslParam1;
            } elseif (!$haveRBAC) {
                $mongoDSN .= $mongoURIPort;
                if ($haveSSL) {
                    $mongoDSN .= '/' . $this->database . $sslParam2;
                }
            } else
                $mongoDSN .= $mongoURIPort . '/' . $this->authDB;

            if (isset($this->RP))
                $mongoOptions['readPreference'] = $this->RP;

            if (is_null($context))
                $manager = new MongoDB\Driver\Manager($mongoDSN, $mongoOptions);
            elseif ($haveSSL)
                $manager = new MongoDB\Driver\Manager($mongoDSN, $mongoOptions, [ 'context' => $context]);
            else {
                echo 'Unable to derive a valid manager object - returning...' . PHP_EOL;
                return;
            }
            $command = new MongoDB\Driver\Command(['ping' => 1]);
            $results = $manager->executeCommand($this->database, $command);
            var_export(current($results->toArray()));
            echo PHP_EOL;
            $this->namespace = $this->database . '.' . $this->table;
            $this->mdbRes = $manager;
            $this->status = true;
        } catch (\MongoDB\Driver\Exception\ConnectionException |
                 \MongoDB\Driver\Exception\InvalidArgumentException |
                 \MongoDB\Driver\Exception\RuntimeException |
                 \MongoDB\Driver\Exception\Exception |
                 Throwable $e) {
            $msg = sprintf('%s@%d-->', basename(__FILE__), __LINE__);
            echo $msg . 'An exception was trapped connecting to mongo' . PHP_EOL;
            echo $e->getMessage() . PHP_EOL;
        }
    }


    /**
     * guid() -- public function
     *
     * this works in all php versions and generates a version 4 UUID.
     *
     * the function takes a single input parameter: $_useBraces -- which defaults to Boolean(false).
     * if $_useBraces is true, then we'll return the GUID surrounded by braces (Microsoft format), otherwise, just
     * return the GUID.
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param bool $_useBraces
     * @return string
     *
     * HISTORY:
     * ========
     * 11-25-18     mks     original coding
     *
     */
    public function guid($_useBraces = false)
    {
        if (function_exists('com_create_guid')) {
            return com_create_guid();
        } else {
            mt_srand((double)microtime() * 10000);                        //optional for php 4.2.0 and up.
            $charId = strtoupper(md5(uniqid(rand(), true)));
            $hyphen = chr(45);          // "-"
            if ($_useBraces) {
                /** @noinspection PhpUnusedLocalVariableInspection */
                $uuid = chr(123);       // "{"
            }
            $uuid = substr($charId, 0, 8) . $hyphen
                .substr($charId, 8, 4) . $hyphen
                .substr($charId,12, 4) . $hyphen
                .substr($charId,16, 4) . $hyphen
                .substr($charId,20,12);
            if ($_useBraces) {
                $uuid .= chr(125);      // "}"
            }
            return $uuid;
        }
    }


    /**
     * validateGUID() -- public function
     *
     * This function requires a single input parameter, a GUID value, which is then parsed against a regExp
     * expression to ensure that the format matches that required to be a valid GUID.
     *
     * Function returns a boolean value to indicate if the GUID is valid or not.
     *
     *
     * @param string $_guid
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 12-02-18     mks     original coding
     *
     */
    public function validateGUID(string $_guid): bool
    {
        return(boolval(preg_match('/^\{?[A-Z0-9]{8}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{12}\}?$/', strtoupper($_guid))));
    }
}
