#include "core/corelink_data_xchg_ip_proto_base.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            corelink_data_xchg_ip_proto_base::corelink_data_xchg_ip_proto_base()
                    : corelink_data_xchg_protocol()
            {
                m_default_channel = add_channel(std::make_shared<ip_protocol_channel_descriptor>(
                        constants::protocols::unknown
                ));
            }

            corelink_data_xchg_ip_proto_base::corelink_data_xchg_ip_proto_base(
                    clvref<corelink::core::network::corelink_data_xchg_ip_proto_base> rhs)
                    : corelink_data_xchg_protocol(rhs)
            {
                // do not copy channel descriptors. Ideally, when a connection manger is copied, only its active
                m_default_channel = add_channel(std::make_shared<ip_protocol_channel_descriptor>(
                        constants::protocols::unknown
                ));
            }

            corelink_data_xchg_ip_proto_base::corelink_data_xchg_ip_proto_base(
                    rvref<corelink::core::network::corelink_data_xchg_ip_proto_base> rhs) noexcept
                    : corelink_data_xchg_protocol(std::move(rhs)),
                      m_channel_descriptors(std::move(rhs.m_channel_descriptors)),
                      m_default_channel(rhs.m_default_channel)
            {}

            corelink_data_xchg_ip_proto_base &corelink_data_xchg_ip_proto_base::operator=(
                    clvref<corelink::core::network::corelink_data_xchg_ip_proto_base>)
            {
                m_default_channel = add_channel(std::make_shared<ip_protocol_channel_descriptor>(
                        constants::protocols::unknown
                ));
                return *this;
            }

            corelink_data_xchg_ip_proto_base &corelink_data_xchg_ip_proto_base::operator=(
                    corelink::core::network::corelink_data_xchg_ip_proto_base &&rhs) noexcept
            {
                m_channel_descriptors = std::move(rhs.m_channel_descriptors);
                m_default_channel = rhs.m_default_channel;
                return *this;
            }

            channel_id_type corelink_data_xchg_ip_proto_base::add_channel(
                    std::shared_ptr<ip_protocol_channel_descriptor> &&channel_descriptor)
            {
                channel_id_type channel_id = 0;
                // generate a random_numbers channel ID. This has got nothing to do with the server. It is just a random_numbers tracker for the client
                do
                {
                    channel_id = utils::random_numbers::get_random_int();
                    // check uniqueness
                } while (!m_channel_descriptors.empty() &&
                         (m_channel_descriptors.find(channel_id) != m_channel_descriptors.end()));

                if (channel_descriptor)
                    channel_descriptor->channel_id = channel_id;

                m_channel_descriptors.insert(
                        {
                                channel_id,
                                std::move(channel_descriptor)
                        });

                return channel_id;
            }

            bool corelink_data_xchg_ip_proto_base::add_and_init_channel(
                    std::shared_ptr<ip_protocol_channel_descriptor> channel_descriptor,
                    corelink::core::network::channel_id_type &channel_id)
            {
                channel_id = add_channel(std::move(channel_descriptor));
                return init(channel_id);
            }

            void corelink_data_xchg_ip_proto_base::remove_channel(corelink::core::network::channel_id_type channel_id)
            {
                if (m_channel_descriptors.find(channel_id) != m_channel_descriptors.end())
                    m_channel_descriptors.erase(channel_id);
            }

            std::shared_ptr<ip_protocol_channel_descriptor> &
            corelink_data_xchg_ip_proto_base::get_channel(corelink::core::network::channel_id_type channel_id)
            {
                auto channel_descriptor = m_channel_descriptors.find(channel_id);
                if (channel_descriptor != m_channel_descriptors.end())
                    return channel_descriptor->second;
                return m_channel_descriptors[m_default_channel];
            }
        }
    }
}