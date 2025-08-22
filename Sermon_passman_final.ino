
// Final update Date - 20/8/2025  Author - Prachet Hire   
// This is a serial monitor based password manager which uses internal 1KB EEPROM of arduino nano to store website credentials.
// Upload this code to arduino and open serial monitor with baud rate 9600, Select no line ending.
// Set a six digit pin to create a digital vault.  
// The domain, username and password are restricted to a fixed size of 15 bytes, so together, a single entry takes 45 bytes.
// We can store upto 22 entries in this 1KB EEPROM of arduino.
  
#include<EEPROM.h>

#define ENC 991
#define MARK 992 
#define PINLOC 993

struct sitecreds {
  char sitename[15] = "";
  char username[15] = "";
  char password[15] = "";
};

char fpin[7];
char rpin[7];
int attempt_count = 3;
bool passflag = false;
bool passflagprt = false;
bool entryflag = false;
bool updateflag = false;
bool knowflag = false;
bool siteflag = false; 
bool usernameflag = false;
bool passwordflag = false;
bool updatepermit = false;
bool inc_entry_cnt = false;
int upsiteloc;
unsigned long timecapture = 0;
const unsigned long timeout = 60000;//1min timeout
bool nopinflag = false;

void rstime()
{
  timecapture = millis();
}

void clearall(bool wpcreds=false) // function to clear all eeprom
{
  if(wpcreds)
    for(int i=0;i<992;i++)
      EEPROM.write(i,0);
  else
    for(int i=0;i<EEPROM.length();i++)
      EEPROM.write(i,0);
      
  Serial.println();
  Serial.println("EEPROM cleared");
}

void clearentry(int frmloc) // clears a single entry of fixed 45 bytes from the location provided
{
  int fl = frmloc*45;
  int el = fl+45;
  for(int i=fl; i<el; i++){
    EEPROM.write(i,0);
  }
  Serial.println("Entry cleared");
}

int get_entry_count() // gets the entry count, 0 is first entry, entry count times 45 gives us entry location
{
  int i = EEPROM.read(ENC);
  return i;
}

void update_entry_count(int e) // increments the entry count after storing entry in eeprom
{
  e+=1;
  if(e>21){
    Serial.println("Memory full on more entries");
    e=22;
  }
   EEPROM.update(ENC,e);
}

void savecreds(String& siten, String& usern, String& pswd, int entrycnt, bool updcntflag) // saves entry and updates entry count if flag set
{
  sitecreds sc;
  strncpy(sc.sitename,siten.c_str(),15);
  strncpy(sc.username,usern.c_str(),15);
  strncpy(sc.password,pswd.c_str(),15);
  EEPROM.put(entrycnt*45,sc);
  Serial.println();
  Serial.print("creds saved at location: ");
  Serial.println(entrycnt*45);
  if(updcntflag){
    update_entry_count(entrycnt);
  }
}

int getlocfrmsite(String& cmpsite) // pass site as string and get entry count, if site doesn't match the return 30
{
  int g = get_entry_count();
  for(int i=0;i<g;i++){
    sitecreds tc;
    EEPROM.get(i*45,tc);
    if(strcmp(cmpsite.c_str(),tc.sitename)==0){
      return i;
    }
  }
  return 30; //if 30 is returned that means we know that none of the entries matched as max entry is 22
}

void printuserpass(int fetchedloc) //gives username and password from the entry count
{
  sitecreds fc; // create object
  EEPROM.get(fetchedloc*45,fc); //store object from location
  Serial.print("Username:");
  Serial.println(fc.username);
  Serial.print("Password:");
  Serial.println(fc.password);
}

void printallsites(int numsites) //prints all the registered sites
{
  for(int i=0;i<numsites;i++){
    sitecreds pc;
    EEPROM.get(i*45,pc);
    Serial.print(i+1);
    Serial.print(".         ");
    Serial.println(pc.sitename);
  }
  Serial.println();
}

void setup() 
{
  Serial.begin(9600);
  if(get_entry_count()==255)// avrdude clears the eeprom by setting it high
  {
    EEPROM.write(ENC,0);
  }
  if(EEPROM.read(MARK)!=3)
  {
    nopinflag = true;
  }
  if(nopinflag==false)
  {
  Serial.println("Vault exists.");  
  Serial.println("wait...");
  delay(3000);
  Serial.println("Enter your 6 digit PIN:");
  }
  else // if nopinflag is true
  {
    Serial.println("Creating new vault.");
    delay(3000);
    Serial.println("Enter new 6 digit PIN:");
  }
}

void(* resetFunc) (void) = 0;//declare reset function @ address 0

