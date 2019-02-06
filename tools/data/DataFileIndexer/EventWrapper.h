/*
 *  EventWrapper.h
 *  DataFileIndexer
 *
 *  Created by bkennedy on 2/19/08.
 *  Copyright 2008 MIT. All rights reserved.
 *
 */

#ifndef SCARAB_DATUM_WRAPPER_H
#define SCARAB_DATUM_WRAPPER_H

#include <Scarab/scarab.h>

#include <dfindex/DataFileUtilities.h>


BEGIN_NAMESPACE_MW


class EventWrapper {
    
private:
    ScarabDatum *datum;
    
public:
    ~EventWrapper() {
        scarab_free_datum(datum);
    }
    
    explicit EventWrapper(ScarabDatum *new_datum = NULL) {
        datum = scarab_copy_datum(new_datum);
    }
    
    EventWrapper(const EventWrapper &other) {
        datum = scarab_copy_datum(other.datum);
    }
    
    EventWrapper& operator=(const EventWrapper &other) {
        scarab_free_datum(datum);
        datum = scarab_copy_datum(other.datum);
        return (*this);
    }
    
    ScarabDatum* getDatum() const {
        return datum;
    }
    
    bool empty() const {
        return (datum == NULL);
    }
    
    int getEventCode() const {
        if (empty()) {
            return -1;
        }
        return data_file_utilities::getScarabEventCode(datum);
    }
    
    MWTime getTime() const {
        if (empty()) {
            return 0LL;
        }
        return data_file_utilities::getScarabEventTime(datum);
    }
    
    ScarabDatum *getPayload() const {
        if (empty()) {
            return NULL;
        }
        return data_file_utilities::getScarabEventPayload(datum);
    }
    
    // This is needed for Python bindings
    bool operator==(EventWrapper const& event) const { return datum == event.datum; }
    
};


END_NAMESPACE_MW


#endif //SCARAB_DATUM_WRAPPER_H
