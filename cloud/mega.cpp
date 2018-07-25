/** @file cloud/mega.cpp
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
#include <unistd.h>
#include <sys/types.h>

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
			if (msg){
				p = start_progress(msg, max);
			}
			break;
		case mega::MegaTransfer::TYPE_DOWNLOAD:
			max = transfer->getTotalBytes();
			if (msg){
				p = start_progress(msg, max);
			}
			break;
		default:
			log_warning("MEGA: Could not start progress due to unknown transfer type.");
		}
	}

	void onTransferUpdate(mega::MegaApi* mega_api, mega::MegaTransfer* transfer){
		(void)mega_api;

		if (p){
			set_progress(p, transfer->getTransferredBytes());
		}
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

		if (p){
			error->getErrorCode() == mega::MegaError::API_OK ? finish_progress(p) : finish_progress_fail(p);
			p = NULL;
		}

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
	progress* p = NULL;
	const char* msg;
};

static std::string string_parent_dir(const char* in){
	std::string ret = in;
	size_t index;
	index = ret.find_last_of('/');
	if (index == std::string::npos){
		return "";
	}
	ret.resize(index + 1);
	return ret;
}

static std::string string_filename(const char* in){
	std::string ret = in;
	size_t index;
	index = ret.find_last_of('/');
	if (index == std::string::npos){
		return "";
	}
	ret = in + index + 1;
	return ret;
}

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

	*out = static_cast<MEGAhandle*>(mega_api);
	return 0;
}

int MEGAmkdir(const char* dir, MEGAhandle* mh){
	std::string parent_path;
	std::string filename;
	mega::MegaNode* node;
	mega::SynchronousRequestListener listener;
	mega::MegaApi* mega_api;

	mega_api = static_cast<mega::MegaApi*>(mh);

	node = mega_api->getNodeByPath(dir);
	if (node){
		log_debug("MEGA: Path already exists");
		delete node;
		return 1;
	}
	delete node;

	parent_path = string_parent_dir(dir);
	if (parent_path.empty()){
		log_error("MEGA: Directory is not a path");
		return -1;
	}

	filename = string_filename(dir);
	if (filename.empty()){
		log_error("MEGA: Invalid path");
		return -1;
	}

	node = mega_api->getNodeByPath(parent_path.c_str());
	if (!node || node->isFile()){
		log_error("MEGA: Parent folder not found");
		delete node;
		return -1;
	}

	mega_api->createFolder(filename.c_str(), node, &listener);
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		delete node;
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

int MEGAreaddir(const char* dir, char*** out, size_t* out_len, MEGAhandle* mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaNodeList* children;
	mega::MegaApi* mega_api;
	char** arr = NULL;
	size_t arr_len = 0;
	int ret = 0;

	mega_api = static_cast<mega::MegaApi*>(mh);

	arr = NULL;
	arr_len = 0;

	path = dir;
	node = mega_api->getNodeByPath(path.c_str());
	if (!node){
		log_error("MEGA: Directory does not exist");
		return -1;
	}
	if (node->isFile()){
		log_error("MEGA: Directory is actually a file.");
		delete node;
		return -1;
	}

	children = mega_api->getChildren(node);
	arr = (char**)malloc(sizeof(*arr) * children->size());
	if (!arr){
		log_enomem();
		ret = -1;
		goto cleanup_freeout;
	}
	for (int i = 0; i < children->size(); ++i){
		mega::MegaNode* n;

		arr_len++;

		n = children->get(i);
		log_info_ex("Reading child %s", n->getName());

		arr[arr_len - 1] = (char*)malloc(strlen(dir) + strlen(n->getName()) + 2);
		if (!arr[arr_len - 1]){
			log_enomem();
			ret = -1;
			goto cleanup_freeout;
		}
		strcpy(arr[arr_len - 1], dir);
		if (arr[arr_len - 1][strlen(arr[arr_len - 1]) - 1] != '/'){
			strcat(arr[arr_len - 1], "/");
		}
		strcat(arr[arr_len - 1], n->getName());
	}

	*out = arr;
	*out_len = arr_len;
	delete children;
	delete node;
	return ret;

cleanup_freeout:
	if (arr){
		for (size_t i = 0; i < arr_len; ++i){
			free(arr[i]);
		}
	}
	delete children;
	delete node;
	return ret;
}

int MEGAstat(const char* file_path, struct stat* out, MEGAhandle* mh){
	mega::MegaNode* node;
	mega::MegaApi* mega_api;

	mega_api = static_cast<mega::MegaApi*>(mh);
	node = mega_api->getNodeByPath(file_path);

	if (!node){
		log_debug_ex("MEGA: File/directory %s not found", file_path);
		return 1;
	}

	if (!out){
		delete node;
		return 0;
	}

	out->st_uid = getuid();
	out->st_gid = getgid();
	/* DO NOT USE node->isFile(). it always returns false for some reason */
	out->st_mode = node->getType() == 0 ? S_IFREG | 0444 : S_IFDIR | 0755;
	out->st_nlink = 1;
	out->st_size = node->getType() == 0 ? node->getSize() : 4096;
	out->st_mtime = node->getType() == 0 ? node->getModificationTime() : node->getCreationTime();
	out->st_ctime = node->getCreationTime();

	delete node;
	return 0;
}

