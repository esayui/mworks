/*!
 * @header ExperimentPackager.h
 *
 * @discussion Controls the packaging and unpackaging of experiment files and
 * all the media files that it references.
 *
 * History:<BR/>
 * paul on 1/25/06 - Created.<BR/>
 * 
 * @copyright MIT
 * @updated 1/25/06
 * @version 1.0.0
 */

#ifndef _EXPERIMENT_PACKAGER_H__
#define _EXPERIMENT_PACKAGER_H__


#include "GenericData.h"
#include <string>
#include "XMLParser.h"
#include "boost/filesystem/path.hpp"


#define M_PACKAGER_FILENAME_STRING "Filename"
#define M_PACKAGER_CONTENTS_STRING "Contents"
#define M_PACKAGER_EXPERIMENT_STRING "Experiment"
#define M_PACKAGER_MEDIA_BUFFERS_STRING "Media Buffers"

#define M_PACKAGER_FULL_NAME "full_path"
#define M_PACKAGER_RELATIVE_NAME "relative_path"

#define M_EXPERIMENT_PACKAGE_NUMBER_ELEMENTS_PER_UNIT 2
#define M_EXPERIMENT_PACKAGE_NUMBER_ELEMENTS 2


BEGIN_NAMESPACE_MW


class ExperimentPackager {
public:
 Datum packageExperiment(const boost::filesystem::path 
							fileWithFullPath);
	
private:
 Datum packageSingleFile(const boost::filesystem::path filepath,
                         const std::string filename);
 Datum packageSingleFile(const Datum &contents,
                         const std::string &filename);
};

class IncludedFilesParser : public XMLParser {

	private:
	
        Datum manifest;
    
        void addDirectory(const std::string &directoryPath, bool recursive);

		// instead of building experiment, just look for path arguments and save
		// the results
        void _processCreateDirective(xmlNode *node) override;
        void _processAnonymousCreateDirective(xmlNode *node) override;
		
		// don't do anything for these
        void _processConnectDirective(xmlNode *node) override { }
        void _processInstanceDirective(xmlNode *node) override { }
        void _processFinalizeDirective(xmlNode *node) override { }
		
	public:
	
        explicit IncludedFilesParser(const std::string &_path);
	
        void parse(bool announce_progress) override;
		const Datum& getIncludedFilesManifest() const { return manifest; }
		
};


END_NAMESPACE_MW


#endif
