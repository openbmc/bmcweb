#ifndef _MULTIPART_READER_H_
#define _MULTIPART_READER_H_

#include "MultipartParser.hpp"

#include <boost/container/flat_map.hpp>

#include <map>
#include <utility>

struct MultipartObj
{
    boost::container::flat_map<std::string, std::string> headers;
    std::string values;
};

class MultipartReader
{
  public:
    typedef void (*PartBeginCallback)(
        const boost::container::flat_map<std::string, std::string>& headers,
        void* userData);
    typedef void (*PartDataCallback)(const char* buffer, size_t size,
                                     void* userData);
    typedef void (*Callback)(void* userData);

  private:
    MultipartParser parser;
    bool headersProcessed;
    std::vector<MultipartObj> multipartObjs;
    boost::container::flat_map<std::string, std::string> currentHeaders;
    std::string currentHeaderName, currentHeaderValue;
    std::string partData;

    void resetReaderCallbacks()
    {
        onPartBegin = NULL;
        onPartData = NULL;
        onPartEnd = NULL;
        onEnd = NULL;
        userData = NULL;
    }

    void setParserCallbacks()
    {
        parser.onPartBegin = cbPartBegin;
        parser.onHeaderField = cbHeaderField;
        parser.onHeaderValue = cbHeaderValue;
        parser.onHeaderEnd = cbHeaderEnd;
        parser.onHeadersEnd = cbHeadersEnd;
        parser.onPartData = cbPartData;
        parser.onPartEnd = cbPartEnd;
        parser.onEnd = cbEnd;
        parser.userData = this;
    }

    static void cbPartBegin(const char* buffer, size_t start, size_t end,
                            void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        self->headersProcessed = false;
        self->currentHeaders.clear();
        self->currentHeaderName.clear();
        self->currentHeaderValue.clear();
    }

    static void cbHeaderField(const char* buffer, size_t start, size_t end,
                              void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        self->currentHeaderName.append(buffer + start, end - start);
    }

    static void cbHeaderValue(const char* buffer, size_t start, size_t end,
                              void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        self->currentHeaderValue.append(buffer + start, end - start);
    }

    static void cbHeaderEnd(const char* buffer, size_t start, size_t end,
                            void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        self->currentHeaders.emplace(
            std::make_pair(self->currentHeaderName, self->currentHeaderValue));
        self->currentHeaderName.clear();
        self->currentHeaderValue.clear();
    }

    static void cbHeadersEnd(const char* buffer, size_t start, size_t end,
                             void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        if (self->onPartBegin != NULL)
        {
            self->onPartBegin(self->currentHeaders, self->userData);
        }
    }

    static void cbPartData(const char* buffer, size_t start, size_t end,
                           void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        if (self->onPartData != NULL)
        {
            self->onPartData(buffer + start, end - start, self->userData);
        }
        self->partData = std::string(buffer + start, end - start);

        MultipartObj multipartObj;
        multipartObj.headers = std::move(self->currentHeaders);
        multipartObj.values = self->partData;
        self->multipartObjs.emplace_back(std::move(multipartObj));
        self->currentHeaders.clear();
        self->currentHeaderName.clear();
        self->currentHeaderValue.clear();
    }

    static void cbPartEnd(const char* buffer, size_t start, size_t end,
                          void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        if (self->onPartEnd != NULL)
        {
            self->onPartEnd(self->userData);
        }
    }

    static void cbEnd(const char* buffer, size_t start, size_t end,
                      void* userData)
    {
        MultipartReader* self = static_cast<MultipartReader*>(userData);
        if (self->onEnd != NULL)
        {
            self->onEnd(self->userData);
        }
    }

  public:
    PartBeginCallback onPartBegin;
    PartDataCallback onPartData;
    Callback onPartEnd;
    Callback onEnd;
    void* userData;

    MultipartReader()
    {
        resetReaderCallbacks();
        setParserCallbacks();
    }

    MultipartReader(const std::string& boundary) : parser(boundary)
    {
        resetReaderCallbacks();
        setParserCallbacks();
    }

    void reset()
    {
        parser.reset();
    }

    void setBoundary(const std::string& boundary)
    {
        parser.setBoundary(boundary);
    }

    size_t feed(const char* buffer, size_t len)
    {
        return parser.feed(buffer, len);
    }

    bool succeeded() const
    {
        return parser.succeeded();
    }

    bool hasError() const
    {
        return parser.hasError();
    }

    bool stopped() const
    {
        return parser.stopped();
    }

    const char* getErrorMessage() const
    {
        return parser.getErrorMessage();
    }

    const std::vector<MultipartObj>& getMultipartObjs()
    {
        return multipartObjs;
    }
};

#endif /* _MULTIPART_READER_H_ */
