#include "music.hpp"

#include <portmidi.h>
#include <iostream>
#include <pthread.h>
#include <sys/time.h>

using namespace std;

static PmError error = pmNoError;
static PortMidiStream* stream;
static pthread_mutex_t stream_mutex = PTHREAD_MUTEX_INITIALIZER;

void music_init() {
    if ((error=Pm_Initialize())) {
        cout << "Error on initialisation: " << Pm_GetErrorText(error) << endl;
        exit(error);
    }

    int num_devices = Pm_CountDevices();
    cout << "Number of MIDI devices: " << num_devices << endl;

    for (int i = 0; i < num_devices; i++) {
        const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
        const char *name = info->name;
        const char *input = info->input ? "input" : "";
        const char *output = info->output ? "output" : "";
        const char *sep = info->input && info->output ? ", " : "";
        cout << "  [" << i << "] " << input << sep << output << " device: " << name << endl;
    }

    if (num_devices > 0) {
        int device_id = 0;
        if (num_devices > 1) {
            cout << "Select MIDI output [0";
            for (int i = 1; i < num_devices; i++) { cout << ", " << i; }
            cout << "]: ";
            cin >> device_id;
        }

        cout << "Opening device " << device_id << endl;

        if ((error=Pm_OpenOutput(&stream, device_id, NULL, 1, 0, 0, 0))) {
            cout << "Error opening device " << device_id << ":" << Pm_GetErrorText(error) << endl;
            exit(error);
        }
    }
}

void music_destroy() {
    pthread_mutex_lock(&stream_mutex);
    if (stream != 0) {
        Pm_Close(stream);
    }
    stream = 0;
    Pm_Terminate();
    pthread_mutex_unlock(&stream_mutex);
}


pthread_mutex_t fakeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t fakeCond = PTHREAD_COND_INITIALIZER;

// Found at http://somethingswhichidintknow.blogspot.com/2009/09/sleep-in-pthread.html
void mywait(int timeInSec)
{
    struct timespec timeToWait;
    struct timeval now;
    int rt;

    gettimeofday(&now,NULL);

    timeToWait.tv_sec = now.tv_sec + timeInSec;
    timeToWait.tv_nsec = now.tv_usec*1000;

    pthread_mutex_lock(&fakeMutex);
    rt = pthread_cond_timedwait(&fakeCond, &fakeMutex, &timeToWait);
    pthread_mutex_unlock(&fakeMutex);
}

static void *music_stop(void *note) {
    mywait(1);
    PmEvent buffer = { Pm_Message(138, (long)note, 0), 0 };
    pthread_mutex_lock(&stream_mutex);
    if (stream != 0) { Pm_Write(stream, &buffer, 1); }
    pthread_mutex_unlock(&stream_mutex);
    pthread_exit(NULL);
}

void music_beat(SoundID sound_id) {
    int note = 0;
    pthread_t temp;

    switch(sound_id) {
        case FIRST_DRUM:
            note = 48;
            break;
        case SECOND_DRUM:
            note = 52;
            break;
        case THIRD_DRUM:
            note = 55;
            break;
        case FOURTH_DRUM:
            note = 59;
            break;
    }

    PmEvent buffer = { Pm_Message(154, note, 64), 0 };
    pthread_mutex_lock(&stream_mutex);
    if (stream != 0) { Pm_Write(stream, &buffer, 1); }
    pthread_mutex_unlock(&stream_mutex);
    pthread_create(&temp, NULL, music_stop, (void *)note);
}