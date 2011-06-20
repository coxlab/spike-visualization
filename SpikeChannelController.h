//
//  SpikeChannelController.h
//  CinderSpikes
//
//  Created by David Cox on 6/17/11.
//  Copyright 2011 The Rowland Institute at Harvard. All rights reserved.
//

#include "ctl_message.pb.h"
#include "spike_wave.pb.h"
#include "SpikeRenderer.h"
#include <boost/shared_ptr.hpp>
#include <zmq.hpp>

#define PRE_TRIGGER 33
#define POST_TRIGGER 33

#define TRIGGER_THRESHOLD       CtlMessage::THRESHOLD
#define AMPLITUDE_MAX           CtlMessage::AMPLITUDE_MAX
#define AMPLITUDE_MIN           CtlMessage::AMPLITUDE_MIN
#define TIME_MAX                CtlMessage::TIME_MAX
#define TIME_MIN                CtlMessage::TIME_MIN
#define AUTOTHRESHOLD_STATE     CtlMessage::AUTOTHRESHOLD_STATE

typedef boost::shared_ptr<zmq::socket_t> SocketPtr;


namespace spike_visualization {



class SpikeChannelController {

  protected:
  
    zmq::context_t msg_ctx;
    SocketPtr spike_socket;
    SocketPtr ctl_out_socket;
    SocketPtr ctl_in_socket;

    SpikeRendererPtr renderer;

    int channel_id;
    
    int adjust_mode;
    double old_adjust_value;

  public:
  
    SpikeChannelController(int ch, SpikeRendererPtr _renderer):
                           msg_ctx(1),
                           channel_id(ch),
                           renderer(_renderer)
    {
        connectSockets();
    }
    
  
    // socket related
    void connectSockets(){
        connectSpikeSocket();
        connectCtlSockets();
    }
    
    void connectSpikeSocket();
    void connectCtlSockets();
    
    void sendCtlEvent(CtlMessage_MessageType param, double value){
        
        CtlMessage ctl_msg;
        ctl_msg.set_channel_id(channel_id);
        ctl_msg.set_time_stamp(0);
        ctl_msg.set_message_type(param);
        ctl_msg.set_value(value);
        
        string serialized;

        ctl_msg.SerializeToString(&serialized);
        zmq::message_t msg(serialized.length());
        memcpy(msg.data(), serialized.c_str(), serialized.length());
        bool rc = ctl_out_socket->send(msg);
    }
    
    
    
    void readSpikeSocket(){
    
        zmq::message_t msg;
        
        // Receive a message 
        bool recvd = spike_socket->recv(&msg, ZMQ_NOBLOCK);
        
        while(recvd){

            string data((const char *)msg.data(), msg.size());
            SpikeWaveBuffer wave;
            wave.ParseFromString(data);
            
            float data_buffer[PRE_TRIGGER+POST_TRIGGER];
            
            for(int i = 0; i < wave.wave_sample_size(); i++){
            
                data_buffer[i] = wave.wave_sample(i);
            }
            
            GLSpikeWavePtr gl_wave(new GLSpikeWave(PRE_TRIGGER+POST_TRIGGER, -PRE_TRIGGER/44100., 1.0/44100., data_buffer));
            
            renderer->pushSpikeWave(gl_wave);
            
            recvd = spike_socket->recv(&msg, ZMQ_NOBLOCK);
        }
    }

