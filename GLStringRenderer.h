/*
 *  GLStringRenderer.h
 *  SpikeVisualization
 *
 *  Created by David Cox on 6/14/10.
 *  Copyright 2010 Harvard University. All rights reserved.
 *
 */
#ifndef GL_STRING_RENDERER_H_
#define GL_STRING_RENDERER_H_

#include <OpenGL/gl.h>
#include <string>
#include <boost/shared_ptr.hpp>

using std::string;
using boost::shared_ptr;


class GLStringRenderer {

    public:

        virtual GLuint stringToTexture(std::string str, float font_size,float *width, float *height) = 0;
};


class GLString {

  protected:
  
    shared_ptr<GLStringRenderer> renderer;
    string current_value;
    GLuint texture;
    float font_size;
    float width, height;

  public:
    
    GLString(shared_ptr<GLStringRenderer> _renderer, float _font_size) :
                        current_value(""),
                        texture(0),
                        font_size(_font_size),
                        renderer(_renderer){  }
    
    
    void operator=(string str){
    
        if(current_value.compare(str) != 0){
            current_value = str;
            if(texture != 0){
                glDeleteTextures(1, &texture);
            }
            texture = renderer->stringToTexture(current_value, font_size, &width, &height);
        }
    }
  
    GLuint toTexture(float *_width, float *_height){
        
        *_width = width;
        *_height = height;
        return texture;
    }

};



#endif
