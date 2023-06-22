#include <iostream>
#include <vector>
#include <cmath>
#include <portaudio.h>
#include <fftw3.h>
#include <sndfile.h>
#include <SDL2/SDL.h>

// Paramètres audio
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 4096;

// Paramètres FFT
const int FFT_SIZE = FRAMES_PER_BUFFER / 2;

// Seuil en décibels pour filtrer les pics de faible amplitude
const double THRESHOLD_DB = 40.0; // Ajustez la valeur du seuil selon vos besoins

// Fréquences prédéfinies pour les échantillons
const double SAMPLE_FREQS[] = { 172.0, 193.88, 215.25, 258.33, 301.25, 322.46, 500.99, 20000.0 };

// Chemins des fichiers d'échantillons préenregistrés
const std::string SAMPLE_FILES[] = { "/home/admin/Desktop/Mi_grave.wav", "/home/admin/Desktop/Sol.wav",
                                     "/home/admin/Desktop/La.wav", "/home/admin/Desktop/Si.wav",
                                     "/home/admin/Desktop/Re.wav", "/home/admin/Desktop/Mi_aigu.wav",
                                    "/home/admin/Desktop/pas_encore_fait.wav", "/home/admin/Desktop/erreur.wav" };

// Structure pour stocker les données de l'analyse FFT
struct FFTData {
    double maxMagnitude;
    double maxFrequency;
};
//Variable Memoire pour gerer la sortie audio
std::string currentglobal ="/home/admin/Desktop/Mi_grave.wav";
std::string memoryglobal1 ="/home/admin/Desktop/Mi_grave.wav";
std::string memoryglobal2 ="/home/admin/Desktop/Mi_grave.wav";
std::string memoryglobal3 ="/home/admin/Desktop/Mi_grave.wav";
std::string playedglobal ="/home/admin/Desktop/Mi_grave.wav";
std::string playingglobal ="/home/admin/Desktop/Mi_grave.wav";


// Fonction pour calculer l'analyse FFT des échantillons audio
FFTData performFFT(const int16_t* samples) {
    FFTData data;
    data.maxMagnitude = -500.0;
    data.maxFrequency = 0.0;

    fftw_complex* fftInput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * FFT_SIZE));
    fftw_complex* fftOutput = reinterpret_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * FFT_SIZE));
    fftw_plan plan = fftw_plan_dft_1d(FFT_SIZE, fftInput, fftOutput, FFTW_FORWARD, FFTW_ESTIMATE);

    // Préparation des données pour l'analyse FFT
    for (int i = 0; i < FFT_SIZE; ++i) {
        fftInput[i][0] = static_cast<double>(samples[i]);
        fftInput[i][1] = 0.0;
    }

    // Exécution de l'analyse FFT
    fftw_execute(plan);

    // Calcul des magnitudes et des fréquences en dB
    double thresholdDB = THRESHOLD_DB;
    double scale = 1.0 / FFT_SIZE;

    for (int i = 0; i < FFT_SIZE; ++i) {
        double real = fftOutput[i][0];
        double imag = fftOutput[i][1];
        double magnitude = std::sqrt(real * real + imag * imag) * scale;
        double magnitudeDB = 20.0 * std::log10(magnitude);

        if (magnitudeDB > thresholdDB && magnitudeDB > data.maxMagnitude) {
            data.maxMagnitude = magnitudeDB;
            data.maxFrequency = static_cast<double>(i) * SAMPLE_RATE / FFT_SIZE;
        }
    }
    fftw_destroy_plan(plan);
    fftw_free(fftInput);
    fftw_free(fftOutput);

    return data;
}

struct AudioData{
    Uint8* pos;
    Uint32 length;
};

void wavCallback(void* userdata, Uint8* wavstream, int streamLength)
{
    AudioData* audio = (AudioData*)userdata;

    if(audio->length == 0)
    {
        return;
    }

    Uint32 length = (Uint32)streamLength;
    length = (length > audio->length ? audio->length : length);

    SDL_memcpy(wavstream, audio->pos, length);

    audio->pos += length;
    audio->length -= length;
}

