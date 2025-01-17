/**
 *  LegacyMacOSOpenGLContextManager.h
 *
 *  Created by David Cox on Thu Dec 05 2002.
 *  Copyright (c) 2002 MIT. All rights reserved.
 */

#ifndef MACOS_OPENGL_CONTEXT_MANAGER__
#define MACOS_OPENGL_CONTEXT_MANAGER__

#include <IOKit/pwr_mgt/IOPMLib.h>

#include "AppleOpenGLContextManager.hpp"
#include "CFObjectPtr.h"


BEGIN_NAMESPACE_MW


class LegacyMacOSOpenGLContextManager : public AppleOpenGLContextManager {
    
public:
    LegacyMacOSOpenGLContextManager();
    ~LegacyMacOSOpenGLContextManager();
    
    int newFullscreenContext(int screen_number, bool opaque) override;
    int newMirrorContext() override;
    
    void releaseContexts() override;
    
    int getNumDisplays() const override;
    
    OpenGLContextLock setCurrent(int context_id) override;
    void clearCurrent() override;
    
    void prepareContext(int context_id, bool useColorManagement) override;
    int createFramebufferTexture(int context_id, bool useColorManagement, int &target, int &width, int &height) override;
    void flushFramebufferTexture(int context_id) override;
    void drawFramebufferTexture(int src_context_id, int dst_context_id) override;
    
private:
    using ColorSyncProfilePtr = cf::ObjectPtr<ColorSyncProfileRef>;
    using ColorSyncTransformPtr = cf::ObjectPtr<ColorSyncTransformRef>;
    
    bool createColorConversionLUT(int context_id);
    std::vector<float> getColorConversionLUTData(int context_id);
    
    static ColorSyncTransformPtr createColorSyncTransform(const ColorSyncProfilePtr &srcProfile,
                                                          const ColorSyncProfilePtr &dstProfile);
    static void getColorConversionLUTData(const ColorSyncTransformPtr &transform, std::vector<float> &lutData);
    
    std::map<int, GLuint> programs, vertexArrays, colorConversionLUTs, framebufferTextures;
    IOPMAssertionID display_sleep_block;
    
};


END_NAMESPACE_MW


#endif


