    void readControlSocket(){
    
        zmq::message_t msg;
        
        // Receive a message 
        bool recvd = ctl_in_socket->recv(&msg, ZMQ_NOBLOCK);
        
        while(recvd){

            string data((const char *)msg.data(), msg.size());
            CtlMessage ctl;
            ctl.ParseFromString(data);
            
            
            switch(ctl.message_type()){
            
                case CtlMessage::THRESHOLD:
                    setTriggerThreshold(ctl.value(), true);
                    break;
                case CtlMessage::AUTOTHRESHOLD_STATE:
                    if(ctl.value() > 0.5){
                        renderer->setAutoThresholdState(AUTO_THRESHOLD_HIGH);
                    } else if(ctl.value() < -0.5){
                        renderer->setAutoThresholdState(AUTO_THRESHOLD_LOW);
                    } else {
                        renderer->setAutoThresholdState(AUTO_THRESHOLD_OFF);
                    }
                    
                    break;
                
                case CtlMessage::AMPLITUDE_MAX:
                    setAmplitudeRangeMax(ctl.value());
                    break;
                case CtlMessage::AMPLITUDE_MIN:
                    setAmplitudeRangeMin(ctl.value());
                    break;
                case CtlMessage::TIME_MAX:
                    setTimeRangeMax(ctl.value());
                    break;
                case CtlMessage::TIME_MIN:
                    setTimeRangeMin(ctl.value());
                    break;
                case CtlMessage::UNITS_PER_VOLT:
                    renderer->setUnitsPerVolt(ctl.value());
                    break;
                case CtlMessage::GAIN:
                    renderer->setAmplifierGain(ctl.value());
                    break;
            }
            
            recvd = ctl_in_socket->recv(&msg, ZMQ_NOBLOCK);
        }
    
    }
  
    // update
    
    void update(){
    
        readSpikeSocket();
        readControlSocket();
    
    }
    
  
  
    // drawing related
    void render(bool flush=true){
        renderer->render(flush);
    }
    
    void setViewport( float x_offset, float y_offset, float width, float height ){
        renderer->setViewDimensions(width, height, x_offset, y_offset);
    }
    
    // UI interaction
    void mouseDown(float x, float y){
        
        SpikeWaveSelectionAction action;
    
        if(renderer->hitTest(x, y, &action)){
            
            adjust_mode = action.action_type;
        
            enterAdjustMode(adjust_mode);
            
            switch(adjust_mode){
                case SP_AMPLITUDE_MAX_SELECT:
                    old_adjust_value = renderer->getAmplitudeRangeMax();
                    break;
                case SP_AMPLITUDE_MIN_SELECT:
                    old_adjust_value = renderer->getAmplitudeRangeMin();
                    break;
                case SP_TIME_MAX_SELECT:
                    old_adjust_value = renderer->getTimeRangeMax();
                    break;
                case SP_TIME_MIN_SELECT:
                    old_adjust_value = renderer->getTimeRangeMin();
                    break;
                case SP_AUTOTHRESH_UP_SELECT:
                    autothresholdUp();
                    break;
                case SP_AUTOTHRESH_DOWN_SELECT:
                    autothresholdDown();
                    break;

                default:
                    break;
            }
        }
            
    }
    
    void scrollWheel(float delta_y, float x, float y){
        
        
        if(!renderer->hitTestEntireView(x,y)){
            return;
        }
        
        if(fabs(delta_y) < 0.5){
            return;
        }
        
        GLfloat data_x, data_y;
        GLfloat current_max = renderer->getAmplitudeRangeMax();
        GLfloat current_min = renderer->getAmplitudeRangeMin();
        
        renderer->convertViewToDataSize(0.0, -delta_y, &data_x, &data_y);
        
        float gain = 1;
        
        setAmplitudeRangeMax(current_max + gain * data_y);
        setAmplitudeRangeMin(current_min - gain * data_y);
         
    }

