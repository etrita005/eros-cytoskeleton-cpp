#include "cytoskeleton/module/stub.h"
#include "cytoskeleton/object/object.h"

#include <iostream>

using namespace com::etrita::eros::cytos::object;
using namespace com::etrita::eros::cytos::module;

class DataServiceModule : public Object {
 public:
  DEFINE_MODULE_FACTORY(DataServiceModule);

  DataServiceModule() {
    std::cout << "DataServiceModule constructed" << std::endl;
  }

  ~DataServiceModule() {
    std::cout << "DataServiceModule destroyed" << std::endl;
  }

  void ProcessData() {
    std::cout << "DataServiceModule processing data" << std::endl;
  }
};

REGISTER_MODULE_SIMPLE(DataServiceModule, "1.0.0", "service/data_service",
                       "Sample data service module for demonstration");

class NetworkServiceModule : public Object {
 public:
  DEFINE_MODULE_FACTORY(NetworkServiceModule);

  NetworkServiceModule() {
    std::cout << "NetworkServiceModule constructed" << std::endl;
  }

  ~NetworkServiceModule() {
    std::cout << "NetworkServiceModule destroyed" << std::endl;
  }

  void SendData() {
    std::cout << "NetworkServiceModule sending data" << std::endl;
  }
};

REGISTER_MODULE(NetworkServiceModule, "1.0.0", "service/network_service",
                "Sample network service module for demonstration",
                "Etrita Team", {"DataServiceModule"});
