//
//  MaskStimulus.cpp
//  LayerStimulus
//
//  Created by Christopher Stawarz on 6/19/18.
//  Copyright © 2018 The MWorks Project. All rights reserved.
//

#include "MaskStimulus.hpp"


BEGIN_NAMESPACE_MW


const std::string MaskStimulus::MASK("mask");
const std::string MaskStimulus::INVERTED("inverted");
const std::string MaskStimulus::STD_DEV("std_dev");
const std::string MaskStimulus::MEAN("mean");
const std::string MaskStimulus::NORMALIZED("normalized");
const std::string MaskStimulus::EDGE_WIDTH("edge_width");


void MaskStimulus::describeComponent(ComponentInfo &info) {
    TransformStimulus::describeComponent(info);
    
    info.setSignature("stimulus/mask");
    
    info.addParameter(MASK);
    info.addParameter(INVERTED, "NO");
    info.addParameter(STD_DEV, "1.0");
    info.addParameter(MEAN, "0.0");
    info.addParameter(NORMALIZED, "NO");
    info.addParameter(EDGE_WIDTH, "0.125");
}


MaskStimulus::MaskStimulus(const ParameterValueMap &parameters) :
    TransformStimulus(parameters),
    maskTypeName(registerVariable(variableOrText(parameters[MASK]))),
    inverted(registerVariable(parameters[INVERTED])),
    std_dev(registerVariable(parameters[STD_DEV])),
    mean(registerVariable(parameters[MEAN])),
    normalized(registerVariable(parameters[NORMALIZED])),
    edgeWidth(registerVariable(parameters[EDGE_WIDTH]))
{ }


void MaskStimulus::draw(boost::shared_ptr<StimulusDisplay> display) {
    current_mask_type_name = maskTypeName->getValue().getString();
    current_mask_type = maskTypeFromName(current_mask_type_name);
    current_inverted = inverted->getValue().getBool();
    current_std_dev = std_dev->getValue().getFloat();
    current_mean = mean->getValue().getFloat();
    current_normalized = normalized->getValue().getBool();
    current_edge_width = edgeWidth->getValue().getFloat();
    
    TransformStimulus::draw(display);
    
    last_mask_type_name = current_mask_type_name;
    last_mask_type = current_mask_type;
    last_inverted = current_inverted;
    last_std_dev = current_std_dev;
    last_mean = current_mean;
    last_normalized = current_normalized;
    last_edge_width = current_edge_width;
}


Datum MaskStimulus::getCurrentAnnounceDrawData() {
    auto announceData = TransformStimulus::getCurrentAnnounceDrawData();
    
    announceData.addElement(STIM_TYPE, "mask");
    announceData.addElement(MASK, last_mask_type_name);
    announceData.addElement(INVERTED, last_inverted);
    
    switch (last_mask_type) {
        case MaskType::gaussian:
            announceData.addElement(STD_DEV, last_std_dev);
            announceData.addElement(MEAN, last_mean);
            announceData.addElement(NORMALIZED, last_normalized);
            break;
            
        case MaskType::raisedCosine:
            announceData.addElement(EDGE_WIDTH, last_edge_width);
            break;
            
        default:
            break;
    }
    
    return announceData;
}


auto MaskStimulus::maskTypeFromName(const std::string &name) -> MaskType {
    if (name == "rectangle") {
        return MaskType::rectangle;
    } else if (name == "ellipse") {
        return MaskType::ellipse;
    } else if (name == "gaussian") {
        return MaskType::gaussian;
    } else if (name == "raised_cosine") {
        return MaskType::raisedCosine;
    }
    merror(M_DISPLAY_MESSAGE_DOMAIN, "Invalid mask type: %s", name.c_str());
    return MaskType::rectangle;
};


gl::Shader MaskStimulus::getVertexShader() const {
    static const std::string source
    (R"(
     uniform mat4 mvpMatrix;
     in vec4 vertexPosition;
     
     out vec2 maskVaryingCoords;
     
     void main() {
         gl_Position = mvpMatrix * vertexPosition;
         maskVaryingCoords = vertexPosition.xy;
     }
     )");
    
    return gl::createShader(GL_VERTEX_SHADER, source);
}


