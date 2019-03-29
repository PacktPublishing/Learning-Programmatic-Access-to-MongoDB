from Models import MongoConnectorDataModel
from pymongo import MongoClient
from pymongo import errors as mongo_errors
import re
import ssl


class MongoConnectorModel:
    """
    __init__() -- constructor method

    This is the constructor method; when the class is instantiated, this code executes.

    The class uses a member, called status, to indicate if the connection to the mongoDB was successful or not.
    Configuration information for the connection is defined in the model/class: MongoConnectorDataModel.py

    This method parses the set-parameters in MongoConnectorDataModel class and builds the correct connection
    directive based on the configuration.  Processing starts as follows:

    1. Assign/Initialize local variables (lvars)
    2. Instantiate the MongoConnectorDataModel class
    3. Assign the read-preference -- if not set, default to primaryPreferred
    4. Create the mongo_uri variable to hold the connection string (argument 1 to MongoClient())
    5. Check if we're using RBAC and, if so, toggle a flag for later processing
    6. Check if we're connecting to a replication-set and, if so, build the replication set member list and tag
    7. Otherwise, use the default URL:PORT (this works same way for sharded clusters)
    8. If SSL is enabled and we'll be connecting over TLS, then build one of two connection requests depending
       on the state of the RBAC flag we set in step 5
    9. If SSL is not enabled, then connect either using RBAC or just a simple connection

    The connection attempt is exception wrapped in a MongoConnectionFailure exception and in a general Exception.
    If an exception is trapped, display the exception message and return.  Otherwise, if we connnected to the
    mongoDB service successfully, toggle the class member status to True and return.

    @author     mshallop@linux.com
    @version    1.0

    HISTORY:
    ========
    12-29-18        mks     original coding
    01-06-19        mks     corrected SSL parameters for connection resource

    """

    # lvar init
    status = False

    def __init__(self):
        # lvar init
        add_auth = False

        connect_data = MongoConnectorDataModel.MongoConnectorDataModel()
        if connect_data.uri is None:
            connect_data.uri = 'localhost'
        if connect_data.port is None:
            connect_data.port = 27017

        # set the read-preference for this connection
        read_preference = connect_data.readPreference if connect_data.readPreference is not None else 'primaryPreferred'

        # set-up the base uri
        mongo_uri = 'mongodb://'

        # check to see if we're using RBAC and, if so, toggle a variable
        if connect_data.login is not None and connect_data.password is not None and connect_data.authDB is not None:
            add_auth = True

        # if we're connecting to a replSet, then build the replSet connect string
        if connect_data.replSetName is not None and connect_data.replSet is not None:
            for replSet_node in connect_data.replSet:
                mongo_uri += replSet_node + ','
            mongo_uri = re.sub(',$', '', mongo_uri)  # strip trailing comma from string
            mongo_uri += '/?replicaSet=' + connect_data.replSetName
        else:           # otherwise, connect to a single node (either mongod or mongos)
            mongo_uri += '%s:%s' % (connect_data.uri, connect_data.port)

        # starting with the most complex option, eval the connection config to see how to connect to mongoDB
        try:
            if connect_data.ssl is not None:  # we have SSL config -- connect to the DB using TLS
                if add_auth:
                    self.res_mongo = MongoClient(mongo_uri,
                                                 ssl=True,
                                                 readPreference=read_preference,
                                                 username=connect_data.login,
                                                 password=connect_data.password,
                                                 authSource=connect_data.authDB,
                                                 authMechanism='SCRAM-SHA-1',
                                                 ssl_certfile=connect_data.ssl[0]['cert_file'],
                                                 ssl_cert_reqs=ssl.CERT_REQUIRED,
                                                 ssl_ca_certs=connect_data.ssl[0]['key_file'])
                else:
                    self.res_mongo = MongoClient(mongo_uri,
                                                 ssl=True,
                                                 readPreference=read_preference,
                                                 connect=False,
                                                 connectTimeoutMS=500,
                                                 serverSelectionTimeoutMS=1000,
                                                 ssl_certfile=connect_data.ssl[0]['cert_file'],
                                                 ssl_cert_reqs=ssl.CERT_REQUIRED,
                                                 ssl_ca_certs=connect_data.ssl[0]['key_file'])
            else:
                # we're not connecting over TLS/SSL
                if add_auth:
                    self.res_mongo = MongoClient(mongo_uri,
                                                 readPreference=read_preference,
                                                 username=connect_data.login,
                                                 password=connect_data.password,
                                                 authSource=connect_data.authDB)
                else:
                    self.res_mongo = MongoClient(mongo_uri, readPreference=read_preference)
            self.status = True
        except (mongo_errors.ConnectionFailure, Exception) as err:
            print('Exception caught: {0}' . format(err))
