<?php
//ini_set('mongodb.debug', 'stderr');

$opts = [
    'ssl' => [
        'peer_name' => '192.168.1.57',
        'verify_peer' => true,
        'verify_expiry' => true,
        'verify_peer_name' => true,
        'allow_self_signed' => true,
        'ca_file' => '/etc/certs/rootCA.pem',
        'pem_file' => '/etc/certs/mongoClient.pem'
    ]
];
$context = stream_context_create($opts);

try {
    $dsn = sprintf("mongodb://192.168.1.57:27017/test?ssl=true");
    $manager = new MongoDB\Driver\Manager($dsn, [], ['context' => $context]);
    $bulk = new MongoDB\Driver\BulkWrite();
    $bulk->insert(array("very" => "important"));
    $manager->executeBulkWrite("test.someTable", $bulk);
    $query = new MongoDB\Driver\Query(array("very" => "important"));
    $cursor = $manager->executeQuery("test.someTable", $query);
    foreach($cursor as $document) {
        var_dump($document->very);
    }
} catch(\MongoDB\Driver\Exception\Exception | Throwable $e) {
    echo get_class($e), ": ", $e->getMessage(), "\n";
}