gl::Shader MaskStimulus::getFragmentShader() const {
    static const std::string source
    (R"(
     const int rectangleMask = 1;
     const int ellipseMask = 2;
     const int gaussianMask = 3;
     const int raisedCosineMask = 4;
     
     const vec2 maskCenter = vec2(0.5, 0.5);
     const float maskRadius = 0.5;
     const float pi = 3.14159265358979323846264338327950288;
     
     uniform int maskType;
     uniform bool inverted;
     uniform float stdDev;
     uniform float mean;
     uniform bool normalized;
     uniform float edgeWidth;
     
     in vec2 maskVaryingCoords;
     
     out vec4 fragColor;
     
     void main() {
         float maskValue;
         float dist;
         float delta;
         float edgeMax;
         float edgeMin;
         
         switch (maskType) {
             case rectangleMask:
                 maskValue = 1.0;
                 break;
                 
             case ellipseMask:
                 //
                 // For an explanation of the edge-smoothing technique used here, see either of the following:
                 // https://rubendv.be/blog/opengl/drawing-antialiased-circles-in-opengl/
                 // http://www.numb3r23.net/2015/08/17/using-fwidth-for-distance-based-anti-aliasing/
                 //
                 dist = distance(maskVaryingCoords, maskCenter);
                 delta = fwidth(dist);
                 maskValue = 1.0 - smoothstep(maskRadius - delta, maskRadius, dist);
                 break;
                 
             case gaussianMask:
                 dist = distance(maskVaryingCoords, maskCenter) / maskRadius;
                 maskValue = exp(-1.0 * (dist - mean) * (dist - mean) / (2.0 * stdDev * stdDev));
                 if (normalized) {
                     maskValue /= stdDev * sqrt(2.0 * pi);
                 }
                 break;
                 
             case raisedCosineMask:
                 //
                 // The equation used here derives from the definition of the raised-cosine filter at
                 // https://en.wikipedia.org/wiki/Raised-cosine_filter
                 //
                 
                 edgeMax = maskRadius;
                 edgeMin = edgeMax - edgeWidth;
                 dist = distance(maskVaryingCoords, maskCenter);
                 
                 maskValue = 0.5 * (1.0 + cos(pi / edgeWidth * (dist - edgeMin)));
                 maskValue *= float(dist > edgeMin && dist <= edgeMax);
                 maskValue += float(dist <= edgeMin);
                 
                 break;
         }
         
         if (inverted) {
             maskValue = 1.0 - maskValue;
         }
         
         fragColor = vec4(1.0, 1.0, 1.0, maskValue);
     }
     )");
    
    return gl::createShader(GL_FRAGMENT_SHADER, source);
}


void MaskStimulus::setBlendEquation() {
    // Blend by multiplying the destination color by the mask color
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
}


void MaskStimulus::prepare(const boost::shared_ptr<StimulusDisplay> &display) {
    TransformStimulus::prepare(display);
    
    maskTypeUniformLocation = glGetUniformLocation(program, "maskType");
    invertedUniformLocation = glGetUniformLocation(program, "inverted");
    stdDevUniformLocation = glGetUniformLocation(program, "stdDev");
    meanUniformLocation = glGetUniformLocation(program, "mean");
    normalizedUniformLocation = glGetUniformLocation(program, "normalized");
    edgeWidthUniformLocation = glGetUniformLocation(program, "edgeWidth");
}


void MaskStimulus::preDraw(const boost::shared_ptr<StimulusDisplay> &display) {
    TransformStimulus::preDraw(display);
    
    glUniform1i(maskTypeUniformLocation, int(current_mask_type));
    glUniform1i(invertedUniformLocation, current_inverted);
    glUniform1f(stdDevUniformLocation, current_std_dev);
    glUniform1f(meanUniformLocation, current_mean);
    glUniform1f(normalizedUniformLocation, current_normalized);
    glUniform1f(edgeWidthUniformLocation, current_edge_width);
}


END_NAMESPACE_MW




