int MEGArename(const char* _old, const char* _new, MEGAhandle* mh){
	mega::MegaNode* n_src;
	mega::MegaNode* n_dst;
	mega::MegaNode* n_tmp;
	mega::MegaApi* mega_api;
	std::string parent_path;
	std::string filename;

	mega_api = static_cast<mega::MegaApi*>(mh);

	if (strcmp(_old, _new) == 0){
		return 0;
	}

	n_src = mega_api->getNodeByPath(_old);
	if (!n_src){
		log_warning_ex("MEGA: File %s not found", _old);
		return -1;
	}

	n_dst = mega_api->getNodeByPath(_new);
	/* if destination folder exists */
	if (n_dst){
		mega::SynchronousRequestListener listener;
		if (n_dst->isFile()){
			log_warning_ex("MEGA: Destination %s already exists", _new);
			delete n_src;
			delete n_dst;
			return -1;
		}

		mega_api->moveNode(n_src, n_dst, &listener);
		if (listener.trywait(MEGA_WAIT_MS) != 0){
			log_error("MEGA: Connection timed out");
			delete n_src;
			delete n_dst;
			return -1;
		}
		delete n_src;
		delete n_dst;

		if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
			log_error_ex2("MEGA: Error moving %s (%s)", _old, listener.getError()->toString());
			return -1;
		}

		return 0;
	}

	/* otherwise create it */
	parent_path = string_parent_dir(_new);
	if (parent_path.empty()){
		log_warning_ex("MEGA: Invalid destination path (%s)", _new);
		delete n_src;
		return -1;
	}

	filename = string_filename(_new);
	if (filename.empty()){
		log_warning_ex("MEGA: Invalid destination path (file) (%s)", _new);
		delete n_src;
		return -1;
	}

	n_dst = mega_api->getNodeByPath(parent_path.c_str());
	if (!n_dst){
		log_warning("MEGA: Destination folder does not exist");
		delete n_src;
		return -1;
	}
	if (n_dst->isFile()){
		log_warning("MEGA: The destination folder is a file");
		delete n_src;
		delete n_dst;
		return -1;
	}

	n_tmp = mega_api->getChildNode(n_dst, filename.c_str());
	if (n_tmp){
		log_warning("MEGA: The destination path already exists");
		delete n_tmp;
		delete n_src;
		delete n_dst;
		return -1;
	}

	{
		mega::SynchronousRequestListener listener;
		mega_api->moveNode(n_src, n_dst, &listener);
		if (listener.trywait(MEGA_WAIT_MS) != 0){
			log_error("MEGA: Connection timed out");
			delete n_src;
			delete n_dst;
			return -1;
		}
		delete n_dst;
		if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
			log_error_ex("MEGA: Error moving file/folder (%s)", listener.getError()->toString());
			delete n_src;
			return -1;
		}

		if (strcmp(n_src->getName(), filename.c_str()) != 0){
			mega::SynchronousRequestListener listener2;
			mega_api->renameNode(n_src, filename.c_str(), &listener2);
			if (listener2.trywait(MEGA_WAIT_MS) != 0){
				log_error("MEGA: Connection timed out");
				delete n_src;
				return -1;
			}
			if (listener2.getError()->getErrorCode() != mega::MegaError::API_OK){
				log_error_ex("MEGA: Error renaming node (%s)", listener2.getError()->toString());
			}
		}
		delete n_src;
	}
	return 0;
}

