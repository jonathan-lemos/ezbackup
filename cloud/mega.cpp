/* mega.cpp -- MEGA integration
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "mega.h"
extern "C"{
#include "keys.h"
#include "../log.h"
#include "../progressbar.h"
}
#include "mega_sdk/include/megaapi.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cerrno>
#include <condition_variable>
#include <mutex>
#include <sys/stat.h>

class ProgressBarTransferListener : public mega::MegaTransferListener{
public:
	ProgressBarTransferListener(){
		notified = false;
		error = NULL;
		transfer = NULL;
		p = NULL;
	}

	~ProgressBarTransferListener(){
		delete error;
		delete transfer;
	}

	void setMsg(const char* msg){
		this->msg = msg;
	}

	const char* getMsg(){
		return msg;
	}

	void onTransferStart(mega::MegaApi* mega_api, mega::MegaTransfer* transfer){
		uint64_t max;
		struct stat st;

		(void)mega_api;

		switch(transfer->getType()){
		case mega::MegaTransfer::TYPE_UPLOAD:
			if (stat(transfer->getFileName(), &st) != 0){
				log_estat(transfer->getFileName());
				break;
			}
			max = st.st_size;
			p = start_progress(msg, max);
			break;
		case mega::MegaTransfer::TYPE_DOWNLOAD:
			max = transfer->getTotalBytes();
			p = start_progress(msg, max);
			break;
		default:
			log_warning("MEGA: Could not start progress due to unknown transfer type.");
		}
	}

	void onTransferUpdate(mega::MegaApi* mega_api, mega::MegaTransfer* transfer){
		(void)mega_api;

		set_progress(p, transfer->getTransferredBytes());
	}

	void onTransferTemporaryError(mega::MegaApi* mega_api, mega::MegaTransfer* transfer, mega::MegaError* error){
		(void)mega_api;
		(void)transfer;
		std::cerr << "MEGA Transfer Error: " << error->toString() << std::endl;
	}

	void onTransferFinish(mega::MegaApi* mega_api, mega::MegaTransfer* transfer, mega::MegaError* error){
		(void)mega_api;

		this->error = error->copy();
		this->transfer = transfer->copy();

		finish_progress(p);
		p = NULL;

		{
			std::unique_lock<std::mutex> lock(m);
			notified = true;
		}
		cv.notify_all();
	}

	void wait(){
		std::unique_lock<std::mutex> lock(m);
		cv.wait(lock, [this]{return notified;});
	}

	void reset(){
		delete transfer;
		delete error;
		if (p){
			finish_progress(p);
		}
		transfer = NULL;
		error = NULL;
		notified = false;
		p = NULL;
	}

	mega::MegaTransfer* getTransfer(){
		return transfer;
	}

	mega::MegaError* getError(){
		return error;
	}

private:
	bool notified;
	mega::MegaError* error;
	mega::MegaTransfer* transfer;
	std::condition_variable cv;
	std::mutex m;
	progress* p;
	const char* msg;
};

int MEGAlogin(const char* email, const char* password, MEGAhandle** out){
	std::string prompt;
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = new mega::MegaApi(MEGA_API_KEY, (const char*)NULL, "ezbackup");

	mega_api->login(email, password, &listener);

	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		return 1;
	}
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		std::cerr << "Failed to login (" << listener.getError()->toString() << ")." << std::endl;
		delete mega_api;
		return 1;
	}

	mega_api->fetchNodes(&listener);
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		return 1;
	}
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Failed to fetch nodes");
		delete mega_api;
		return -1;
	}

	*out = (MEGAhandle*)mega_api;
	return 0;
}

int MEGAmkdir(const char* dir, MEGAhandle* mh){
	std::string path;
	std::string spath;
	size_t index;
	mega::MegaNode* node;
	mega::SynchronousRequestListener listener;
	mega::MegaApi* mega_api;

	mega_api = static_cast<mega::MegaApi*>(mh);
	path = dir;

	node = mega_api->getNodeByPath(path.c_str());
	if (node){
		log_debug("MEGA: Path already exists");
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
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		return 1;
	}
	delete node;

	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Error creating folder (%s)");
		return -1;
	}

	log_debug_ex("MEGA: Created folder %s successfully", dir);
	return 0;
}

int MEGAreaddir(const char* dir, struct file_node*** out, size_t* out_len, MEGAhandle* mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaNodeList* children;
	mega::MegaApi* mega_api;
	int ret = 0;

	mega_api = static_cast<mega::MegaApi*>(mh);
	(*out) = NULL;

	path = dir;
	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error("MEGA: Directory does not exist");
		return -1;
	}

	children = mega_api->getChildren(node);
	*out = NULL;
	*out_len = 0;
	for (int i = 0; i < children->size(); ++i){
		struct file_node* tmp = (struct file_node*)malloc(sizeof(struct file_node));
		mega::MegaNode* n;

		if (!tmp){
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}

		(*out_len)++;
		*out = (struct file_node**)realloc(*out, sizeof(**out) * *out_len);
		if (!(*out)){
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}

		(*out)[*out_len - 1] = (struct file_node*)malloc(sizeof(struct file_node));
		if (!((*out)[*out_len - 1])){
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}

		n = children->get(i);
		log_debug_ex("Reading child %s", n->getName());
		(*out)[*out_len - 1]->name = (char*)malloc(strlen(dir) + strlen(n->getName()) + 2);
		if (!(*out)[*out_len - 1]->name){
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}
		strcpy((*out)[*out_len - 1]->name, dir);
		if (dir[strlen(dir) - 1] != '/'){
			strcat((*out)[*out_len - 1]->name, "/");
		}
		strcat((*out)[*out_len - 1]->name, n->getName());
		(*out)[*out_len - 1]->time = n->getCreationTime();
	}

	delete children;
	delete node;
	return ret;

cleanup_freeout:
	if (*out){
		for (size_t i = 0; i < *out_len; ++i){
			free((*out)[i]);
		}
	}
	delete children;
	delete node;
	return ret;
}

int MEGAdownload(const char* download_path, const char* out_file, const char* msg, MEGAhandle* mh){
	std::string path;
	std::string spath;
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	ProgressBarTransferListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	path += download_path;

	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error("MEGA: File not found");
		return -1;
	}

	listener.setMsg(msg);
	mega_api->startDownload(node, out_file, &listener);
	log_info("Downloading file...");
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Failed to download file");
	}

	return 0;
}

int MEGAupload(const char* in_file, const char* upload_dir, const char* msg, MEGAhandle* mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	ProgressBarTransferListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	path = upload_dir;

	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error("MEGA: Folder not found");
		return -1;
	}

	listener.setMsg(msg);
	mega_api->startUpload(in_file, node, &listener);
	log_info("Upload file...");
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Failed to upload file");
	}

	return 0;
}

int MEGArm(const char* file, MEGAhandle* mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	path = file;

	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error("MEGA: File not found");
		return -1;
	}

	mega_api->remove(node, &listener);
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		return 1;
	}
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		std::cerr << "Failed to remove " << path << " (" << listener.getError()->toString() << ")." << std::endl;
		delete mega_api;
		return 1;
	}

	return 0;
}

int MEGAlogout(MEGAhandle* mh){
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	mega_api->logout(&listener);
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		return 1;
	}
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_warning_ex("MEGA: failed to log out (%s)", listener.getError()->toString());
		delete mega_api;
		return -1;
	}

	delete mega_api;
	return 0;
}
