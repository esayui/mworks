/*
 *  MSSWGamepadPlugin.cpp
 *  MSSWGamepadPlugin
 *
 *  Created by bkennedy on 8/14/08.
 *  Copyright 2008 MIT. All rights reserved.
 *
 */

#include "MSSWGamepadPlugin.h"
#include "MSSWGamepadFactory.h"

MW_SYMBOL_PUBLIC
Plugin *getPlugin(){
    return new mMSSWGamepadPlugin();
}


void mMSSWGamepadPlugin::registerComponents(shared_ptr<mw::ComponentRegistry> registry) {
    registry->registerFactory("iodevice/sidewinder_gamepad", boost::make_shared<mMSSWGamepadFactory>());
}
