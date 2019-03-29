<?php
/**
 * cUserManager.inc -- PHP Class Extends aMongoToolBox abstraction
 *
 * This class is the "User Manager" class for our project:  Learning MongoDB, Second Edition, Programming with MongoDB.
 *
 * The intent of the class is to encapsulate all user functionality for a back-end system.  Specifically, to handle
 * user functionality such as:
 *    - Validate User Account (login/password challenges)
 *    - Check for Pre-Existing Account
 *    - Etc.  (Other functionality t.b.d.)
 *
 * The responsibility of this class is to provide validation and verification of data assigned to class members, or
 * passed into class method constructors, and to prepare data for database operations with the aMongoToolbox abstraction
 * class.  In other words, this class should contain no direct-to-mongo functionality but, rather, invoke those
 * method using the validated data.
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

// load the MongoToolbox abstraction class, wrapping the load within an ParseError trap, until we install an autoloader
try {
    require_once "aMongoToolbox.inc";
} catch (ParseError $p) {
    $msg = sprintf("%s@%d-->", basename(__FILE__), __LINE__);
    echo $msg . 'Parse error encountered!' . PHP_EOL;
    echo $p->getMessage() . PHP_EOL;
    exit(1);
}


/**
 * Class cUserManager
 *
 * This class handles all first-level processing for User-related events.  The class has the responsibility to collect
 * and validate data before submitting CRUD event requests deeper into the class stack.
 */
class cUserManager extends aMongoToolbox
{
    /**
     * cUserManager constructor.
     *
     * The constructor method takes a single input parameter -- an associative array containing the key->value pairs
     * that determine the login method.
     *
     * The input array requires the following fields:
     *
     *    - uri: this is the address for the mongo connection
     *    - port: this is the port the mongo service is listening on at the URI above
     *    - user: if provided, this contains the user's name required for Roll-Based-Access-Controls (RBAC)
     *    - password: if provided, and this is required if the user is provided, the user's RBAC password
     *    - authDB: if provided, the name of the mongo database against which the user's access controls are auth'd
     *    - replSetName:  if we're connecting to a replication set, we must provide the repl-set name
     *    - replSetMembers: if we're connecting to a replication set, then this is an indexed array containing the
     *                      the URI and port numbers (e.g.: 192.168.1.1:27017) for each repl-set member
     *    - RP: Defines the read-preference for the mongoDB service
     *    - database: the name of the database we'll connect to
     *    - table: the name of the table we'll connect to
     *    - ssl: an associative array with one key "ssl" that points to a sub-array, also associative, or well-defined
     *           key-value pairs that set-up a TLS connection to the remote mongoDB service
     *
     *
     *
     * @param array $_mongoConnectData -- associative array containing misc. connection information
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
    public function __construct(array $_mongoConnectData)
    {
        // use a language construct to assign a list of variables in one operation
        list($this->uri, $this->port, $this->user,
             $this->password, $this->authDB, $this->replSetName,
             $this->replSetMembers, $this->RP, $this->database,
             $this->table, $this->ssl) = $_mongoConnectData;
        // invoke the parent constructor
        parent::__construct();
    }


    /**
     * addUsers() -- public function
     *
     * This function requires an array of user data.  Input can be submitted in one of two formats:
     *
     *    -- an associative array containing at most 1 record
     *    -- an indexed array of associative arrays containing more than one record
     *
     * The associative arrays are key-value pairs that match the user table schema (keys) and data (values).
     *
     * If we receive an array of the first type (single record, associative) then convert the array into an indexed
     * array of associative arrays.  Otherwise leave the input data alone.
     *
     * Pass the entire data structure to the createRecords() method which is designed be the generic CREATE method
     * (CRUD) for the class stack.  The return value from createRecords() is passed through and back to the calling
     * client.
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
     * 11-18-18     mks     original coding
     *
     */
    public function addUsers(array $_data): bool
    {
        if (key($_data) !== 0 and count($_data) > 1)
            $_data = [$_data];
        return($this->createRecords($_data));
    }


    /**
     * deleteUser() -- public function
     *
     * This function requires a single string as input - we're going to delete a user based on the email address
     * submitted by the client.
     *
     * If we validate the email address itself, (preventing wildcard and null queries), then we'll pass a mongo
     * query to the deleteRecords() method in the mongo toolbox.  Since deleteRecords() returns a boolean, we'll
     * pass-through the return value to the calling client.
     *
     * todo: add a password to the input s.t. we first validate (as in a login) the user's delete request
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param string $_email
     * @return bool
     *
     * HISTORY:
     * ========
     * 12-16-18     mks     original coding
     *
     */
    public function deleteUser(string $_email): bool
    {
        if (!$this->validateEmailAddress($_email)) {
            return false;
        }
        return($this->deleteRecords(['email' => $_email], true));
    }


