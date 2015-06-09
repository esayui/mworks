//
//  PythonPlugin.cpp
//  PythonTools
//
//  Created by Christopher Stawarz on 6/1/15.
//  Copyright (c) 2015 MWorks Project. All rights reserved.
//

#include "RunPythonScriptAction.h"


BEGIN_NAMESPACE_MW


class PythonPlugin : public Plugin {
    void registerComponents(boost::shared_ptr<ComponentRegistry> registry) override {
        registry->registerFactory<StandardComponentFactory, RunPythonScriptAction>();
    }
};


extern "C" Plugin* getPlugin() {
    return new PythonPlugin();
}


END_NAMESPACE_MW
