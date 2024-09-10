// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

extern "C"
{
#include <nghttp2/nghttp2.h>
}

#include "logging.hpp"

#include <span>

/* This file contains RAII compatible adapters for nghttp2 structures.  They
 * attempt to be as close to a direct call as possible, while keeping the RAII
 * lifetime safety for the various classes.  Because of this, they use the same
 * naming as nghttp2, so ignore naming violations.
 */

// NOLINTBEGIN(readability-identifier-naming,
// readability-make-member-function-const)

struct nghttp2_session;

struct nghttp2_session_callbacks
{
    friend nghttp2_session;
    nghttp2_session_callbacks()
    {
        nghttp2_session_callbacks_new(&ptr);
    }

    ~nghttp2_session_callbacks()
    {
        nghttp2_session_callbacks_del(ptr);
    }

    nghttp2_session_callbacks(const nghttp2_session_callbacks&) = delete;
    nghttp2_session_callbacks&
        operator=(const nghttp2_session_callbacks&) = delete;
    nghttp2_session_callbacks(nghttp2_session_callbacks&&) = delete;
    nghttp2_session_callbacks& operator=(nghttp2_session_callbacks&&) = delete;

    void setSendCallback(nghttp2_send_callback sendCallback)
    {
        nghttp2_session_callbacks_set_send_callback(ptr, sendCallback);
    }

    void setOnFrameRecvCallback(nghttp2_on_frame_recv_callback recvCallback)
    {
        nghttp2_session_callbacks_set_on_frame_recv_callback(ptr, recvCallback);
    }

    void setOnStreamCloseCallback(nghttp2_on_stream_close_callback onClose)
    {
        nghttp2_session_callbacks_set_on_stream_close_callback(ptr, onClose);
    }

    void setOnHeaderCallback(nghttp2_on_header_callback onHeader)
    {
        nghttp2_session_callbacks_set_on_header_callback(ptr, onHeader);
    }

    void setOnBeginHeadersCallback(
        nghttp2_on_begin_headers_callback onBeginHeaders)
    {
        nghttp2_session_callbacks_set_on_begin_headers_callback(
            ptr, onBeginHeaders);
    }

    void setSendDataCallback(nghttp2_send_data_callback onSendData)
    {
        nghttp2_session_callbacks_set_send_data_callback(ptr, onSendData);
    }
    void setBeforeFrameSendCallback(
        nghttp2_before_frame_send_callback beforeSendFrame)
    {
        nghttp2_session_callbacks_set_before_frame_send_callback(
            ptr, beforeSendFrame);
    }
    void
        setAfterFrameSendCallback(nghttp2_on_frame_send_callback afterSendFrame)
    {
        nghttp2_session_callbacks_set_on_frame_send_callback(ptr,
                                                             afterSendFrame);
    }
    void setAfterFrameNoSendCallback(
        nghttp2_on_frame_not_send_callback afterSendFrame)
    {
        nghttp2_session_callbacks_set_on_frame_not_send_callback(
            ptr, afterSendFrame);
    }

    void setOnDataChunkRecvCallback(
        nghttp2_on_data_chunk_recv_callback afterDataChunkRecv)
    {
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
            ptr, afterDataChunkRecv);
    }

  private:
    nghttp2_session_callbacks* get()
    {
        return ptr;
    }

    nghttp2_session_callbacks* ptr = nullptr;
};

struct nghttp2_session
{
    explicit nghttp2_session(nghttp2_session_callbacks& callbacks)
    {
        if (nghttp2_session_server_new(&ptr, callbacks.get(), nullptr) != 0)
        {
            BMCWEB_LOG_ERROR("nghttp2_session_server_new failed");
            return;
        }
    }

    ~nghttp2_session()
    {
        nghttp2_session_del(ptr);
    }

    // explicitly uncopyable
    nghttp2_session(const nghttp2_session&) = delete;
    nghttp2_session& operator=(const nghttp2_session&) = delete;

    nghttp2_session(nghttp2_session&& other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }

    nghttp2_session& operator=(nghttp2_session&& other) noexcept = delete;

    int submitSettings(std::span<nghttp2_settings_entry> iv)
    {
        return nghttp2_submit_settings(ptr, NGHTTP2_FLAG_NONE, iv.data(),
                                       iv.size());
    }
    void setUserData(void* object)
    {
        nghttp2_session_set_user_data(ptr, object);
    }

    ssize_t memRecv(std::span<const uint8_t> buffer)
    {
        return nghttp2_session_mem_recv(ptr, buffer.data(), buffer.size());
    }

    std::span<const uint8_t> memSend()
    {
        const uint8_t* bytes = nullptr;
        ssize_t size = nghttp2_session_mem_send(ptr, &bytes);
        return {bytes, static_cast<size_t>(size)};
    }

    int submitResponse(int32_t streamId, std::span<const nghttp2_nv> headers,
                       const nghttp2_data_provider* dataPrd)
    {
        return nghttp2_submit_response(ptr, streamId, headers.data(),
                                       headers.size(), dataPrd);
    }

  private:
    nghttp2_session* ptr = nullptr;
};

struct nghttp2_hd_inflater_ex
{
    nghttp2_hd_inflater* ptr = nullptr;

  public:
    nghttp2_hd_inflater_ex()
    {
        if (nghttp2_hd_inflate_new(&ptr) != 0)
        {
            BMCWEB_LOG_ERROR("nghttp2_hd_inflater_new failed");
        }
    }

    ssize_t hd2(nghttp2_nv* nvOut, int* inflateFlags, const uint8_t* in,
                size_t inlen, int inFinal)
    {
        return nghttp2_hd_inflate_hd2(ptr, nvOut, inflateFlags, in, inlen,
                                      inFinal);
    }

    int endHeaders()
    {
        return nghttp2_hd_inflate_end_headers(ptr);
    }

    nghttp2_hd_inflater_ex(const nghttp2_hd_inflater_ex&) = delete;
    nghttp2_hd_inflater_ex& operator=(const nghttp2_hd_inflater_ex&) = delete;
    nghttp2_hd_inflater_ex& operator=(nghttp2_hd_inflater_ex&&) = delete;
    nghttp2_hd_inflater_ex(nghttp2_hd_inflater_ex&& other) = delete;

    ~nghttp2_hd_inflater_ex()
    {
        if (ptr != nullptr)
        {
            nghttp2_hd_inflate_del(ptr);
        }
    }
};
// NOLINTEND(readability-identifier-naming,
// readability-make-member-function-const)
