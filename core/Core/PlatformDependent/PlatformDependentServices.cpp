/**
 * PlatformDependentServices.cpp
 *
 * History:
 * David Cox on 10/20/04 - Created.
 * Paul Jankunas on 03/23/05 - Changed scriptingPath function to return
 *                  a standardized Mac OS X path.
 * 
 * Copyright 2004 MIT. All rights reserved.
 */

#include "PlatformDependentServices.h"

#include <boost/filesystem/operations.hpp>

#ifdef	__APPLE__
#import <Foundation/Foundation.h>
#endif


BEGIN_NAMESPACE_MW

	
#ifdef __APPLE__
    const char * DATA_FILE_PATH = "MWorks/Data";
	const char * PLUGIN_PATH = "/Library/Application Support/MWorks/Plugins/Core";
	const char * SCRIPTING_PATH = "/Library/Application Support/MWorks/Scripting";
	const char * CONFIG_PATH = "MWorks/Configuration";
	const char * EXPERIMENT_INSTALL_PATH = "MWorks/Experiment Cache";
#else
#   error Unsupported platform
#endif
	
	const char * DATA_FILE_EXT = ".mwk2";
	const char * EXPERIMENT_TEMPORARY_DIR_NAME = "tmp";
	const char * VARIABLE_FILE_EXT = "_var.xml";
	const char * EXPERIMENT_STORAGE_DIR_NAME = "Experiment Storage";
	const char * SAVED_VARIABLES_DIR_NAME = "Saved Variables";
	const char * SETUP_VARIABLES_FILENAME = "setup_variables.xml";
	
	boost::filesystem::path pluginPath() {
#if TARGET_OS_IPHONE
        return (boost::filesystem::path(NSBundle.mainBundle.bundlePath.UTF8String) /
                boost::filesystem::path("Frameworks"));
#else
		return boost::filesystem::path(PLUGIN_PATH);
#endif
	}
	
	boost::filesystem::path scriptingPath() {
		return boost::filesystem::path(SCRIPTING_PATH);
	}
	
	boost::filesystem::path dataFilePath() {
		namespace bf = boost::filesystem;
        bf::path data_file_path;
        
        @autoreleasepool {
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
            if (paths && [paths count]) {
                data_file_path = (bf::path([[paths objectAtIndex:0] UTF8String]) /
                                  bf::path(DATA_FILE_PATH));
            }
        }
        
		return data_file_path;
	}
	
	boost::filesystem::path userPath() {
		namespace bf = boost::filesystem;
        bf::path user_path;
        
        @autoreleasepool {
            NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
            if (paths && [paths count]) {
                user_path = bf::path([[paths objectAtIndex:0] UTF8String]) / bf::path(CONFIG_PATH);
            }
        }

		return user_path;
	}
	
	boost::filesystem::path localPath() {
		namespace bf = boost::filesystem;
		return bf::path("/Library/Application Support") / bf::path(CONFIG_PATH);
	}
	
    boost::filesystem::path userSetupVariablesFile() {
        return prependUserPath(SETUP_VARIABLES_FILENAME);
    }

    boost::filesystem::path setupVariablesFile() {
        namespace bf = boost::filesystem;
        bf::path setupPath(userSetupVariablesFile());
        if (setupPath.empty() || !bf::is_regular_file(setupPath)) {
            setupPath = prependLocalPath(SETUP_VARIABLES_FILENAME);
        }
        return setupPath;
    }
	
	boost::filesystem::path resourcePath() {
		std::string resource_path;
		
        @autoreleasepool {
            for(NSBundle *framework in [NSBundle allFrameworks]) {
                if([[[[framework bundlePath] lastPathComponent] stringByDeletingPathExtension] isEqualToString:@"MWorksCore"]) {
                    resource_path = [[framework resourcePath] cStringUsingEncoding:NSASCIIStringEncoding];
                }
            }
        }
        
		return boost::filesystem::path(resource_path);
	}
	
	boost::filesystem::path experimentInstallPath() {
		namespace bf = boost::filesystem;
        
        @autoreleasepool {
            return (bf::path([NSTemporaryDirectory() UTF8String]) / bf::path(EXPERIMENT_INSTALL_PATH));
        }
	}
	
	boost::filesystem::path experimentStorageDirectoryName() {
		return boost::filesystem::path(EXPERIMENT_TEMPORARY_DIR_NAME);
	}
	
	boost::filesystem::path prependScriptingPath(const std::string scriptFile){
		return scriptingPath() / boost::filesystem::path(scriptFile);
	}
	
	boost::filesystem::path prependUserPath(const std::string file){
		return userPath() / boost::filesystem::path(file);
	}
	
	boost::filesystem::path prependLocalPath(const std::string file){
		return localPath() / boost::filesystem::path(file);
	}
	
	boost::filesystem::path prependResourcePath(const std::string file){
		return resourcePath() / boost::filesystem::path(file);
	}
	
    boost::filesystem::path prependDataFilePath(const std::string &filename) {
        namespace bf = boost::filesystem;
        auto path = bf::path(filename);
        // Prepend the standard data file path only if filename is not itself an absolute
        // path.  This allows users to save data files anywhere they want, so long as they
        // include an absolute path in the filename.
        if (!path.is_absolute()) {
            path = dataFilePath() / path;
        }
        return path;
    }
	
	std::string appendDataFileExtension(const std::string filename_) {
		return appendFileExtension(filename_, DATA_FILE_EXT);
	}
	
	std::string appendFileExtension(const std::string &filename_, const std::string &ext_) {
		std::string filename(filename_);
		std::string ext(ext_);
		
		// check to see if there already is an extension
		if(filename.length() <= ext.length() || filename.substr(filename.length()-ext.length()) != ext) {
			filename.append(ext);
		}	
		
		return filename;		
	}
	
	std::string fileExtension(const std::string &file) {
		return file.substr(file.rfind(".")+1, file.length()-file.rfind(".")-1);
	}
	
	std::string removeFileExtension(const std::string file) {
		return file.substr(0,file.rfind("."));
	}
	
	boost::filesystem::path getLocalExperimentStorageDir(const std::string expName) {
		namespace bf = boost::filesystem;
		
		return getLocalExperimentPath(expName) / experimentStorageDirectoryName();
		
	}
	
	boost::filesystem::path getLocalExperimentPath(const std::string expName) {
		namespace bf = boost::filesystem;
		
		return experimentInstallPath() / bf::path(expName);
	}
    
    boost::filesystem::path getExperimentSavedVariablesPath(const std::string expName) {
		namespace bf = boost::filesystem;
        return (dataFilePath().parent_path() /
                bf::path(EXPERIMENT_STORAGE_DIR_NAME) /
                bf::path(expName) /
                bf::path(SAVED_VARIABLES_DIR_NAME));
    }
	
	std::string appendVarFileExt(const std::string expName) {
		return expName + VARIABLE_FILE_EXT;
	}


std::string getVersionString() {
    @autoreleasepool {
        std::string versionString("<UNKNOWN>");
        
        NSBundle *bundle = [NSBundle bundleWithIdentifier:@"" PRODUCT_BUNDLE_IDENTIFIER];
        if (bundle) {
            NSString *version = [bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
            NSString *buildVersion = [bundle objectForInfoDictionaryKey:@"CFBundleVersion"];
            if (version && buildVersion) {
                versionString = [NSString stringWithFormat:@"%@ (%@)", version, buildVersion].UTF8String;
            }
        }
        
        return versionString;
    }
}


END_NAMESPACE_MW
