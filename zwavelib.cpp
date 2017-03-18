//-----------------------------------------------------------------------------
//
//	zwavelib.cpp
//
//	OpenZWave Control Panel support routines
//
//	Copyright (c) 2010 Greg Satz <satz@iranger.com>
//	All rights reserved.
//
// SOFTWARE NOTICE AND LICENSE
// This work (including software, documents, or other related items) is being
// provided by the copyright holders under the following license. By obtaining,
// using and/or copying this work, you (the licensee) agree that you have read,
// understood, and will comply with the following terms and conditions:
//
// Permission to use, copy, and distribute this software and its documentation,
// without modification, for any purpose and without fee or royalty is hereby
// granted, provided that you include the full text of this NOTICE on ALL
// copies of the software and documentation or portions thereof.
//
// THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS
// MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
// LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR
// PURPOSE OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE
// ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
//
// COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR
// CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR
// DOCUMENTATION.
//
// The name and trademarks of copyright holders may NOT be used in advertising
// or publicity pertaining to the software without specific, written prior
// permission.  Title to copyright in this software and any associated
// documentation will at all times remain with copyright holders.
//-----------------------------------------------------------------------------

#include <string.h>
#include "ozwcp.h"

const char *valueGenreStr (ValueID::ValueGenre vg)
{
  switch (vg) {
  case ValueID::ValueGenre_Basic:
    return "basic";
  case ValueID::ValueGenre_User:
    return "user";
  case ValueID::ValueGenre_Config:
    return "config";
  case ValueID::ValueGenre_System:
    return "system";
  case ValueID::ValueGenre_Count:
    return "count";
  }
  return "unknown";
}

ValueID::ValueGenre valueGenreNum (char const *str)
{
  if (strcmp(str, "basic") == 0)
    return ValueID::ValueGenre_Basic;
  else if (strcmp(str, "user") == 0)
    return ValueID::ValueGenre_User;
  else if (strcmp(str, "config") == 0)
    return ValueID::ValueGenre_Config;
  else if (strcmp(str, "system") == 0)
    return ValueID::ValueGenre_System;
  else if (strcmp(str, "count") == 0)
    return ValueID::ValueGenre_Count;
  else
    return (ValueID::ValueGenre)255;
}

const char *valueTypeStr (ValueID::ValueType vt)
{
  switch (vt) {
  case ValueID::ValueType_Bool:
    return "bool";
  case ValueID::ValueType_Byte:
    return "byte";
  case ValueID::ValueType_Decimal:
    return "decimal";
  case ValueID::ValueType_Int:
    return "int";
  case ValueID::ValueType_List:
    return "list";
  case ValueID::ValueType_Schedule:
    return "schedule";
  case ValueID::ValueType_String:
    return "string";
  case ValueID::ValueType_Short:
    return "short";
  case ValueID::ValueType_Button:
    return "button";
  case ValueID::ValueType_Raw:
    return "raw";
  }
  return "unknown";
}

ValueID::ValueType valueTypeNum (char const *str)
{
  if (strcmp(str, "bool") == 0)
    return ValueID::ValueType_Bool;
  else if (strcmp(str, "byte") == 0)
    return ValueID::ValueType_Byte;
  else if (strcmp(str, "decimal") == 0)
    return ValueID::ValueType_Decimal;
  else if (strcmp(str, "int") == 0)
    return ValueID::ValueType_Int;
  else if (strcmp(str, "list") == 0)
    return ValueID::ValueType_List;
  else if (strcmp(str, "schedule") == 0)
    return ValueID::ValueType_Schedule;
  else if (strcmp(str, "short") == 0)
    return ValueID::ValueType_Short;
  else if (strcmp(str, "string") == 0)
    return ValueID::ValueType_String;
  else if (strcmp(str, "button") == 0)
    return ValueID::ValueType_Button;
  else if (strcmp(str, "raw") == 0)
    return ValueID::ValueType_Raw;
  else
    return (ValueID::ValueType)255;
}

const char *nodeBasicStr (uint8 basic)
{
  switch (basic) {
  case 1:
    return "Controller";
  case 2:
    return "Static Controller";
  case 3:
    return "Slave";
  case 4:
    return "Routing Slave";
  }
  return "unknown";
}

