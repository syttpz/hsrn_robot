#pragma once

#include "commons/base_includes.hpp"
#include "commons/string_includes.hpp"
#include "utils/json.hpp"
#include "corelink_client_constants.hpp"
#include "corelink_network_constants.hpp"
#include "corelink_data_xchg_typedefs.hpp"

namespace corelink
{
    namespace client
    {
        namespace request_response
        {
            namespace requests
            {
                /**
                 * @brief Defines the base type for all corelink control API requests
                 * This is done so that JSON responses can be deserialized to native types and that function interfaces
                 * are standardized.
                 */
                struct CORELINK_EXPORT corelink_server_request_base
                {
                    /**
                     * @brief Default constructor
                     */
                    corelink_server_request_base() = default;

                    /**
                     * @brief Copy constructor
                     * @param rhs Value to copy from
                     */
                    corelink_server_request_base(clvref<corelink_server_request_base>) = default;

                    /**
                     * @brief Move constructor
                     * @param rhs Instance to move data from
                     */
                    corelink_server_request_base(rvref<corelink_server_request_base>) noexcept
                    {}

                    /**
                     * @brief Destructor
                     */
                    ~corelink_server_request_base() = default;
                };

                /**
                 * @brief Corelink server authenticate request parameters <br/>
                 * @note The client library <b>silently</b> inserts a token value, which you cannot currently override
                 */
                struct CORELINK_EXPORT authenticate_client_request : public corelink_server_request_base
                {
                    /**
                     * @brief corresponds to the request JSON key username
                     */
                    const std::string username{};
                    /**
                     * @brief corresponds to the request JSON key password
                     */
                    const std::string password{};

                    /**
                     * @brief default constructor
                     */
                    authenticate_client_request()
                    {}

                    /**
                     * @brief Parameterized cctor
                     * @param user username for the auth request
                     * @param pwd password for the auth request
                     */
                    authenticate_client_request(
                            clvref<std::string> user,
                            clvref<std::string> pwd)
                            : username(user), password(pwd)
                    {}

                    /**
                     * @brief Copy cctor
                     * @param rhs object to copy from
                     */
                    authenticate_client_request(clvref<authenticate_client_request> rhs) :
                            corelink_server_request_base(rhs), username(rhs.username), password(rhs.password)
                    {}

                    /**
                     * @brief Move cctor
                     * @param rhs Object to move from
                     */
                    authenticate_client_request(rvref<authenticate_client_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)), username(rhs.username),
                            password(rhs.password)
                    {}

                    ~authenticate_client_request()
                    {}
                };

                /**
                 * @brief Corelink describe function request parameters <br/>
                 * @note the client library silently inserts a token value, which you cannot currently override
                 */
                struct CORELINK_EXPORT describe_function_request : public corelink_server_request_base
                {
                    std::string function_name;

                    describe_function_request()
                    {}

                    explicit describe_function_request(clvref<std::string> name) : function_name(name)
                    {}

                    describe_function_request(clvref<describe_function_request> rhs) :
                            corelink_server_request_base(rhs),
                            function_name(rhs.function_name)
                    {}

                    describe_function_request(rvref<describe_function_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)),
                            function_name(std::move(rhs.function_name))
                    {}
                };

                /**
                 * @brief Corelink describe function request parameters <br/>
                 * Note - the client library silently inserts a token value, which you cannot currently override
                 */
                struct CORELINK_EXPORT modify_workspace_request : public corelink_server_request_base
                {
                    std::string workspace_name;

                    modify_workspace_request() : corelink_server_request_base()
                    {}

                    explicit modify_workspace_request(clvref<std::string> ws_name) :
                            corelink_server_request_base(), workspace_name(ws_name)
                    {}

                    modify_workspace_request(clvref<modify_workspace_request> rhs) :
                            corelink_server_request_base(rhs), workspace_name(rhs.workspace_name)
                    {}

                    modify_workspace_request(rvref<modify_workspace_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)), workspace_name(std::move(rhs.workspace_name))
                    {}
                };

                /**
                 * @brief Corelink modify request function request parameters <br/>
                 * Note - the client library silently inserts a token value, which you cannot currently override
                 */
                struct CORELINK_EXPORT modify_user_request : public corelink_server_request_base
                {
                    std::string username{};
                    std::string password{};
                    std::string first_name{};
                    std::string last_name{};
                    std::string email_id{};
                    bool admin{};

                    modify_user_request() : corelink_server_request_base()
                    {}

