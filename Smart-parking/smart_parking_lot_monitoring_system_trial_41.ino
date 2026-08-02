struct ParkingSpace
{
  int id;
  bool occupied;
  byte ledPin;
  byte entryButton;
  byte exitButton;
  bool lastEntryState;
  bool lastExitState;
};
ParkingSpace *spaces;
const int TOTAL_SPACES = 4;
int occupiedCount = 0;
unsigned long previousMillis = 0;
const long interval = 100;
void initializeParking();
void updateParking();
void printStatus();
void handleEntry(ParkingSpace *space);
void handleExit(ParkingSpace *space);
void setup()
{
  Serial.begin(9600);
  spaces = new ParkingSpace[TOTAL_SPACES];
  initializeParking();
  Serial.println("SMART PARKING SYSTEM");
  Serial.println("---");
}
void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    updateParking();
    printStatus();
  }
}
void initializeParking()
{
  byte ledPins[] = {2,3,4,5};
  byte entryPins[] = {6,7,8,9};
  byte exitPins[] = {10,11,12,13};
  for(int i=0;i<TOTAL_SPACES;i++)
  {
    spaces[i].id=i+1;
    spaces[i].occupied=false;
    spaces[i].ledPin=ledPins[i];
    spaces[i].entryButton=entryPins[i];
    spaces[i].exitButton=exitPins[i];
    spaces[i].lastEntryState=LOW;
    spaces[i].lastExitState=LOW;
    pinMode(spaces[i].ledPin,OUTPUT);
    pinMode(spaces[i].entryButton,INPUT);
    pinMode(spaces[i].exitButton,INPUT);
    digitalWrite(spaces[i].ledPin,LOW);
  }
}
void updateParking()
{
  for(int i=0;i<TOTAL_SPACES;i++)
  {
    ParkingSpace *space=&spaces[i];
    handleEntry(space);
    handleExit(space);
    digitalWrite(space->ledPin,space->occupied);
  }
}
void handleEntry(ParkingSpace *space)
{
  bool currentState=digitalRead(space->entryButton);
  if(currentState==HIGH && space->lastEntryState==LOW)
  {
    if(!space->occupied)
    {
      if(occupiedCount<TOTAL_SPACES)
      {
        space->occupied=true;
        occupiedCount++;
        Serial.print("Vehicle entered Space ");
        Serial.println(space->id);
      }
      else
      {
        Serial.println("ERROR: Parking Full");
      }
    }
    else
    {
      Serial.print("ERROR: Space ");
      Serial.print(space->id);
      Serial.println(" already occupied.");
    }
  }
  space->lastEntryState=currentState;
}
void handleExit(ParkingSpace *space)
{
  bool currentState=digitalRead(space->exitButton);
  if(currentState==HIGH && space->lastExitState==LOW)
  {
    if(space->occupied)
    {
      if(occupiedCount>0)
      {
        space->occupied=false;
        occupiedCount--;
        Serial.print("Vehicle exited Space ");
        Serial.println(space->id);
      }
    }
    else
    {
      Serial.print("ERROR: Space ");
      Serial.print(space->id);
      Serial.println(" already empty.");
    }
  }
  space->lastExitState=currentState;
}
void printStatus()
{
  static unsigned long lastPrint=0;
  if(millis()-lastPrint<1000)
    return;
  lastPrint=millis();
  Serial.println("-------------------------");
  for(int i=0;i<TOTAL_SPACES;i++)
  {
    Serial.print("Space ");
    Serial.print(spaces[i].id);
    Serial.print(": ");
    if(spaces[i].occupied)
      Serial.println("Occupied");
    else
      Serial.println("Available");
  }
  Serial.print("Occupied: ");
  Serial.println(occupiedCount);
  Serial.print("Available: ");
  Serial.println(TOTAL_SPACES-occupiedCount);
  if(occupiedCount==TOTAL_SPACES)
    Serial.println("STATUS: FULL");
  else
    Serial.println("STATUS: SPACE AVAILABLE");
}