// Fonction de rappel audio
int audioCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
                  const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) {
    const int16_t* samples = reinterpret_cast<const int16_t*>(inputBuffer);
    FFTData fftData = performFFT(samples);
    
    if (fftData.maxMagnitude > THRESHOLD_DB) {

        std::cout << "Pic de fréquence détecté : Fréquence = " << fftData.maxFrequency << " Hz, Amplitude = " << fftData.maxMagnitude << " dB" << std::endl;
        
            
        // Trouver l'échantillon correspondant à la fréquence détectée
        double closestFreq = SAMPLE_FREQS[0];
        double minDiff = std::abs(fftData.maxFrequency - closestFreq);
        int closestIndex = 0;
            
        for (int i = 1; i < sizeof(SAMPLE_FREQS) / sizeof(double); ++i) {
            double diff = std::abs(fftData.maxFrequency - SAMPLE_FREQS[i]);
            if (diff < minDiff) {
                closestFreq = SAMPLE_FREQS[i];
                minDiff = diff;
                closestIndex = i;
            }
        }

            memoryglobal3 = memoryglobal2;
            memoryglobal2 = memoryglobal1;
            memoryglobal1 = currentglobal;
            currentglobal= SAMPLE_FILES[closestIndex];
    
    }
    return paContinue;
}

int main() {
    // Initialisation de PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Erreur lors de l'initialisation de PortAudio : " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Paramètres du flux audio
    PaStreamParameters inputParameters;
    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

    // Ouverture du flux audio en utilisant la fonction de rappel
    PaStream* stream;
    err = Pa_OpenStream(&stream, &inputParameters, nullptr, SAMPLE_RATE, FRAMES_PER_BUFFER, paNoFlag, audioCallback, &inputParameters);
    if (err != paNoError) {
        std::cerr << "Erreur lors de l'ouverture du flux audio : " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // Démarrage du flux audio
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Erreur lors du démarrage du flux audio : " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "Enregistrement audio en cours..." << std::endl;
    std::cout << "Appuyez sur Entrée pour arrêter." << std::endl;
    //std::cin.ignore();

    while(1)
    {
    if(currentglobal != playedglobal)
        {
            if(currentglobal == memoryglobal1 && memoryglobal1 == memoryglobal2 && memoryglobal2 == memoryglobal3) {
                playingglobal = currentglobal;
                const std::string &filenameconv = playingglobal;
                const char* filename = filenameconv.c_str();
                SDL_Init(SDL_INIT_AUDIO);

                SDL_AudioSpec wavSpec;
                Uint8* wavStart;
                Uint32 wavLength;

                if(SDL_LoadWAV(filename, &wavSpec, &wavStart, &wavLength) == NULL)
                {
                    // TODO: Proper error handling
                    std::cerr << "Error: "
                        << " could not be loaded as an audio file" << std::endl;
                    return 1;
                }

                AudioData audio;
                audio.pos = wavStart;
                audio.length = wavLength;

                wavSpec.callback = wavCallback;
                wavSpec.userdata = &audio;

                SDL_AudioDeviceID device = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL,
                        SDL_AUDIO_ALLOW_ANY_CHANGE);
                if(device == 0)
                {
                    std::cerr << "Error: " << SDL_GetError() << std::endl;
                    return 1;
                }

                SDL_PauseAudioDevice(device, 0);

                while(audio.length > 0)
                {
                    SDL_Delay(100);
                }

                SDL_CloseAudioDevice(device);
                SDL_FreeWAV(wavStart);
                SDL_Quit();

                std::cout << "Échantillon joué : " << playingglobal << std::endl;
                playedglobal = playingglobal;
            }
        }
    }

    // Arrêt du flux audio
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "Erreur lors de l'arrêt du flux audio : " << Pa_GetErrorText(err) << std::endl;
    }

    // Fermeture du flux audio
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "Erreur lors de la fermeture du flux audio : " << Pa_GetErrorText(err) << std::endl;
    }

    // Terminaison de PortAudio
    Pa_Terminate();

    return 0;
}
