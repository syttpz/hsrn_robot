#pragma once

/**
 * @def CORELINK_DISABLE_RAPIDJSON
 * @brief Disable usage of rapidjson for JSON support in Corelink C++ client
 * @details If defined, the Corelink RapidJSON Wrapper will be disabled. However, note that this will break compilation
 * unless you provide a wrapper with the same name and API
 */

#include "commons/base_includes.hpp"
#include "commons/associative_containers_includes.hpp"
#include "commons/json_includes.hpp"

namespace corelink
{
    namespace utils
    {
        /**
         * @brief provides a wrapper over JSON libraries. Corelink supports a wrapper over RapidJSON,
         * however, if the interface is maintained, another JSON library can be wrapped and used
         */
        class CORELINK_EXPORT json
        {
            /**
             * @var RapidJSON document object
             */
            rapidjson::Document m_document;
        public:
            /**
             * Constructor
             * @param root_type_array if true, the default object created at the root level will be a JSON array.
             * if false, the default object created at the root level will be a JSON object. The default value is <b>false</b>
             */
            explicit json(bool root_type_array = false)
            {
                if (root_type_array)
                    m_document.SetArray();
                else
                    m_document.SetObject();
            }

            /**
             * Constructor
             * @param json_str initialize the JSON object with a JSON string
             * @details This constructor variant will try to parse the input string as JSON. However, if the string
             * passed is not valid JSON, it will throw
             */
            explicit json(in<std::string> json_str)
            {
                m_document.Parse(json_str.c_str(), json_str.size());
            }

            /**
             * @brief Move constructor
             * @param rhs json object to move from
             */
            json(rvref<json> rhs) noexcept
                    : m_document(std::move(rhs.m_document))
            {}

            /**
             * @brief Move assignment operator
             * @param rhs json object to move from
             * @return reference to the current object to which document was moved to
             */
            json &operator=(rvref<json> rhs) noexcept
            {
                m_document = std::move(rhs.m_document);
                return *this;
            }

            /**
             * @brief Copy assignment operator
             * @param rhs object to copy document from
             * @return reference to the current object to which document was copied to
             */
            json &operator=(clvref<json> rhs)
            {
                m_document.CopyFrom(rhs.m_document, m_document.GetAllocator());
                return *this;
            }

            /**
             * @brief Copy constructor
             * @param rhs object to copy document from
             */
            json(clvref<json> rhs)
            {
                (*this) = rhs; // use the copy assignment operator.
            }

            /**
             * Default destructor
             */
            ~json() = default;

            /**
             * @brief () operator overload.
             * @return a reference to the underlying JSON object. It is advisable to use this only where you want to
             * query the JSON object directly. Use the member functions of json to mutate the elements.
             */
            rapidjson::Document &operator()()
            {
                return m_document;
            }

            /**
             * @brief () operator overload for const objects
             * @return a const reference to the underlying JSON document object. It is advisable to use this only where you want to
             * query the JSON object directly. Use the member functions of json to mutate the elements.
             * Note that this function returns a const reference, so mutability and certain member accesses will be
             * limited
             */
            clvref<rapidjson::Document> operator()() const
            {
                return m_document;
            }

            /**
             * @brief get the value associated with the key as an integer. Note that this function can throw if the
             * underlying value is non-convertible to an integral type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return signed 32 bit integer value
             */
            inline int32_t get_int(in<std::string> name, int32_t def_val = 0) const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsInt()
                       ? member->value.GetInt() : def_val;
            }

            /**
             * @brief get the value associated with the key as an integer. Note that this function can throw if the
             * underlying value is non-convertible to an integral type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return signed 64 bit integer value
             */
            inline int64_t get_int64(in<std::string> name, int64_t def_val = 0) const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsInt64()
                       ? member->value.GetInt64() : def_val;
            }