const char *cclassStr (uint8 cc)
{
  switch (cc) {
  case 0x00:
    return "NO OPERATION";
  case 0x20:
    return "BASIC";
  case 0x21:
    return "CONTROLLER REPLICATION";
  case 0x22:
    return "APPLICATION STATUS";
  case 0x23:
    return "ZIP SERVICES";
  case 0x24:
    return "ZIP SERVER";
  case 0x25:
    return "SWITCH BINARY";
  case 0x26:
    return "SWITCH MULTILEVEL";
  case 0x27:
    return "SWITCH ALL";
  case 0x28:
    return "SWITCH TOGGLE BINARY";
  case 0x29:
    return "SWITCH TOGGLE MULTILEVEL";
  case 0x2A:
    return "CHIMNEY FAN";
  case 0x2B:
    return "SCENE ACTIVATION";
  case 0x2C:
    return "SCENE ACTUATOR CONF";
  case 0x2D:
    return "SCENE CONTROLLER CONF";
  case 0x2E:
    return "ZIP CLIENT";
  case 0x2F:
    return "ZIP ADV SERVICES";
  case 0x30:
    return "SENSOR BINARY";
  case 0x31:
    return "SENSOR MULTILEVEL";
  case 0x32:
    return "METER";
  case 0x33:
    return "COLOR";
  case 0x34:
    return "ZIP ADV CLIENT";
  case 0x35:
    return "METER PULSE";
  case 0x38:
    return "THERMOSTAT HEATING";
  case 0x40:
    return "THERMOSTAT MODE";
  case 0x42:
    return "THERMOSTAT OPERATING STATE";
  case 0x43:
    return "THERMOSTAT SETPOINT";
  case 0x44:
    return "THERMOSTAT FAN MODE";
  case 0x45:
    return "THERMOSTAT FAN STATE";
  case 0x46:
    return "CLIMATE CONTROL SCHEDULE";
  case 0x47:
    return "THERMOSTAT SETBACK";
  case 0x4C:
    return "DOOR LOCK LOGGING";
  case 0x4E:
    return "SCHEDULE ENTRY LOCK";
  case 0x50:
    return "BASIC WINDOW COVERING";
  case 0x51:
    return "MTP WINDOW COVERING";
  case 0x56:
    return "CRC16 ENCAP";
  case 0x5A:
    return "DEVICE RESET LOCALLY";
  case 0x5B:
    return "CENTRAL SCENE";
  case 0x5E:
    return "ZWAVE PLUS INFO";
  case 0x60:
    return "MULTI INSTANCE";
  case 0x62:
    return "DOOR LOCK";
  case 0x63:
    return "USER CODE";
  case 0x66:
    return "BARRIER OPERATOR";
  case 0x70:
    return "CONFIGURATION";
  case 0x71:
    return "ALARM";
  case 0x72:
    return "MANUFACTURER SPECIFIC";
  case 0x73:
    return "POWERLEVEL";
  case 0x75:
    return "PROTECTION";
  case 0x76:
    return "LOCK";
  case 0x77:
    return "NODE NAMING";
  case 0x7A:
    return "FIRMWARE UPDATE MD";
  case 0x7B:
    return "GROUPING NAME";
  case 0x7C:
    return "REMOTE ASSOCIATION ACTIVATE";
  case 0x7D:
    return "REMOTE ASSOCIATION";
  case 0x80:
    return "BATTERY";
  case 0x81:
    return "CLOCK";
  case 0x82:
    return "HAIL";
  case 0x84:
    return "WAKE UP";
  case 0x85:
    return "ASSOCIATION";
  case 0x86:
    return "VERSION";
  case 0x87:
    return "INDICATOR";
  case 0x88:
    return "PROPRIETARY";
  case 0x89:
    return "LANGUAGE";
  case 0x8A:
    return "TIME";
  case 0x8B:
    return "TIME PARAMETERS";
  case 0x8C:
    return "GEOGRAPHIC LOCATION";
  case 0x8D:
    return "COMPOSITE";
  case 0x8E:
    return "MULTI INSTANCE ASSOCIATION";
  case 0x8F:
    return "MULTI CMD";
  case 0x90:
    return "ENERGY PRODUCTION";
  case 0x91:
    return "MANUFACTURER PROPRIETARY";
  case 0x92:
    return "SCREEN MD";
  case 0x93:
    return "SCREEN ATTRIBUTES";
  case 0x94:
    return "SIMPLE AV CONTROL";
  case 0x95:
    return "AV CONTENT DIRECTORY MD";
  case 0x96:
    return "AV RENDERER STATUS";
  case 0x97:
    return "AV CONTENT SEARCH MD";
  case 0x98:
    return "SECURITY";
  case 0x99:
    return "AV TAGGING MD";
  case 0x9A:
    return "IP CONFIGURATION";
  case 0x9B:
    return "ASSOCIATION COMMAND CONFIGURATION";
  case 0x9C:
    return "SENSOR ALARM";
  case 0x9D:
    return "SILENCE ALARM";
  case 0x9E:
    return "SENSOR CONFIGURATION";
  case 0xEF:
    return "MARK";
  case 0xF0:
    return "NON INTEROPERABLE";
  }
  return "UNKNOWN";
}