int MEGAdownload(const char* download_path, const char* out_file, const char* msg, MEGAhandle* mh){
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	ProgressBarTransferListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	node = mega_api->getNodeByPath(download_path);
	if (!node){
		log_warning_ex("MEGA: File %s not found", download_path);
		return -1;
	}
	if (!node->isFile()){
		log_warning_ex("MEGA: %s is a directory, not a file.", download_path);
		delete node;
		return -1;
	}

	listener.setMsg(msg);
	mega_api->startDownload(node, out_file, &listener);
	log_info("Downloading file...");
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Failed to download file");
		delete node;
		return -1;
	}

	delete node;
	return 0;
}

int MEGAupload(const char* in_file, const char* upload_dir, const char* msg, MEGAhandle* mh){
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	ProgressBarTransferListener listener;
	std::string parent_dir = "";

	mega_api = static_cast<mega::MegaApi*>(mh);

	/* get folder node */
	node = mega_api->getNodeByPath(upload_dir);
	if (node && node->isFile()){
		log_warning("MEGA: The upload file already exists.");
		delete node;
		return -1;
	}
	/* if the path does not specify a valid directory */
	if (!node){
		/* attempt to get the node of the parent directory */
		parent_dir = string_parent_dir(upload_dir);
		if (parent_dir.empty() || (node = mega_api->getNodeByPath(parent_dir.c_str())) == NULL){
			log_error("MEGA: Folder not found");
			return -1;
		}
	}

	/* upload the file */
	listener.setMsg(msg);
	mega_api->startUpload(in_file, node, &listener);
	log_info("Uploading file...");
	listener.wait();
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		log_error("MEGA: Failed to upload file");
		delete node;
		return -1;
	}

	delete node;
	/* if upload_dir is not a directory */
	if (!parent_dir.empty()){
		/* rename the file from in_file to upload_dir */
		mega::MegaNode* node_uploaded;
		mega::SynchronousRequestListener srl;

		std::string filename_old = parent_dir;
		std::string filename_new;
		if (filename_old[filename_old.length() - 1] != '/'){
			filename_old += '/';
		}
		filename_new = filename_old;
		filename_old += string_filename(in_file);
		filename_new = string_filename(upload_dir);

		node_uploaded = mega_api->getNodeByPath(filename_old.c_str());
		if (!node_uploaded){
			log_warning_ex("MEGA: Failed to fetch uploaded path %s.", filename_old.c_str());
			return 0;
		}

		mega_api->renameNode(node_uploaded, filename_new.c_str(), &srl);
		if (srl.trywait(MEGA_WAIT_MS) != 0){
			log_warning("MEGA: Connection timed out while renaming file.");
			delete node_uploaded;
			return 0;
		}
		if (srl.getError()->getErrorCode() != mega::MegaError::API_OK){
			log_warning_ex2("MEGA: Failed to rename %s to %s while uploading.", filename_old.c_str(), filename_new.c_str());
			delete node_uploaded;
			return 0;
		}
		delete node_uploaded;
	}

	return 0;
}

int MEGArm(const char* file, MEGAhandle* mh){
	std::string path;
	mega::MegaNode* node;
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	node = mega_api->getNodeByPath(file);
	if (!node){
		log_warning("MEGA: File not found");
		return -1;
	}

	mega_api->remove(node, &listener);
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		delete node;
		return 1;
	}
	if (listener.getError()->getErrorCode() != mega::MegaError::API_OK){
		std::cerr << "Failed to remove " << path << " (" << listener.getError()->toString() << ")." << std::endl;
		delete node;
		return 1;
	}

	delete node;
	return 0;
}

int MEGAlogout(MEGAhandle* mh){
	mega::MegaApi* mega_api;
	mega::SynchronousRequestListener listener;

	mega_api = static_cast<mega::MegaApi*>(mh);

	mega_api->logout(&listener);
	if (listener.trywait(MEGA_WAIT_MS) != 0){
		std::cerr << "Connection timed out" << std::endl;
		delete mega_api;
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
