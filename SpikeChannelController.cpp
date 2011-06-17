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


void SpikeChannelController::connectSpikeSocket(){
                
    std::cerr << "changing to channel " << channel_id << std::endl;
    
    // generate a fresh socket
    spike_socket = shared_ptr<zmq::socket_t>(new zmq::socket_t(msg_ctx, ZMQ_PUB));
    
    uint64_t hwm = 1000;
    spike_socket->setsockopt(ZMQ_HWM, &hwm, sizeof(uint64_t));
    
    // construct the url
    ostringstream filename_stream, url_stream;
    
    // hacky filesystem manipulation
    filename_stream << "/tmp/spike_channels";
    
    string mkdir_command("mkdir -p ");
    mkdir_command.append(filename_stream.str());
    system(mkdir_command.c_str());
    
    filename_stream << "/" << channel_id;
    string touch_command("touch ");
    touch_command.append(filename_stream.str());
    system(touch_command.c_str());
    
    url_stream << "ipc://" << filename_stream.str();
    try {
        spike_socket->bind(url_stream.str().c_str());
        std::cerr << "ZMQ server bound successfully to " << url_stream.str() << std::endl;
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
    ostringstream receive_filename_stream, send_filename_stream, receive_url_stream, send_url_stream;
    
    // hacky filesystem manipulation
    receive_filename_stream << "/tmp/spike_channels/ctl";
    send_filename_stream << receive_filename_stream.str();
    
    // swapped for client
    receive_filename_stream << "/out/" << channel_id;
    send_filename_stream << "/in/" << channel_id;
    
    string mkdir_command("mkdir -p ");
    mkdir_command.append(receive_filename_stream.str());
    system(mkdir_command.c_str());

    mkdir_command = "mkdir -p ";
    mkdir_command.append(send_filename_stream.str());
    system(mkdir_command.c_str());
    
    
    
    string touch_command("touch ");
    touch_command.append(send_filename_stream.str());
    system(touch_command.c_str());
    
    touch_command = "touch ";
    touch_command.append(receive_filename_stream.str());
    system(touch_command.c_str());
    
    // connect receive socket
    receive_url_stream << "ipc://" << receive_filename_stream.str();
    try {
        ctl_in_socket->connect(receive_url_stream.str().c_str());
        std::cerr << "ZMQ ctl client bound successfully to " << receive_url_stream.str() << std::endl;
    } catch (zmq::error_t& e) {
        std::cerr << "ZMQ (ctl receive): " << e.what() << std::endl;
    }
    
    // bind send socket
    send_url_stream << "ipc://" << send_filename_stream.str();
    try {
        ctl_out_socket->bind(send_url_stream.str().c_str());
        std::cerr << "ZMQ ctl server bound successfully to " << send_url_stream.str() << std::endl;
    } catch (zmq::error_t& e) {
        std::cerr << "ZMQ (ctl send): " << e.what() << " (" << send_url_stream.str() << ")" << std::endl;
    }

    
}