void loop() {
  if(nopinflag){
    if(Serial.available()>0)
    {
    rstime();
    String fstrpin = Serial.readString();
    if(fstrpin.length()==6)
    {
      fstrpin.toCharArray(fpin,7); 
      EEPROM.put(PINLOC,fpin);
      EEPROM.write(MARK,3);
      Serial.println("PIN stored in EEPROM");
      Serial.println("wait...");
      delay(3000);
      Serial.println("Enter your 6 digit PIN:");
      nopinflag = false;
    }
    else//pin length not 6
    {
      Serial.println("Enter only six digit pin");
      Serial.println("Enter new 6 digit PIN:");
    }
    }
  }
  else // if nopinflag is false
  {
  if(passflag == false){
  if(Serial.available()>0)
  {
    rstime();
    String pass = Serial.readString();
    EEPROM.get(PINLOC,rpin);
    if(strcmp(pass.c_str(),rpin)==0)
    {
      Serial.println("Matched"); 
      passflag = true; 
      passflagprt = true;
    }
    else
    {
      Serial.print("Wrong PIN ");
      attempt_count -= 1;
      Serial.print(attempt_count);
      Serial.println(" attempts left.");
      if(attempt_count==1){
        Serial.println("last attempt. Wrong pin will erase data.");
      }
      Serial.println("Enter your 6 digit PIN:");
      if(attempt_count==0){
        Serial.println("clearing flash");
        clearall();
        resetFunc();  //call reset
      }
    }
   }
  }
  if((passflag == true) && (entryflag == false) && (updateflag == false) && (knowflag == false) && (updatepermit == false))
  {
    int memst = get_entry_count();// get entry count after adding entry
    if(passflagprt == true){
      Serial.println();
      Serial.print("This is your ");
      Serial.print(memst + 1); //entry starts from 0 so adding 1
      Serial.println(" entry, max is 22.");
      Serial.println("e - entry, u - update, k - know, w - wipe, r - restart, d - domain names");
      passflagprt = false;
    }  
    if(Serial.available()>0){
      rstime();
      String infochar = Serial.readString();
      if(infochar.length()==1)
      {
      if(infochar == "e")
      {
        // after adding the 21st entry staring from 0 the entry count updates to 22 this means memory full.
        if(memst<22) // enter entry mode only when entry count is less than 22 
        {
        Serial.println("Entry mode");
        Serial.println("Enter domain name");
        entryflag = true;
        siteflag = true;
        } 
        else{
          Serial.println("Entries not accepted.");
          passflagprt = true;
        }
      }
      else if(infochar == "u")
      {
        Serial.println("update mode");
        Serial.println("Enter entry number.");
        updateflag = true;
      }
      else if(infochar == "k")
      {
        Serial.println("know mode");
        Serial.println("Enter domain name");
        knowflag = true;
      }
      else if(infochar == "w")
      {
        Serial.println("Erasing all locations");
        clearall(true);//don't wipe pin just creds
        passflagprt = true;
      }
      else if(infochar == "r")
      {
        Serial.println("RE");
        resetFunc(); 
      }
       else if(infochar == "d")
      {
        Serial.println("Printing all stored domain names");
        Serial.println("Entry no.   Domains");
        printallsites(memst);
        passflagprt = true;
      }
      else
      {
        Serial.println("Invalid, enter valid char.");
        passflagprt = true;
      }
     }//infochar length
     else // infochar length exceed 1
     {
       Serial.println("Enter single valid char.");
       passflagprt = true;
     }
    }
  }
  if(entryflag == true || updatepermit == true)
  {
    static String sn;
    static String un;
    static String pw;
    int ec;
    if(siteflag == true){
      if(Serial.available()>0){
         rstime();
         sn = Serial.readString();
         if(sn.length()>15){
           Serial.println("Domain name greater than 15.");
           Serial.println("Entry not stored.");
           sn = "";
           entryflag = false;
           updatepermit = false;
           passflagprt = true;
         }
         else{
           siteflag = false;
           usernameflag = true;
           Serial.println(sn);
           Serial.println("Enter username ");
         }
      }
    }
    if(usernameflag == true){
      if(Serial.available()>0){
         rstime();
         un = Serial.readString();
         if(un.length()>15){
           Serial.println("Username greater than 15.");
           Serial.println("Entry not stored.");
           un = "";
           entryflag = false;
           updatepermit = false;
           passflagprt = true;
         }
         else{
           usernameflag = false;
           passwordflag = true;
           Serial.println(un);
           Serial.println("Enter password");
         }
      }
    }
    if(passwordflag == true){
      if(Serial.available()>0){
         rstime();
         pw = Serial.readString();
         if(pw.length()>15){
           Serial.println("Password greater than 15.");
           Serial.println("Entry not stored.");
           pw = "";
           entryflag = false;
           updatepermit = false;
           passflagprt = true;
         }
         else{
          for(int i=0;i<pw.length();i++)
          {
            Serial.print("*");
          }
          Serial.println();
          passwordflag = false;
         }
      }
    }
    if((passwordflag==false) && (usernameflag == false) && (siteflag == false))
    {  
      if(updatepermit==true)
      {
        ec = upsiteloc-1;
        Serial.print("Updating on ");
        Serial.print(upsiteloc);
        Serial.println(" entry");
        updatepermit = false;
        inc_entry_cnt = false;
      }
      else if(entryflag==true)
      {
        ec = get_entry_count();
        entryflag = false;
        inc_entry_cnt = true;
      }
      savecreds(sn,un,pw,ec,inc_entry_cnt);
      passflagprt = true;
    }
  }
  if(knowflag == true){
    static String cmpsiten;
    int siteloc;
    if(Serial.available()>0)
    {
      rstime();
      cmpsiten = Serial.readString();
      siteloc = getlocfrmsite(cmpsiten);
        if(siteloc!=30){
          printuserpass(siteloc);
          knowflag = false;
          passflagprt = true;
        }
        else{
          Serial.println("Invalid domain name");
          knowflag = false;
          passflagprt = true;
        }
    }
  }
  if(updateflag == true){
    static String updstring;
    if(Serial.available()>0){
      rstime();
      updstring = Serial.readString();
      upsiteloc = updstring.toInt();
      Serial.println("You have entered");
      Serial.println(upsiteloc);
      if((upsiteloc<23) && (upsiteloc>0))
      {
       clearentry(upsiteloc-1);
       updatepermit = true;
       siteflag = true;
       updateflag = false;
       Serial.println("Enter new domain name");
      }
      else{
        Serial.println("Invalid location");
        updatepermit = false;
        updateflag = false;
        passflagprt = true;
      }
    }
   }
  }//end of nopinflag false
  
  if((millis()-timecapture)>timeout)//if no input on serial and if time passes above 1min then reset
  {
    Serial.println("TO");
    resetFunc(); 
  }
}
