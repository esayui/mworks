/*
 *  CoxlabCorePlugins.cpp
 *  CoxlabCorePlugins
 *
 *  Created by David Cox on 4/29/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "NE500.h"


BEGIN_NAMESPACE_MW


class NE500Plugin : public Plugin {
    void registerComponents(boost::shared_ptr<mw::ComponentRegistry> registry) override {
        registry->registerFactory(std::string("iodevice/ne500"),
                                  (ComponentFactory *)(new NE500DeviceFactory()));
        registry->registerFactory(std::string("iochannel/ne500"),
                                  (ComponentFactory *)(new NE500DeviceChannelFactory()));
    }
};


extern "C" Plugin * getPlugin() {
    return new NE500Plugin();
}


END_NAMESPACE_MW
