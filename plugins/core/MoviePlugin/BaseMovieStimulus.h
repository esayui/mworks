//
//  BaseMovieStimulus.h
//  MovieStimulusPlugin
//
//  Created by Christopher Stawarz on 8/6/12.
//
//

#ifndef __MovieStimulusPlugin__BaseMovieStimulus__
#define __MovieStimulusPlugin__BaseMovieStimulus__

#include "BaseFrameListStimulus.h"


BEGIN_NAMESPACE_MW


class BaseMovieStimulus : public BaseFrameListStimulus {
    
public:
    static const std::string FRAMES_PER_SECOND;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit BaseMovieStimulus(const ParameterValueMap &parameters);
    
    bool needDraw(shared_ptr<StimulusDisplay> display) MW_OVERRIDE;
    Datum getCurrentAnnounceDrawData() MW_OVERRIDE;
    
private:
    void startPlaying() MW_OVERRIDE;
    
    int getNominalFrameNumber() MW_OVERRIDE;
    
    shared_ptr<Variable> framesPerSecond;
    
    double framesPerUS;
    
};


END_NAMESPACE_MW


#endif /* !defined(__MovieStimulusPlugin__BaseMovieStimulus__) */