    /**
     * updateUser() --  public function
     *
     * This function requires two input parameters:
     *
     * $_key  - this is the GUID value assigned to a specific user account to be updated
     * $_data - this is the payload data of key-value pairs that are the update data
     *
     * The method returns a boolean value to indicate if the update successfully completed.
     *
     * We generate a list of allowable field names as means of ensuring data quality control -- specifically, we don't
     * want users to add fields to a user record that are outside of spec.
     *
     * We begin by validating that we received a valid guid (if not, add an diagnostic message to the error container
     * and return a boolean(false) to the calling client.
     *
     * Next, we ensure that we have data in the $_data array and then we copy the data to a temporary variable that
     * will allow us to delete values from the array if the keys do not match the values in the allowed key list.
     * While looping through the data payload, we also validate that the value of the data provided matches the
     * expected type.  A type mismatch will cause the field to be dropped.
     *
     * While looping through the data, if we encounter a password field, call the method to hash the password and
     * replace the clear-text value with the hashed value.
     *
     * If data remains following validation, create a payload bundle and submit it to the updateRecords() method.
     * The return value from updateRecords() is passed directly back to the calling client without evaluation.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param string $_key
     * @param array $_data
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 12-02-18     mks     original coding
     *
     */
    public function updateUser(string $_key, array $_data): bool
    {
        $allowedFieldsForUpdate = [
            'flName' => 'string',
            'password' => 'string',
            'username' => 'string',
            'email' => 'string',
            'phones' => 'array'
        ];
        if (!$this->validateGUID($_key)) {
            $this->errors[] = 'This is not a valid GUID: ' . $_key . PHP_EOL;
            return false;
        }
        if (!is_array($_data) or empty($_data)) {
            $this->errors[] = 'Expecting an array of update data - received empty value' . PHP_EOL;
            return false;
        }
        $dataCopy = $_data;
        foreach ($dataCopy as $key => $value) {
            if (!array_key_exists($key, $allowedFieldsForUpdate)) {
                $this->errors[] = $key . ' is not a valid member of the users record' . PHP_EOL;
                unset($dataCopy[$key]);
            } elseif (gettype($_data[$key]) != $allowedFieldsForUpdate[$key]) {
                $msg = $key . ' expecting type ' . $allowedFieldsForUpdate[$key];
                $msg .= ', received type: ' . gettype($_data[$key]) . PHP_EOL;
                $this->errors[] = $msg;
                unset($dataCopy[$key]);
            } elseif ($key == 'password') {
                $dataCopy[$key] = $this->hashPassword($value);
            }
        }
        if (!empty($dataCopy)) {
            $query = ['token' => $_key];
            $options = ['multi' => false, 'upsert' => false];
            return $this->updateRecords($query, $dataCopy, $options);
        } else {
            $this->errors[] = 'Data provided to the update did not meet minimum requirements - cannot update';
            return false;
        }
    }


    /**
     * validateNewUserData -- public method
     *
     * This method is the entry point for validating user data.  Specifically, the method validates the user's submitted
     * password, email address, and username.  This method is called when we need to create a new user account and we
     * must first validate/approve the data in the request.
     *
     * The method requires a single associative array as it's only input and this array must contain the columns/keys
     * of: 'username', 'password', and 'email'.  If not, then the data fetched will be empty and will fail validation.
     *
     * Once all the data has been validated, then the password is hashed and, if successfully hashed, the 60-character
     * hash key fill be returned.  Otherwise, the method will return a null value and it's the client's responsibility
     * to check the return payload.
     *
     * For creating new users, the following conditions must all be met:
     *
     *    -- The user's password must meet the length requirements
     *    -- The user's email must be valid
     *    -- The user's email and username must not already exist in the database
     *
     * Successful processing is denoted by returning a 60-character password hash back to the calling client.
     *
     *
     * @author  mshallop@linux.coml
     * @version 1.0
     *
     * @param array $_data
     * @return null|string
     *
     *
     * HISTORY:
     * ========
     * 11-18-18     mks     original coding
     *
     */
    public function validateNewUserData(array $_data): ?string
    {
        $userName = $_data['username'];
        $userPassword = $_data['password'];
        $userEmail = $_data['email'];

        if (!$this->validatePasswordLength($userPassword)) return null;
        if (!$this->validateEmailAddress($userEmail)) return null;
        if (!$this->validateUsernameAndEmail($userName, $userEmail)) return null;
        return ($this->hashPassword($userPassword));
    }


    /**
     * validatePasswordLength() -- private method
     *
     * This method requires a single input parameter, a string, which is the user's submitted password in clear-text
     * format.
     *
     * The method checks to ensure that the string length of the password falls between (inclusive) the min and max
     * values and, is so, returns a Boolean(true) value.  If not, we'll populate the member error list container with
     * the appropriate message and return a Boolean(false) value to the calling client.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param string $_password
     * @return bool
     *
     *
     * HISTORY:
     * ========
     * 11-18-18     mks     original coding
     *
     */
    private function validatePasswordLength(string $_password): bool
    {
        if (strlen($_password) < 8 or strlen($_password) > 16) {
            $this->errors[] = 'Password is incorrect length';
            return false;
        }
        return true;
    }


