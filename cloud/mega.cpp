/* mega.cpp -- MEGA integration
 *
 * Copyright (c) 2018 Jonathan Lemos
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "mega.h"
extern "C"{
#include "../error.h"
#include "../crypt.h"
#include "../progressbar.h"
}
#include "mega_sdk/include/megaapi.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <fstream>
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
				log_warning(__FL__, "MEGA: Could not determine size of %s (%s)", transfer->getFileName(), strerror(errno));
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
			log_warning(__FL__, "MEGA: Could not start progress due to unknown transfer type.");
		}
	}

	void onTransferUpdate(mega::MegaApi* mega_api, mega::MegaTransfer* transfer){
		(void)mega_api;

		set_progress(p, transfer->getTransferredBytes());
	}

	void onTransferFinish(mega::MegaApi* mega_api, mega::MegaTransfer* transfer, mega::MegaError* error){
		(void)mega_api;

		this->error = error->copy();
		this->transfer = transfer->copy();

		{
			std::unique_lock<std::mutex> lock(m);
			notified = true;
		}

		cv.notify_all();
		finish_progress(p);
		p = NULL;
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

int MEGAlogin(const char* email, const char* password, MEGAhandle* out){
	const char* API_KEY = "***REMOVED***";
	std::string prompt;
	char pwbuffer[1024];
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = new mega::MegaApi(API_KEY, (const char*)NULL, "ezbackup");

	if (!password){
		prompt = "Enter password for ";
		prompt += email;
		prompt += ":";

		if (crypt_getpassword(prompt.c_str(), NULL, pwbuffer, sizeof(pwbuffer)) != 0){
			log_error(__FL__, "MEGA: Failed to read password from terminal.");
			crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
			return -1;
		}
		mega_api->login(email, pwbuffer, &listener);
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
	}
	else{
		mega_api->login(email, password, &listener);
	}
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		std::cout << "Failed to login (" << listener.getError()->toString() << ")." << std::endl;
		return 1;
	}

	mega_api->fetchNodes(&listener);
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error(__FL__, "MEGA: Failed to fetch nodes");
		return -1;
	}

	*out = (void*)mega_api;
	return 0;
}

int MEGAmkdir(const char* dir, MEGAhandle mh){
	std::string path;
	std::string spath;
	size_t index;
	mega::MegaNode* node;
	mega::SynchronousRequestListener listener;
	mega::MegaApi* mega_api;

	mega_api = (mega::MegaApi*)mh;
	path = dir;

	node = mega_api->getNodeByPath(path.c_str());
	if (node){
		log_debug(__FL__, "MEGA: Path already exists");
		return 1;
	}

	spath = path;
	index = spath.find_last_of('/');
	/* if we didn't find any matches */
	if (index == std::string::npos){
		log_error(__FL__, "MEGA: Invalid path");
		return -1;
	}
	spath.resize(index + 1);

	node = mega_api->getNodeByPath(spath.c_str());
	if (!node || node->isFile()){
		log_error(__FL__, "MEGA: Parent folder not found");
		delete node;

		return -1;
	}

	mega_api->createFolder(path.c_str() + index + 1, node, &listener);
	listener.wait();
	delete node;

	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error(__FL__, "MEGA: Error creating folder (%s)");
		return -1;
	}

	log_debug(__FILE__, __LINE__, "MEGA: Created folder %s successfully", dir);
	return 0;
}

int MEGAreaddir(const char* dir, char*** out, size_t* out_len, MEGAhandle mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaNodeList* children;
	mega::MegaApi* mega_api;

	mega_api = (mega::MegaApi*)mh;

	path = dir;
	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error(__FL__, "MEGA: Directory does not exist");
		return -1;
	}

	children = mega_api->getChildren(node);
	*out_len = 0;
	for (int i = 0; i < children->size(); ++i){
		(*out_len)++;
		*out = (char**)realloc(*out, sizeof(**out) * *out_len);
		if (!(*out)){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}

		mega::MegaNode* n = children->get(i);
		log_debug(__FL__, "Reading child %s", n->getName());
		*out[*out_len - 1] = (char*)malloc(strlen(n->getName()) + 1);
		if (!(*out)[*out_len - 1]){
			log_fatal(__FL__, STR_ENOMEM);
			return -1;
		}

		strcpy(*out[*out_len - 1], n->getName());
	}

	delete children;
	delete node;
	return 0;
}

int MEGAdownload(const char* download_path, const char* out_file, const char* msg, MEGAhandle mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	ProgressBarTransferListener listener;

	mega_api = (mega::MegaApi*)mh;

	path = "/";
	path += download_path;

	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error(__FL__, "MEGA: File not found");
		return -1;
	}

	listener.setMsg(msg);
	mega_api->startDownload(node, out_file, &listener);
	log_info(__FL__, "Downloading file...");
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error(__FL__, "MEGA: Failed to download file");
	}

	return 0;
}

int MEGAupload(const char* in_file, const char* upload_dir, const char* msg, MEGAhandle mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	ProgressBarTransferListener listener;

	mega_api = (mega::MegaApi*)mh;

	path = upload_dir;

	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error(__FL__, "MEGA: Folder not found");
		return -1;
	}

	listener.setMsg(msg);
	mega_api->startUpload(in_file, node, &listener);
	log_info(__FL__, "Upload file...");
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error(__FL__, "MEGA: Failed to upload file");
	}

	return 0;
}

int MEGAlogout(MEGAhandle mh){
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = (mega::MegaApi*)mh;

	mega_api->logout(&listener);
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_warning(__FL__, "MEGA: failed to log out (%s)", listener.getError()->toString());
		delete mega_api;
		return -1;
	}

	delete mega_api;
	return 0;
}
