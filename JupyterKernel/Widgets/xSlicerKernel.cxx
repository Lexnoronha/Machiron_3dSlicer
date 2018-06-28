/***************************************************************************
* Copyright (c) 2016, Johan Mabille and Sylvain Corlay                     *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include "xSlicerKernel.h"

#include "xeus/xkernel.hpp"
#include "xeus/xguid.hpp"
#include "xeus/xkernel_core.hpp"

#if (defined(__linux__) || defined(__unix__))
#define LINUX_PLATFORM
#elif (defined(_WIN32) || defined(_WIN64))
#define WINDOWS_PLATFORM
#endif

#if defined(LINUX_PLATFORM)
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

namespace xeus
{

    void build_start_msg(xkernel_core::authentication_ptr& auth,
                         const std::string& kernel_id,
                         const std::string& user_name,
                         const std::string& session_id,
                         zmq::multipart_t& wire_msg)
    {
        std::string topic = "kernel_core." + kernel_id + ".status";
        xjson content;
        content["execution_state"] = "starting";

        xpub_message msg(topic,
                         make_header("status", user_name, session_id),
                         xjson::object(),
                         xjson::object(),
                         std::move(content),
                         buffer_sequence());
        std::move(msg).serialize(wire_msg, *auth);
    }

    xSlicerKernel::xSlicerKernel(const xconfiguration& config,
                                 const std::string& user_name,
                                 interpreter_ptr interpreter,
                                 server_builder builder)
        : m_config(config),
          m_user_name(user_name),
          p_interpreter(std::move(interpreter)),
          m_builder(builder)
    {
    }

    void xSlicerKernel::start()
    {
        m_kernel_id = new_xguid();
        m_session_id = new_xguid();

        using authentication_ptr = xkernel_core::authentication_ptr;
        authentication_ptr auth = make_xauthentication(m_config.m_signature_scheme, m_config.m_key);

        zmq::multipart_t start_msg;
        build_start_msg(auth, m_kernel_id, m_user_name, m_session_id, start_msg);

        p_server = m_builder(m_context, m_config);

        p_core = kernel_core_ptr(new xkernel_core(m_kernel_id, m_user_name, m_session_id,
                                                  std::move(auth), p_server.get(), p_interpreter.get()));

        p_interpreter->configure();
        p_server->start(start_msg);
    }

}
