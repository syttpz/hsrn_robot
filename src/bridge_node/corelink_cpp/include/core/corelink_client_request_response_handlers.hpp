#pragma once

#include "commons/associative_containers_includes.hpp"
#include "commons/base_includes.hpp"
#include "corelink_client_request_response_payload_prototypes.hpp"
#include "corelink_client_channel_descriptors.hpp"

namespace corelink
{
    namespace client
    {
        struct CORELINK_EXPORT request_response_handler
        {
            /**
             * @brief Function parameters prototype for response handler callback functions
             * @details This serves as a function prototype for response callback functions which encapsulate all the data
             * from the server <b>returned to the user</b>. Functions of this kind are also used for server initiated callback functions.
             */
            using user_completion_handler_type = std::function<
                    void(core::network::channel_id_type,
                         in<std::string>,
                         std::shared_ptr<request_response::responses::corelink_server_response_base>)>;

            /**
             * @typedef request_handler_with_args_type
             * @brief Function parameters prototype for handling and formatting server request functions
             */
            using request_handler_with_args_type = std::function<
                    utils::json(
                            in<std::shared_ptr<corelink_client_control_channel_descriptor>>,
                            in<std::shared_ptr<request_response::requests::corelink_server_request_base>>)>;

            /**
             * @typedef response_handler_type
             * @brief Function parameters prototype for response handler callback functions
             * @details This serves as a function prototype for response callback functions which encapsulate raw data
             * from the server. Functions of this kind are also used for server initiated callback functions.
             */
            using response_handler_type = std::function<void(core::network::channel_id_type,
                                                             in<utils::json>,
                                                             in<user_completion_handler_type>)>;

            /**
             * @var request_handler
             * @brief request handler for managing and formatting outgoing corelink server requests
             */
            request_handler_with_args_type request_handler;
            /**
             * @var response_handler
             * @brief response handler for managing and formatting incoming response objects
             */
            response_handler_type response_handler;

            /**
             * @brief default cctor
             */
            request_response_handler() = default;

            /**
             * @brief Parametrized constructor
             * @param req_handler Function object for handling and formatting outgoing requests
             * @param resp_handler Function object for handling and formatting incoming responses
             */
            request_response_handler(in<request_handler_with_args_type> req_handler,
                                     in<response_handler_type> resp_handler) :
                    request_handler(req_handler), response_handler(resp_handler)
            {}

            /**
             * @brief Copy constructor
             * @param rhs Object to copy properties from
             */
            request_response_handler(clvref<request_response_handler> rhs) :
                    request_handler(rhs.request_handler),
                    response_handler(rhs.response_handler)
            {}

            /**
             * @brief Copy assignment operator
             * @param rhs Object to copy properties from
             * @return a reference of the object to which the properties have been copied to
             */
            request_response_handler &operator=(clvref<request_response_handler> rhs)
            {
                request_handler = rhs.request_handler;
                response_handler = rhs.response_handler;
                return *this;
            }

            /**
             * @brief Move cctor
             * @param rhs Object to move properties from
             */
            request_response_handler(rvref<request_response_handler> rhs) noexcept:
                    request_handler(std::move(rhs.request_handler)),
                    response_handler(std::move(rhs.response_handler))
            {}

            /**
             * @brief Move assignment operator
             * @param rhs Object to move properties from
             * @return a reference of the object to which the properties have been moved to
             */
            request_response_handler &operator=(rvref<request_response_handler> rhs)
            {
                request_handler = std::move(rhs.request_handler);
                response_handler = std::move(rhs.response_handler);
                return *this;
            }
        };

