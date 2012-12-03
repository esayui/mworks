/*
 *  dfindex.h
 *  DataFileIndexer
 *
 *  Created by bkennedy on 2/20/08.
 *  Copyright 2008 MIT. All rights reserved.
 *
 */

#ifndef dfindex_
#define dfindex_

#include "DataFileIndexer.h"
#include "boost/filesystem/path.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/archive/text_iarchive.hpp"
#include "EventWrapper.h"
#include <MWorksCore/Utilities.h>
using namespace mw;

class dfindex
	{
	private:
		DataFileIndexer dfi;
		const boost::filesystem::path mwk_data_file;

		boost::filesystem::path indexFile() const;
		void save() const;
		void load();
	public:
		dfindex(const boost::filesystem::path &data_file);

        std::string getFilePath() const {
            return mwk_data_file.string();
        }
        
        unsigned int getNEvents() const {
            return dfi.getNEvents();
        }
        
        MWTime getMinimumTime() const {
            return dfi.getMinimumTime();
        }
        
        MWTime getMaximumTime() const {
            return dfi.getMaximumTime();
        }
        
		void getEvents(std::vector<EventWrapper> &events,
                       const std::vector<unsigned int> &event_codes,
                       MWTime lower_bound = MIN_MONKEY_WORKS_TIME(), 
                       MWTime upper_bound = MAX_MONKEY_WORKS_TIME()) const
        {
            dfi.getEvents(events, event_codes, lower_bound, upper_bound);
        }
        
        DataFileIndexer::EventsIterator getEventsIterator(const std::vector<unsigned int> &event_codes,
                                                          MWTime lower_bound = MIN_MONKEY_WORKS_TIME(),
                                                          MWTime upper_bound = MAX_MONKEY_WORKS_TIME()) const
        {
            return dfi.getEventsIterator(event_codes, lower_bound, upper_bound);
        }
	};


#endif //dfindex_


























