#include <Vector.h>

/**
 * @file at_parsing.h
 *
 */
#ifndef AT_PARSING
#define AT_PARSING

#include <Arduino.h>
#define TINY_GSM_MODEM_SIM7000
#include <TinyGsmClient.h>


class message_indata {
  public: 
      String phone_number;
      String message_datetime;
      String message;
};


class ATParser {
  public:
    Vector<message_indata> messages;
    ATParser(TinyGsm a_modem);
    void parse_at(String data);
  private:
    TinyGsm* local_modem;
    message_indata messages_storage_array[10];
    String part_storage_array[50];
    String row_storage_array[50];
    void parse_message(Vector<String> data, int *index);
    void split_string(String str, Vector<String> *strs, String sep, String qualifier = "\"");
    


};

#endif
