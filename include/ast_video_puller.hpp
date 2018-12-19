#pragma once

#include <ast_video_types.hpp>
#include <boost/asio.hpp>
#include <cassert>
#include <iostream>
#include <mutex>
#include <vector>

namespace ast_video
{

//
// Cursor struct is used in User Mode
//
struct AstCurAttributionTag
{
    unsigned int posX;
    unsigned int posY;
    unsigned int curWidth;
    unsigned int curHeight;
    unsigned int curType; // 0:mono 1:color 2:disappear cursor
    unsigned int curChangeFlag;
};

//
// For storing Cursor Information
//
struct AstCursorTag
{
    AstCurAttributionTag attr;
    // unsigned char     icon[MAX_CUR_OFFSETX*MAX_CUR_OFFSETY*2];
    unsigned char *icon; //[64*64*2];
};

//
// For select image format, i.e. 422 JPG420, 444 JPG444, lumin/chrom table, 0
// ~ 11, low to high
//
struct FeaturesTag
{
    short jpgFmt; // 422:JPG420, 444:JPG444
    short luminTbl;
    short chromTbl;
    short toleranceNoise;
    int w;
    int h;
    unsigned char *buf;
};

//
// For configure video engine control registers
//
struct ImageInfo
{
    short doImageRefresh; // Action 0:motion 1:fullframe 2:quick cursor
    char qcValid;         // quick cursor enable/disable
    unsigned int len;
    int crypttype;
    char cryptkey[16];
    union
    {
        FeaturesTag features;
        AstCursorTag cursorInfo;
    } parameter;
};

class SimpleVideoPuller
{
  public:
    SimpleVideoPuller() : imageInfo(){};

    void initialize()
    {
        std::cout << "Opening /dev/video\n";
        videoFd = open("/dev/video", O_RDWR);
        if (videoFd == 0)
        {
            std::cout << "Failed to open /dev/video\n";
            throw std::runtime_error("Failed to open /dev/video");
        }
        std::cout << "Opened successfully\n";
    }

    RawVideoBuffer readVideo()
    {
        assert(videoFd != 0);
        RawVideoBuffer raw;

        imageInfo.doImageRefresh = 1; // full frame refresh
        imageInfo.qcValid = 0;        // quick cursor disabled
        imageInfo.parameter.features.buf =
            reinterpret_cast<unsigned char *>(raw.buffer.data());
        imageInfo.crypttype = -1;
        std::cout << "Writing\n";

        int status;
        /*
        status = write(videoFd, reinterpret_cast<char*>(&imageInfo),
                            sizeof(imageInfo));
        if (status != sizeof(imageInfo)) {
          std::cout << "Write failed.  Return: " << status << "\n";
          perror("perror output:");
        }

        std::cout << "Write done\n";
        */
        std::cout << "Reading\n";
        status = read(videoFd, reinterpret_cast<char *>(&imageInfo),
                      sizeof(imageInfo));
        std::cout << "Done reading\n";

        if (status != 0)
        {
            std::cerr << "Read failed with status " << status << "\n";
        }

        raw.buffer.resize(imageInfo.len);

        raw.height = imageInfo.parameter.features.h;
        raw.width = imageInfo.parameter.features.w;
        if (imageInfo.parameter.features.jpgFmt == 422)
        {
            raw.mode = YuvMode::YUV420;
        }
        else
        {
            raw.mode = YuvMode::YUV444;
        }
        return raw;
    }

  private:
    int videoFd{};
    ImageInfo imageInfo;
};

#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
class AsyncVideoPuller
{
  public:
    using video_callback = std::function<void(RawVideoBuffer &)>;

    explicit AsyncVideoPuller(boost::asio::io_context &ioService) :
        imageInfo(), devVideo(ioService, open("/dev/video", O_RDWR))
    {
        videobuf = std::make_shared<RawVideoBuffer>();

        imageInfo.doImageRefresh = 1; // full frame refresh
        imageInfo.qcValid = 0;        // quick cursor disabled
        imageInfo.parameter.features.buf =
            reinterpret_cast<unsigned char *>(videobuf->buffer.data());
        imageInfo.crypttype = -1;
    };

    void registerCallback(video_callback &callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        callbacks.push_back(callback);
        startRead();
    }

    void startRead()
    {
        auto mutableBuffer = boost::asio::buffer(&imageInfo, sizeof(imageInfo));
        boost::asio::async_read(devVideo, mutableBuffer,
                                [this](const boost::system::error_code &ec,
                                       std::size_t bytes_transferred) {
                                    if (ec)
                                    {
                                        std::cerr << "Read failed with status "
                                                  << ec << "\n";
                                    }
                                    else
                                    {
                                        this->readDone();
                                    }
                                });
    }

    void readDone()
    {
        std::cout << "Done reading\n";
        videobuf->buffer.resize(imageInfo.len);

        videobuf->height = imageInfo.parameter.features.h;
        videobuf->width = imageInfo.parameter.features.w;
        if (imageInfo.parameter.features.jpgFmt == 422)
        {
            videobuf->mode = YuvMode::YUV420;
        }
        else
        {
            videobuf->mode = YuvMode::YUV444;
        }
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto &callback : callbacks)
        {
            // TODO(ed) call callbacks async and double buffer frames
            callback(*videobuf);
        }
    }

  private:
    std::shared_ptr<RawVideoBuffer> videobuf;
    boost::asio::posix::stream_descriptor devVideo;
    ImageInfo imageInfo;
    std::mutex callbackMutex;
    std::vector<video_callback> callbacks;
};
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
} // namespace ast_video
