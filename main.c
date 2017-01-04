#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <locale.h>
#include <alsa/asoundlib.h>
#include <assert.h>
#include <termios.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <endian.h>
#include <sndfile.h>

#define  log_inf(fmt, ...) printf("\033[1;37m%s %04u :"fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_imp(fmt, ...) printf("\033[0;32;32m%s %04u :"fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_wrn(fmt, ...) printf("\033[0;35m%s %04u :"fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)
#define  log_err(fmt, ...) printf("\033[0;32;31m%s %04u :"fmt"\033[m", __FILE__, __LINE__, ##__VA_ARGS__)

static snd_pcm_t * alsa_open(const char * name) {
	int ret = -1;
	snd_pcm_t *handle = NULL;

	ret = snd_pcm_open(&handle, name, SND_PCM_STREAM_CAPTURE, 0);
	if (ret < 0) {
		log_err("snd_pcm_open failed\n");
		return NULL;
	}

	return handle;
}

static int alsa_set_hw_params(snd_pcm_t *handle) {
	int ret = -1;
	int rate = 48000;
	snd_pcm_hw_params_t *hw_params = NULL;

	ret = snd_pcm_hw_params_malloc(&hw_params);
	if (ret < 0) {
		log_err("snd_pcm_hw_params_malloc failed\n");
		return -1;
	}

	ret = snd_pcm_hw_params_any(handle, hw_params);
	if (ret < 0) {
		log_err("snd_pcm_hw_params_any failed\n");
		return -1;
	}

	ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (ret < 0) {
		log_err("snd_pcm_hw_params_set_access failed\n");
		return -1;
	}

	ret = snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE);
	if (ret < 0) {
		log_err("snd_pcm_hw_params_set_format failed\n");
		return -1;
	}

	ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rate, 0);
	if (ret < 0) {
		log_err("snd_pcm_hw_params_set_rate_near failed\n");
		return -1;
	}

	ret = snd_pcm_hw_params_set_channels(handle, hw_params, 2);
	if (ret < 0) {
		log_err("snd_pcm_hw_params_set_channels failed\n");
		return -1;
	}

	ret = snd_pcm_hw_params(handle, hw_params);
	if (ret < 0) {
		log_err("snd_pcm_hw_params failed\n");
		return -1;
	}

	snd_pcm_hw_params_free(hw_params);

	return snd_pcm_prepare(handle);
}

static SNDFILE * alsa_sndfile_open() {
	SNDFILE *file;
	SF_INFO sfinfo;

	memset(&sfinfo, 0, sizeof(sfinfo));

	sfinfo.samplerate = 48000;
	sfinfo.channels = 2;
	sfinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

	if (!(file = sf_open("sine.wav", SFM_WRITE, &sfinfo))) {
		log_err("Error : Not able to open output file.\n");
		return NULL;
	}

	return file;
}

static int alsa_sndfile_write(SNDFILE * file, short * buf, int frames) {
	long frames_w = 0;

	frames_w = sf_writef_short(file, buf, frames);

	if (frames_w != frames) {
		log_err("write sndfile failed\n");
		sf_close(file);
		return -1;
	}

	return 0;
}

static int alsa_sndfile_close(SNDFILE * file) {
	sf_write_sync(file);
	sf_close(file);
	return 0;
}


static int alsa_read(snd_pcm_t * handle) {
	SNDFILE *file;
	int numFrames = 1024;
	short *buffer = (short *) malloc(2 * numFrames * sizeof(short));
	if (buffer == NULL) {
		log_err("malloc failed\n");
		return -1;
	}

	file = alsa_sndfile_open();
	int all = 0;
	while (1) {
		int pcmreturn = snd_pcm_readi(handle, buffer, numFrames);
		log_imp("pcmreturn = %d, data[0]=[%d], data[1]=[%d], data[2]=[%d], data[3]=[%d]\n", pcmreturn, buffer[0], buffer[1], buffer[2], buffer[3]);
		if (pcmreturn < 0) {
			snd_pcm_prepare(handle);
		}

		if (all > 2 * 1024 * 1024) {
			log_imp("alsa_sndfile_close pcmreturn = %d, data[0]=[%d], data[1]=[%d], data[2]=[%d], data[3]=[%d]\n", pcmreturn, buffer[0], buffer[1], buffer[2], buffer[3]);
			alsa_sndfile_close(file);
			return 0;
		} else {
			alsa_sndfile_write(file, buffer, numFrames);
			all += pcmreturn;
			log_imp("alsa_sndfile_write pcmreturn = %d, data[0]=[%d], data[1]=[%d], data[2]=[%d], data[3]=[%d]\n", pcmreturn, buffer[0], buffer[1], buffer[2], buffer[3]);
		}
	}
}

static int alsa_close(snd_pcm_t * handle) {
	snd_pcm_close(handle);
}



int main(int argc, char *argv[]) {
	int ret = -1;
	snd_pcm_t * handle = NULL;

	handle = alsa_open("sysdefault");
	if (handle == NULL) {
		return -1;
	}

	ret = alsa_set_hw_params(handle);
	if (ret < 0) {
		return -1;
	}

	alsa_read(handle);

	alsa_close(handle);
	return 0;
}
