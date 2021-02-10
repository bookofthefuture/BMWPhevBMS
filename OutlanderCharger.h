//Modified from original charge controller sketch from Dilbert on Open Inverter forums
// stripped down to remove push button (charging triggered by handle), serial comms, and LEDs (since short on pins and under bonnet)

//const int CHARGER_STATUS_LED = 14;
//const int PROX_STATUS_LED = 15;
//const int CHARGER_POWER =  16;
const int TARGET_VOLTAGE = 300; // CHARGER IGNORES THIS BUT SCRIPT USES IT TO STOP CHARGE WHEN TARGET VOLTAGE REACHED.
const int CHARGE_CURRENT = 10; // 10X SCALING SO 10 = 1A
const int PP_PIN = A2;

//void outlander_setup() {
//  pinMode(CHARGER_STATUS_LED, OUTPUT);
//  pinMode(PROX_STATUS_LED, OUTPUT);
//  pinMode(CHARGER_POWER, OUTPUT);
//  pinMode(21, INPUT_PULLUP);
//}

//int last_button=HIGH;
int state = 0;
int prox;
int prox_state = 0;
int flag;
unsigned int timer = 0;
int last_state = 0;
int count = 0;
int charger_evse_req = 3;
int pilot;
int charger_hb = 0;
int charger_current_sp = 0;
int charger_voltage_sp = 0;
int slow_flag_counter = 0;
int slow_flag = 0;
int bat_voltage = 0;
int evse_pilot = 0;
int charger_can_alive = 0;
int supply_voltage = 0;

void sendEVSEData(int val, int hb)
{
  // using Can2 on MCP2515 and ACAN2515 via Can2.h
  Can2Frame.Can2_ext  = false;
  Can2Frame.Can2_rtr  = false;
  Can2Frame.Can2_id   = 0x285;
  Can2Frame.Can2_len  = 8;
  Can2Frame.Can2_d[0] = 0;
  Can2Frame.Can2_d[1] = 0;
  Can2Frame.Can2_d[2] = val;
  Can2Frame.Can2_d[3] = 9;
  Can2Frame.Can2_d[4] = hb;
  Can2Frame.Can2_d[5] = 0;
  if (val == 0xb6)
    Can2Frame.Can2_d[6] = 0;
  else
    Can2Frame.Can2_d[6] = 8;
  Can2Frame.Can2_d[7] = 0xa;

  Can2Send(Can2Frame);
}

void sendChargerSPData(int voltage, int current)
{
  voltage *= 10;
  // using Can2 on MCP2515 and ACAN2515 via Can2.h
  Can2Frame.Can2_ext  = false;
  Can2Frame.Can2_rtr  = false;
  Can2Frame.Can2_id   = 0x286;
  Can2Frame.Can2_len  = 8;
  Can2Frame.Can2_d[0] = voltage >> 8;
  Can2Frame.Can2_d[1] = voltage &= 0xFF;
  Can2Frame.Can2_d[2] = current;
  Can2Frame.Can2_d[3] = 51;
  Can2Frame.Can2_d[4] = 0x0;
  Can2Frame.Can2_d[5] = 0x0;
  Can2Frame.Can2_d[6] = 0xa;
  Can2Frame.Can2_d[7] = 0x0;

  Can2Send(Can2Frame);
}

void printFrame(CAN_FRAME &frame) {
  Serial.print("ID: 0x");
  Serial.print(frame.id, HEX);
  Serial.print(" Len: ");
  Serial.print(frame.length);
  Serial.print(" Data: 0x");
  for (int count = 0; count < frame.length; count++) {
    Serial.print(frame.data.bytes[count], HEX);
    Serial.print(" ");
  }
  Serial.print("\r\n");
}


