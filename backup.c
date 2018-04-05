#include "readfile.h"
#include "crypt.h"
#include "fileiterator.h"
#include "error.h"
#include "checksum.h"
#include "options.h"
#include <string.h>
#include <sys/resource.h>

struct backup_params{
	TAR*            tp;
	struct TMPFILE* tfp_hashes;
	struct TMPFILE* tfp_hashes_prev;
	struct options* opt;
};

int __coredumps(int enable){
	static struct rlimit rl_prev;
	struct rlimit rl;
	int ret = 0;
	if (!enable){
		if (getrlimit(RLIMIT_CORE, &rl_prev) != 0){
			log_warning(__FL__, "Failed to get previous core dump limits");
			ret = -1;
		}
		rl.rlim_cur = 0;
		rl.rlim_max = 0;
		if (setrlimit(RLIMIT_CORE, &rl) != 0){
			log_warning(__FL__, "Failed to disable core dumps");
			ret = -1;
		}
	}
	else{
		if (setrlimit(RLIMIT_CORE, &rl_prev) != 0){
			log_warning(__FL__, "Failed to restore previous core dump limits");
			ret = -1;
		}
	}
	return ret;
}

int disable_core_dumps(void){
	return __coredumps(0);
}

int enable_core_dumps(void){
	return __coredumps(1);
}

int extract_prev_checksums(const char* in, const char* out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys fk;
	struct TMPFILE* tfp_decrypt = NULL;
	const EVP_CIPHER* enc = NULL;
	int ret = 0;

	if (!in || !out || !enc_algorithm){
		log_error(__FL__, STR_ENULL);
		return -1;
	}

	if (disable_core_dumps() != 0){
		log_debug(__FL__, "Did not disable core dumps");
	}

	enc = crypt_get_cipher(enc_algorithm);

	if (crypt_set_encryption(enc, &fk) != 0){
		log_debug(__FL__, "crypt_set_encryption() failed");
		ret = -1;
		goto cleanup;
	}

	if (crypt_extract_salt(in, &fk) != 0){
		log_debug(__FL__, "crypt_extract_salt() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_getpassword("Enter decryption password", NULL, pwbuffer, sizeof(pwbuffer))) != 0){
		log_debug(__FL__, "crypt_getpassword() failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		log_debug(__FL__, "crypt_gen_keys() failed)");
		ret = -1;
		goto cleanup;
	}
	crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);

	if ((tfp_decrypt = temp_fopen("/var/tmp/decrypt_XXXXXX", "w+b")) == NULL){
		log_debug(__FL__, "temp_file() for file_decrypt failed");
		ret = -1;
		goto cleanup;
	}

	if ((crypt_decrypt_ex(in, &fk, out, verbose, "Decrypting file...")) != 0){
		crypt_free(&fk);
		log_debug(__FL__, "crypt_decrypt() failed");
		ret = -1;
		goto cleanup;
	}

	if (tar_extract_file(tfp_decrypt->name, "/checksums", out) != 0){
		log_debug(__FL__, "tar_extract_file() failed");
		ret = -1;
		goto cleanup;
	}

	shred_file(tfp_decrypt->name);

cleanup:
	crypt_free(&fk);

	if (tfp_decrypt && temp_fclose(tfp_decrypt) != 0){
		log_debug(__FL__, STR_EFCLOSE);
	}

	if (enable_core_dumps() != 0){
		log_debug(__FL__, "enable_core_dumps() failed");
	}
	return ret;
}

int encrypt_file(const char* in, const char* out, const char* enc_algorithm, int verbose){
	char pwbuffer[1024];
	struct crypt_keys fk;
	const EVP_CIPHER* enc;
	int ret;

	/* disable core dumps if possible */
	if (disable_core_dumps() != 0){
		log_warning(__FL__, "Core dumps could not be disabled\n");
	}

	enc = crypt_get_cipher(enc_algorithm);

	if (crypt_set_encryption(enc, &fk) != 0){
		log_debug(__FL__, "Could not set encryption type");
		ret = -1;
		goto cleanup;
	}

	if (crypt_gen_salt(&fk) != 0){
		log_debug(__FL__, "Could not generate salt");
		ret = -1;
		goto cleanup;
	}

	/* PASSWORD IN MEMORY */
	while ((ret = crypt_getpassword("Enter encryption password",
					"Verify encryption password",
					pwbuffer,
					sizeof(pwbuffer))) > 0){
		printf("\nPasswords do not match\n");
	}
	if (ret < 0){
		log_debug(__FL__, "crypt_getpassword() failed");
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		ret = -1;
		goto cleanup;
	}

	if ((crypt_gen_keys((unsigned char*)pwbuffer, strlen(pwbuffer), NULL, 1, &fk)) != 0){
		crypt_scrub(pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
		log_debug(__FL__, "crypt_gen_keys() failed");
		ret = -1;
		goto cleanup;
	}
	/* don't need to scrub entire buffer, just where the password was
	 * and a little more so attackers don't know how long the password was */
	crypt_scrub((unsigned char*)pwbuffer, strlen(pwbuffer) + 5 + crypt_randc() % 11);
	/* PASSWORD OUT OF MEMORY */

	if ((crypt_encrypt_ex(in, &fk, out, verbose, "Encrypting file...")) != 0){
		crypt_free(&fk);
		log_debug(__FL__, "crypt_encrypt() failed");
		ret = -1;
		goto cleanup;
	}

cleanup:
	/* shreds keys as well */
	crypt_free(&fk);
	if (enable_core_dumps() != 0){
		log_debug(__FL__, "enable_core_dumps() failed");
	}
	return 0;
}

int does_file_exist(const char* file){
	struct stat st;
	return stat(file, &st) == 0;
}

int backup(struct options* opt, const struct options* opt_prev){
	struct backup_params bp;
	memset(&bp, 0, sizeof(bp));
	bp.opt = opt;

	/* load previous hash file */
	if (opt_prev &&
			opt_prev->prev_backup &&
			opt_prev->hash_algorithm == bp.opt->hash_algorithm){

	}
	return 0;
}