    void mouseDragged( float x, float y ) {
        
        
        if(!adjust_mode){
            return;
        }
        
        GLfloat data_x, data_y;
        
        renderer->convertGlobalViewToDataCoordinates(x, y, &data_x, &data_y);
        
       
        if(adjust_mode == SP_TRIGGER_THRESHOLD_SELECT){
            
            float new_threshold = data_y;
            
            // TODO: send threshold somewhere meaningful
            
            setTriggerThreshold(new_threshold);
            
        } else if(adjust_mode == SP_AMPLITUDE_MAX_SELECT){
            
            if(data_y <= 0){
                return;
            }
            
            GLfloat new_max, new_min;
            
            new_max = old_adjust_value *  (old_adjust_value / data_y);
        
            new_min = -new_max;
            
            setAmplitudeRangeMax(new_max);
            setAmplitudeRangeMin(new_min);
            
        } else if(adjust_mode == SP_AMPLITUDE_MIN_SELECT){
            
            if(data_y >= 0){
                return;
            }
            
            GLfloat new_min = old_adjust_value * (old_adjust_value / data_y);
            GLfloat new_max = -new_min;
            
            setAmplitudeRangeMax(new_max);
            setAmplitudeRangeMin(new_min);
            
        } else if(adjust_mode == SP_TIME_MAX_SELECT){
            
            if(data_x <= 0){
                return;
            }
            GLfloat new_max = old_adjust_value * (old_adjust_value / data_x);
            GLfloat new_min = -new_max;
            
            setTimeRangeMax(new_max);
            setTimeRangeMin(new_min);
            
        } else if(adjust_mode == SP_TIME_MIN_SELECT){
            
            if(data_x >= 0){
                return;
            }
            
            GLfloat new_min = old_adjust_value * (old_adjust_value / data_x);
            GLfloat new_max = -new_min;
            
            setTimeRangeMax(new_max);
            setTimeRangeMin(new_min);
        }
        
    }


    void mouseUp( float x, float y) {
        
        exitAdjustMode(adjust_mode);
        adjust_mode = -1;
    }

    


    // state management
    void setTriggerThreshold(double value, bool silent = false){
        renderer->setTriggerThreshold(value);
        if(!silent){
            sendCtlEvent(TRIGGER_THRESHOLD, value);
        }
    }


    void setAmplitudeRangeMax(double value, bool silent = false){
        renderer->setAmplitudeRangeMax(value);
        if(!silent){
            sendCtlEvent(AMPLITUDE_MAX, value);
        }
    }

    void setAmplitudeRangeMin(double value, bool silent = false){
        renderer->setAmplitudeRangeMin(value);
        if(!silent){
            sendCtlEvent(AMPLITUDE_MIN, value);
        }
    }


    void setTimeRangeMax(double value, bool silent = false){
        renderer->setTimeRangeMax(value);
        if(!silent){
            sendCtlEvent(TIME_MAX, value);
        }
    }

    void setTimeRangeMin(double value, bool silent = false){
        renderer->setTimeRangeMin(value);
        if(!silent){
            sendCtlEvent(TIME_MIN, value);
        }
    }
    
    void autothresholdUp(bool silent = false) {
        
        int current = renderer->getAutoThresholdState();
        
        if(current == AUTO_THRESHOLD_HIGH){
            renderer->setAutoThresholdState(AUTO_THRESHOLD_OFF);

            if(!silent){
                sendCtlEvent(AUTOTHRESHOLD_STATE, 0.0);
            }
            
            return;
        }
        


        renderer->setAutoThresholdState(AUTO_THRESHOLD_HIGH);
        if(!silent){
            sendCtlEvent(AUTOTHRESHOLD_STATE, 1.0);
        }        
    }


    void autothresholdDown(bool silent = false){
        
        int current = renderer->getAutoThresholdState();
        
        if(current == AUTO_THRESHOLD_LOW){
            
            renderer->setAutoThresholdState(AUTO_THRESHOLD_OFF);
            if(!silent){
                sendCtlEvent(AUTOTHRESHOLD_STATE, 0.0);
            }
            return;
        }
                
        renderer->setAutoThresholdState(AUTO_THRESHOLD_LOW);
        if(!silent){
            sendCtlEvent(AUTOTHRESHOLD_STATE, -1.0);
        }

    }

    void enterAdjustMode( int mode ){  }

    void exitAdjustMode( int mode ){ }

};


typedef boost::shared_ptr< SpikeChannelController >  SpikeChannelControllerPtr;

}