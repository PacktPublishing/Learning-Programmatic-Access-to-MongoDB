from Models import MongoToolbox
from Models import HelperModel
from validate_email import validate_email
import time

"""
UserModel.py -- Model for handling various user-related tasks that aren't generic enough for the Helper model.

Note that we need to import both the toolBox and Helper models.


@author     mshallop@linux.com
@version    1.0


HISTORY:
========
01-06-19        mks     original coding

"""


class UserModel:
    """
    UserModel -- the application user-model -- a class to handle user-management.

    There is one assumption in this class - that we've already instantiated the connection to the mongo resource
    we'll be using for this class.

    @author     mshallop@linux.com
    @version    1.0

    HISTORY:
    ========
    01-06-19        mks     original coding

    """
    mongo_toolbox = None
    error_stack = []

    def __init__(self, mongo_toolbox):
        """
        __init__() -- UserModel Instantiation Class

        The __init__ method requires a single input parameter - the connector resource object that was generated in
        the MongoConnectorModel.

        We simply copy the resource into a member for later use.

        :param mongo_toolbox:  mongo resource object generated in the MongoConnector model

        @author     mshallop@linux.com
        @version    1.0

        HISTORY:
        ========
        01-06-19        mks     original coding

        """
        self.mongo_toolbox = MongoToolbox.MongoToolbox(mongo_toolbox)

    def validate_new_user_data(self, data):
        """
        validate_new_user_data() -- UserModel Method

        This method requires a single input parameter - a dictionary containing the user data necessary to create
        an account.

        This information is presumed to be pulled from a web-client front-end... in this application stack, we simulate
        this via the MongoConnectorDataModel class.

        Once we extract the data from the dictionary, we validate the components to meet minimum requirements -- at any
        time during validation, if an error is raised, we'll return a Boolean(false) value along with a string message
        containing diagnostic information.

        We also submit a request to the MongoToolbox to confirm that neither the user's username nor their email address
        is already in-use in the database.

        @author     mshallop@linux.com
        @version    1.0

        :param data:  dictionary containing the user's username, email, and password
        :return: Boolean value indicating if validation of the user data was successful, includes a diagnostic message
        as a string value if not.

        HISTORY:
        ========
        01-06-19        mks     original coding

        """

        user_name = data['username']
        user_password = data['password']
        user_email = data['email']

        status, err_msg = HelperModel.validate_password_length(user_password)
        if status is False:
            self.error_stack.append(err_msg)
            return status
        is_valid = validate_email(user_email, check_mx=True, verify=True)
        if is_valid is False:
            self.error_stack.append('email: ' + user_email + ' failed validation')
            return False
        return self.mongo_toolbox.check_for_existing_account(user_name, user_email)

    def insert_new_user(self, user_data):
        """
        insert_new_user() -- UserModel method

        This method requires a dictionary of user data (username, password, email) that is passed-through to the
        MongoToolbox method to create a new user account.

        Since validation occurs previous to this method, there is no validation in this method.

        The method returns a Boolean value, passed through directly from the add_user() method, to indicate if the
        user was added to the mongo collection successfully or not.

        If the add_user generates an error message, we simply print it out here.

        todo - incorporate any error diagnostics into the global error stack

        @author     mshallop@linux.com
        @version    1.0

        :param user_data: dictionary containing the user's username, password, and email address
        :return: Boolean indicating if the account was successfully created or not

        HISTORY:
        ========
        01-06-19        mks     original coding

        """
        return self.mongo_toolbox.add_user(user_data)

    def update_user(self, user_data):
        """
        update_user() -- userModel method

        This method is called when we want to execute a query to update one or more user records.  There is one
        input parameter to the method - a dictionary containing key value pairs representing new/replacement data.

        Note that there must be one specific key in the user_data array:  "target_user" -- as the goal of this method
        is to update a user record, we'll use the "target_user" key to indicate which user record will be updated by
        providing that username as the value pair.

        Within the method, we extract that target_user and use that to create the query filter.
        We remove the target_user data from the user_data array.
        We inject the last_updated field into the update data.
        We reformat the update data into a mongoDB $set directive for the update operation.

        :param user_data:   a dictionary containing both the query filter and the update data
        :return:            a boolean value received from the toolbox method indicating event success or failure
        """
        # extract the query filter
        query_filter = {"username": user_data['target_user']}
        del user_data["target_user"]
        # inject the updated time into the record
        user_data['last_updated'] = int(time.time())
        if 'password' in user_data:
            user_data['password'] = HelperModel.hash_string(user_data['password'])
        user_data = {"$set": user_data}
        return self.mongo_toolbox.update_one_record(query_filter, user_data)

    def delete_user(self, user_data):
        """
        delete_user() -- userModel method

        This method is called when we need to delete, one or more, users from the user collection.  There is one input
        parameter to the method - a string containing the user name of the target user to be deleted.

        LIMITATION:  only one user can be deleted at a time, and only by the user name.

        todo:  expand input parameters to allow for more complex filters by passing in a dictionary

        :param user_data: string containing the username that will be removed from the collection
        :return:          boolean value indicating if the delete request was successfully processed

        @author     mshallop@linux.com
        @version    1.0

        HISTORY:
        ========
        01-20-19        mks     original coding

        """
        query_filter = {"username": user_data}
        return self.mongo_toolbox.delete_records(query_filter)
