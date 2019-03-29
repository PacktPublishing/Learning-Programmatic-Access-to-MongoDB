from pymongo import errors as mongo_errors
from Models import HelperModel as Helper
import time

"""
MongoToolbox.py  -- Model for all things mongoDB

This file contains the code that interfaces with the mongoDB API.  All (any) code that interacts with mongoDB should
be located in this, and only this, model to minimize maintenance and debugging on events that interact with the 
py-mongo package.  

@author     mshallop@linux.com
@version    1.0

HISTORY:
========
01-06-19        mks     original coding begins
01-20-19        mks     refactored for scalable processing and generic data handling

"""


class MongoToolbox:
    # set up some class member variables
    status = False
    mongo_resource = None
    database = None
    collection = None
    new_user_id = None
    number_records_matched = None
    number_records_updated = None
    number_records_deleted = None

    def __init__(self, mongo_resource):
        """
        __init__()  --  class instantiation method

        The class instantiation method assigns default parameters to class members that were derived during
        the connection (to mongodb) instantiation.

        There is a required input parameter to the method - this is the mongo-resource object that was created
        when we instantiated the mongoConnector model.  From this, we'll derive and assign the mongodb resources
        to class member variables.

        @author     mshallop@linux.com
        @version    1.0

        :param mongo_resource: this is the mongodb resource that was instantiated in the mongoConnector class
        instantiation.

        HISTORY:
        ========
        01-06-19    mks     original coding

        """
        self.mongo_resource = mongo_resource
        self.database = self.mongo_resource.test  # name of our database
        self.collection = self.database.users  # name of the database collection

    def check_for_existing_account(self, user, email):
        """
        check_for_existing_account() -- mongoToolbox method

        This method requires two input parameters:

        The user parameter is the clear-text for the account's user name.
        The email parameter is the clear-text for the account's email address.

        The purpose of this method is to search the database looking for either the user's username or the user's
        email address.  If either strings are located in the database, then we're going to return a boolean false,
        and a diagnostic message, indicating that the search "failed".  (Objective is that the username/email
        combination does not already pre-exist in the database.)

        We don't validate the inputs, with respect to min or max lengths, etc., as validation has already occurred.

        Build a mongodb query to search for any record with the username OR with the email.  If neither are found in
        the db search, return a boolean(true) value and no diagnostic message.

        :param user:  string containing the user's username
        :param email: string containing the user's email address
        :exception: traps mongo and general exception on the query request
        :return: returns a boolean value to indicate if the username/email was not found (TRUE) and an empty (None)
        value for the diagnostic.  Otherwise, if either the username or the email address pre-exists in the db, then
        return a Boolean(False) with a second return value, a string, that contains a diagnostic message.

        @author     mshallop@linux.com
        @version    1.0

        HISTORY:
        ========
        01-06-19        mks     original coding

        """
        user_list = []
        try:
            found = self.collection.find({"$or": [{"username": user}, {"email": email}]})
            for user in found:
                user_list.append(user['username'])
            if len(user_list) == 0:
                return True
            else:
                print('username or email is already in-use')
                return False
        except (mongo_errors.PyMongoError, Exception) as e:
            print('a mongo exception was trapped: %s - %s' % (e.__class__, e))
            return False

    def add_user(self, user_data):
        """
        add_user() -- mongoToolbox method

        This method requires a single input parameter:

        user_data -- this is an dictionary containing an aggregation of all the user'data needed to create a new
        user account.

        Because this data payload is built internally (to this back-end), there is no validation as we'll assume that
        the dictionary keys "password", "email" and "username" exist in the dictionary.

        We'll hash the password replacing the clear-text version and then invoke the insert_one_record() method to
        add the new user to the users table.  The return value indicating success or failed from the method is
        passed directly back to the calling client without evaluation; it's the responsibility of the calling client
        to evaluate the return value.

        @author     mshallop@linux.com
        @version    1.0

        :param user_data: a dictionary of key-value pairs representing the user data that will be inserted into mongodb
        :return: a boolean and, if an error, a diagnostic message (explaining why the insert failed)

        HISTORY:
        ========
        01-06-19        mks     original coding

        """
        user_data['password'] = Helper.hash_string(user_data['password'])
        return self.insert_one_record([user_data])

    def insert_one_record(self, data, db=None, collection=None):
        """
        insert_one_record() -- mongoToolbox method

        this method was written to make the mongo toolbox scalable and generic (with respect to data classes).  The
        main assumption here is that the data payload, which is a dictionary list, has already been evaluated as having
        one record resulting in the invocation of this method.

        There are a total of three (user) input parameters, the last two being optional:

        The first input parameter is a single record's worth of data as a dictionary structure
        The second input is optional and is a string containing the name of the database to which the data will be
        written -- this is used to override the db setting set in the constructor.
        The third parameters is also optional, is also a string, and contains the name of the collection to which the
        record will be written, overriding the default set in the constructor.

        We inject a record guid and a processing time into the record to be added to the collection and then call the
        pyMongo insert_one() method.  This is exception-wrapped and will result in an error message and a False return
        value if an exception is raised.  Otherwise, the record is inserted -- we'll save the mongo _id value in a
        member variable in case the client needs it later and return a Boolean(true) on a successful insert event.

        :param data:        a list containing a single dictionary record
        :param db:          optional - alternative database name
        :param collection:  optional - alternative collection name
        :return:            Boolean indicating if the record was successfully inserted

        @author     mshallop@linux.com
        @version    1.0

        HISTORY:
        ========
        01-20-19        mks     original coding

        """
        # check if we're going to override the default db or collection
        if db is not None:
            self.database = self.mongo_resource.db
        if collection is not None:
            self.collection = collection
        # ensure that data only has one record
        if len(data) == 1:
            try:
                # inject meta fields into record
                data[0]["token"] = Helper.generate_guid()
                data[0]["created"] = int(time.time())
                # invoke the pycharm insert_one() method
                result = self.collection.insert_one(data[0])
                self.new_user_id = result.inserted_id
                return True
            except (mongo_errors.PyMongoError, Exception) as e:
                print('a mongo exception was trapped: %s - %s' % (e.__class__, e))
                return False
        else:
            print('Error - data payload for insert_one_record contained more than one record')
            return False

    def insert_many_records(self, data, db=None, collection=None):
        """
        insert_many_records() -- mongoToolbox method

        This method is used to perform a database insert when we have more than a single record to be inserted into a
        collection.  There are three input parameters, two of which are optional, to this method:

        The data parameter is an iterable of documents to insert.
        The db parameter is optional and can be used to override the destination database set in the constructor
        The collection parameter is also optional and can be used to override the collection set in the constructor

        We loop through the list of documents and we first inject the meta fields into every document.  When that's
        completed, we call the pyMongo insert_many() method to insert all of the records in a single query -- this is,
        of course, exception-trapped.

        If the insert successfully completes, we return a Boolean(true) to the calling client and store the list of
        mongo _id's in the local member.

        Otherwise, we display the exception error message and return a Boolean(false) to the calling client.

        @author     mshallop@linux.com
        @version    1.0

        :param data:        an iterable of documents
        :param db:          optional override string value to replace the default db set in the constructor
        :param collection:  optional override string value to replace the default collection set in the constructor
        :return:            Boolean indicating if the multi-record insert request completed successfully or not

        HISTORY:
        ========
        01-20-19        mks     original coding

        """
        if db is not None:
            self.database = self.mongo_resource.db
        if collection is not None:
            self.collection = collection
        # ensure that the data has more than 1 record
        if len(data) <= 1:
            print('insert_many_records requires a data-set with more than one record')
            return False
        try:
            for i in range(0, len(data)):
                data[i]["token"] = Helper.generate_guid()
                data[i]["created"] = int(time.time())
            result = self.collection.insert_many(data, ordered=False)
            self.new_user_id = result.inserted_ids
            return True
        except (mongo_errors.PyMongoError, Exception) as e:
            print('a mongo exception was trapped: %s - %s' % (e.__class__, e))
            return False

    def update_one_record(self, query, update, upsert_value=False, db=None, collection=None):
        """
        update_one_record() -- mongoToolbox method

        This method handles updating one, and only one, record in a collection.  The following input parameters are
        available:

        query:  this is a final-form query-filter that's passed directly to pyMongo:  { key: value, ... }  The target
        record to be updated must satisfy this query.  If more than one record is returned by the query, mongo will
        randomly update one of those records -- so make sure your key is predicated on a unique field!
        update:  this is a final-form $set directive to mongo.  For mongo updates, we use the $set directive which
        basically looks like: { $set : { f1: v1, ... Fn: vn }}  This is basically a list of the fields that will be
        modified/added to the record.
        upsert_value: A Boolean value defaulting to False -- if set to true: if no record satisfies the query filter,
        then the update payload will be inserted into the collection as a new record.
        db: string value allowing the calling client to switch to a new db within the same connected resource
        collection: string value allowing the calling client to switch to a new collection with the named db

        There are two class member variables (number_records_matched, number_records_updated) which are set to the
        appropriate values and are accessible by the calling client.

        @author     mshallop@linux.com
        @verion     1.0

        :param query:           array containing final form query filter for mongo
        :param update:          array containing $set directive for mongo update operation
        :param upsert_value:    boolean value for upsert, defaults to false
        :param db:              string value: select a different db within the same connected resource
        :param collection:      string value: select a different collection with the named db
        :return:                Boolean indicating if the update command completed successfully

        HISTORY:
        ========
        01-20-19        mks     original coding

        """
        self.number_records_matched = 0
        self.number_records_updated = 0
        if db is not None:
            self.database = self.mongo_resource.db
        if collection is not None:
            self.collection = collection
        try:
            result = self.collection.update_one(query, update, upsert=upsert_value)
            self.number_records_matched = result.matched_count
            self.number_records_updated = result.modified_count
            return True
        except (mongo_errors.PyMongoError, Exception) as e:
            print('a mongo exception was trapped: %s - %s' % (e.__class__, e))
            return False

    def update_many_records(self, query, update, upsert_value=False, db=None, collection=None):
        """
        update_many_records() -- mongoToolbox method

        This method is identical to the previous update_one_record except that it calls the pyMongo method
        update_many() instead of update_one().

        The following input parameters are
        available:

        query:  this is a final-form query-filter that's passed directly to pyMongo:  { key: value, ... }  The target
        record to be updated must satisfy this query.  If more than one record is returned by the query, mongo will
        randomly update one of those records -- so make sure your key is predicated on a unique field!
        update:  this is a final-form $set directive to mongo.  For mongo updates, we use the $set directive which
        basically looks like: { $set : { f1: v1, ... Fn: vn }}  This is basically a list of the fields that will be
        modified/added to the record.
        upsert_value: A Boolean value defaulting to False -- if set to true: if no record satisfies the query filter,
        then the update payload will be inserted into the collection as a new record.
        db: string value allowing the calling client to switch to a new db within the same connected resource
        collection: string value allowing the calling client to switch to a new collection with the named db

        There are two class member variables (number_records_matched, number_records_updated) which are set to the
        appropriate values and are accessible by the calling client.

        @author     mshallop@linux.com
        @verion     1.0

        :param query:           array containing final form query filter for mongo
        :param update:          array containing $set directive for mongo update operation
        :param upsert_value:    boolean value for upsert, defaults to false
        :param db:              string value: select a different db within the same connected resource
        :param collection:      string value: select a different collection with the named db
        :return:                Boolean indicating if the update command completed successfully

        HISTORY:
        ========
        01-20-19        mks     original coding

        """
        self.number_records_updated = 0
        self.number_records_matched = 0
        if db is not None:
            self.database = self.mongo_resource.db
        if collection is not None:
            self.collection = collection
        try:
            # do not need the old "multi=true" param - that's implied by update_many()
            result = self.collection.update_many(query, update, upsert=upsert_value)
            self.number_records_matched = result.matched_count
            self.number_records_updated = result.modified_count
            return True
        except (mongo_errors.PyMongoError, Exception) as e:
            print('a mongo exception was trapped: %s - %s' % (e.__class__, e))
            return False

    def delete_records(self, query_filter, db=None, collection=None, multi=False):
        """
        delete_records() -- mongoToolbox method

        This method is the delete event handler for mongo -- unlike previous methods (insert, update), the delete
        method now supports both delete_one() and delete_many() pyMongo invocations.

        The input parameters are as follows:

        query_filter:   the query, in array format, of the filter that must be satisfied in order to remove a record
        db:             a string value containing the name of an alternative database that exists in the same resource
        collection:     a string value containing the name of the collection to use, if not the default
        multi:          a boolean value that drives if we'll invoke delete_one() (false) or delete_many() (true)

        We start by resetting the member variable that reports back on how many records were removed in the operation.
        Next, we processing the DB and collection overrides...
        Then we test the multi parameter and if false (default), we invoke the delete_one() method in pyMongo - else
        we invoke pyMongo's delete_many() method.  Both use the same input parameter.

        :param query_filter: the query filter, in array format, that determines which records are deleted
        :param db:           a string value, optional, containing the name of an alternative database
        :param collection:   a string value, optional, containing the name of an alternative collection
        :param multi:        a boolean value, optional, indicating which pyMongo delete operation to invoke
        :return:             a boolean value to indicate if the delete request was successfully processed or not

        @author     mshallop@linux.com
        @version    1.0

        HISTORY:
        ========
        01-20-19        mks     original coding

        """
        self.number_records_deleted = 0
        if db is not None:
            self.database = self.mongo_resource.db
        if collection is not None:
            self.collection = collection
        try:
            if multi is False:
                result = self.collection.delete_one(query_filter)
            else:
                result = self.collection.delete_many(query_filter)
            self.number_records_deleted = result.deleted_count
            return True
        except (mongo_errors.PyMongoError, Exception) as e:
            print('a mongo exception was trapped: %s - %s' % (e.__class__, e))
            return False