    /**
     * validateEmailAddress() - private method
     *
     * This method requires a single input parameter to the method:  a string value containing the user's submitted
     * email address.
     *
     * The method breaks down the email string into three components using regular-expression matching:
     *    -- given the email input string is set to:  mshallop@linux.com  the preg_match will create the following
     *       array-based strings:
     *       -- $matches[0] = 'mshallop@linux.com'      // the entire email string
     *       -- $matches[1] = 'mshallop'                // everything to the left of the '@' sign
     *       -- $matches[2] = 'linux.com'               // everything to the right of the '@' sign
     *
     * We use PHP's filter_var() method with the FILTER_VALIDATE_EMAIL option to validate the format of the submitted
     * email address.  (That it matches the RFC specification.)
     *
     * And then we validate that the domain exists as an MX (mail) record with the checkdnsrr() method.
     *
     * The method returns a Boolean value - so if the method returns true, the email address is correctly formed and
     * there exists an MX record for the domain (the user can receive email).  If non of these conditions are
     * satisfied, then a Boolean(false) is returned instead.  For any error encounter, we'll also populate the class
     * error container with an appropriate message.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param string $_email
     * @return bool
     *
     * HISTORY:
     * ========
     * 11-18-18     mks     original coding
     *
     */
    private function validateEmailAddress(string $_email): bool
    {
        $exp = "/^(.*)@(.*)$/";
        preg_match($exp, $_email, $matches);

        if (!empty($matches[1]) and (!filter_var($matches[0], FILTER_VALIDATE_EMAIL))) {
            $this->errors[] = 'Email format invalid';
            return false;
        }
        if (empty($matches[2])) {
            $this->errors[] = 'No domain submitted';
            return false;
        }

        if (!@checkdnsrr($matches[2], 'MX')) {
            $this->errors[] = 'Not a valid domain: ' . $matches[2];
            return false;
        }
        return true;
    }


    /**
     * hashPassword() -- private method
     *
     * This method requires a single input parameter:  a string containing the clear-text password submitted by
     * the user.  We assume that the calling client has already validated the input parameter (not null, correct
     * length, etc.) so we pass the password directly to the password_hash() method.
     *
     * For hashing passwords, we'll accept the default salt key (generated in the function) and specify that we want
     * to use the BCRYPT algorithm.
     *
     * Evaluate the return value from the password_hash() method and if it's Boolean(false) return a null value to the
     * calling client and populate the error container with a message.  Otherwise, return the 60-character hash string.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param string $_password
     * @return null|string
     *
     *
     * HISTORY:
     * ========
     * 11-18-18     mks     original coding
     *
     */
    private function hashPassword(string $_password): ?string
    {
        // example string return: $2y$12$QjSH496pcT5CEbzjD/vtVeH03tfHKFy36d4J0Ltp3lRtee9HDxY3K
        $rc = password_hash($_password, PASSWORD_BCRYPT);
        if ($rc === false) {
            $this->errors = 'password: ' . $_password . ' failed to hash' . PHP_EOL;
            return false;
        }
        return $rc;
    }


    /**
     * validateUsernameAndEmail() -- private method
     *
     * This method has two input parameters, both of which are required:
     *
     * $_username - string defining the username as stored in the database
     * $_email    - string defining the user's email as stored in the database
     *
     * This is a single-use method designed to check/ensure that an existing email or username is not already in-use
     * within the database.  The method returns a boolean value.
     *
     * If the return value is true, then the provided username and email values are NOT in-use, currently, in the
     * database.  If the return value is false, then either (a) a processing error was generated or (b) either one,
     * or the other, or both, pre-exist in the database.
     *
     * Note that the user can only supply the value for searching -- the columns (keys) are hard-coded.
     *
     * If an error is generated during processing, it will be implicitly returned via the $errors member variable.
     *
     *
     * @author  mshallop@linux.com
     * @version 1.0
     *
     * @param string $_username
     * @param $_email
     * @return bool
     *
     * HISTORY:
     * ========
     * 11-15-18     mks     original coding
     *
     */
    private function validateUsernameAndEmail(string $_username, string $_email): bool
    {
        $query = [ STRING_QUERY => [ "\$or" => [[ 'username' => $_username], ['email' => $_email]]]];
        $query[STRING_PROJECTION] = ['_id' => 0];
        if (!$this->fetchRecords($query)) {
            $this->errors[] = 'fetch query failed to execute' . PHP_EOL;
            return false;
        }
        if (count($this->data) != 0) {
            $this->errors[] = 'The requested username or email is already in-use';
            return false;
        }
        return true;
    }
}