            /**
             * @brief get the value associated with the key as std::string. Note that this function can throw if the
             * underlying value is non-convertible to string type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return std::string object with string representation of the value
             */
            inline std::string get_str(in<std::string> name, in<std::string> def_val = "") const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsString()
                       ? member->value.GetString() : def_val;
            }

            /**
             * @brief get the value associated with the key as double. Note that this function can throw if the
             * underlying value is non-convertible to floating point type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return signed double value
             */
            inline double get_double(in<std::string> name, double def_val = 0.0) const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsDouble()
                       ? member->value.GetDouble() : def_val;
            }

            /**
             * @brief get the value associated with the key as bool. Note that this function can throw if the
             * underlying value is non-convertible to boolean type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return boolean value associated with the key
             */
            inline bool get_bool(in<std::string> name, bool def_val = false) const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsBool()
                       ? member->value.GetBool() : def_val;
            }

            /**
             * @brief get the value associated with the key as an unsigned integer. Note that this function can throw if the
             * underlying value is non-convertible to an integral type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return unsigned 32 bit integer value
             */
            inline uint32_t get_uint(in<std::string> name, uint32_t def_val = false) const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsUint()
                       ? member->value.GetUint() : def_val;
            }

            /**
             * @brief get the value associated with the key as an unsigned integer. Note that this function can throw if the
             * underlying value is non-convertible to an integral type
             * @param name key to look up in the object
             * @param def_val in case key is not found, specify a default value
             * @return unsigned 64 bit integer value
             */
            inline uint64_t get_uint64(in<std::string> name, uint64_t def_val = false) const
            {
                auto member = m_document.FindMember(name.c_str());
                return member != m_document.MemberEnd()
                       && member->value.IsUint64()
                       ? member->value.GetUint64() : def_val;
            }

            /**
             * @details Parse a JSON string as JSON object
             * @param json_str JSON string
             * @param error error string if parse results in an error. empty if the conversion is successful
             * @return true if JSON parse was ok. false if it failed. please inspect error string if ret val is false.
             */
            bool try_parse(const std::string &json_str, std::string &error) noexcept
            {
                try
                {
                    m_document.Parse(json_str.c_str(), json_str.size());
                    return true;
                }
                catch (const std::exception &e)
                {
                    error += e.what();
                }
                return false;
            }

            /**
             * @details Serialise the JSON object to string for transport or redirect to streams
             * @note It is important to note that serializing the JSON to a string will be in memory, so be careful when
             * you do this for larger objects.
             * @param pretty_print Set to <b>true</b> if you wish to pretty print JSON with indentation. Defaulted to <b>false</b>
             * @return underlying JSON object as a string
             */
            std::string to_string(bool pretty_print = false) const
            {
                rapidjson::StringBuffer sb;
                if (pretty_print)
                {
                    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
                    m_document.Accept(writer);

                }
                else
                {
                    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
                    m_document.Accept(writer);
                }
                return sb.GetString();
            }

            /**
             * @brief Append multiple key-value pairs as an object to the root JSON array,
             * where value type of each KV pair is scalar native type
             * @tparam T native type. please do not specify custom types here as JSON values don't support it.
             * @param arr_items vector of key value pairs which will be appended to the root JSON
             * @return reference to itself.
             */
            template<class T>
            json &append(const std::vector<std::map<std::string, T>> &arr_items)
            {
                if (m_document.IsArray() && !arr_items.empty())
                {
                    for (auto &arr_item: arr_items)
                    {
                        if (!arr_item.empty())
                        {
                            rapidjson::Value my_value(rapidjson::kObjectType);
                            for (auto &obj_item: arr_item)
                            {
                                my_value.AddMember(
                                        rapidjson::Value(obj_item.first.c_str(), m_document.GetAllocator()),
                                        rapidjson::Value(obj_item.second),
                                        m_document.GetAllocator()
                                );
                            }
                            m_document.PushBack(my_value.Move(), m_document.GetAllocator());
                        }
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key-value pairs as an object to the root JSON array,
             * where value type of each KV pair is string type
             * @param arr_items vector of key value pairs which will be appended to the root JSON
             * @return reference to itself.
             */
            json &append(const std::vector<std::map<std::string, std::string>> &arr_items)
            {
                if (m_document.IsArray() && !arr_items.empty())
                {
                    for (auto &arr_item: arr_items)
                    {
                        if (arr_item.empty())
                            continue;

                        rapidjson::Value my_value(rapidjson::kObjectType);
                        for (auto &obj_item: arr_item)
                        {
                            my_value.AddMember(
                                    rapidjson::Value(obj_item.first.c_str(), m_document.GetAllocator()),
                                    rapidjson::Value(obj_item.second.c_str(), m_document.GetAllocator()),
                                    m_document.GetAllocator()
                            );
                        }
                        m_document.PushBack(my_value.Move(), m_document.GetAllocator());
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key-value pairs as an object to the root JSON array,
             * where value type of each KV pair is an array of scalar native type values
             * @tparam T native type. please do not specify custom types here as JSON values don't support it.
             * @param arr_items vector of key value pairs which will be appended to the root JSON
             * @return reference to itself.
             */
            template<class T>
            json &append(const std::vector<std::map<std::string, std::vector<T>>> &arr_items)
            {
                if (m_document.IsArray() && !arr_items.empty())
                {
                    for (auto &arr_item: arr_items)
                    {
                        if (arr_item.empty())
                            continue;

                        rapidjson::Value my_object(rapidjson::kObjectType);
                        for (auto &map_item: arr_item)
                        {
                            if (map_item.first.empty())
                                continue;

                            rapidjson::Value my_arr_value(rapidjson::kArrayType);
                            for (auto &one_value: map_item.second)
                            {
                                my_arr_value.PushBack(rapidjson::Value(one_value).Move(), m_document.GetAllocator());
                            }
                            my_object.AddMember(
                                    rapidjson::Value(map_item.first.c_str(), m_document.GetAllocator()),
                                    my_arr_value,
                                    m_document.GetAllocator()
                            );
                        }
                        m_document.PushBack(my_object, m_document.GetAllocator());
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key-value pairs as an object to the root JSON array,
             * where value type of each KV pair is an array of string type values
             * @param arr_items vector of key value pairs which will be appended to the root JSON
             * @return reference to itself.
             */
            json &append(const std::vector<std::map<std::string, std::vector<std::string>>> &arr_items)
            {
                if (m_document.IsArray() && !arr_items.empty())
                {
                    for (auto &arr_item: arr_items)
                    {
                        if (arr_item.empty())
                            continue;

                        rapidjson::Value my_object(rapidjson::kObjectType);
                        for (auto &map_item: arr_item)
                        {
                            if (map_item.first.empty())
                                continue;

                            rapidjson::Value my_arr_value(rapidjson::kArrayType);
                            for (auto &one_value: map_item.second)
                            {
                                my_arr_value.PushBack(
                                        rapidjson::Value(one_value.c_str(), m_document.GetAllocator()).Move(),
                                        m_document.GetAllocator());
                            }
                            my_object.AddMember(
                                    rapidjson::Value(map_item.first.c_str(), m_document.GetAllocator()),
                                    my_arr_value,
                                    m_document.GetAllocator()
                            );
                        }
                        m_document.PushBack(my_object, m_document.GetAllocator());
                    }
                }
                return *this;
            }

            /**
             * @brief Append a key value pair to the root JSON object where the value type is a native type.
             * @note If the root JSON object is an Array, this function will add the key value pair as
             * an object to the root array
             * @tparam T native type
             * @param key json object key
             * @param val json object native type value
             * @return reference to itself
             */
            template<class T>
            json &append(const std::string &key, const T &val)
            {
                if (!key.empty())
                {
                    if (m_document.IsObject() && !m_document.HasMember(key.c_str()))
                    {
                        m_document.AddMember(
                                rapidjson::Value(key.c_str(), m_document.GetAllocator()),
                                rapidjson::Value(val),
                                m_document.GetAllocator());
                    }
                    else if (m_document.IsArray())
                    {
                        std::map<std::string, T> mp = {
                                std::make_pair(key, val)
                        };
                        std::vector<std::map<std::string, T>> vec = {mp};
                        append(vec);
                    }
                }
                return *this;
            }

            /**
             * @brief Append a key value pair to the root JSON object where the value type is a string
             * @note Note that if the root JSON object is an Array, this function will add the key value pair as
             * an object to the root array
             * @param key json object key
             * @param val string type value
             * @return reference to itself
             */
            json &append(const std::string &key, const std::string &val)
            {
                if (!key.empty())
                {
                    if (m_document.IsObject() && !m_document.HasMember(key.c_str()))
                    {
                        m_document.AddMember(
                                rapidjson::Value(key.c_str(), m_document.GetAllocator()),
                                rapidjson::Value(val.c_str(), m_document.GetAllocator()),
                                m_document.GetAllocator()
                        );
                    }
                    else if (m_document.IsArray())
                    {
                        std::map<std::string, std::string> mp = {
                                std::make_pair(key, val)
                        };
                        append({mp});
                    }
                }
                return *this;
            }

            /**
             * @brief Append a key value pair to the root JSON object where the value type is an array of native type values
             * @note Note that if the root JSON object is an Array, this function will add the key value pair as
             * an object to the root array
             * @tparam native value type
             * @param key string key
             * @param val array of native type values
             * @return reference to itself
             */
            template<class T>
            json &append(const std::string &key, const std::vector<T> &val)
            {
                if (!key.empty())
                {
                    if (m_document.IsObject() && !m_document.HasMember(key.c_str()))
                    {
                        rapidjson::Value value(rapidjson::kArrayType);
                        if (!val.empty())
                        {
                            for (auto &arr_item: val)
                                value.PushBack(rapidjson::Value(arr_item).Move(), m_document.GetAllocator());
                        }
                        m_document.AddMember(
                                rapidjson::Value(key.c_str(), m_document.GetAllocator()),
                                value,
                                m_document.GetAllocator());
                    }
                    else if (m_document.IsArray())
                    {
                        std::map<std::string, std::vector<T>> mp = {
                                std::make_pair(key, val)
                        };
                        std::vector<decltype(mp)> vec = {mp};
                        append(vec);
                    }
                }
                return *this;
            }

            /**
             * @brief Append a key value pair to the root JSON object where the value type is an array of strings
             * @note Note that if the root JSON object is an Array, this function will add the key value pair as
             * an object to the root array
             * @param key string key
             * @param val array of strings
             * @return reference to itself
             */
            json &append(const std::string &key, const std::vector<std::string> &val)
            {
                if (!key.empty())
                {
                    if (m_document.IsObject() && !m_document.HasMember(key.c_str()))
                    {
                        rapidjson::Value value(rapidjson::kArrayType);
                        if (!val.empty())
                        {
                            for (auto &arr_item: val)
                                value.PushBack(
                                        rapidjson::Value(arr_item.c_str(), m_document.GetAllocator()),
                                        m_document.GetAllocator()
                                );
                        }
                        m_document.AddMember(
                                rapidjson::Value(key.c_str(), m_document.GetAllocator()),
                                value,
                                m_document.GetAllocator());
                    }
                    else if (m_document.IsArray())
                    {
                        std::map<std::string, std::vector<std::string>> mp = {
                                std::make_pair(key, val)
                        };
                        std::vector<decltype(mp)> vec = {mp};
                        append(vec);
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key value pair to the root JSON object where the value type is a native type
             * @note Note that if the root JSON object is an Array, this function will add the key value pairs as
             * an object to the root array
             * @tparam T native value type
             * @param kv_items key value pairs of items to be added to the root object
             * @return reference to itself
             */
            template<class T>
            json &append(const std::map<std::string, T> &kv_items)
            {
                if (!kv_items.empty())
                {
                    if (m_document.IsObject())
                    {
                        for (auto &item: kv_items)
                        {
                            m_document.AddMember(rapidjson::Value(item.first.c_str(), m_document.GetAllocator()),
                                                 rapidjson::Value(item.second),
                                                 m_document.GetAllocator());
                        }
                    }
                    else if (m_document.IsArray())
                    {
                        append(
                                {kv_items}
                        );
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key value pair to the root JSON object where the value type is value of string type
             * @note Note that if the root JSON object is an Array, this function will add the key value pairs as
             * an object to the root array
             * @param kv_items key value pairs that you wish to add to the root JSON object.
             * @return reference to itself
             */
            json &append(const std::map<std::string, std::string> &kv_items)
            {
                if (!kv_items.empty())
                {
                    if (m_document.IsObject())
                    {
                        for (auto &item: kv_items)
                        {
                            m_document.AddMember(
                                    rapidjson::Value(item.first.c_str(), m_document.GetAllocator()),
                                    rapidjson::Value(item.second.c_str(), m_document.GetAllocator()),
                                    m_document.GetAllocator()
                            );
                        }
                    }
                    else if (m_document.IsArray())
                    {
                        std::vector<std::map<std::string, std::string>> v{kv_items};
                        append(v);
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key value pair to the root JSON object where the value type is an array of native type values
             * @note Note that if the root JSON object is an Array, this function will add the key value pairs as
             * an object to the root array
             * @tparam T native type
             * @param kv_items key value pairs that you wish to add to the root JSON object.
             * @return reference to itself.
             */
            template<class T>
            json &append(const std::map<std::string, std::vector<T>> &kv_items)
            {
                if (!kv_items.empty())
                {
                    if (m_document.IsObject())
                    {
                        for (auto &item: kv_items)
                        {
                            rapidjson::Value value(rapidjson::kArrayType);
                            for (const T &arr_item: item.second)
                            {
                                value.PushBack(rapidjson::Value(arr_item).Move(), m_document.GetAllocator());
                            }

                            m_document.AddMember(rapidjson::Value(item.first.c_str(), m_document.GetAllocator()), value,
                                                 m_document.GetAllocator());
                        }
                    }
                    else if (m_document.IsArray())
                    {
                        append({kv_items});
                    }
                }
                return *this;
            }

            /**
             * @brief Append multiple key value pair to the root JSON object where the value type is an array of string type values
             * @note Note that if the root JSON object is an Array, this function will add the key value pairs as
             * an object to the root array
             * @param kv_items key value pairs that you wish to add to the root JSON object.
             * @return reference to itself.
             */
            json &append(const std::map<std::string, std::vector<std::string>> &kv_items)
            {
                if (!kv_items.empty())
                {
                    if (m_document.IsObject())
                    {
                        for (auto &item: kv_items)
                        {
                            rapidjson::Value value(rapidjson::kArrayType);
                            if (item.second.empty())
                                continue;
                            for (auto &arr_item: item.second)
                            {
                                value.PushBack(rapidjson::Value(
                                                       arr_item.c_str(),
                                                       m_document.GetAllocator()
                                               ).Move(),
                                               m_document.GetAllocator());
                            }

                            m_document.AddMember(rapidjson::Value(item.first.c_str(), m_document.GetAllocator()), value,
                                                 m_document.GetAllocator());
                        }
                    }
                    else if (m_document.IsArray())
                    {
                        std::vector<std::map<std::string, std::vector<std::string>>> v{kv_items};
                        append(v);
                    }
                }
                return *this;
            }

            /**
             * @details Remove an element from the root JSON object based on a key, if found
             * @note This function only works when the root JSON type is an object
             * @param key key of the entry to drop
             * @return reference to itself
             */
            CORELINK_CPP_ATTR_MAYBE_UNUSED
            json &remove(const std::string &key)
            {
                if (!key.empty() && m_document.IsObject())
                {
                    if (m_document.HasMember(key.c_str()))
                        m_document.RemoveMember(key.c_str());
                }
                return *this;
            }

            /**
             * @details Remove all elements from the root JSON document
             * @return reference to itself
             */
            CORELINK_CPP_ATTR_MAYBE_UNUSED json &clear()
            {
                if (!m_document.Empty())
                    m_document.Clear();
                return *this;
            }
        };
    }
}