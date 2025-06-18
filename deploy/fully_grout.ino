#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads; /* Use this for the 16-bit version */
// Define chip select pin for micro SD
const int chipSelect = 2;

// Monitoring interval of GWL in second
#define monitoring_interval 10

// Global variables
long baudrate = 115200;
unsigned long last_acquisition_time;
char current_filename[20];

void setup()
{
    // Begin serial for debug
    Serial.begin(baudrate);
    delay(500);

    // See if the card is present and can be initialized:
    Serial.print("Initializing SD card...");
    if (!SD.begin(chipSelect))
    {
        Serial.println("Card failed, or not present");
    }
    Serial.println("card initialized.");

    // Get new filename
    get_file_count();
    Serial.print("Current file name:");
    Serial.println(current_filename);

    ads.setGain(GAIN_TWOTHIRDS); // 2/3x gain +/- 6.144V  1 bit = 0.1875mV
    ads.begin();
    Wire.begin(); // For RTC SDA SCL activation

    // Ready for acquisition
    Serial.println("Fully grout data logger ready...");
    Serial.println(F("Written by yinjeh.ngui@gmail.com."));
    delay(1000);
    // initialize_pins();
}

void loop()
{
    if (millis() > last_acquisition_time + monitoring_interval*1000)
    {
        // Get current acquisition time
        last_acquisition_time = millis();
        
        // Perform acquisition at defined intervals
        // Measure gwl from all adc oirt
        uint16_t pressure = measure_gwl(0);
        uint16_t gwl1 = measure_gwl(1);
        uint16_t gwl2 = measure_gwl(2);

        // Consolidate data to save
        char buffer[30];
        sprintf(buffer, "%i,%i,%i", pressure, gwl1, gwl2);

        // Print structured data to serial port
        Serial.println(buffer);

        // Save data
        save_to_sd(buffer);

        
    }
}

void get_file_count()
{
    // Initialize file name based on the last numbered file on SD card
    // upon every reset, provide filename from last_file_count here
    // Use loop to check for filename
    int current_file_index = 1;
    int current_file_exist = 1;
    int fn_count = 1;
    while (current_file_exist)
    {
        sprintf(current_filename, "%d.txt", fn_count);
        // Loop check for the next available filename
        if (SD.exists(current_filename))
        {
            Serial.print(current_filename);
            Serial.println(F(" exists."));
        }
        else
        {
            Serial.print(current_filename);
            Serial.println(F(" doesn't exist."));
            Serial.print(F("- Current file is recorded as "));
            Serial.println(current_filename);
            current_file_index = fn_count;
            current_file_exist = 0;
        }
        fn_count += 1;
    }    
}

int16_t adc_averaged(int target_pin)
{
    // Get averaged ADC measurement to remove noise
    int sampling_interval = 50; // ms
    int sampling_amount = 20;   // amount
    int sensor_value = 0;
    int16_t adc_mean = 0;
    int32_t sum = 0;
    int i = 0;
    while (i < sampling_amount)
    {
        // Read value from ADS1115
        sensor_value = ads.readADC_SingleEnded(target_pin); // take a sample
        // Read value from on-board ADC
        // sensor_value = analogRead(target_pin);
        sum += sensor_value; // Add them all up
        i += 1;
        delay(sampling_interval); // in ms, Sampling time = sampling_interval * sampling_amount
    }
    adc_mean = sum / sampling_amount; // Take the average. Note that it's customary to not start variable names with a capital. Style makes your code readable.
    return adc_mean;
}

int16_t measure_gwl(int gwl_pin)
{
    // Measure GWL through onboard ADC
    int16_t gwlcm = adc_averaged(gwl_pin);
    // Exclude excessive values when no water
    if (gwlcm > 65500)
    {
        gwlcm = 0;
    }
    return gwlcm;
}

void save_to_sd(char* buffer)
{    
    // Filename is from get_file_count() using var current_filename
    // Open file
    File dataFile = SD.open(current_filename, FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile)
    {
        dataFile.println(buffer);
        dataFile.close();
        // print to the serial port too:
        // Serial.println(F("Data written to microSD="));
        // Serial.println(currentData);
    }
    // if the file isn't open, pop up an error:
    else
    {
        Serial.print(F("Error opening "));
        Serial.println(current_filename);
    }
}
