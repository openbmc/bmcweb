//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <array>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::udp;

int main(int argc, char* argv[]) {
  try {
    boost::asio::io_service io_service;

    udp::resolver resolver(io_service);
    udp::resolver::query query(udp::v4(), "10.243.48.31", "623");
    udp::endpoint receiver_endpoint = *resolver.resolve(query);

    udp::socket socket(io_service);
    socket.open(udp::v4());

    std::string username = "root";
    std::string password = "two";
    uint8_t privilege_level = 4;
    uint8_t seq_number = 1;
    uint8_t command = 0x38;  // AUTH_CAP_CMD
    // std::array<uint8_t> ChannelAuthCap{0x80 | (0x0f & channel),
    // privilege_level&0x0f};

    uint8_t srcaddr = 0x81;  // 0xC8?
    uint8_t dstaddr = 0x20;

    uint8_t net_function = 0x06;
    uint8_t lun = 0;

    uint8_t netfn_lun = static_cast<uint8_t>((net_function << 2) + lun);
    uint8_t channel_number =
        0x0E + 0x80;  // E is defined in spec as this channel
                      // 0x80 is requesting IPMI 2.0
    uint8_t byte1 = static_cast<uint8_t>(channel_number | 0x80);
    boost::array<uint8_t, 2> payload{byte1, privilege_level};

    int payload_sum = 0;
    for (auto element : payload) {
      payload_sum += element;
    }

    uint8_t chk1 = (1 + ((~(dstaddr + netfn_lun)) & 0xff)) & 0xff;
    uint8_t chk2 = srcaddr + (seq_number << 2) + command + payload_sum;
    chk2 = (1 + ((~chk2) & 0xff)) & 0xff;

    uint8_t ptype = 0;
    uint8_t session_id = 0;

    uint8_t seq_number2 = 0;
    uint8_t seq_number_lun = (seq_number << 2) + lun;

    uint8_t seq2 = 0xff;  //????
    uint8_t rmcp_class = 0x07;

    std::vector<uint8_t> send_buf = {
        0x06,       0x00,           seq2,
        rmcp_class, 0x06,           ptype,
        session_id, seq_number2,    0x00,
        0x00,       0x00,           0x00,
        0x00,       0x00,           0x09,
        0x00,       dstaddr,        netfn_lun,
        chk1,       srcaddr,        seq_number_lun,
        command,    channel_number, privilege_level,
        chk2};

    for (auto character : send_buf) {
      std::cout << std::hex << static_cast<unsigned>(character) << " ";
    }
    std::cout << std::endl;
    socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);

    boost::array<unsigned char, 32> recv_buf;
    udp::endpoint sender_endpoint;
    size_t len =
        socket.receive_from(boost::asio::buffer(recv_buf), sender_endpoint);

    for (auto character : recv_buf) {
      std::cout << std::hex << static_cast<unsigned>(character) << " ";
    }

    std::cout << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}