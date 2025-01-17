/**
* Server.cpp
 *
 * History:
 * paul on 1/26/06 - Created.
 *
 * Copyright 2006 MIT. All rights reserved.
 */

#include "Server.h"

#include "Experiment.h"
#include "SystemEventFactory.h"
#include "StandardSystemEventHandler.h"
#include "ExperimentPackager.h"
#include "DataFileManager.h"
#include "VariableSave.h"
#include "VariableLoad.h"
#include "ZeroMQUtilities.hpp"


BEGIN_NAMESPACE_MW


SINGLETON_INSTANCE_STATIC_DECLARATION(Server)


Server::Server() :
    RegistryAwareEventStreamInterface(M_SERVER_MESSAGE_DOMAIN),
    incoming_event_buffer(new EventBuffer()),
    listenPort(19989)
{
    registry = global_variable_registry;
    
    // dont know where else this would be handled?
    startDataFileManager();
}


Server::~Server() {
    stopServer();
	if (incomingListener) {
		incomingListener->killListener();
	}
	if (outgoingListener) {
		outgoingListener->killListener();
	}
}


bool Server::startServer() {
    incomingListener = boost::make_shared<EventListener>(incoming_event_buffer,
                                                         boost::make_shared<StandardSystemEventHandler>());
    incomingListener->startListener();
    
    outgoingListener = boost::make_shared<EventListener>(global_outgoing_event_buffer, shared_from_this());
    outgoingListener->startListener();
    
    std::string address;
    if (hostname.empty()) {
        address = "*";  // Bind to all available interfaces
    } else if (!zeromq::resolveHostname(hostname, address)) {
        return false;
    }
    
    server.reset(new ZeroMQServer(incoming_event_buffer,
                                  global_outgoing_event_buffer,
                                  address,
                                  listenPort + 1,
                                  listenPort));
    
    if (!server->start()) {
        return false;
    }
    
    return true;
}


void Server::stopServer() {
    if (server) {
        server->stop();
        // Destroy server to ensure that ZeroMQ sockets are closed
        server.reset();
    }
}


void Server::putEvent(shared_ptr<Event> event){
    incoming_event_buffer->putEvent(event);
}

void Server::saveVariables(const boost::filesystem::path &file) {
	VariableSave::saveExperimentwideVariables(file);
}

void Server::loadVariables(const boost::filesystem::path &file) {
	VariableLoad::loadExperimentwideVariables(file);
}

bool Server::openExperiment(const std::string &expPath) {
	namespace bf = boost::filesystem;
	
    ExperimentPackager packer;
	
    Datum experiment(packer.packageExperiment(bf::path(expPath)));
	if(experiment.isUndefined()) {
		merror(M_SERVER_MESSAGE_DOMAIN, 
			   "Failed to create a valid packaged experiment.");
		return false; 		
	}
	shared_ptr<Event> new_experiment(new Event(RESERVED_SYSTEM_EVENT_CODE, 
												 experiment));
	putEvent(new_experiment);
	
	return true;
}

bool Server::closeExperiment(){
	if(!GlobalCurrentExperiment){
		return false;
	}
	
	string expName(GlobalCurrentExperiment->getExperimentName());

	putEvent(SystemEventFactory::closeExperimentControl(expName));
	return true;
}

void Server::startDataFileManager() {
    auto dataFileManager = DataFileManager::instance(false);
    if (!dataFileManager) {
        DataFileManager::registerInstance(boost::make_shared<DataFileManager>());
    }
}

bool Server::isExperimentRunning() {
	shared_ptr <StateSystem> state_system = StateSystem::instance();
	return state_system->isRunning();
}

void Server::startExperiment() {
	putEvent(SystemEventFactory::startExperimentControl());        
}

void Server::stopExperiment() {
	putEvent(SystemEventFactory::stopExperimentControl());        
}

MWTime Server::getReferenceTime() {
	shared_ptr <Clock> clock = Clock::instance();
	return clock->getSystemReferenceTime();
}


void Server::handleEvent(shared_ptr<Event> evt) {
	handleCallbacks(evt);
}


void Server::setListenPort(int port) {
    listenPort = port;
}

void Server::setHostname(const std::string &name) {
    hostname = name;
}

bool Server::isStarted() {
    if (server) {
        return server->isStarted();
    }
    return false;
}

bool Server::isExperimentLoaded() {
	return (GlobalCurrentExperiment.use_count() > 0) ;
}


END_NAMESPACE_MW




























