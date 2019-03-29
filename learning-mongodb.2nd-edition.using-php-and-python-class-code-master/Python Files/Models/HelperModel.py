import uuid
import bcrypt

"""
    HelpModel.py

    This is a python include file for various helper functions -- sometimes, I want functions to be re-usable for
    more than one application, or more than Model/Module within an application - in these situations, I create a 
    helper file, like this one, to store these multi-use type functions.

    @author     mshallop@linux.com
    @version    1.0

    HISTORY:
    ========
    01-06-19        mks     original coding

"""


def generate_guid():
    """
    generate_guid() -- helper function

    This function generates a 36-character GUID (with hashes... GUIDs without hashes are 32-characters) and converts
    the alpha chars to upper-case.

    @author     mshallop@linux.com
    @version    1.0

    :return: returns a 36-character GUID (with hashes) converted to upper case letters

    HISTORY:
    ========
    01-06-19        mks     original coding

    """
    return str(uuid.uuid4()).upper()


def hash_string(some_string):
    """
    hash_string() -- helper function

    This function requires a single input parameter to the method:

    a string value that should be the user's password -- this is the value that will be encrypted.

    The function returns the encrypted password.

    @author     mshallop@linux.com
    @version    1.0

    :param some_string:  the string to be bcrypt'd-hashed
    :return: returns the encrypted strung

    HISTORY:
    ========
    01-06-19        mks     original coding

    """
    return bcrypt.hashpw(some_string.encode(), bcrypt.gensalt())


def validate_password_length(password):
    """
    validate_password_length() -- general function

    This method requires one input parameter - a string containing the user's clear-text password.

    The method simply validates the password based on length - the password must be more than 7 characters and less
    than 16 characters to be accepted by the system.

    todo -- replace the numeric constraints with self-descriptive constants
    todo -- other validations (special chars, num, upper & lower-case, etc.)

    @author     mshallop@linux.com
    @version    1.0

    :param password: string value containing the user's clear-text password
    :return: boolean value indicating if the password meets minimum validation requirements and, if the password failed
    validation, then also return a diagnostic message.

    HISTORY:
    ========
    01-06-19        mks     original coding

    """
    return_value = True
    return_message = None

    if len(password) < 8 or len(password) > 16:
        return_message = 'Password incorrect length - must be between 8 and 16 chars!'
        return_value = False

    return return_value, return_message