void charge_control(int packvoltage) {

  // changed this to get pack voltage from BMS instead of charger
  bat_voltage = packvoltage;


  // get supply voltage and pilot signal from charger over CAN
  CANMessage incoming;

  if (Can2.available() > 0) {
    Can2.receive(incoming);
    //    printFrame(incoming);

    if (incoming.id == 0x389) {

      supply_voltage = incoming.data[1];
      charger_can_alive++;
    }
    if (incoming.id == 0x38A) {
      evse_pilot = incoming.data[3];
      charger_can_alive++;
    }
  }

  //Send voltage and current data to charger
  if (count == 50) {
    sendChargerSPData(charger_voltage_sp, charger_current_sp);
  }
  // Send on/off information to charger
  else if ((count % 10) == 0) {
    sendEVSEData(charger_evse_req, charger_hb);
  }

  if (count > 100) {

    //Maybe able to get rid of flag but slow flag counter is used for more than flashing LEDs!
    if (flag) {
      flag = 0;
    }
    else {
      flag = 1;
    }


    if (slow_flag_counter < 10)slow_flag_counter++;
    else {
      slow_flag_counter = 0;
      charger_hb++;
      charger_hb &= 0x1F;
      if (slow_flag)slow_flag = 0;
      else slow_flag = 1;
    }

    // Read the voltage on the PP line through a voltage bridge. Tie to 3v3 with a 1K2 reSistor.
    prox = analogRead(PP_PIN);
    //    Serial.print("Prox: ");
    //    Serial.println(prox);

    if (prox < 200) {
      //      digitalWrite(PROX_STATUS_LED, HIGH);
      prox_state = 1;
    }
    else {
      //      digitalWrite(PROX_STATUS_LED, LOW);
      prox_state = 0;
    }


    // State machine that moves through the phases to pull in the EVSE and begin charging
    switch (state) {

      case 0:
        if (prox_state == 1) {
          state++;
          //          digitalWrite(CHARGER_POWER, HIGH);
          charger_evse_req = 3;
        }
        break;

      case 1:
        //        Serial.println("State: 1");
        // start sequence for pulling in EVSE
        if (timer > 10) {
          charger_evse_req = 0x16;
          state++;
          //          digitalWrite(CHARGER_STATUS_LED, HIGH);
          //          Serial.print("bat_voltage ");
          //          Serial.println(bat_voltage);
        }

        break;

      case 2:
        //        Serial.println("State: 2");
        //EV
        if (timer > 10) {
          charger_evse_req = 0xb6;
          state++;
          //          Serial.print("evse_pilot ");
          //          Serial.println(evse_pilot);
        }
        break;

      case 3:
        Serial.println("State: 3");
        if (timer > 10) {

          //Check EVSE has pulled in, by checking pwoer to charger!
          //           Serial.print("supply_voltage ");
          //          Serial.println(supply_voltage);

          //if EVSE hasn't pulled in, quit and flash fault LED

          charger_current_sp = 10;
          charger_voltage_sp = TARGET_VOLTAGE + 10;
          // Ramp charge current to TARGET CURRENT
          // when we hit top of ramp, jump to the next state
          state++;
        }
        break;

      case 4:
        //        Serial.println("State: 4");

        //Charging
        //Monitor battery voltage, only bring it to TARGET VOLTAGE, before dropping out

        //        digitalWrite(CHARGER_STATUS_LED, flag);
        if (prox_state != 1) {
          state = 0;
          charger_current_sp = 0;
          charger_voltage_sp = 0;
        }

        if (bat_voltage > TARGET_VOLTAGE) {
          state = 5;
          charger_current_sp = 0;
          charger_voltage_sp = 0;
          //           Serial.println("charge complete");
          //           Serial.println(bat_voltage);
        }

        break;

      case 5:
        //        Serial.println("State: 5");
        //Charge Complete state
        //      digitalWrite(CHARGER_STATUS_LED, slow_flag);

        break;

      case 6:
        //        Serial.println("State: 6");
        //fault state!

        break;

    }
    //Reset timer on each state change
    if (timer < 65535)timer++;
    if (state != last_state) timer = 0;

    last_state = state;

    //Serial.println(analogRead(A0));

    count = 0;
  }
  else {
    count++;
  }

  delay(1);

}
