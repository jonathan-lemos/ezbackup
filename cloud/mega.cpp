#include "../error.h"
#include <megaapi.h>
#include <iostream>

static int MEGAmkdir(std::string str, mega::MegaApi* mega_api){
	std::string path;
	std::string spath;
	size_t index;
	mega::MegaNode* node;
	mega::SynchronousRequestListener listener;

	path = "/";
	path += str;

	node = mega_api->getNodeByPath(path.c_str());
	if (node){
		puts_debug("MEGA: Path already exists");
		return 1;
	}

	spath = path;
	index = spath.find_last_of('/');
	/* if we didn't find any matches */
	if (index == std::string::npos){
		log_error("MEGA: Invalid path");
		return -1;
	}
	spath.resize(index + 1);

	node = mega_api->getNodeByPath(spath.c_str());
	if (!node || node->isFile()){
		log_error("MEGA: Parent folder not found");
		delete node;

		return -1;
	}

	mega_api->createFolder(path.c_str() + index + 1, node, &listener);
	listener.wait();
	delete node;

	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Error creating folder");
		return -1;
	}

	log_debug(__FILE__, __LINE__, "MEGA: Created folder %s successfully", str.c_str());
	return 0;
}