        /**
         * @brief Contains "keys" to corelink client and server side functions.
         * These are used to look up appropriate request and response handlers for a function type
         */
        CORELINK_EXPORT enum class corelink_functions
        {
            /**
             * Corelink client -> server request for initiating an authentication request
             */
            authenticate,
            keep_alive,
            list_client_functions,
            list_server_functions,
            describe_client_function,
            describe_server_function,
            list_workspaces,
            add_workspace,
            remove_workspace,
            set_default_workspace,
            get_default_workspace,
            add_user,
            change_password,
            remove_user,
            list_users,
            add_group,
            add_group_user,
            remove_group_user,
            change_group_owner,
            remove_group,
            list_groups,
            list_streams,
            stream_info,
            subscribe,
            unsubscribe,
            disconnect,
            expire_user_session,
            create_sender,
            create_receiver,
            server_callback_on_update,
            server_callback_on_subscribed,
            server_callback_on_stale,
            server_callback_on_dropped
        };
#if (defined(CORELINK_CPP14_SUPPORTED) || defined(CORELINK_CPP17_SUPPORTED))
        using hasher_type = std::hash<corelink_functions>;
#elif defined(CORELINK_CPP11_SUPPORTED)

        struct CORELINK_EXPORT enum_hasher
        {
            template<typename t>
            std::size_t operator()(t _) const
            { return static_cast<std::size_t>(_); }
        };

        using hasher_type = enum_hasher;
#endif

