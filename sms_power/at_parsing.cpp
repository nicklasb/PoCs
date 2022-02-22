 /**
 * @file at_parsing.cpp
 *
 */


#include "at_parsing.h"


ATParser::ATParser (TinyGsm a_modem) {
  local_modem = &a_modem;
  messages.setStorage(this->messages_storage_array);

}



void ATParser::split_string(String str, Vector<String> *strs, String sep, String qualifier) {
  
  int index = 0;

  bool inside = false;
  

    // Split the string into substrings
  while (str.length() > 0)
  {
    
    if ((qualifier != "") && ((String)str[0] == qualifier)) {
      str = str.substring(1);
      
      index = str.indexOf(qualifier);
      if (index == -1) {
        // Case where string ends without delimiter..
        strs->push_back(str);
        break;  
      }
      strs->push_back(str.substring(0, index));
      str = str.substring(index +sep.length() + 1);

      continue;
    }
    index = str.indexOf(sep);
    if (index == -1) // No separator found
    {
      strs->push_back(str);
      break;
    }
    else
    { 
      strs->push_back(str.substring(0, index));
      str = str.substring(index + sep.length());

    }
  }


}

void ATParser::parse_message(Vector<String> data, int *index) {
  
  Vector<String> parts(this->part_storage_array);


  //Serial.println("Message found!"); 

  // Handle CMT - Add comma
  data[*index].replace("+CMT: ", "+CMT: ,");
  
  split_string(data[*index], &parts, ","); 

  message_indata* indata = new message_indata();
  
  if (parts.size() != 4) {
    Serial.print("Bad first row in data: Wrong number of parts: ");
    Serial.println(parts.size());
  }

  // Handle multipart
  indata->phone_number = parts[1];
  indata->message_datetime = parts[3];

  ++*index;


  while (*index < data.size() ) {
    if (indata->message  != "") {
      indata->message+= "\r\n";
    }
    indata->message+= data[*index];
    ++*index;
    
  }
  /*
  Serial.print("Phone number: ");
  Serial.println(indata->phone_number);     
  Serial.print("Datetime: ");
  Serial.println(indata->message_datetime);     
  Serial.print("Full message:");
  Serial.println(indata->message); 
  */
  messages.push_back(*indata);
  
  /*  
  Serial.print("this->messages.max_size():");
  Serial.println(this->messages.max_size()); 
  */
  
  ++*index;
  
}


void ATParser::parse_at(String data) {
  
  Vector<String> rows(this->row_storage_array);
  int count, i;

  split_string(data, &rows,  "\r\n", "");


    // Show the resulting substrings
  for (int i = 0; i < rows.size(); i++)
  {
    /*
    Serial.print("Row ");
    Serial.print(i);
    Serial.print(":");
    Serial.println(rows[i]);
    */
    
    // Add class for SMS message
    if (rows[i].substring(0, 5) == "+CMGL" || rows[i].substring(0, 4) == "+CMT"){
      parse_message(rows, &i);
    }
  }  
}
