//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

// This file is used for the 'meteostation' demo. It instantiates an instance of an AEA that is willing to sell weather data
// at a certain price.
//
// If its operation is successful, it should connect to a Node (or Nodes when possible) and accept 'conversations' from interested parties


#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include "clara.hpp"

// FETCH.ai
#include "client.h"
#include "messages.h"

/////////////////////////////////////////////////////////////////////////////////////
// Code designed to read weather data from piwws (written by Paul Bramall)
typedef struct
{
    std::string date_time;
    float indoor_humidity;
    float indoor_temperature;
    float outdoor_humidity;
    float outdoor_temperature;
    float indoor_pressure_mb;
    float outdoor_pressure_mb;
    float field7;   // I don't yet know what these other fields are, so I've
    float field8;   // just left them as placeholders.  They'll be things like
    float field9;   // windspeed & direction, rainfall, etc.
    float field10;
    float field11;
} WeatherReading;

std::vector<WeatherReading> getWeatherReadingsFromFile(std::string filename)
{
    std::vector<WeatherReading> readings; // This is the value that is ultimately returned

    std::ifstream  data(filename);

    std::string line;
    while(std::getline(data, line))
    {
        WeatherReading weatherReading;
        std::vector<std::string> elements;
        std::string element;
        std::stringstream  lineStream(line);

        while(std::getline(lineStream, element, ',')) // File is CSV, so split on comma
        {
            elements.push_back(element);
        }

        weatherReading.date_time           = elements[0];
        weatherReading.indoor_humidity     = std::stof(elements[1]);
        weatherReading.indoor_temperature  = std::stof(elements[2]);
        weatherReading.outdoor_humidity    = std::stof(elements[3]);
        weatherReading.outdoor_temperature = std::stof(elements[4]);
        weatherReading.indoor_pressure_mb  = std::stof(elements[5]);
        weatherReading.outdoor_pressure_mb = std::stof(elements[6]);
        weatherReading.field7              = std::stof(elements[7]);
        weatherReading.field8              = std::stof(elements[8]);
        weatherReading.field9              = std::stof(elements[9]);
        weatherReading.field10             = std::stof(elements[10]);
        weatherReading.field11             = std::stof(elements[11]);

        readings.push_back(weatherReading);
    }
    return readings;
}

WeatherReading getLatestWeatherReadingFromFile(std::string filename)
{
    std::vector<WeatherReading>readings = getWeatherReadingsFromFile(filename);
    return readings.back();
}

std::string getCurrentHourlyFilename()
{
    const int len_of_pretty_time{24};
    char pretty_time[len_of_pretty_time]; // Example "2017/2017-11/2017-03-14"
    time_t rawtime;
    struct tm *timeinfo;
    std::time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Note, the line below pads the days & months with leading zeros. I don't know if pywws
    // actually does this for the files it creates (I developed this code on 15th November).
    std::strftime(pretty_time, len_of_pretty_time, "%Y/%Y-%m/%Y-%m-%d", timeinfo);

    std::string filestem{"/home/pi/weather/data/hourly"};
    std::string filepath = filestem + '/' + pretty_time + ".txt";

    return filepath;
}

WeatherReading getLatestWeatherReading()
{
    std::string filename   = getCurrentHourlyFilename();
    WeatherReading reading = getLatestWeatherReadingFromFile(filename);
    return reading;
}

// End weather station reading code
/////////////////////////////////////////////////////////////////////////////////////

class MeteoStation : public Client
{

public:

  // Create a subclass of Client (soon to be renamed to agent/AEA)
  // an important thing to note is that in the constructor of the parent class you
  // must pass a function pointer that takes a Conversation. This defines what your 
  // network handling code does with new Conversations.
  // In this instance it will add them to your thread safe Queue
  MeteoStation(const std::string &id, const char *host) :
    Client{id, host, [this](std::unique_ptr<Conversation> c) { _conversations.push(std::move(c));}}
  {
    // Here we decide what attributes we *could* provide (see OEF handover wiki document)
    // format: {attribute, type, optional}
    Attribute wind        { "wind_speed",   Type::Bool, true};
    Attribute temperature { "temperature",  Type::Bool, true};
    Attribute air         { "air_pressure", Type::Bool, true};
    Attribute humidity    { "humidity",     Type::Bool, true};

    // We then create a DataModel for this, personalise it by creating an Instance,
    // and register it with the Node (connected during Client construction)
    std::vector<Attribute> attributes{wind,temperature,air,humidity};
    DataModel weather{"weather_data", attributes, stde::optional<std::string>{"All possible weather data."}};
    Instance s{weather, {{"wind_speed", "true"}, {"temperature", "true"}, {"air_pressure", "true"}, {"humidity", "true"}}};
    registerAgent(s);

    _thread = std::make_unique<std::thread>([this]() { run(); });
  }

  MeteoStation(const MeteoStation &) = delete;
  MeteoStation operator=(const MeteoStation &) = delete;

  ~MeteoStation()
  {
    if(_thread)
    {
      _thread->join();
    }
  }

private:

  // popping from the _conversations queue will block until there is at least one element
  void run()
  {
    while(1){
      process(_conversations.pop());
    }
  }

  void process(std::unique_ptr<Conversation> conversation)
  {
    // Pretty basic implementation, assumes that all messages are from agents (rather than Node).
    // Also assumes the message from the agent was some sort of price request
    // Also assumes the next message is whether that agent accepted the price
    AgentMessage am = conversation->pop();
    std::cerr << "From " << am.origin() << " content " << am.content() << std::endl;
    conversation->sendMsg<Price>(Price{_unitPrice});
    std::cerr << "Price sent\n";
    auto a = conversation->popMsg<Accepted>();
    if(a.status())
    {
      // let's send the data
      std::cerr << "I won!\n";
      // TODO: replace bogus value with ones from the station.
      //WeatherReading wr = getLatestWeatherReading();
      WeatherReading wr; // This is done as there often is no weather station connected which will cause it to segfault
      wr.outdoor_temperature = 1.1;
      wr.outdoor_pressure_mb = 2.2;
      wr.outdoor_humidity    = 3.3;


      Data temp{"temperature", "float", {std::to_string(wr.outdoor_temperature)}};
      conversation->sendMsg<Data>(temp);
      Data air{"air pressure", "float", {std::to_string(wr.outdoor_pressure_mb)}};
      conversation->sendMsg<Data>(air);
      Data humid{"humidity", "float", {std::to_string(wr.outdoor_humidity)}};
      conversation->sendMsg<Data>(humid);
    } else
    {
      std::cerr << "I lost\n";
    }
  }

  float                                _unitPrice = 0.1;
  std::unique_ptr<std::thread>         _thread;
  Queue<std::unique_ptr<Conversation>> _conversations;

};

int main(int argc, char* argv[])
{
  bool showHelp = false;
  std::string host;

  auto parser = clara::Help(showHelp) + clara::Arg(host, "host")("Host address to connect.");
  auto result = parser.parse(clara::Args(argc, argv));

  if(showHelp || argc == 1)
  {
    std::cout << parser << std::endl;
  } else
  {
    try
    {
      MeteoStation m{"RealMeteoStation", host.c_str()};
    } catch(std::exception &e)
    {
      std::cerr << "Exception " << e.what() << std::endl;
    }
  }

  return 0;
}