        const static std::unordered_map<corelink_functions, request_response_handler, hasher_type>
                corelink_functions_request_response_handlers =
                {
                        {
                                corelink_functions::server_callback_on_update,     request_response_handler(
                                nullptr,
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler
                                )
                                {
                                    auto response = std::make_shared<request_response::responses::server_cb_on_update_response>();

                                    response->status_code = response_data().HasMember("statusCode") ?
                                                            response_data.get_int("statusCode", -1) :
                                                            0;
                                    if (response->status_code == 0)
                                    {
                                        response->function = response_data.get_str("function");
                                        response->type = response_data.get_str("type");
                                        response->stream_id = response_data.get_int("streamID");
                                        if (response_data().HasMember("apps"))
                                        {
                                            auto app_list = response_data()["apps"].GetArray();
                                            for (auto &app: app_list)
                                                response->apps.push_back(app.GetString());
                                        }
                                        response->receiver_id = response_data.get_int("receiverID");
                                        response->user = response_data.get_str("user");
                                        response->meta = response_data.get_str("meta");
                                    }
                                    response->message = response_data.get_str("message");

                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::server_callback_on_subscribed, request_response_handler(
                                nullptr,
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::server_cb_on_subscribed_response>();
                                    response->status_code = response_data().HasMember("statusCode") ?
                                                            response_data.get_int("statusCode", -1) :
                                                            0;

                                    if (response->status_code == 0)
                                    {
                                        response->function = response_data.get_str("function");
                                        response->stream_id = response_data.get_int("senderID");
                                        if (response_data().HasMember("app"))
                                        {
                                            auto app_list = response_data()["app"].GetArray();
                                            for (auto &app: app_list)
                                                response->apps.push_back(app.GetString());
                                        }
                                        // this is a hack. I hate it.
                                        // the problem is that the server sends back the receiver ID as a string and not an int32_t.
                                        // this field would be useless if we just leave it as is.
                                        // so we make some adjustments that in case this is fixed in the future
                                        // we don't break this code again.
                                        response->receiver_id = response_data.get_int("receiverID", -1);
                                        if (response->receiver_id == -1)
                                        {
                                            const auto receiver_id = response_data.get_str("receiverID");
                                            if (!receiver_id.empty())
                                                response->receiver_id = std::stoi(receiver_id);
                                        }
                                        response->user = response_data.get_str("user");
                                        response->meta = response_data.get_str("meta");
                                    }
                                    response->message = response_data.get_str("message");

                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::server_callback_on_stale,      request_response_handler(
                                nullptr,
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::server_cb_on_stale_response>();
                                    response->status_code = response_data().HasMember("statusCode") ?
                                                            response_data.get_int("statusCode", -1) :
                                                            0;

                                    if (response->status_code == 0)
                                    {
                                        response->function = response_data.get_str("function");
                                        response->stream_id = response_data.get_int("streamID");
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::server_callback_on_dropped,    request_response_handler(
                                nullptr,
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {

                                    auto response = std::make_shared<request_response::responses::server_cb_on_dropped_response>();
                                    response->status_code = response_data().HasMember("statusCode") ?
                                                            response_data.get_int("statusCode", -1) :
                                                            0;

                                    if (response->status_code == 0)
                                    {
                                        response->function = response_data.get_str("function");
                                        response->stream_id = response_data.get_int("streamID");
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::authenticate,                  request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> /*control_channel_descriptor*/,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto auth_client_req = std::static_pointer_cast<request_response::requests::authenticate_client_request>(
                                            parameters);
                                    utils::json request;
                                    request.append(
                                            std::map<std::string, std::string>(
                                                    {
                                                            {"function", "auth"},
                                                            {"username", auth_client_req->username},
                                                            {"password", auth_client_req->password}
                                                    }));
                                    return request;
                                },
                                nullptr
                        )
                        },
                        {
                                corelink_functions::list_client_functions,         request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json request;
                                    request.append(
                                            std::map<std::string, std::string>(
                                                    {
                                                            {"function", "listFunctions"},
                                                            {"token",    control_channel_descriptor->auth_token}
                                                    }));
                                    return request;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::list_client_functions_response>();
                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        if (response_data().HasMember("functionList"))
                                        {
                                            auto json_function_list = response_data()["functionList"].GetArray();
                                            for (auto &fn: json_function_list)
                                                response->functions.push_back(fn.GetString());
                                        }
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::list_server_functions,         request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json request;
                                    request.append(
                                            std::map<std::string, std::string>(
                                                    {
                                                            {"function", "listServerFunctions"},
                                                            {"token",    control_channel_descriptor->auth_token}
                                                    }));
                                    return request;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::list_server_functions_response>();
                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        if (response_data().HasMember("functionList"))
                                        {
                                            auto json_function_list = response_data()["functionList"].GetArray();
                                            for (auto &fn: json_function_list)
                                                response->functions.push_back(fn.GetString());
                                        }
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::keep_alive,                    request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json request;
                                    request.append(
                                            std::map<std::string, std::string>(
                                                    {
                                                            {"function", "keepAlive"},
                                                            {"token",    control_channel_descriptor->auth_token}
                                                    }));
                                    return request;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::describe_client_function,      request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    utils::json request;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "describeFunction"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {
                                                     "functionName",
                                                                 std::static_pointer_cast<request_response::requests::describe_function_request>(
                                                                         parameters)->function_name
                                                    }
                                            });
                                    request.append(request_params);
                                    return request;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_json>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    response->response = response_data;
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::describe_server_function,      request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "describeServerFunction"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {
                                                     "functionName",
                                                                 std::static_pointer_cast<request_response::requests::describe_function_request>(
                                                                         parameters)->function_name
                                                    }
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_json>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    response->response = response_data;
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::list_workspaces,               request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "listWorkspaces"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::list_workspaces_response>();

                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        if (response_data().HasMember("workspaceList"))
                                        {
                                            auto json_workspaces_list = response_data()["workspaceList"].GetArray();
                                            for (auto &ws: json_workspaces_list)
                                                response->workspaces.push_back(ws.GetString());
                                        }
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::add_workspace,                 request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function",  "addWorkspace"},
                                                    {"token",     control_channel_descriptor->auth_token},
                                                    {"workspace", std::static_pointer_cast<request_response::requests::modify_workspace_request>(
                                                            parameters)->workspace_name}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::remove_workspace,              request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function",  "rmWorkspace"},
                                                    {"token",     control_channel_descriptor->auth_token},
                                                    {"workspace", std::static_pointer_cast<request_response::requests::modify_workspace_request>(
                                                            parameters)->workspace_name}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::set_default_workspace,         request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function",  "setDefaultWorkspace"},
                                                    {"token",     control_channel_descriptor->auth_token},
                                                    {"workspace", std::static_pointer_cast<request_response::requests::modify_workspace_request>(
                                                            parameters)->workspace_name}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::get_default_workspace,         request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "getDefaultWorkspace"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::get_default_workspace_response>();
                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        response->workspace_name = response_data.get_str("workspace");
                                    }
                                    response->message = response_data.get_str("message");

                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::add_user,                      request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto user_request = std::static_pointer_cast<request_response::requests::modify_user_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "addUser"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"username", user_request->username},
                                                    {"password", user_request->password},
                                                    {"first",    user_request->first_name},
                                                    {"last",     user_request->last_name},
                                                    {"email",    user_request->email_id}
                                            });
                                    json.append(request_params)
                                            .append("admin", user_request->admin);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::change_password,               request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto user_request = std::static_pointer_cast<request_response::requests::modify_user_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "password"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"password", user_request->password},
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::remove_user,                   request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto user_request = std::static_pointer_cast<request_response::requests::modify_user_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "rmUser"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"username", user_request->username},
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::list_users,                    request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "listUsers"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::add_group,                     request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto modify_group_request = std::static_pointer_cast<request_response::requests::modify_group_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "addGroup"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"group",    modify_group_request->group_name}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::add_group_user,                request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto modify_group_request = std::static_pointer_cast<request_response::requests::modify_group_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "addUserGroup"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"group",    modify_group_request->group_name},
                                                    {"user",     modify_group_request->username}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::remove_group_user,             request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto modify_group_request = std::static_pointer_cast<request_response::requests::modify_group_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "rmUserGroup"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"group",    modify_group_request->group_name},
                                                    {"user",     modify_group_request->username}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::change_group_owner,            request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto modify_group_request = std::static_pointer_cast<request_response::requests::modify_group_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "changeOwner"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"group",    modify_group_request->group_name},
                                                    {"user",     modify_group_request->username}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::remove_group,                  request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto modify_group_request = std::static_pointer_cast<request_response::requests::modify_group_request>(
                                            parameters);

                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "rmGroup"},
                                                    {"token",    control_channel_descriptor->auth_token},
                                                    {"group",    modify_group_request->group_name}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::list_groups,                   request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "listGroups"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append(request_params);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::list_groups_response>();
                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        if (response_data().HasMember("listGroup"))
                                        {
                                            auto groups_json = response_data()["listGroup"].GetArray();
                                            for (auto &group: groups_json)
                                                response->groups.push_back(group.GetString());
                                        }
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::list_streams,                  request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto list_streams_request = std::static_pointer_cast<request_response::requests::list_streams_request>(
                                            parameters);
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "listStreams"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append(request_params);
                                    json.append("workspaces", list_streams_request->workspaces);
                                    json.append("types", list_streams_request->types);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_json>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    response->response = response_data;
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::stream_info,                   request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto stream_info_request = std::static_pointer_cast<request_response::requests::stream_info_request>(
                                            parameters);
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "streamInfo"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append("streamID", stream_info_request->stream_id);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_json>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    response->response = response_data;
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::subscribe,                     request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto stream_info_request = std::static_pointer_cast<request_response::requests::modify_stream_subscription_request>(
                                            parameters);
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "subscribe"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append(request_params);
                                    json.append("receiverID", stream_info_request->receiver_id);
                                    json.append("streamIDs", stream_info_request->stream_ids);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::modify_stream_subscription_response>();

                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        if (response_data().HasMember("streamList"))
                                        {
                                            auto modified_streams = response_data()["streamList"].GetArray();
                                            for (auto &stream: modified_streams)
                                            {
                                                request_response::responses::stream_info s;
                                                s.stream_id = stream.HasMember("streamID") ? stream["streamID"].GetInt()
                                                                                           : 0;
                                                s.type = stream.HasMember("type") ? stream["type"].GetString() : "";
                                                s.meta = stream.HasMember("meta") ? stream["meta"].GetString() : "";
                                                s.user = stream.HasMember("user") ? stream["user"].GetString() : "";
                                                if (stream.HasMember("apps"))
                                                    for (auto &app: stream["apps"].GetArray())
                                                        s.apps.push_back(app.GetString());

                                                response->streams.push_back(s);
                                            }
                                        }
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::unsubscribe,                   request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto stream_info_request = std::static_pointer_cast<request_response::requests::modify_stream_subscription_request>(
                                            parameters);
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "unsubscribe"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    json.append("receiverID", stream_info_request->receiver_id);
                                    json.append("streamID", stream_info_request->stream_ids);
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_json>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    response->response = response_data;
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::disconnect,                    request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto disconnect_request_params = std::static_pointer_cast<request_response::requests::disconnect_streams_request>(
                                            parameters);
                                    utils::json request_params;
                                    request_params.append("function", std::string("disconnect"))
                                            .append("token", control_channel_descriptor->auth_token)
                                            .append("streamIDs",
                                                    disconnect_request_params->streamIDs)
                                            .append("types", disconnect_request_params->types)
                                            .append("workspaces", disconnect_request_params->types);
                                    return request_params;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::disconnect_streams_response>();
                                    if ((response->status_code = response_data.get_int("statusCode", -1)) == 0)
                                    {
                                        if (response_data().HasMember("streamList"))
                                        {
                                            for (auto &item: response_data()["streamList"].GetArray())
                                                response->streamIDs.push_back(item.GetInt());
                                        }
                                    }
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::expire_user_session,           request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> /*parameters*/)
                                {
                                    utils::json json;
                                    auto request_params = std::map<std::string, std::string>(
                                            {
                                                    {"function", "expire"},
                                                    {"token",    control_channel_descriptor->auth_token}
                                            });
                                    return json;
                                },
                                [](core::network::channel_id_type channel_id,
                                   in<utils::json> response_data,
                                   in<request_response_handler::user_completion_handler_type> completion_handler)
                                {
                                    auto response = std::make_shared<request_response::responses::corelink_server_response_base>();
                                    response->status_code = response_data.get_int("statusCode");
                                    response->message = response_data.get_str("message");
                                    completion_handler(channel_id, "", response);
                                }
                        )
                        },
                        {
                                corelink_functions::create_sender,                 request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto create_sender_request = std::static_pointer_cast<request_response::requests::modify_sender_stream_request>(
                                            parameters);
                                    utils::json request;
                                    std::map<std::string, std::string> request_string_params = {
                                            {"function", "sender"},
                                            {"token",    control_channel_descriptor->auth_token},
                                            {"proto",    create_sender_request->protocol.proto_name()}
                                    };
                                    request.append(request_string_params);
                                    request.append("alert", create_sender_request->alert);
                                    request.append("port", 0);
                                    request.append("IP", control_channel_descriptor->client_ip);
                                    if (!create_sender_request->workspace.empty())
                                        request.append("workspace", create_sender_request->workspace);
                                    if (create_sender_request->stream_id > 0)
                                        request.append("streamID", create_sender_request->stream_id);
                                    if (create_sender_request->forwarded_from_stream > 0)
                                        request.append("from", create_sender_request->forwarded_from_stream);
                                    if (!create_sender_request->stream_type.empty())
                                        request.append("type", create_sender_request->stream_type);
                                    if (!create_sender_request->meta.empty())
                                        request.append("meta", create_sender_request->meta);
                                    return request;
                                },
                                nullptr
                        )
                        },
                        {
                                corelink_functions::create_receiver,               request_response_handler(
                                [](in<std::shared_ptr<corelink_client_control_channel_descriptor>> control_channel_descriptor,
                                   in<std::shared_ptr<request_response::requests::corelink_server_request_base>> parameters)
                                {
                                    auto create_sender_request = std::static_pointer_cast<request_response::requests::modify_receiver_stream_request>(
                                            parameters);
                                    utils::json request;
                                    std::map<std::string, std::string> request_string_params = {
                                            {"function", "receiver"},
                                            {"token",    control_channel_descriptor->auth_token},
                                            {"proto",    create_sender_request->protocol.proto_name()}
                                    };
                                    request.append(request_string_params);
                                    request.append("alert", create_sender_request->alert);
                                    request.append("echo", create_sender_request->echo);
                                    request.append("port", 0);
                                    request.append("IP", control_channel_descriptor->client_ip);
                                    if (!create_sender_request->workspace.empty())
                                        request.append("workspace", create_sender_request->workspace);
                                    if (create_sender_request->receiver_stream_id > 0)
                                        request.append("receiverID", create_sender_request->receiver_stream_id);
                                    if (!create_sender_request->stream_ids.empty())
                                        request.append("streamIDs", create_sender_request->stream_ids);
                                    if (!create_sender_request->stream_type.empty())
                                        request.append("type", create_sender_request->stream_type);
                                    if (!create_sender_request->meta.empty())
                                        request.append("meta", create_sender_request->meta);
                                    return request;
                                },
                                nullptr
                        )
                        }
                };
    }
}