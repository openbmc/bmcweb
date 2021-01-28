#pragma once
#include <nghttp2/nghttp2.h>

#include <span>

struct nghttp2_session_ptr;

struct Nghttp2SessionCallbacksPtr
{
    friend nghttp2_session_ptr;
    Nghttp2SessionCallbacksPtr()
    {
        nghttp2_session_callbacks_new(&ptr);
    }

    ~Nghttp2SessionCallbacksPtr()
    {
        nghttp2_session_callbacks_del(ptr);
    }

    Nghttp2SessionCallbacksPtr(const Nghttp2SessionCallbacksPtr&) = delete;
    Nghttp2SessionCallbacksPtr&
        operator=(const Nghttp2SessionCallbacksPtr&) = delete;
    Nghttp2SessionCallbacksPtr(Nghttp2SessionCallbacksPtr&&) = delete;
    Nghttp2SessionCallbacksPtr&
        operator=(Nghttp2SessionCallbacksPtr&&) = delete;

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
        nghttp2_session_callbacks_set_on_begin_headers_callback(ptr,
                                                                onBeginHeaders);
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

  private:
    nghttp2_session_callbacks* get()
    {
        return ptr;
    }

    nghttp2_session_callbacks* ptr = nullptr;
};

struct nghttp2_session_ptr
{
    nghttp2_session_ptr(Nghttp2SessionCallbacksPtr& callbacks)
    {
        if (nghttp2_session_server_new(&ptr, callbacks.get(), nullptr) != 0)
        {
            BMCWEB_LOG_ERROR << "nghttp2_session_server_new failed";
            return;
        }
    }

    ~nghttp2_session_ptr()
    {
        nghttp2_session_del(ptr);
    }

    nghttp2_session_ptr() = default;

    // explicitly uncopyable
    nghttp2_session_ptr(const nghttp2_session_ptr&) = delete;
    nghttp2_session_ptr& operator=(const nghttp2_session_ptr&) = delete;

    nghttp2_session_ptr(nghttp2_session_ptr&& other) noexcept
    {
        *this = std::move(other);
    }

    nghttp2_session_ptr& operator=(nghttp2_session_ptr&& other) noexcept
    {
        if (this != &other)
        {
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

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

    ssize_t send(const uint8_t** data)
    {
        return nghttp2_session_mem_send(ptr, data);
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