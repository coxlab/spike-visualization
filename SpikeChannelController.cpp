//
//  SpikeChannelController.cpp
//  CinderSpikes
//
//  Created by David Cox on 6/17/11.
//  Copyright 2011 The Rowland Institute at Harvard. All rights reserved.
//

#include "SpikeChannelController.h"
#include <sstream>

using namespace spike_visualization;


#define HOST_ADDRESS    "tcp://127.0.0.1"
#define SPIKE_BASE_PORT     8000
#define CTL_IN_BASE_PORT    9000
#define CTL_OUT_BASE_PORT   10000

void SpikeChannelController::connectSpikeSocket(){
                
    std::cerr << "changing to channel " << channel_id << std::endl;
    
    // generate a fresh socket
    spike_socket = shared_ptr<zmq::socket_t>(new zmq::socket_t(msg_ctx, ZMQ_SUB));
    
    uint64_t hwm = 1000;
    spike_socket->setsockopt(ZMQ_HWM, &hwm, sizeof(uint64_t));
    
    // subscribe to all messages on receive
    spike_socket->setsockopt(ZMQ_SUBSCRIBE, NULL, 0);
    
    // construct the url
    ostringstream filename_stream, url_stream;
    
    string host_address = HOST_ADDRESS;
    
    url_stream << host_address << ":" << SPIKE_BASE_PORT + channel_id;
    try {
        spike_socket->connect(url_stream.str().c_str());
        std::cerr << "ZMQ client bound successfully to " << url_stream.str() << std::endl;
    } catch (zmq::error_t& e) {
        std::cerr << "ZMQ: " << e.what() << std::endl;
    }
}


void SpikeChannelController::connectCtlSockets(){
    
    // Create send and receive sockets
    ctl_in_socket = shared_ptr<zmq::socket_t>(new zmq::socket_t(msg_ctx, ZMQ_SUB));
    ctl_out_socket = shared_ptr<zmq::socket_t>(new zmq::socket_t(msg_ctx, ZMQ_PUB));
    
    uint64_t hwm = 1000;
    ctl_in_socket->setsockopt(ZMQ_HWM, &hwm, sizeof(uint64_t));
    ctl_out_socket->setsockopt(ZMQ_HWM, &hwm, sizeof(uint64_t));                
    
    // subscribe to all messages on receive
    ctl_in_socket->setsockopt(ZMQ_SUBSCRIBE, NULL, 0);
    
    // construct the url
    ostringstream receive_url_stream, send_url_stream;
    
     
    string host_address = HOST_ADDRESS;
    
    // connect receive socket
    receive_url_stream << host_address << ":" << CTL_OUT_BASE_PORT + channel_id;
    try {
        ctl_in_socket->connect(receive_url_stream.str().c_str());
        std::cerr << "ZMQ ctl client bound successfully to " << receive_url_stream.str() << std::endl;
    } catch (zmq::error_t& e) {
        std::cerr << "ZMQ (ctl receive): " << e.what() << std::endl;
    }
    
    // bind send socket
    send_url_stream << host_address << ":" << CTL_IN_BASE_PORT + channel_id;
    try {
        ctl_out_socket->bind(send_url_stream.str().c_str());
        std::cerr << "ZMQ ctl server bound successfully to " << send_url_stream.str() << std::endl;
    } catch (zmq::error_t& e) {
        std::cerr << "ZMQ (ctl send): " << e.what() << " (" << send_url_stream.str() << ")" << std::endl;
    }

    
}