uint8 cclassNum (char const *str)
{
  if (strcmp(str, "NO OPERATION") == 0)
    return 0x00;
  else if (strcmp(str, "BASIC") == 0)
    return 0x20;
  else if (strcmp(str, "CONTROLLER REPLICATION") == 0)
    return 0x21;
  else if (strcmp(str, "APPLICATION STATUS") == 0)
    return 0x22;
  else if (strcmp(str, "ZIP SERVICES") == 0)
    return 0x23;
  else if (strcmp(str, "ZIP SERVER") == 0)
    return 0x24;
  else if (strcmp(str, "SWITCH BINARY") == 0)
    return 0x25;
  else if (strcmp(str, "SWITCH MULTILEVEL") == 0)
    return 0x26;
  else if (strcmp(str, "SWITCH ALL") == 0)
    return 0x27;
  else if (strcmp(str, "SWITCH TOGGLE BINARY") == 0)
    return 0x28;
  else if (strcmp(str, "SWITCH TOGGLE MULTILEVEL") == 0)
    return 0x29;
  else if (strcmp(str, "CHIMNEY FAN") == 0)
    return 0x2A;
  else if (strcmp(str, "SCENE ACTIVATION") == 0)
    return 0x2B;
  else if (strcmp(str, "SCENE ACTUATOR CONF") == 0)
    return 0x2C;
  else if (strcmp(str, "SCENE CONTROLLER CONF") == 0)
    return 0x2D;
  else if (strcmp(str, "ZIP CLIENT") == 0)
    return 0x2E;
  else if (strcmp(str, "ZIP ADV SERVICES") == 0)
    return 0x2F;
  else if (strcmp(str, "SENSOR BINARY") == 0)
    return 0x30;
  else if (strcmp(str, "SENSOR MULTILEVEL") == 0)
    return 0x31;
  else if (strcmp(str, "METER") == 0)
    return 0x32;
  else if (strcmp(str, "ZIP ADV SERVER") == 0)
    return 0x33;
  else if (strcmp(str, "ZIP ADV CLIENT") == 0)
    return 0x34;
  else if (strcmp(str, "METER PULSE") == 0)
    return 0x35;
  else if (strcmp(str, "THERMOSTAT HEATING") == 0)
    return 0x38;
  else if (strcmp(str, "THERMOSTAT MODE") == 0)
    return 0x40;
  else if (strcmp(str, "THERMOSTAT OPERATING STATE") == 0)
    return 0x42;
  else if (strcmp(str, "THERMOSTAT SETPOINT") == 0)
    return 0x43;
  else if (strcmp(str, "THERMOSTAT FAN MODE") == 0)
    return 0x44;
  else if (strcmp(str, "THERMOSTAT FAN STATE") == 0)
    return 0x45;
  else if (strcmp(str, "CLIMATE CONTROL SCHEDULE") == 0)
    return 0x46;
  else if (strcmp(str, "THERMOSTAT SETBACK") == 0)
    return 0x47;
  else if (strcmp(str, "DOOR LOCK LOGGING") == 0)
    return 0x4C;
  else if (strcmp(str, "SCHEDULE ENTRY LOCK") == 0)
    return 0x4E;
  else if (strcmp(str, "BASIC WINDOW COVERING") == 0)
    return 0x50;
  else if (strcmp(str, "MTP WINDOW COVERING") == 0)
    return 0x51;
  else if (strcmp(str, "MULTI INSTANCE") == 0)
    return 0x60;
  else if (strcmp(str, "DOOR LOCK") == 0)
    return 0x62;
  else if (strcmp(str, "USER CODE") == 0)
    return 0x63;
  else if (strcmp(str, "BARRIER OPERATOR") == 0)
    return 0x66;
  else if (strcmp(str, "CONFIGURATION") == 0)
    return 0x70;
  else if (strcmp(str, "ALARM") == 0)
    return 0x71;
  else if (strcmp(str, "MANUFACTURER SPECIFIC") == 0)
    return 0x72;
  else if (strcmp(str, "POWERLEVEL") == 0)
    return 0x73;
  else if (strcmp(str, "PROTECTION") == 0)
    return 0x75;
  else if (strcmp(str, "LOCK") == 0)
    return 0x76;
  else if (strcmp(str, "NODE NAMING") == 0)
    return 0x77;
  else if (strcmp(str, "FIRMWARE UPDATE MD") == 0)
    return 0x7A;
  else if (strcmp(str, "GROUPING NAME") == 0)
    return 0x7B;
  else if (strcmp(str, "REMOTE ASSOCIATION ACTIVATE") == 0)
    return 0x7C;
  else if (strcmp(str, "REMOTE ASSOCIATION") == 0)
    return 0x7D;
  else if (strcmp(str, "BATTERY") == 0)
    return 0x80;
  else if (strcmp(str, "CLOCK") == 0)
    return 0x81;
  else if (strcmp(str, "HAIL") == 0)
    return 0x82;
  else if (strcmp(str, "WAKE UP") == 0)
    return 0x84;
  else if (strcmp(str, "ASSOCIATION") == 0)
    return 0x85;
  else if (strcmp(str, "VERSION") == 0)
    return 0x86;
  else if (strcmp(str, "INDICATOR") == 0)
    return 0x87;
  else if (strcmp(str, "PROPRIETARY") == 0)
    return 0x88;
  else if (strcmp(str, "LANGUAGE") == 0)
    return 0x89;
  else if (strcmp(str, "TIME") == 0)
    return 0x8A;
  else if (strcmp(str, "TIME PARAMETERS") == 0)
    return 0x8B;
  else if (strcmp(str, "GEOGRAPHIC LOCATION") == 0)
    return 0x8C;
  else if (strcmp(str, "COMPOSITE") == 0)
    return 0x8D;
  else if (strcmp(str, "MULTI INSTANCE ASSOCIATION") == 0)
    return 0x8E;
  else if (strcmp(str, "MULTI CMD") == 0)
    return 0x8F;
  else if (strcmp(str, "ENERGY PRODUCTION") == 0)
    return 0x90;
  else if (strcmp(str, "MANUFACTURER PROPRIETARY") == 0)
    return 0x91;
  else if (strcmp(str, "SCREEN MD") == 0)
    return 0x92;
  else if (strcmp(str, "SCREEN ATTRIBUTES") == 0)
    return 0x93;
  else if (strcmp(str, "SIMPLE AV CONTROL") == 0)
    return 0x94;
  else if (strcmp(str, "AV CONTENT DIRECTORY MD") == 0)
    return 0x95;
  else if (strcmp(str, "AV RENDERER STATUS") == 0)
    return 0x96;
  else if (strcmp(str, "AV CONTENT SEARCH MD") == 0)
    return 0x97;
  else if (strcmp(str, "SECURITY") == 0)
    return 0x98;
  else if (strcmp(str, "AV TAGGING MD") == 0)
    return 0x99;
  else if (strcmp(str, "IP CONFIGURATION") == 0)
    return 0x9A;
  else if (strcmp(str, "ASSOCIATION COMMAND CONFIGURATION") == 0)
    return 0x9B;
  else if (strcmp(str, "SENSOR ALARM") == 0)
    return 0x9C;
  else if (strcmp(str, "SILENCE ALARM") == 0)
    return 0x9D;
  else if (strcmp(str, "SENSOR CONFIGURATION") == 0)
    return 0x9E;
  else if (strcmp(str, "MARK") == 0)
    return 0xEF;
  else if (strcmp(str, "NON INTEROPERABLE") == 0)
    return 0xF0;
  else if (strcmp(str, "COLOR") == 0)
    return 0x33;
  else
    return 0xFF;
}

const char *controllerErrorStr (Driver::ControllerError err)
{
  switch (err) {
  case Driver::ControllerError_None:
    return "None";
  case Driver::ControllerError_ButtonNotFound:
    return "Button Not Found";
  case Driver::ControllerError_NodeNotFound:
    return "Node Not Found";
  case Driver::ControllerError_NotBridge:
    return "Not a Bridge";
  case Driver::ControllerError_NotPrimary:
    return "Not Primary Controller";
  case Driver::ControllerError_IsPrimary:
    return "Is Primary Controller";
  case Driver::ControllerError_NotSUC:
    return "Not Static Update Controller";
  case Driver::ControllerError_NotSecondary:
    return "Not Secondary Controller";
  case Driver::ControllerError_NotFound:
    return "Not Found";
  case Driver::ControllerError_Busy:
    return "Controller Busy";
  case Driver::ControllerError_Failed:
    return "Failed";
  case Driver::ControllerError_Disabled:
    return "Disabled";
  case Driver::ControllerError_Overflow:
    return "Overflow";
  }
  return "Unknown";
}
