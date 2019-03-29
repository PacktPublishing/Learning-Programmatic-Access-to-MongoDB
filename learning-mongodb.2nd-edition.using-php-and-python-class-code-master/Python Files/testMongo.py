from Models import MongoConnectorModel
from Models import MongoConnectorDataModel
from Models import UserModel
from shared import constants

mongo_object = MongoConnectorModel.MongoConnectorModel()

if mongo_object.status is False:
    print('Failed to connect to DB using configuration!')
    exit(1)
else:
    print('Successfully connected to mongoDB!')

program_data = MongoConnectorDataModel.MongoConnectorDataModel()
user_model = UserModel.UserModel(mongo_object.res_mongo)

# the selected operation for this iteration
current_operation = constants.OP_DELETE

#  since python doesn't have switch-case, we'll use stodgy if-elif

if current_operation == constants.OP_CREATE:
    user_data = program_data.newUser   # extract new user data from the static data model
    user_data = user_data[0]

    result = user_model.validate_new_user_data(user_data)

    if result is False:
        for i in range(0, len(user_model.error_stack)):
            print(user_model.error_stack[i])
    else:
        # insert the new user
        result = user_model.insert_new_user(user_data)
        if result is True:
            print("Successfully created user account for: " + user_data['username'])
elif current_operation == constants.OP_UPDATE:
    user_data = program_data.update_data
    user_data = user_data[0]
    user_data['target_user'] = program_data.newUser[0]['username']
    result = user_model.update_user(user_data)

    if result is False:
        print('update user request has failed')
    else:
        print('user record was successfully updated!')
elif current_operation == constants.OP_DELETE:
    user_data = program_data.newUser
    user_data = user_data[0]
    user_data = user_data['username']
    result = user_model.delete_user(user_data)
    if result is False:
        print('delete user record request has failed')
    else:
        print('user: ' + user_data + ' successfully deleted')
