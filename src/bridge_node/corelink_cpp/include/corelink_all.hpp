#pragma once

/// **************************** INCLUDES START ****************************

#include "commons/platform_macros.hpp"
#include "commons/base_includes.hpp"
#include "commons/associative_containers_includes.hpp"
#include "commons/container_adaptors_includes.hpp"
#include "commons/sequence_container_includes.hpp"
#include "commons/string_includes.hpp"
#include "commons/concurrency_includes.hpp"
#include "commons/reader_writer_lock_shim.hpp"
#include "commons/asio_includes.hpp"
#include "commons/json_includes.hpp"
#include "commons/stream_includes.hpp"


#include "utils/system.hpp"
#include "utils/concurrent_counter.hpp"
#include "utils/concurrent_queue.hpp"
#include "utils/json.hpp"
#include "utils/random_gen.hpp"

#include "core/corelink_network_constants.hpp"
#include "core/corelink_data_xchg_typedefs.hpp"
#include "core/corelink_data_xchg_protocol.hpp"
#include "core/corelink_data_xchg_ip_proto_base.hpp"
#include "core/corelink_data_xchg_raw_socket_protocol_context_manager.hpp"
#include "core/corelink_data_xchg_websocket_proto_manager.hpp"
#include "core/corelink_data_xchg_tcp_proto_manager.hpp"
#include "core/corelink_data_xchg_udp_proto_manager.hpp"

#include "core/corelink_client_constants.hpp"
#include "core/corelink_client_channel_descriptors.hpp"
#include "core/corelink_client_request_response_handlers.hpp"
#include "core/corelink_client_request_response_payload_prototypes.hpp"
#include "core/corelink_client.hpp"

/// **************************** INCLUDES   END ****************************