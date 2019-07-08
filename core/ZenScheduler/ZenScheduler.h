/*
 *  ZenScheduler.h
 *  MWorksCore
 *
 *  Created by David Cox on 6/16/05.
 *  Copyright 2005 MIT. All rights reserved.
 *
 */

#ifndef ZEN_SCHEDULER_H
#define ZEN_SCHEDULER_H

#include <MWorksCore/Scheduler.h>
#include <MWorksCore/Plugin.h>


BEGIN_NAMESPACE_MW


#ifndef LOW_PRIORITY_MODE
#define ZEN_SCHEDULER_PLUGIN_NAME	"ZenScheduler"
#else
#define ZEN_SCHEDULER_PLUGIN_NAME	"LowPriorityScheduler"
#endif


///////////////////////////////////////////////////////////////////////////////////////
// Plugin entry point
//
// The extern "C" bit ensures that the name of this function is not mangled (C++
// requires name-mangling in order to achieve function overriding.  Our plugin host
// needs to know what name to look for, and for this plugin architecture, that name
// is 'getPlugin'
///////////////////////////////////////////////////////////////////////////////////////

extern "C"{
    Plugin *getPlugin();
}


#ifdef LOW_PRIORITY_MODE
namespace low_priority_scheduler{
#else
	namespace zen_scheduler {
#endif
		
		class ZenScheduler; // patience...
		
		
		class ZenScheduleTask : public ScheduleTask {
			
			//**********************
			// Scheduling parameters
			//**********************
			
			// who owns this task?
			const shared_ptr<ZenScheduler> scheduler;
			
			
			
		public:
			
			ZenScheduleTask(const std::string &description,
							 const shared_ptr<ZenScheduler> &_scheduler,
							 boost::function<void *()> _functor,
							 MWTime _start_time,
							 MWTime _initial_delay, 
							 MWTime _repeat_interval, 
							 int _ntimes, 
							 TaskPriority priority,
							 MissedExecutionBehavior _behave,
							 MWTime _warn_slop, 
							 MWTime _fail_slop);
			
			
            const std::string& getDescription() const {
                return description;
            }
            
			TaskPriority getPriority() const {
				return priority;
			}
			
			const shared_ptr<ZenScheduler>& getScheduler() const {
				return scheduler;
			}
            
            using ScheduleTask::execute;
            
		};
		
		
		class ZenScheduler : public mw::Component, public Scheduler, public ComponentFactory  {
			
		private:
			
            boost::shared_ptr <Clock> the_clock;
			
		public:
			
            explicit ZenScheduler(const shared_ptr<Clock> &a_clock) :
                the_clock(a_clock)
            { }
			
			// factory method
			virtual shared_ptr<mw::Component> createObject(
														std::map<std::string, std::string> parameters,
														ComponentRegistry *reg){
				shared_ptr <Clock> a_clock = Clock::instance();
				return shared_ptr<mw::Component>(new ZenScheduler(a_clock));
			}
			
			
            shared_ptr<ScheduleTask> scheduleUS(const std::string &description,
                                                MWTime initial_delay,
                                                MWTime repeat_interval,
                                                int ntimes,
                                                boost::function<void *()> _functor,
                                                TaskPriority _priority,
                                                MWTime _warn_slop_us,
                                                MWTime _fail_slop_us,
                                                MissedExecutionBehavior behav) override;
			
			
            const boost::shared_ptr<Clock>& getClock() const {
                return the_clock;
            }
			
		};
		
		
		class ZenSchedulerPlugin : public Plugin {
			
			virtual void registerComponents(shared_ptr<ComponentRegistry> registry) {
				//cerr << "Registering zen scheduler as: " << ZEN_SCHEDULER_PLUGIN_NAME << endl;
				registry->registerFactory(ZEN_SCHEDULER_PLUGIN_NAME,
                                          boost::make_shared<ZenScheduler>(shared_ptr<Clock>()));
			}
		};
		
	} // end namespace


END_NAMESPACE_MW
	

#endif