                    modify_user_request(clvref<modify_user_request> rhs) :
                            corelink_server_request_base(rhs),
                            username(rhs.username),
                            password(rhs.password),
                            first_name(rhs.first_name),
                            last_name(rhs.last_name),
                            email_id(rhs.email_id),
                            admin(rhs.admin)
                    {}

                    modify_user_request(rvref<modify_user_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)),
                            username(std::move(rhs.username)),
                            password(std::move(rhs.password)),
                            first_name(std::move(rhs.first_name)),
                            last_name(std::move(rhs.last_name)),
                            email_id(std::move(rhs.email_id)),
                            admin(rhs.admin)
                    {}
                };

                /**
                 * @brief Corelink modify group request function request parameters <br/>
                 * Note - the client library silently inserts a token value, which you cannot currently override.
                 */
                struct CORELINK_EXPORT modify_group_request : public corelink_server_request_base
                {
                    std::string group_name{};
                    std::string username{};

                    modify_group_request() : corelink_server_request_base()
                    {}

                    modify_group_request(clvref<modify_group_request> rhs) :
                            corelink_server_request_base(rhs),
                            group_name(rhs.group_name),
                            username(rhs.username)
                    {}

                    modify_group_request(rvref<modify_group_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)),
                            group_name(std::move(rhs.group_name)),
                            username(std::move(rhs.username))
                    {}
                };

                /**
                 * @brief Corelink list streams request function request parameters <br/>
                 * Note - the client library silently inserts a token value, which you cannot currently override.
                 */
                struct CORELINK_EXPORT list_streams_request : public corelink_server_request_base
                {
                    /**
                     * @brief Workspaces, if any, to filter by and fetch streams
                     */
                    std::vector<std::string> workspaces{};
                    /**
                     * @brief Corelink stream topics, if any, to filter by and fetch streams
                     */
                    std::vector<std::string> types{};

                    list_streams_request() : corelink_server_request_base()
                    {}

                    list_streams_request(clvref<list_streams_request> rhs) :
                            corelink_server_request_base(rhs),
                            workspaces(rhs.workspaces),
                            types(rhs.types)
                    {}

                    list_streams_request(rvref<list_streams_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)),
                            workspaces(std::move(rhs.workspaces)),
                            types(std::move(rhs.types))
                    {}
                };

                /**
                 * @brief Corelink stream information request function request parameters <br/>
                 * Note - the client library silently inserts a token value, which you cannot currently override.
                 */
                struct CORELINK_EXPORT stream_info_request : public corelink_server_request_base
                {
                    /**
                     * @brief stream if of the stream for which information needs to be fetched
                     */
                    constants::corelink_stream_id_type stream_id{};

                    stream_info_request() : corelink_server_request_base()
                    {}

                    stream_info_request(clvref<stream_info_request> rhs) :
                            corelink_server_request_base(rhs), stream_id(rhs.stream_id)
                    {}

                    stream_info_request(rvref<stream_info_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)), stream_id(rhs.stream_id)
                    {}
                };

                /**
                 * @brief Stream subscription modification request
                 */
                struct CORELINK_EXPORT modify_stream_subscription_request : public corelink_server_request_base
                {
                    constants::corelink_stream_id_type receiver_id{};
                    std::vector<constants::corelink_stream_id_type> stream_ids{};

                    modify_stream_subscription_request() : corelink_server_request_base()
                    {}

                    modify_stream_subscription_request(
                            constants::corelink_stream_id_type rx_id,
                            clvref<std::vector<constants::corelink_stream_id_type>> sub_to)
                            : receiver_id(rx_id), stream_ids(sub_to)
                    {}

                    modify_stream_subscription_request(clvref<modify_stream_subscription_request> rhs)
                            : corelink_server_request_base(rhs), receiver_id(rhs.receiver_id),
                              stream_ids(rhs.stream_ids)
                    {}

                    modify_stream_subscription_request(rvref<modify_stream_subscription_request> rhs) noexcept
                            : corelink_server_request_base(std::move(rhs)), receiver_id(rhs.receiver_id),
                              stream_ids(std::move(rhs.stream_ids))
                    {}
                };

                struct CORELINK_EXPORT modify_data_stream_request_base : public corelink_server_request_base
                {
                    const core::network::constants::protocols::protocol &protocol;
                    bool alert = false;
                    bool echo = false;
                    std::string workspace{};
                    std::string stream_type{};
                    std::string meta{};
                    core::network::on_error_type_1 on_error = nullptr;
                    core::network::on_init_type_1 on_init = nullptr;
                    core::network::on_uninit_type_1 on_uninit = nullptr;
                    bool use_secure_connect = false;
                    std::string client_certificate_path{};

                    explicit modify_data_stream_request_base(clvref<core::network::constants::protocols::protocol> p)
                            : corelink_server_request_base(), protocol(p)
                    {}

                    modify_data_stream_request_base(clvref<modify_data_stream_request_base> rhs)
                            :
                            corelink_server_request_base(rhs), protocol(rhs.protocol),
                            alert(rhs.alert),
                            echo(rhs.echo),
                            workspace(rhs.workspace),
                            stream_type(rhs.stream_type),
                            meta(rhs.meta),
                            on_error(rhs.on_error),
                            on_init(rhs.on_init),
                            on_uninit(rhs.on_uninit),
                            use_secure_connect(rhs.use_secure_connect),
                            client_certificate_path(rhs.client_certificate_path)
                    {}

                    modify_data_stream_request_base(rvref<modify_data_stream_request_base> rhs) noexcept
                            : corelink_server_request_base(std::move(rhs)),
                              protocol(rhs.protocol),
                              alert(rhs.alert),
                              echo(rhs.echo),
                              workspace(std::move(rhs.workspace)),
                              stream_type(std::move(rhs.stream_type)),
                              meta(std::move(rhs.meta)),
                              on_error(std::move(rhs.on_error)),
                              on_init(std::move(rhs.on_init)),
                              on_uninit(std::move(rhs.on_uninit)),
                              use_secure_connect(rhs.use_secure_connect),
                              client_certificate_path(std::move(rhs.client_certificate_path))
                    {}
                };

                struct CORELINK_EXPORT modify_sender_stream_request : public modify_data_stream_request_base
                {
                    constants::corelink_stream_id_type stream_id = -1;
                    constants::corelink_stream_id_type forwarded_from_stream = -1;
                    core::network::on_send_type_1 on_send = nullptr;

                    explicit modify_sender_stream_request(clvref<core::network::constants::protocols::protocol> p)
                            : modify_data_stream_request_base(p)
                    {}

                    modify_sender_stream_request(clvref<modify_sender_stream_request> rhs)
                            : modify_data_stream_request_base(rhs),
                              stream_id(rhs.stream_id),
                              forwarded_from_stream(rhs.forwarded_from_stream),
                              on_send(rhs.on_send)
                    {}

                    modify_sender_stream_request(rvref<modify_sender_stream_request> rhs) noexcept
                            :
                            modify_data_stream_request_base(std::move(rhs)), stream_id(rhs.stream_id),
                            forwarded_from_stream(rhs.forwarded_from_stream),
                            on_send(std::move(rhs.on_send))
                    {}
                };

                struct CORELINK_EXPORT modify_receiver_stream_request : public modify_data_stream_request_base
                {
                    constants::corelink_stream_id_type receiver_stream_id = -1;
                    std::vector<constants::corelink_stream_id_type> stream_ids{};
                    std::function<
                            void(core::network::channel_id_type,
                                 clvref<constants::corelink_stream_id_type>,
                                 clvref<utils::json>,
                                 std::vector<uint8_t>)>
                            on_receive = nullptr;

                    explicit modify_receiver_stream_request(clvref<core::network::constants::protocols::protocol> p)
                            : modify_data_stream_request_base(p)
                    {}

                    modify_receiver_stream_request(clvref<modify_receiver_stream_request> rhs)
                            : modify_data_stream_request_base(rhs), receiver_stream_id(rhs.receiver_stream_id),
                              stream_ids(rhs.stream_ids),
                              on_receive(rhs.on_receive)
                    {}

                    modify_receiver_stream_request(rvref<modify_receiver_stream_request> rhs) noexcept
                            : modify_data_stream_request_base(std::move(rhs)),
                              receiver_stream_id(rhs.receiver_stream_id),
                              stream_ids(std::move(rhs.stream_ids)),
                              on_receive(std::move(rhs.on_receive))
                    {}
                };

                struct CORELINK_EXPORT disconnect_streams_request : public corelink_server_request_base
                {
                    std::vector<std::string> workspace{};
                    std::vector<std::string> types{};
                    std::vector<constants::corelink_stream_id_type> streamIDs{};

                    disconnect_streams_request()
                    {}

                    disconnect_streams_request(lvref<disconnect_streams_request> rhs) :
                            corelink_server_request_base(rhs),
                            workspace(rhs.workspace),
                            types(rhs.types),
                            streamIDs(rhs.streamIDs)
                    {}

                    disconnect_streams_request(rvref<disconnect_streams_request> rhs) noexcept:
                            corelink_server_request_base(std::move(rhs)),
                            workspace(std::move(rhs.workspace)),
                            types(std::move(rhs.types)),
                            streamIDs(std::move(rhs.streamIDs))
                    {}
                };
            }

            namespace responses
            {
                struct CORELINK_EXPORT corelink_server_response_base
                {
                    int32_t status_code = 0;
                    std::string message{};

                    corelink_server_response_base()
                    {}

                    corelink_server_response_base(clvref<corelink_server_response_base> rhs)
                            : status_code(rhs.status_code),
                              message(rhs.message)
                    {}

                    corelink_server_response_base(rvref<corelink_server_response_base> rhs) noexcept
                            : status_code(rhs.status_code),
                              message(std::move(rhs.message))
                    {}

                    ~corelink_server_response_base() = default;
                };

                struct CORELINK_EXPORT corelink_server_response_json : public corelink_server_response_base
                {
                    corelink::utils::json response;

                    corelink_server_response_json() : corelink_server_response_base()
                    {}

                    corelink_server_response_json(clvref<corelink_server_response_json> rhs)
                            : corelink_server_response_base(rhs)
                    {}

                    corelink_server_response_json(rvref<corelink_server_response_json> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)), response(std::move(rhs.response))
                    {}
                };

                struct CORELINK_EXPORT list_client_functions_response : public corelink_server_response_base
                {
                    std::vector<std::string> functions;

                    list_client_functions_response() : corelink_server_response_base()
                    {}

                    list_client_functions_response(clvref<list_client_functions_response> rhs)
                            : corelink_server_response_base(rhs), functions(rhs.functions)
                    {}

                    list_client_functions_response(rvref<list_client_functions_response> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)), functions(std::move(rhs.functions))
                    {}
                };

                struct CORELINK_EXPORT list_server_functions_response : public corelink_server_response_base
                {
                    std::vector<std::string> functions;

                    list_server_functions_response() : corelink_server_response_base()
                    {}

                    list_server_functions_response(clvref<list_server_functions_response> rhs)
                            : corelink_server_response_base(rhs),
                              functions(rhs.functions)
                    {}

                    list_server_functions_response(rvref<list_server_functions_response> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)), functions(std::move(rhs.functions))
                    {}
                };

                struct CORELINK_EXPORT list_workspaces_response : public corelink_server_response_base
                {
                    std::vector<std::string> workspaces;

                    list_workspaces_response() : corelink_server_response_base()
                    {}

                    list_workspaces_response(lvref<list_workspaces_response> rhs)
                            : corelink_server_response_base(rhs),
                              workspaces(rhs.workspaces)
                    {}

                    list_workspaces_response(rvref<list_workspaces_response> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)), workspaces(std::move(rhs.workspaces))
                    {}
                };

                struct CORELINK_EXPORT get_default_workspace_response : public corelink_server_response_base
                {
                    std::string workspace_name;

                    get_default_workspace_response() : corelink_server_response_base()
                    {}

                    get_default_workspace_response(clvref<get_default_workspace_response> rhs)
                            : corelink_server_response_base(rhs), workspace_name(rhs.workspace_name)
                    {}

                    get_default_workspace_response(rvref<get_default_workspace_response> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)),
                              workspace_name(std::move(rhs.workspace_name))
                    {}
                };

                struct CORELINK_EXPORT list_groups_response : public corelink_server_response_base
                {
                    std::vector<std::string> groups{};

                    list_groups_response() : corelink_server_response_base()
                    {}

                    list_groups_response(clvref<list_groups_response> rhs) :
                            corelink_server_response_base(rhs), groups(rhs.groups)
                    {}

                    list_groups_response(rvref<list_groups_response> rhs) noexcept:
                            corelink_server_response_base(std::move(rhs)), groups(std::move(rhs.groups))
                    {}
                };

                struct CORELINK_EXPORT stream_info
                {
                    constants::corelink_stream_id_type stream_id;
                    std::string type;
                    std::string meta;
                    std::string user;
                    std::vector<std::string> apps;
                };

                struct CORELINK_EXPORT modify_stream_subscription_response : public corelink_server_response_base
                {
                    std::vector<stream_info> streams{};

                    modify_stream_subscription_response() : corelink_server_response_base()
                    {}

                    modify_stream_subscription_response(clvref<modify_stream_subscription_response> rhs)
                            : corelink_server_response_base(rhs), streams(rhs.streams)
                    {}

                    modify_stream_subscription_response(rvref<modify_stream_subscription_response> rhs) noexcept:
                            corelink_server_response_base(std::move(rhs)), streams(std::move(rhs.streams))
                    {}
                };

                struct CORELINK_EXPORT disconnect_streams_response : public corelink_server_response_base
                {
                    std::vector<constants::corelink_stream_id_type> streamIDs{};

                    disconnect_streams_response() = default;

                    disconnect_streams_response(lvref<disconnect_streams_response> rhs) :
                            corelink_server_response_base(rhs),
                            streamIDs(rhs.streamIDs)
                    {}

                    disconnect_streams_response(rvref<disconnect_streams_response> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)),
                              streamIDs(std::move(rhs.streamIDs))
                    {}
                };

                struct CORELINK_EXPORT server_cb_base : public corelink_server_response_base
                {
                    std::string function{};

                    server_cb_base() : corelink_server_response_base()
                    {}

                    server_cb_base(clvref<server_cb_base> rhs)
                            : corelink_server_response_base(rhs), function(rhs.function)
                    {}

                    server_cb_base(rvref<server_cb_base> rhs) noexcept
                            : corelink_server_response_base(std::move(rhs)), function(std::move(rhs.function))
                    {}
                };

                struct CORELINK_EXPORT server_cb_on_update_response : public server_cb_base
                {
                    constants::corelink_stream_id_type receiver_id{};
                    constants::corelink_stream_id_type stream_id{};
                    std::string user{};
                    std::vector<std::string> apps{};
                    std::string type{};
                    std::string meta{};

                    server_cb_on_update_response() : server_cb_base()
                    {}

                    server_cb_on_update_response(clvref<server_cb_on_update_response> rhs)
                            : server_cb_base(rhs), receiver_id(rhs.receiver_id),
                              stream_id(rhs.stream_id), user(rhs.user),
                              apps(rhs.apps), type(rhs.type), meta(rhs.meta)
                    {}

                    server_cb_on_update_response(rvref<server_cb_on_update_response> rhs) noexcept
                            : server_cb_base(std::move(rhs)), receiver_id(rhs.receiver_id),
                              stream_id(rhs.stream_id), user(std::move(rhs.user)),
                              apps(std::move(rhs.apps)), type(std::move(rhs.type)),
                              meta(std::move(rhs.meta))
                    {}
                };

                struct CORELINK_EXPORT server_cb_on_subscribed_response : public server_cb_base
                {
                    constants::corelink_stream_id_type receiver_id{};
                    constants::corelink_stream_id_type stream_id{};
                    std::string user{};
                    std::vector<std::string> apps{};
                    std::string meta{};

                    server_cb_on_subscribed_response() : server_cb_base()
                    {}

                    server_cb_on_subscribed_response(clvref<server_cb_on_subscribed_response> rhs)
                            : server_cb_base(rhs),
                              receiver_id(rhs.receiver_id), stream_id(rhs.stream_id),
                              user(rhs.user), apps(rhs.apps),
                              meta(rhs.meta)
                    {}

                    server_cb_on_subscribed_response(rvref<server_cb_on_subscribed_response> rhs) noexcept
                            : server_cb_base(std::move(rhs)), receiver_id(rhs.receiver_id),
                              stream_id(rhs.stream_id),
                              user(std::move(rhs.user)),
                              apps(std::move(rhs.apps)),
                              meta(std::move(rhs.meta))
                    {}
                };

                struct CORELINK_EXPORT server_cb_on_stale_response : public server_cb_base
                {
                    constants::corelink_stream_id_type stream_id{};

                    server_cb_on_stale_response() : server_cb_base()
                    {}

                    server_cb_on_stale_response(clvref<server_cb_on_stale_response> rhs)
                            : server_cb_base(rhs), stream_id(rhs.stream_id)
                    {}

                    server_cb_on_stale_response(rvref<server_cb_on_stale_response> rhs) noexcept
                            : server_cb_base(std::move(rhs)), stream_id(rhs.stream_id)
                    {}
                };

                struct CORELINK_EXPORT server_cb_on_dropped_response : public server_cb_base
                {
                    constants::corelink_stream_id_type stream_id{};

                    server_cb_on_dropped_response() : server_cb_base()
                    {}

                    server_cb_on_dropped_response(clvref<server_cb_on_dropped_response> rhs)
                            : server_cb_base(rhs), stream_id(rhs.stream_id)
                    {}

                    server_cb_on_dropped_response(rvref<server_cb_on_dropped_response> rhs) noexcept
                            : server_cb_base(std::move(rhs)), stream_id(rhs.stream_id)
                    {}
                };
            }
        }
    }
}
