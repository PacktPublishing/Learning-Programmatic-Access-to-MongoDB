<?php
/**
 * testMongo.php -- Test Stub Program
 *
 * This is the test-stub for our demonstration program.  From this program, we instantiate (invoke) the cUserManager
 * class which, in turn, instantiates our class abstractions aMongoToolBox (where our CRUD code is located), and
 * aMongoConnectors (where we create our connection payload and connect to the mongoDB service), so that we can
 * manipulate (CRUD) our user data.
 *
 * In practice, we'd use the program to test our program functionality, usually along with a debugger, to ensure
 * we've met all requirements and that the program performs as expected.
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

// load the cUserManager Module as we're directly invoking this class
try {
    include "cUserManager.inc";
} catch (ParseError $p) {
    $msg = sprintf("%s@%d-->", basename(__FILE__), __LINE__);
    echo $msg . 'Parse error encountered!' . PHP_EOL;
    echo $p->getMessage() . PHP_EOL;
    exit(1);
}

const OP_CREATE = 1;
const OP_FETCH = 2;
const OP_UPDATE = 3;
const OP_DELETE = 4;

$thisOP = OP_DELETE;            // this value will change depending on the type of testing we doing

// replicationSet Nodes listed as an indexed array
$rs = [
    '192.168.1.133:27018',
    '192.168.1.116:27018',
    '192.168.1.141:27018'
];

// connectivity data -- depending on which fields are completed defines the connection type
$data = [
    $uri = '192.168.1.57',
    $port = 27017,
//    $login = 'gaAdmin',
//    $pass = 'einstein',
//    $authDB = 'admin',
//    $replSetName = 'namasteShard1',
//    $replSet = $rs,
    $login = '',
    $pass = '',
    $authDB = '',
    $replSetName = '',
    $replSet = null,
    $readPreference = 'secondaryPreferred',
    $database = 'test',
    $table = 'users',
    $ssl = [ 'ssl' => [
        'peer_name' => '192.168.1.57',
        'verify_peer' => true,
        'verify_expiry' => true,
        'verify_peer_name' => true,
        'allow_self_signed' => true,
        'ca_file' => '/etc/certs/rootCA.pem',
        'pem_file' => '/etc/certs/mongoClient.pem'
    ]]
];

$newUser = [ 'username' => 'mshallop',
             'password' => 'letmein!',
             'email' => 'mshallop@linux.com'
           ];

$updateData = [
    'flName' => 'Sterling Archer',
    'password' => 'SerenityValley!',
    'phones' => [
        'home' => '555-1212',
        'work' => '555-1211'
    ]
];

$userKey = 'AC9C0E60-F9C4-37D0-7DB1-03DCE3A9AD96';

// instantiate the class stack and output the object on success
try {
    $toolBox = new cUserManager($data);
    if (!$toolBox->status) {
        echo 'Could not instantiate the class stack!' . PHP_EOL;
        foreach ($toolBox->errors as $error)
            echo $error . PHP_EOL;
    } else {
        echo 'Successfully Connected to mongoDB!!!' . PHP_EOL;
    }
    switch ($thisOP) {
        case OP_CREATE :
            $hashedPassword = $toolBox->validateNewUserData($newUser);
            if (!is_null($hashedPassword)) {
                $newUser['password'] = $hashedPassword;
                if (!$toolBox->addUsers($newUser)) {
                    echo 'Create new user request failed!' . PHP_EOL;
                    foreach ($toolBox->errors as $error)
                        echo $error . PHP_EOL;
                    var_export($toolBox->errors);
                    echo PHP_EOL;
                } else {
                    echo 'User ' . $newUser['username'] . ' account was successfully created!' . PHP_EOL;
                    var_export($toolBox->info);
                    echo PHP_EOL;
                }
            }
        break;
        case OP_UPDATE :
            if (!$toolBox->updateUser($userKey, $updateData)) {
                var_export ($toolBox->errors);
            } else {
                echo 'User GUID: ' . $userKey . ' record was successfully updated!' . PHP_EOL;
                var_export($toolBox->info);
                echo PHP_EOL;
            }
        break;
        case OP_DELETE :
            $email = 'mshallop@linux.com';
            echo 'Delete user: ' . $email . ' request ';
            if (!$toolBox->deleteUser($email)) {
                echo 'has failed!' . PHP_EOL;
                foreach ($toolBox->errors as $error)
                    echo $error . PHP_EOL;
            } else {
                echo 'has succeeded!' . PHP_EOL;
            }
        break;
    }
} catch (TypeError $t) {
    $msg = sprintf("%s@%d-->", basename(__FILE__), __LINE__);
    echo $msg . 'Caught TypeError: ' . PHP_EOL;
    echo $t->getMessage() . PHP_EOL;
    exit(1);
}
