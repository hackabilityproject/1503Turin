
void setup() {
pinMode (13, OUTPUT);
Serial.begin (9600);
}

void loop() {
  int v1 = analogRead(A0);
  int v2 = analogRead(A2);
  float mv1 = ( v1/1024.0)*5000; //value in millivolt
  mv1 = mv1-500; //value in millivolt after °C
  float cel = mv1/10; //°C degrees considering +10mV = +1°C

  Serial.print(cel);
  Serial.print(" C");

  v2 = map(v2, 0, 1024, 20, 45);
  
  Serial.print(" - trig");
  Serial.print(v2);
  Serial.println(); 
if (v2 > cel)
{
digitalWrite (13, HIGH);
}
else
{
digitalWrite (13, LOW);
}
delay(5000);        // delay in between reads for stability
}
