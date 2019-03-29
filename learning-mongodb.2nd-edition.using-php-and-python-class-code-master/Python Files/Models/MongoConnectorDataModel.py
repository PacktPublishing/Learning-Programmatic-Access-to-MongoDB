"""
MongoConnectorDataModel -- Configuration Class

This class is used to store configuration information for connecting to the remote mongoDB service.

If a field is not-applicable, then ensure that it is populated with a None value.

Also included are some data payloads used for testing various CRUD operations

@author     mshallop@linux.com
@version    1.0

HISTORY:
========
12-29-18        mks     original coding

"""


class MongoConnectorDataModel:

    def __init__(self):
        self.uri = '192.168.1.57'
        self.port = 27017
        self.login = None
        self.password = None
        self.authDB = None
        self.replSetName = None
        self.replSet = [{
            'node1': 'host1:port1',
            'node2': 'host2:port2',
            'node3': 'host3:port3'
        }]
        self.readPreference = 'secondaryPreferred'
        self.database = 'test'
        self.table = 'users'
        self.ssl = [{
            'peer_name': '192.1688.1.57',
            'verify_peer': True,
            'verify_expiry': True,
            'verify_peer_name': True,
            'allow_self_signed': True,
            'key_file': '/etc/certs/rootCA.pem',
            'cert_file': '/etc/certs/mongoClient.pem'        # ssl cert file
        }]
        self.newUser = [{
            'username': 'mshallop',
            'password': 'letmein!',
            'email': 'mshallop@linux.com'
        }]
        self.update_data = [{
            'flName': 'Pam Poovey',
            'password': 'Dp you want ants?',
            'phone': [{
                'home': '555-1212',
                'work': '555-1213'
            }]
        }]
