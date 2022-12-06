// ----------------------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
#include <Windows.h>
#elif defined(__linux__)

#endif

#include <cstdint>
#include <ctime>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <complex>

// HackRF includes
#include <hackrf.h>

// Custom Includes
// #include <iq_utils.h>
#include <hackrf_common.h>

// Project Includes

const double pi = 3.14159265358979323846;
const double pi2 = 2.0 * 3.14159265358979323846;

const std::complex<double> j(0, 1);

//const uint64_t block_size = 262144 >> 1;
//static std::vector<std::complex<int8_t>> samples;
static std::vector<uint8_t> samples;

//static std::vector<std::complex<int8_t>>::iterator samples_itr = samples.begin();
static uint64_t blocks_captured;

static uint64_t data_index;
static bool tx_complete;


//-----------------------------------------------------------------------------
inline void sleep_ms(uint32_t value)
{

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
    Sleep(value);
#else
    const timespec delay[] = { 0, (uint32_t)(value * 1000000) };
    nanosleep(delay, NULL);
#endif

}   // end of sleep_ms


//-----------------------------------------------------------------------------
int tx_callback(hackrf_transfer* transfer)
{
    size_t bytes_to_xfer = transfer->buffer_length;
    size_t bytes_remaining = samples.size() - data_index;

    // check the current index and the transfer size to see where we are at in the samples
    
    // start at index == 0
    // if the size of the samples buffer is larger than the transfer->buffer length then fill the transfer buffer
    // increment the index by the number of bytes_to_xsfer
    if (bytes_remaining >= bytes_to_xfer)
    {
        //std::copy(samples.begin()+data_index, samples.begin() + data_index+ bytes_to_xfer, transfer->buffer);
        transfer->buffer = reinterpret_cast<uint8_t*>(&samples[data_index]);
        transfer->valid_length = bytes_to_xfer;
        data_index += bytes_to_xfer;
        return 0;
    }
    else
    {
        // if the number of the remaining samples is less than the transfer buffer fill the buffer with just those samples
        //std::copy(samples.begin() + data_index, samples.begin() + data_index + bytes_remaining, transfer->buffer);
        transfer->buffer = reinterpret_cast<uint8_t*>(&samples[data_index]); 
        transfer->valid_length = bytes_remaining;
        data_index = 0;
        tx_complete = true;
        return 0;
    }
    
    // set the transfer valid length to the number of remaining bytes
}   // end of tx_callback

static void tx_complete_callback(hackrf_transfer* transfer, int success)
{
    // If a transfer failed to complete, stop the main loop.
    if (!success) {
        std::cout << "Error setting tx_complete_callback" << std::endl;
        return;
    }


}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{

    uint32_t idx = 0, jdx = 0;
    
    // hackrf specific structs
    hackrf_device* dev = NULL;

    double sample_rate = 10000000;
    uint64_t freq = 30000000;
    uint32_t tx_gain = 0;

    int32_t rv;
    int bp = 1;
    uint8_t board_id = 0;

    // determine how many blocks to capture
    uint64_t num_blocks = 100;

    std::complex<double> tmp_val;

    // allocate the memory for the samples, but do not actually init the container
    //samples.reserve(num_blocks * block_size);

    // NOT SURE WHAT THESE ARE FOR
    // //generate IQ samples - simple FSK
    uint32_t data = 0xAB42F58C;     // random 32-bit data
    // uint32_t bit_mask = 1;

    // the number of samples per bit
    uint32_t bit_samples = 50000;

    // the frequency offset for the FSK modulation - 100kHz, normalized by the sample rate
    double freq_offset = 10000.0;

    double amplitude = 120;

    for (jdx = 0; jdx < bit_samples; ++jdx)
    {
        tmp_val = amplitude * (std::exp(j * pi2 * freq_offset * (double)jdx) + 1.0);
        samples.push_back((uint8_t)(tmp_val.real()));
        samples.push_back((uint8_t)(tmp_val.imag()));
    }


    std::string save_filename = "../test_hackrf_save.bin";
    // write_iq_data(save_filename, samples);


    try
    {
        std::cout << "did I get here" << std::endl;
        rv = hackrf_init();

        rv = select_hackf(&dev);
        if (rv != HACKRF_SUCCESS)
        {
            std::cout << "error opening HackRF: " << std::string(hackrf_error_name((enum hackrf_error)rv)) << std::endl;
        }

        rv = hackrf_board_id_read(dev, &board_id);
        if (rv == HACKRF_SUCCESS)
        {
            std::cout << "HackRF board ID: " << std::string(hackrf_board_id_name((enum hackrf_board_id)board_id)) << std::endl;
        }
        else
        {
            std::cout << "error getting board id: " << std::string(hackrf_error_name((enum hackrf_error)rv)) << std::endl;
        }

        std::cout << "Setting the sample rate:" << std::endl;
        rv = hackrf_set_sample_rate(dev, sample_rate);


        for (int freq=30000000; freq<50000000; freq=freq+25000)
        {
            std::cout << "Setting the frequency to:" << freq << std::endl;
            rv |= hackrf_set_freq(dev, freq);
            
            // rv |= hackrf_set_txvga_gain(dev, tx_gain);
            if (rv != HACKRF_SUCCESS)
            {
                fprintf(stderr, "hackrf failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)rv), rv);
                //return EXIT_FAILURE;
            }
            
            sleep_ms(100);

            hackrf_stop_tx(dev);


            // data_index = 0;
            // tx_complete = false;

            // HackRF sends data in 262144 byte blocks
            std::cout << "hackrf_start_tx:" << std::endl;
            rv =    hackrf_start_tx(dev, tx_callback, NULL);
            if (rv != HACKRF_SUCCESS) 
            {
                fprintf(stderr, "hackrf_start_?x() failed: %s (%d)\n", hackrf_error_name((enum hackrf_error)rv), rv);
                return EXIT_FAILURE;
            }
        }





        // for (idx = 0; idx < 200; ++idx)
        // {

        //     //while ((hackrf_is_streaming(dev) == HACKRF_TRUE));
        //     while (!tx_complete)
        //     {
        //         sleep_ms(10);
        //     }

        //     data_index = 0;
        //     tx_complete = false;

        //     //rv = hackrf_set_tx_block_complete_callback(dev, tx_complete_callback);


        //     // wait for the correct number of blocks to be collected

        //     // stop the transmit callback
        //     //rv = hackrf_stop_tx(dev);
        //     std::cout << "loop #: " << idx << std::endl;
        //     bp = 0;
        // }
        // rv = hackrf_stop_tx(dev);

        // // save the samples to a file
        // //std::string save_filename = "../test_hackrf_save.bin";
        // //write_iq_data(save_filename, samples);

        // int bp = 2;

    }
    catch (std::exception e)
    {
        std::cout << "error: " << e.what() << std::endl;
    }
    // initialize the hackrf - required before opening
    
    rv = hackrf_close(dev); 
    rv = hackrf_exit();
    


}   // end of main
