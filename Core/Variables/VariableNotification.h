/*
 *  VariableNotification.h
 *  MonkeyWorksCore
 *
 *  Created by David Cox on 8/3/05.
 * Paul Jankunas on 1/24/06 - Adding virtual destructor.
 *
 *  Copyright 2005 MIT. All rights reserved.
 */

#ifndef _VARIABLE_NOTIFICATION_H_
#define _VARIABLE_NOTIFICATION_H_

#include "GenericData.h"
#include "LinkedList.h"
#include "boost/function.hpp"


namespace mw {
// A base class for asynchronous notification when an Param's value is changed
// Among other things, allows asynchronous IO (outputs)
class VariableNotification : public LinkedListNode<VariableNotification> {

	public:
        VariableNotification();
		
        virtual ~VariableNotification();
        virtual void notify(const Data& data, MonkeyWorksTime time); // called when a new value is set
};

    
    
class VariableCallbackNotification : public VariableNotification {
 
protected:
    
    boost::function<void (const Data& data, MonkeyWorksTime time)> functor;
    
public:
    
    VariableCallbackNotification(boost::function<void (const Data& data, MonkeyWorksTime time)> _functor){
        functor = _functor;
    }
    
    virtual void notify(const Data& data, MonkeyWorksTime time){ // called when a new value is set
        functor(data, time);
    }
    
};
    
}
#endif